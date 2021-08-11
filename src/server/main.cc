#include <absl/status/statusor.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/strip.h>
#include <absl/time/time.h>
#include <arrow/io/memory.h>
#include <google/cloud/storage/client.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <iostream>
#include <optional>
#include <string_view>

#include "scan_service.grpc.pb.h"

namespace {

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

absl::StatusOr<std::string> ReadBlob(
    google::cloud::storage::Client* const gcs_client,
    const std::string_view blob_path) {
  const auto& split_path = SplitBlobPath(blob_path);
  if (!split_path.ok()) {
    return absl::InvalidArgumentError(
        absl::StrCat("Invalid path", split_path.status().message()));
  }

  try {
    const absl::Time start_time = absl::Now();
    auto reader = gcs_client->ReadObject(std::string(split_path->first),
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
    std::cout << "Read " << blob_path << " (" << *content_length << "B) in "
              << (end_time - start_time) << std::endl;
    return result;
  } catch (const std::exception& e) {
    // Unfortunately the googe-cloud-storage library throws exceptions.
    return absl::InternalError(absl::StrCat("Exception during reading of ",
                                            blob_path, ": ", e.what()));
  }
}

class ScanServiceImpl final : public cpg::ScanService::Service {
  grpc::Status Load(grpc::ServerContext* const context,
                    const cpg::LoadRequest* const request,
                    cpg::LoadResponse* const response) override {
    auto gcs_client = google::cloud::storage::Client::CreateDefaultClient();
    if (!gcs_client.ok()) {
      return grpc::Status(grpc::StatusCode::INTERNAL,
                          absl::StrCat("Failed to create GCS client: ",
                                       gcs_client.status().message()));
    }

    for (const auto& blob_path : request->blob_paths()) {
      const auto blob = ReadBlob(&*gcs_client, blob_path);
      if (!blob.ok()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            absl::StrCat("Failed to read blob ", blob_path,
                                         ": ", blob.status().message()));
      }

      // TODO(@lgruen): read into Parquet table.
      // const auto buffer_reader = arrow::io::BufferReader(*blob);
      response->add_blob_sizes(blob->size());
    }

    return grpc::Status::OK;
  }
};

void RunServer(const int port) {
  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();

  const std::string server_address = absl::StrCat("[::]:", port);
  std::cout << "Starting server on " << server_address << std::endl;
  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  ScanServiceImpl scan_service;
  builder.RegisterService(&scan_service);

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
