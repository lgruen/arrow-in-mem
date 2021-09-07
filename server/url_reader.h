#pragma once

#include <absl/status/statusor.h>

#include <memory>
#include <string_view>

namespace seqr {

class UrlReader {
 public:
  virtual ~UrlReader() = default;

  virtual absl::StatusOr<std::string> Read(std::string_view url) const = 0;
};

// Reads from a standard file system.
absl::StatusOr<std::unique_ptr<UrlReader>> MakeFileReader();

// Reads from Google Cloud Storage.
absl::StatusOr<std::unique_ptr<UrlReader>> MakeGcsReader();

}  // namespace seqr
