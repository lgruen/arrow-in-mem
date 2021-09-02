# seqr query backend

An experimental backend that could be an alternative to the Elasticsearch deployment that seqr currently uses.

To convert the annotated Hail tables to the [Apache Arrow](https://arrow.apache.org/) format that this backend uses, see the [`pipeline`](pipeline) directory.

## Local testing

The `base` stage in the [`Dockerfile`](Dockerfile) takes a while to build, which is why
using Docker caching is worthwhile to usually skip right to the `server` stage:

```bash
gcloud config set project seqr-308602

gcloud auth configure-docker australia-southeast1-docker.pkg.dev

IMAGE=australia-southeast1-docker.pkg.dev/seqr-308602/seqr-project/seqr-query-backend:latest

DOCKER_BUILDKIT=1 docker build --build-arg BUILDKIT_INLINE_CACHE=1 --tag seqr-query-backend --cache-from=$IMAGE .

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

DOCKER_BUILDKIT=1 docker build --build-arg BUILDKIT_INLINE_CACHE=1 --build-arg CMAKE_BUILD_TYPE=Debug --target server --cache-from=$IMAGE --tag seqr-query-backend-debug .

docker run --privileged --init -it -e PORT=8080 -p 8080:8080 seqr-query-backend-debug
```

Within the container, run the server in `lldb`:

```bash
lldb /src/build/server/server
```

## Cloud Run deployment

```bash
IMAGE=australia-southeast1-docker.pkg.dev/seqr-308602/seqr-project/seqr-query-backend:latest

gcloud run deploy --region=australia-southeast1 --no-allow-unauthenticated --concurrency=1 --max-instances=100 --cpu=4 --memory=8Gi --service-account=seqr-query-backend@seqr-308602.iam.gserviceaccount.com --image=$IMAGE seqr-query-backend
```

To use the test client with the Cloud Run deployment, either set the
`GOOGLE_APPLICATION_CREDENTIALS` environment variable or run the client from a Compute
Engine VM. The associated service account needs to have invoker permissions for the
Cloud Run deployment.

```bash
CLOUD_RUN_URL=$(gcloud run services describe seqr-query-backend --platform managed --region australia-southeast1 --format 'value(status.url)')

cd src/client

./client_cli.py --query_text_proto_file=example_query.textproto --cloud_run_url=$CLOUD_RUN_URL
```

For gRPC debugging, the following environment variables are helpful:

```bash
export GRPC_VERBOSITY=DEBUG
export GRPC_TRACE=secure_endpoint
```
