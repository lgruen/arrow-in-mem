#pragma once

#include <absl/status/statusor.h>
#include <grpcpp/grpcpp.h>

#include <memory>

namespace seqr {

absl::StatusOr<std::unique_ptr<grpc::Server>> CreateServer(int port);

}  // namespace seqr