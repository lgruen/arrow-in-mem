FROM debian:bullseye-slim as builder

RUN apt update && \
    apt install -y ca-certificates lsb-release wget clang pkg-config libgflags-dev libgoogle-glog-dev && \
    wget https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
    apt install -y ./apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
    apt update && \
    apt install -y libarrow-dev libparquet-dev

WORKDIR /build
COPY src /build

# To avoid a segfault with a statically linked pthread, we use "--whole-archive":
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=52590
RUN clang++ --std=c++20 -Wall -O3 -static -o main main.cc -lssl -lcrypto -ldl -Wl,--whole-archive -lpthread -Wl,--no-whole-archive -lgflags -lglog

FROM debian:bullseye-slim

# ca-certificates is required for server certificate verification.
RUN apt update && apt install -y ca-certificates

COPY --from=builder /build/main .
