#include <arrow/io/memory.h>

#include <iostream>
#include <string_view>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/status/statusor.h"
#include "absl/strings/str_cat.h"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "cpp-httplib/httplib.h"

ABSL_FLAG(std::string, cloud_storage_access_token, "",
          "GCS access token, e.g. `gcloud auth print-access-token`");
ABSL_FLAG(std::string, blob_path,
          "gcp-public-data--gnomad/release/3.1.1/vcf/genomes/"
          "gnomad.genomes.v3.1.1.sites.chrY.vcf.bgz.tbi",
          "the path to the GCS blob to download");

absl::StatusOr<std::string> ReadBlob(const std::string_view path) {
  httplib::Client gcs_client("https://storage.googleapis.com");
  gcs_client.set_url_encode(true);
  const std::string url = absl::StrCat("/", path, "?alt=media");
  const httplib::Headers headers = {
      {"Authorization",
       absl::StrCat("Bearer ",
                    absl::GetFlag(FLAGS_cloud_storage_access_token))}};
  const auto res = gcs_client.Get(url.c_str(), headers);
  if (res) {
    return res->body;
  }
  return absl::UnknownError(absl::StrCat("Failed to read blob ", path,
                                         ", error code: ", res.error()));
}

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);

  const auto blob = ReadBlob(absl::GetFlag(FLAGS_blob_path));
  if (!blob.ok()) {
    std::cerr << blob.status() << std::endl;
    return 1;
  }

  std::cout << "blob size: " << blob->size() << std::endl;
  const auto buffer_reader = arrow::io::BufferReader(*blob);
  // TODO(@lgruen): read Parquet table.

  return 0;
}
