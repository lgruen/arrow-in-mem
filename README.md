# seqr query backend

An experimental backend that could be an alternative to the Elasticsearch deployment that seqr currently uses.

Implements a simple [query API](src/proto/seqr_query_service.proto) using [gRPC](https://grpc.io/).

To convert seqr's annotated Hail tables to the [Apache Arrow](https://arrow.apache.org/) format that this backend uses, see the [`pipeline`](pipeline) directory.

## Docker stages

To reduce image build times and reduce final image size, the build is split into
multiple stages:

- `base`: build dependencies
- `server`: compilation of the server application and tests
- `deploy`: stripped down image for deployment

To cache the intermediate stages to the registry, we use
[BuildKit](https://medium.com/titansoft-engineering/docker-build-cache-sharing-on-multi-hosts-with-buildkit-and-buildx-eb8f7005918e)
in the [GitHub Actions workflow](.github/workflows/deploy.yaml).

## Local testing

```bash
gcloud config set project seqr-308602

gcloud auth configure-docker australia-southeast1-docker.pkg.dev

docker buildx create --name builder --use

IMAGE=australia-southeast1-docker.pkg.dev/seqr-308602/seqr-project/seqr-query-backend:dev

docker buildx build --tag=seqr-query-backend --cache-from=type=registry,ref=$IMAGE --load .

docker run --init -it -e PORT=8080 -p 8080:8080 seqr-query-backend
```

In another terminal, run the client:

```bash
cd src/client

pip3 install -r requirements.txt

./client_cli.py --query_text_proto_file=example_query.textproto
```

For debug builds, run:

```bash
IMAGE=australia-southeast1-docker.pkg.dev/seqr-308602/seqr-project/seqr-query-backend:debug

docker buildx build --tag=seqr-query-backend-debug --build-arg=CMAKE_BUILD_TYPE=Debug --target=server --cache-from=type=registry,ref=$IMAGE .

docker run --privileged --init -it -e PORT=8080 -p 8080:8080 seqr-query-backend-debug
```

Within the container, run the server in `lldb`:

```bash
lldb /src/build/server/seqr_query_backend
```

## Cloud Run deployment

To use the test client with the Cloud Run deployment, either set the
`GOOGLE_APPLICATION_CREDENTIALS` environment variable or run the client from a Compute
Engine VM. The associated service account needs to have invoker permissions for the
Cloud Run deployment.

```bash
CLOUD_RUN_URL=$(gcloud run services describe seqr-query-backend-dev --platform=managed --region=australia-southeast1 --format='value(status.url)')

cd src/client

./client_cli.py --query_text_proto_file=example_query.textproto --cloud_run_url=$CLOUD_RUN_URL
```

For gRPC debugging, the following environment variables are helpful:

```bash
export GRPC_VERBOSITY=DEBUG
export GRPC_TRACE=secure_endpoint
```
