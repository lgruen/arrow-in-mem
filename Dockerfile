ARG BASE_IMAGE
FROM $BASE_IMAGE as builder

COPY src /src

RUN mkdir -p /src/build && cd /src/build && \
    cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=20 && \
    make -j8

RUN /build/extract-elf-so --cert /src/build/server/server && \
    mkdir /rootfs && cd /rootfs && \
    tar xf /rootfs.tar

FROM scratch

COPY --from=builder /rootfs /

CMD ["/usr/local/bin/server"]
