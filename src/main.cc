#include <gflags/gflags.h>
#include <glog/logging.h>

#include <iostream>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "cpp-httplib/httplib.h"

DEFINE_string(cloud_storage_access_token, "",
              "GCS access token, e.g. `gcloud auth print-access-token`");
DEFINE_string(blob_path,
              "gcp-public-data--gnomad/release/3.1.1/vcf/genomes/"
              "gnomad.genomes.v3.1.1.sites.chrY.vcf.bgz.tbi",
              "the path to the GCS blob to download");

std::string ReadBlob(const std::string& path) {
  httplib::Client gcs_client("https://storage.googleapis.com");
  gcs_client.set_url_encode(true);
  const std::string url = std::string("/") + path + "?alt=media";
  const httplib::Headers headers = {
      {"Authorization",
       std::string("Bearer ") + FLAGS_cloud_storage_access_token}};
  const auto res = gcs_client.Get(url.c_str(), headers);
  if (res) {
    return res->body;
  }
  std::cerr << "Failed to read blob (" << path
            << "), error code: " << res.error() << std::endl;
  return {};
}

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  const std::string blob = ReadBlob(FLAGS_blob_path);
  std::cout << "blob size: " << blob.size() << std::endl;
  return 0;
}
