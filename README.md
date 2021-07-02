# arrow-in-mem

Experiments with in-memory Arrow datasets

## Build

```bash
docker build --tag arrow-in-mem-base -f Dockerfile.base .

# Application code changes don't require recompiling all the libraries in the base image.
docker build --build-arg BASE_IMAGE=arrow-in-mem-base --tag arrow-in-mem .
```
