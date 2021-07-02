ARG BASE_IMAGE
FROM $BASE_IMAGE as builder

COPY src /src

RUN cd /src && \
    mkdir build && \
    cd build && \
    cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_STANDARD=20 && \
    make -j8

RUN /build/extract-elf-so --cert /src/build/main && \
    mkdir /build/rootfs && cd /build/rootfs && \
    tar xf /build/rootfs.tar

FROM scratch

COPY --from=builder /build/rootfs /

CMD ["/usr/local/bin/main"]
