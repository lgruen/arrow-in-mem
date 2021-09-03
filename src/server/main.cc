#include <cstdlib>

#include "server.h"

int main(int argc, char** argv) {
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

  auto server = seqr::CreateServer(port);
  if (!server.ok()) {
    std::cerr << server.status() << std::endl;
    return 1;
  }

  (*server)->Wait();

  return 0;
}
