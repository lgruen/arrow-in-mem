#include <absl/base/thread_annotations.h>
#include <absl/status/statusor.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/strip.h>
#include <absl/synchronization/blocking_counter.h>
#include <absl/synchronization/mutex.h>
#include <absl/time/time.h>
#include <arrow/io/memory.h>
#include <google/cloud/storage/client.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <cassert>
#include <cstddef>
#include <functional>
#include <iostream>
#include <optional>
#include <queue>
#include <string_view>
#include <thread>  // NOLINT(build/c++11)
#include <vector>

#include "seqr_query_service.grpc.pb.h"

namespace {

constexpr size_t kNumThreadPoolWorkers = 32;

// Adapted from the Abseil thread pool.
class ThreadPool {
 public:
  explicit ThreadPool(const int num_threads) {
    for (int i = 0; i < num_threads; ++i) {
      threads_.push_back(std::thread(&ThreadPool::WorkLoop, this));
    }
  }

  ThreadPool(const ThreadPool&) = delete;
  ThreadPool& operator=(const ThreadPool&) = delete;

  ~ThreadPool() {
    {
      absl::MutexLock l(&mu_);
      for (size_t i = 0; i < threads_.size(); i++) {
        queue_.push(nullptr);  // Shutdown signal.
      }
    }
    for (auto& t : threads_) {
      t.join();
    }
  }

  // Schedule a function to be run on a ThreadPool thread immediately.
  void Schedule(std::function<void()> func) {
    assert(func != nullptr);
    absl::MutexLock l(&mu_);
    queue_.push(std::move(func));
  }

 private:
  bool WorkAvailable() const ABSL_EXCLUSIVE_LOCKS_REQUIRED(mu_) {
    return !queue_.empty();
  }

  void WorkLoop() {
    while (true) {
      std::function<void()> func;
      {
        absl::MutexLock l(&mu_);
        mu_.Await(absl::Condition(this, &ThreadPool::WorkAvailable));
        func = std::move(queue_.front());
        queue_.pop();
      }
      if (func == nullptr) {  // Shutdown signal.
        break;
      }
      func();
    }
  }

  absl::Mutex mu_;
  std::queue<std::function<void()>> queue_ ABSL_GUARDED_BY(mu_);
  std::vector<std::thread> threads_;
};

class UrlReader {
 public:
  virtual ~UrlReader() = default;

  virtual absl::StatusOr<std::string> Read(absl::string_view url) const = 0;
};

// Returns the bucket and file name from a complete gs:// path.
absl::StatusOr<std::pair<std::string_view, std::string_view>> SplitBlobPath(
    std::string_view blob_path) {
  if (!absl::ConsumePrefix(&blob_path, "gs://")) {
    return absl::InvalidArgumentError("Missing gs:// prefix");
  }
  const size_t slash_pos = blob_path.find_first_of('/');
  if (slash_pos == std::string_view::npos) {
    return absl::InvalidArgumentError("Incomplete blob path");
  }
  return std::make_pair(blob_path.substr(0, slash_pos),
                        blob_path.substr(slash_pos + 1));
}

namespace gcs = google::cloud::storage;

class GcsReader : public UrlReader {
 public:
  absl::StatusOr<std::string> Read(const absl::string_view url) const override {
    if (!url.starts_with("gs://")) {
      return absl::InvalidArgumentError(absl::StrCat("Unsupported URL: ", url));
    }

    // Make a copy of the GCS client for thread-safety.
    gcs::Client gcs_client = shared_gcs_client_;
    const auto& split_path = SplitBlobPath(url);
    if (!split_path.ok()) {
      return absl::InvalidArgumentError(
          absl::StrCat("Invalid path", split_path.status().message()));
    }

    try {
      const absl::Time start_time = absl::Now();
      auto reader = gcs_client.ReadObject(std::string(split_path->first),
                                          std::string(split_path->second));
      if (reader.bad()) {
        return absl::InvalidArgumentError(
            absl::StrCat("Failed to read blob: ", reader.status().message()));
      }

      std::optional<int> content_length;
      for (const auto& header : reader.headers()) {
        if (header.first == "content-length") {
          int value = 0;
          if (!absl::SimpleAtoi(header.second, &value)) {
            return absl::NotFoundError(
                "Couldn't parse content-length header value");
          }
          content_length = value;
        }
      }
      if (!content_length) {
        return absl::NotFoundError("Couldn't find content-length header");
      }

      std::string result(*content_length, 0);
      reader.read(result.data(), *content_length);
      if (reader.bad()) {
        return absl::InvalidArgumentError(
            absl::StrCat("Failed to read blob: ", reader.status().message()));
      }
      const absl::Time end_time = absl::Now();
      std::cout << "Read " << url << " (" << *content_length << "B) in "
                << (end_time - start_time) << std::endl;
      return result;
    } catch (const std::exception& e) {
      // Unfortunately the googe-cloud-storage library throws exceptions.
      return absl::InternalError(
          absl::StrCat("Exception during reading of ", url, ": ", e.what()));
    }
  }

 private:
  // Share connection pool, but need to make copies for thread-safety.
  gcs::Client shared_gcs_client_{
      google::cloud::Options{}.set<gcs::ConnectionPoolSizeOption>(
          kNumThreadPoolWorkers)};
};

class QueryServiceImpl final : public seqr::QueryService::Service {
 public:
  QueryServiceImpl(const UrlReader& url_reader) : url_reader_(url_reader) {}

 private:
  grpc::Status Query(grpc::ServerContext* const context,
                     const seqr::QueryRequest* const request,
                     seqr::QueryResponse* const response) override {
    const size_t num_urls = request->data_urls_size();
    std::vector<absl::StatusOr<size_t>> results(num_urls);
    absl::BlockingCounter blocking_counter(num_urls);
    for (size_t i = 0; i < num_urls; ++i) {
      thread_pool_.Schedule([&url_reader = url_reader_,
                             &url = request->data_urls(i), &result = results[i],
                             &blocking_counter] {
        const auto data = url_reader.Read(url);
        if (data.ok()) {
          result = data->size();
        } else {
          result = absl::InvalidArgumentError(absl::StrCat(
              "Failed to read URL ", url, ": ", data.status().message()));
        }
        blocking_counter.DecrementCount();
      });
    }

    blocking_counter.Wait();
    for (const auto& result : results) {
      if (!result.ok()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            std::string(result.status().message()));
      }
      response->add_data_sizes(*result);
    }

    return grpc::Status::OK;
  }

  ThreadPool thread_pool_{kNumThreadPoolWorkers};
  const UrlReader& url_reader_;
};

void RunServer(const int port) {
  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();

  const std::string server_address = absl::StrCat("[::]:", port);
  std::cout << "Starting server on " << server_address << std::endl;
  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  GcsReader gcs_reader;
  QueryServiceImpl query_service(gcs_reader);
  builder.RegisterService(&query_service);

  builder.BuildAndStart()->Wait();
}

}  // namespace

int main(int argc, char** argv) {
  // Get the port number from the environment.
  const char* const port_env = std::getenv("PORT");
  if (port_env == nullptr) {
    std::cerr << "PORT environment variable not set" << std::endl;
    return 1;
  }

  int port = 0;
  if (!absl::SimpleAtoi(port_env, &port)) {
    std::cerr << "Failed to parse PORT environment variable" << std::endl;
    return 1;
  }

  RunServer(port);

  return 0;
}
