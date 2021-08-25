# seqr query backend

An experimental backend that could be an alternative to the Elasticsearch deployment that seqr currently uses.

## Build

Build the "base" image, which contains library dependencies that rarely need updating,
but take a while to build, like `absl`, `google-cloud-cpp`, and `arrow`.

```bash
docker build --tag seqr-query-backend-base -f Dockerfile.base .
```

To compile the application, the above base image is used, which cuts down the
compilation time for incremental changes significantly:

```bash
docker build --build-arg BASE_IMAGE=seqr-query-backend-base --tag seqr-query-backend .
```

The resulting image uses a multi-stage build to reduce the image size, only copying the
executable and required shared library binaries.

## Local testing

```bash
docker run --init -it -e PORT=8080 -p 8080:8080 seqr-query-backend
```

### Test client

```bash
pip3 install -r src/client/requirements.txt

src/client/client.py --arrow_urls_file=$HOME/arrow_urls.txt
```

## Cloud Run deployment

```bash
gcloud config set project seqr-308602

IMAGE=australia-southeast1-docker.pkg.dev/seqr-308602/seqr-project/seqr-query-backend:latest
docker tag seqr-query-backend $IMAGE
docker push $IMAGE

gcloud run deploy --region=australia-southeast1 --no-allow-unauthenticated --concurrency=1 --max-instances=100 --cpu=4 --memory=8Gi --service-account=seqr-query-backend@seqr-308602.iam.gserviceaccount.com --image=$IMAGE seqr-query-backend
```

### Test client

Either set the `GOOGLE_APPLICATION_CREDENTIALS` environment variable or run the client from a Compute Engine VM. The associated service account needs to invoker permissions for the Cloud Run deployment.

```bash
cd src/client
./client.py --cloud_run_url=$(gcloud run services describe seqr-query-backend --platform managed --region australia-southeast1 --format 'value(status.url)') --blob_paths_file=blob_paths.txt
```

For gRPC debugging, the following environment variables are helpful:

```bash
export GRPC_VERBOSITY=DEBUG
export GRPC_TRACE=secure_endpoint
```

## Debug build

This drops into `lldb`:

```bash
docker build --build-arg CMAKE_BUILD_TYPE=Debug --tag seqr-query-backend-base-debug -f Dockerfile.base .

docker build --build-arg BASE_IMAGE=seqr-query-backend-base-debug --tag seqr-query-backend-debug -f Dockerfile.debug .

docker run --privileged --init -it -e PORT=8080 -p 8080:8080 seqr-query-backend-debug
```
