FROM alpine:3.14 as builder

RUN apk add \
    autoconf \
    bash \
    clang \
    cmake \
    g++ \
    linux-headers \
    make \
    openssl-dev \
    openssl-libs-static

# Build absl. We're installing into a system location as this is an isolated container
# environment.
COPY src/abseil-cpp /abseil-cpp
RUN cd /abseil-cpp && \
    mkdir build && \
    cd build && \
    cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_CXX_STANDARD=20 \
    -DBUILD_SHARED_LIBS=FALSE && \
    make -j8 install

# Build arrow.
COPY src/arrow /arrow
RUN cd /arrow/cpp && \
    mkdir build && \
    cd build && \
    cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_CXX_STANDARD=20 \
    -DARROW_BUILD_SHARED=OFF \
    -DARROW_PARQUET=ON \
    -DARROW_WITH_SNAPPY=ON && \
    make -j8 install

# Build the executable. Keeping the library dependencies as separate layers allows
# Docker to cache results when only the application code is changed.
COPY src /src
RUN cd /src && \
    mkdir build && \
    cd build && \
    cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ && \
    make -j8

FROM alpine:3.14

# Copy the statically built executable into a fresh base image.
COPY --from=builder /src/build/main .
