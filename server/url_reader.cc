#include "url_reader.h"

#include <absl/flags/declare.h>
#include <absl/flags/flag.h>
#include <absl/strings/strip.h>
#include <google/cloud/storage/client.h>

ABSL_DECLARE_FLAG(int, num_threads);

namespace seqr {

namespace gcs = google::cloud::storage;

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

class GcsReader : public UrlReader {
 public:
  absl::StatusOr<std::string> Read(const std::string_view url) const override {
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
          absl::GetFlag(FLAGS_num_threads))};
};

}  // namespace

absl::StatusOr<std::unique_ptr<UrlReader>> MakeFileReader() {
  return absl::UnimplementedError("FileReader is not implemented");
}

absl::StatusOr<std::unique_ptr<UrlReader>> MakeGcsReader() {
  return std::make_unique<GcsReader>();
}

}  // namespace seqr
