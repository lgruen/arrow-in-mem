#include <absl/status/statusor.h>
#include <absl/strings/str_cat.h>
#include <absl/strings/strip.h>
#include <arrow/io/memory.h>
#include <google/cloud/storage/client.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <iostream>
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

class ScanServiceImpl final : public cpg::ScanService::Service {
  grpc::Status Load(grpc::ServerContext* const context,
                    const cpg::LoadRequest* const request,
                    cpg::LoadReply* const reply) override {
    std::cout << "Load..." << std::endl;
    std::cout << "Before client..." << std::endl;
    auto gcs_client = google::cloud::storage::Client::CreateDefaultClient();
    std::cout << "After client..." << std::endl;
    if (!gcs_client) {
      std::cerr << "x3" << std::endl;
      return grpc::Status(grpc::StatusCode::INTERNAL,
                          absl::StrCat("Failed to create GCS client: ",
                                       gcs_client.status().message()));
    }
    std::cerr << "x1" << std::endl;

    for (const auto& blob_path : request->blob_path()) {
      std::cerr << "x2" << std::endl;
      std::cerr << blob_path << std::endl;
      const auto& split_path = SplitBlobPath(blob_path);
      if (!split_path.ok()) {
        return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                            absl::StrCat("Invalid blob path ", blob_path, ": ",
                                         split_path.status().message()));
      }
      std::cerr << "x3" << std::endl;
      std::cerr << "x5" << std::string(split_path->first) << ", "
                << std::string(split_path->second) << std::endl;

      try {
        const auto reader = gcs_client->ReadObject(
            std::string(split_path->first), std::string(split_path->second));
        std::cerr << "x4" << std::endl;
        if (!reader) {
          return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                              absl::StrCat("Failed to read blob ", blob_path,
                                           ": ", reader.status().message()));
        }

        std::cerr << "x5" << std::endl;
        std::cout << "Headers for blob " << blob_path << ":" << std::endl;
        const auto& headers = reader.headers();
        for (const auto& kv : headers) {
          std::cout << "  " << kv.first << ", " << kv.second << std::endl;
        }
      } catch (const std::exception& e) {
        std::cerr << "oh noes: " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "oh noooooes: " << std::endl;
      }
    }

    // TODO(@lgruen): read Parquet table.
    // const auto buffer_reader = arrow::io::BufferReader(*blob);
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
