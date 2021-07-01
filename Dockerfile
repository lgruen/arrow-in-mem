FROM alpine:3.14 as builder

RUN apk add g++ clang make cmake openssl-dev openssl-libs-static linux-headers

WORKDIR /app
COPY src /app/src

RUN mkdir build && \
    cd build && \
    CXX=clang++ cmake ../src -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=FALSE -DOPENSSL_USE_STATIC_LIBS=TRUE && \
    make -j8

FROM alpine:3.14

COPY --from=builder /app/build/main .
