cmake_minimum_required(VERSION 3.5)

project(arrow_in_mem)

set(CMAKE_CXX_STANDARD 20)
set(OPENSSL_USE_STATIC_LIBS TRUE)

find_package(absl REQUIRED)
find_package(Arrow REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(main main.cc)

target_link_libraries(main -static
    absl::base
    absl::flags
    absl::flags_parse
    absl::status
    absl::statusor
    absl::strings
    arrow_static
    OpenSSL::SSL)