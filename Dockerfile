FROM alpine:3.14 AS builder

RUN apk add g++ clang openssl-dev openssl-libs-static

WORKDIR /build
COPY src /build
RUN clang++ --std=c++20 -Wall -O3 -static -o main main.cc -lssl -lcrypto

FROM alpine:3.14

COPY --from=builder /build/main .

CMD ["./main"]
