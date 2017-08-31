# -*- coding: utf-8 -*-

# Conversion table used by the JSON API generator.
#
# Keys are the C++ type.  Values are the Javascript type.

typenameConversion = {
  'block_id_type': 'UInt32',
  'bool': 'Bool',
  'chain::block_id_type': 'FixedBytes32',
  'double': 'Double',
  'empty': None,
  'fc::time_point_sec': 'Time',
  'AccountName': 'UInt16',
  'signature_type': 'Signature',
  'string': 'String',
  'uint32_t': 'UInt32',
  'uint64_t': 'UInt64',
  'vector< char >': 'Bytes',
  'vector< fc::variant >': 'Vector',
  'vector< Name >': 'Name[]',
}
