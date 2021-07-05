find_package(Threads)

set(SOURCES
    main.cc
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})

find_package(absl REQUIRED)
find_package(protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(google_cloud_cpp_storage REQUIRED)
find_package(Arrow REQUIRED)

add_executable(server ${SOURCES})
target_link_libraries(server PRIVATE
    absl::base
    absl::status
    absl::statusor
    absl::strings
    arrow_shared
    google-cloud-cpp::storage
    gRPC::grpc++_reflection
    proto
)
