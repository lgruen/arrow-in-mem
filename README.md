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

```bash
gcloud config set project leo-dev-290304

IMAGE=australia-southeast1-docker.pkg.dev/leo-dev-290304/ar-sydney/arrow-in-mem:latest
docker tag arrow-in-mem $IMAGE
docker push $IMAGE

gcloud run deploy --region=australia-southeast1 --no-allow-unauthenticated --concurrency=1 --max-instances=100 --cpu=4 --memory=8Gi --service-account=arrow-in-mem@leo-dev-290304.iam.gserviceaccount.com --image=$IMAGE arrow-in-mem
```

## Client

Either set the `GOOGLE_APPLICATION_CREDENTIALS` environment variable or run the client from a Compute Engine VM. The associated service account needs to invoker permissions for the Cloud Run deployment.

```bash
cd src/client
./client.py --cloud_run_url=$(gcloud run services describe arrow-in-mem --platform managed --region australia-southeast1 --format 'value(status.url)') --blob_paths=gs://some/path,gs://another/path
```

For gRPC debugging, the following environment variables are helpful:

```bash
export GRPC_VERBOSITY=DEBUG
export GRPC_TRACE=secure_endpoint
```
