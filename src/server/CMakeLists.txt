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

find_library(TCMALLOC_LIB NAMES tcmalloc)
if(TCMALLOC_LIB)
   message("Found TCMALLOC_LIB: ${TCMALLOC_LIB}")
else()
   message(FATAL_ERROR "TCMALLOC_LIB library not found")
endif()

add_executable(server ${SOURCES})

target_compile_options(server PRIVATE -Wall -Werror)

target_link_libraries(server PRIVATE
    absl::base
    absl::status
    absl::statusor
    absl::strings
    # We're linking Arrow statically due to
    # https://issues.apache.org/jira/browse/ARROW-13256.
    arrow_static
    arrow_dataset
    google-cloud-cpp::storage
    gRPC::grpc++_reflection
    proto
    ${TCMALLOC_LIB}
)
