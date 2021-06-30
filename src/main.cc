#include <iostream>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "cpp-httplib/httplib.h"

// TODO(@lgruen): Pass access token for Authorization header.
std::string ReadBlob(const std::string& path) {
  httplib::Client gcs_client("https://storage.googleapis.com");
  gcs_client.set_url_encode(true);
  const std::string url = std::string("/") + path + "?alt=media";
  const auto res = gcs_client.Get(url.c_str());
  if (res) {
    return res->body;
  }
  std::cerr << "Failed to read blob (" << path
            << "), error code: " << res.error() << std::endl;
  return {};
}

int main(int argc, char** argv) {
  // TODO(@lgruen): Add glob or similar.
  static constexpr char blob_path[] = "bucket/path";
  const std::string blob = ReadBlob(blob_path);
  std::cout << "blob size: " << blob.size() << std::endl;
  return 0;
}
