cmake_minimum_required(VERSION 3.5)

project(arrow_in_mem)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(abseil-cpp)

find_package(OpenSSL REQUIRED)

add_executable(main main.cc)

target_link_libraries(main
    absl::base -static
    absl::flags
    absl::flags_parse
    absl::status
    absl::statusor
    absl::strings
    OpenSSL::SSL)