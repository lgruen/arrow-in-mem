#!/usr/bin/env python3

import click
import os
import google.auth.transport.requests
import google.auth.transport.grpc
import scan_service_pb2
import scan_service_pb2_grpc


def _remove_prefix(s: str, prefix: str) -> str:
    return s[len(prefix) :] if s.startswith(prefix) else s


def _get_id_token_credentials(auth_request, audience):
    google_application_credentials = os.getenv('GOOGLE_APPLICATION_CREDENTIALS')
    if google_application_credentials:
        from google.oauth2 import service_account

        credentials = service_account.IDTokenCredentials.from_service_account_file(
            google_application_credentials, target_audience=audience
        )
    else:
        from google.auth import compute_engine

        credentials = compute_engine.IDTokenCredentials(
            auth_request, audience, use_metadata_identity_endpoint=True
        )

    credentials.refresh(auth_request)
    return credentials


@click.command()
@click.option(
    '--cloud_run_url',
    required=True,
    help='Cloud Run deployment URL, like https://arrow-in-mem-3bbubtd33q-ts.a.run.app',
)
@click.option(
    '--blob_paths_file',
    required=True,
    help='A file with one line per blob to fetch, with entries like "gs://some/blob"',
)
def main(cloud_run_url, blob_paths_file):
    cloud_run_domain = _remove_prefix(cloud_run_url, 'https://')

    auth_request = google.auth.transport.requests.Request()
    credentials = _get_id_token_credentials(auth_request, cloud_run_url)

    channel = google.auth.transport.grpc.secure_authorized_channel(
        credentials, auth_request, f'{cloud_run_domain}:443'
    )

    stub = scan_service_pb2_grpc.ScanServiceStub(channel)

    request = scan_service_pb2.LoadRequest()
    with open(blob_paths_file) as f:
        blob_paths = [line.strip() for line in f]
    request.blob_paths.extend(blob_paths)

    response = stub.Load(request)
    for blob_path, blob_size in zip(blob_paths, response.blob_sizes):
        print(f'{blob_path}: {blob_size}')


if __name__ == '__main__':
    main()