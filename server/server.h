#pragma once

#include <absl/status/statusor.h>
#include <grpcpp/grpcpp.h>

#include <memory>

#include "url_reader.h"

namespace seqr {

absl::StatusOr<std::unique_ptr<grpc::Server>> CreateServer(
    int port, const UrlReader& url_reader);

}  // namespace seqr