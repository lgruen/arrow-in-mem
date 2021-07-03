#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/status/statusor.h>
#include <absl/strings/str_cat.h>
#include <arrow/io/memory.h>
#include <google/cloud/storage/client.h>

#include <iostream>
#include <string_view>

ABSL_FLAG(std::string, blob_bucket, "gcp-public-data--gnomad",
          "the bucket name of the GCS blob to download");

ABSL_FLAG(
    std::string, blob_path,
    "release/3.1.1/vcf/genomes/gnomad.genomes.v3.1.1.sites.chrY.vcf.bgz.tbi",
    "the path within the bucket of the GCS blob to download");

namespace gcs = google::cloud::storage;

// Returns the blob contents of gs://bucket_name/path.
absl::StatusOr<std::string> ReadBlob(gcs::Client* const client,
                                     const std::string_view bucket_name,
                                     const std::string_view path) {
  const auto reader =
      client->ReadObject(std::string(bucket_name), std::string(path));
  if (!reader) {
    return absl::UnknownError(absl::StrCat("Failed to read blob gs://",
                                           bucket_name, "/", path, ": ",
                                           reader.status().message()));
  }

  const auto& headers = reader.headers();
  for (const auto& kv : headers) {
    std::cout << kv.first << ", " << kv.second << std::endl;
  }

  return std::string();
}

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);

  auto client = gcs::Client::CreateDefaultClient();
  if (!client) {
    std::cerr << "Failed to create GCS client: " << client.status()
              << std::endl;
    return 1;
  }

  const auto blob = ReadBlob(&(*client), absl::GetFlag(FLAGS_blob_bucket),
                             absl::GetFlag(FLAGS_blob_path));
  if (!blob.ok()) {
    std::cerr << blob.status() << std::endl;
    return 1;
  }

  std::cout << "blob size: " << blob->size() << std::endl;
  const auto buffer_reader = arrow::io::BufferReader(*blob);
  // TODO(@lgruen): read Parquet table.

  return 0;
}
