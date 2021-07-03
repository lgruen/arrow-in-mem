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

#include "scan_service.pb.h"

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
  Status Load(grpc::ServerContext* const context,
              const cpg::LoadRequest* const request,
              cpg::LoadReply* constreply) override {
    auto gcs_client = google::cloud::storage::Client::CreateDefaultClient();
    if (!gcs_client) {
      std::cerr << "Failed to create GCS client: " << gcs_client.status()
                << std::endl;
      return 1;
    }

    for (const auto& blob_path : request->blob_path()) {
      const auto& split_path = SplitBlobPath(blob_path);
      if (!split_path.ok()) {
        std::cerr << "Invalid blob path " << blob_path << ": "
                  << split_path.status() << std::endl;
        return grpc::Status::INVALID_ARGUMENT;
      }

      const auto reader =
          client->ReadObject(split_path.first, split_path.second);
      if (!reader) {
        std::cerr << "Failed to read blob " << blob_path << ": "
                  << reader.status().message();
        return grpc::Status::INVALID_ARGUMENT;
      }

      std::cout << "Headers for blob " << blob_path << ":" << std::endl;
      const auto& headers = reader.headers();
      for (const auto& kv : headers) {
        std::cout << "  " << kv.first << ", " << kv.second << std::endl;
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
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  ServerBuilder builder;
  ScanServiceImpl scan_service;
  builder.RegisterService(&scan_service);

  builder.BuildAndStart()->Wait();
}

}  // namespace

int main(int argc, char** argv) {
  // Get the port number from the environment.
  const std::string_view = std::getenv("PORT");
  if (!port_env) {
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
