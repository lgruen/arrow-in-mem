#!/usr/bin/env python3

import click
import os
import google.auth.transport.requests
import google.auth.transport.grpc
import seqr_query_service_pb2
import seqr_query_service_pb2_grpc


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
    help='Cloud Run deployment URL, like https://seqr-query-backend-3bbubtd33q-ts.a.run.app',
)
@click.option(
    '--arrow_urls_file',
    required=True,
    help='A file with one line per URL to fetch, with entries like "gs://some/blob"',
)
def main(cloud_run_url, arrow_urls_file):
    cloud_run_domain = _remove_prefix(cloud_run_url, 'https://')

    auth_request = google.auth.transport.requests.Request()
    credentials = _get_id_token_credentials(auth_request, cloud_run_url)

    channel = google.auth.transport.grpc.secure_authorized_channel(
        credentials, auth_request, f'{cloud_run_domain}:443'
    )

    stub = seqr_query_service_pb2_grpc.QueryServiceStub(channel)

    request = seqr_query_service_pb2.QueryRequest()
    with open(arrow_urls_file) as f:
        arrow_urls = [line.strip() for line in f]
    request.arrow_urls.extend(arrow_urls)

    response = stub.Query(request)
    for arrow_url, num_rows in zip(arrow_urls, response.num_rows):
        print(f'{arrow_url}: {num_rows} rows')


if __name__ == '__main__':
    main()