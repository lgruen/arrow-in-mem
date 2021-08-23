# -*- coding: utf-8 -*-
# Generated by the protocol buffer compiler.  DO NOT EDIT!
# source: seqr_query_service.proto
"""Generated protocol buffer code."""
from google.protobuf import descriptor as _descriptor
from google.protobuf import message as _message
from google.protobuf import reflection as _reflection
from google.protobuf import symbol_database as _symbol_database
# @@protoc_insertion_point(imports)

_sym_db = _symbol_database.Default()




DESCRIPTOR = _descriptor.FileDescriptor(
  name='seqr_query_service.proto',
  package='seqr',
  syntax='proto3',
  serialized_options=None,
  create_key=_descriptor._internal_create_key,
  serialized_pb=b'\n\x18seqr_query_service.proto\x12\x04seqr\"L\n\x0cQueryRequest\x12\x12\n\narrow_urls\x18\x01 \x03(\t\x12\x18\n\x0bmax_results\x18\x02 \x01(\x03H\x00\x88\x01\x01\x42\x0e\n\x0c_max_results\"!\n\rQueryResponse\x12\x10\n\x08num_rows\x18\x01 \x03(\x03\x32\x42\n\x0cQueryService\x12\x32\n\x05Query\x12\x12.seqr.QueryRequest\x1a\x13.seqr.QueryResponse\"\x00\x62\x06proto3'
)




_QUERYREQUEST = _descriptor.Descriptor(
  name='QueryRequest',
  full_name='seqr.QueryRequest',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='arrow_urls', full_name='seqr.QueryRequest.arrow_urls', index=0,
      number=1, type=9, cpp_type=9, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
    _descriptor.FieldDescriptor(
      name='max_results', full_name='seqr.QueryRequest.max_results', index=1,
      number=2, type=3, cpp_type=2, label=1,
      has_default_value=False, default_value=0,
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
    _descriptor.OneofDescriptor(
      name='_max_results', full_name='seqr.QueryRequest._max_results',
      index=0, containing_type=None,
      create_key=_descriptor._internal_create_key,
    fields=[]),
  ],
  serialized_start=34,
  serialized_end=110,
)


_QUERYRESPONSE = _descriptor.Descriptor(
  name='QueryResponse',
  full_name='seqr.QueryResponse',
  filename=None,
  file=DESCRIPTOR,
  containing_type=None,
  create_key=_descriptor._internal_create_key,
  fields=[
    _descriptor.FieldDescriptor(
      name='num_rows', full_name='seqr.QueryResponse.num_rows', index=0,
      number=1, type=3, cpp_type=2, label=3,
      has_default_value=False, default_value=[],
      message_type=None, enum_type=None, containing_type=None,
      is_extension=False, extension_scope=None,
      serialized_options=None, file=DESCRIPTOR,  create_key=_descriptor._internal_create_key),
  ],
  extensions=[
  ],
  nested_types=[],
  enum_types=[
  ],
  serialized_options=None,
  is_extendable=False,
  syntax='proto3',
  extension_ranges=[],
  oneofs=[
  ],
  serialized_start=112,
  serialized_end=145,
)

_QUERYREQUEST.oneofs_by_name['_max_results'].fields.append(
  _QUERYREQUEST.fields_by_name['max_results'])
_QUERYREQUEST.fields_by_name['max_results'].containing_oneof = _QUERYREQUEST.oneofs_by_name['_max_results']
DESCRIPTOR.message_types_by_name['QueryRequest'] = _QUERYREQUEST
DESCRIPTOR.message_types_by_name['QueryResponse'] = _QUERYRESPONSE
_sym_db.RegisterFileDescriptor(DESCRIPTOR)

QueryRequest = _reflection.GeneratedProtocolMessageType('QueryRequest', (_message.Message,), {
  'DESCRIPTOR' : _QUERYREQUEST,
  '__module__' : 'seqr_query_service_pb2'
  # @@protoc_insertion_point(class_scope:seqr.QueryRequest)
  })
_sym_db.RegisterMessage(QueryRequest)

QueryResponse = _reflection.GeneratedProtocolMessageType('QueryResponse', (_message.Message,), {
  'DESCRIPTOR' : _QUERYRESPONSE,
  '__module__' : 'seqr_query_service_pb2'
  # @@protoc_insertion_point(class_scope:seqr.QueryResponse)
  })
_sym_db.RegisterMessage(QueryResponse)



_QUERYSERVICE = _descriptor.ServiceDescriptor(
  name='QueryService',
  full_name='seqr.QueryService',
  file=DESCRIPTOR,
  index=0,
  serialized_options=None,
  create_key=_descriptor._internal_create_key,
  serialized_start=147,
  serialized_end=213,
  methods=[
  _descriptor.MethodDescriptor(
    name='Query',
    full_name='seqr.QueryService.Query',
    index=0,
    containing_service=None,
    input_type=_QUERYREQUEST,
    output_type=_QUERYRESPONSE,
    serialized_options=None,
    create_key=_descriptor._internal_create_key,
  ),
])
_sym_db.RegisterServiceDescriptor(_QUERYSERVICE)

DESCRIPTOR.services_by_name['QueryService'] = _QUERYSERVICE

# @@protoc_insertion_point(module_scope)
