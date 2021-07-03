# arrow-in-mem

Experiments with in-memory Arrow datasets.

## Build

Build the "base" image, which contains library dependencies that rarely need updating,
but take a while to build, like `absl`, `google-cloud-cpp`, and `arrow`.

```bash
docker build --tag arrow-in-mem-base -f Dockerfile.base .
```

To compile the application, the above base image is used, which cuts down the
compilation time for incremental changes significantly:

```bash
docker build --build-arg BASE_IMAGE=arrow-in-mem-base --tag arrow-in-mem .
```

The resulting image uses a multi-stage build to reduce the image size, only copying the
executable and required shared library binaries.

## Run

Download a JSON key for a GCP service account, then mount that in the container:

```bash
docker run -e PORT=80 -e GOOGLE_APPLICATION_CREDENTIALS=/gsa-key/key.json -v $HOME/Downloads/key.json:/gsa-key/key.json -p 8080:80
```

## Requests

Use the `grpc_cli` tool to send RPC requests to the server:

```bash
# TODO(@lgruen)
grpc_cli
```
