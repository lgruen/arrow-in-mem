#!/usr/bin/env python3

import os
import sys

import scan_service_pb2
import scan_service_pb2_grpc

import google.auth
import google.auth.transport.requests
import google.auth.transport.grpc

CLOUD_RUN_URL = os.getenv('CLOUD_RUN_URL')
assert CLOUD_RUN_URL
CLOUD_RUN_DOMAIN = CLOUD_RUN_URL.removeprefix('https://')

assert len(sys.argv) > 1

credentials, _ = google.auth.default()
auth_req = google.auth.transport.requests.Request()
channel = google.auth.transport.grpc.secure_authorized_channel(credentials, auth_req, f'{CLOUD_RUN_DOMAIN}:443')
stub = scan_service_pb2_grpc.ScanServiceStub(channel)
req = scan_service_pb2.LoadRequest()
req.blob_path.extend(sys.argv[1:])
resp = stub.Load(req)

