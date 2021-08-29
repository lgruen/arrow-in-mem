#!/usr/bin/env python3

import click
import os
import seqr_query_service_pb2
import seqr_query_service_pb2_grpc
import time
import pyarrow


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
    '--query_text_proto_file',
    required=True,
    help='File name to a QueryRequest protobuf in text format (not binary!).',
)
@click.option(
    '--cloud_run_url',
    help='Cloud Run deployment URL, like https://seqr-query-backend-3bbubtd33q-ts.a.run.app. If not given, will attempt to connect to localhost:8080, which is useful for debugging.',
)
def main(query_text_proto_file, cloud_run_url):
    if cloud_run_url:
        cloud_run_domain = _remove_prefix(cloud_run_url, 'https://')

        import google.auth.transport.requests

        auth_request = google.auth.transport.requests.Request()
        credentials = _get_id_token_credentials(auth_request, cloud_run_url)

        import google.auth.transport.grpc

        channel = google.auth.transport.grpc.secure_authorized_channel(
            credentials, auth_request, f'{cloud_run_domain}:443'
        )
    else:
        import grpc

        channel = grpc.insecure_channel('localhost:8080')

    stub = seqr_query_service_pb2_grpc.QueryServiceStub(channel)

    with open(query_text_proto_file, 'rt') as text_proto:
        import google.protobuf.text_format

        request = google.protobuf.text_format.Parse(
            text_proto.read(), seqr_query_service_pb2.QueryRequest()
        )

    response = stub.Query(request)

    print(f'number of rows: {response.num_rows}')

    start_time = time.time()
    buffer = pyarrow.py_buffer(response.record_batches)
    table = pyarrow.ipc.RecordBatchFileReader(buffer).read_all()
    end_time = time.time()
    print(f'Table deserialization took {end_time - start_time}s')

    assert table.num_rows() == response.num_rows()


if __name__ == '__main__':
    main()