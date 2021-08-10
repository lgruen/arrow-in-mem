#!/usr/bin/env python3

import os
import sys

import scan_service_pb2
import scan_service_pb2_grpc

import google.auth.compute_engine
import google.auth.transport.requests
import google.auth.transport.grpc


CLOUD_RUN_URL = os.getenv('CLOUD_RUN_URL')
assert CLOUD_RUN_URL

def remove_prefix(s: str, prefix: str) -> str:
    return s[len(prefix):] if s.startswith(prefix) else s

CLOUD_RUN_DOMAIN = remove_prefix(CLOUD_RUN_URL, 'https://')

assert len(sys.argv) > 1

auth_req = google.auth.transport.requests.Request()
credentials = google.auth.compute_engine.IDTokenCredentials(
    auth_req, CLOUD_RUN_URL, use_metadata_identity_endpoint=True
)
credentials.refresh(auth_req)
channel = google.auth.transport.grpc.secure_authorized_channel(credentials, auth_req, f'{CLOUD_RUN_DOMAIN}:443')
stub = scan_service_pb2_grpc.ScanServiceStub(channel)
req = scan_service_pb2.LoadRequest()
req.blob_path.extend(sys.argv[1:])
resp = stub.Load(req)
