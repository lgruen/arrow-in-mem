cmake_minimum_required(VERSION 3.5)

project(arrow_in_mem)

find_package(absl REQUIRED)
find_package(Arrow REQUIRED)
find_package(google_cloud_cpp_storage REQUIRED)
find_package(httplib REQUIRED)

add_executable(main main.cc)

target_link_libraries(main PRIVATE
    absl::base
    absl::flags
    absl::flags_parse
    absl::status
    absl::statusor
    absl::strings
    arrow_static
    google-cloud-cpp::storage
    httplib::httplib)
