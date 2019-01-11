#!/usr/bin/env python
import unittest

from utils import *


class TestProtobufExtract(SQLiteProtobufTestCase, unittest.TestCase):
  __PROTOBUF__ = '''
  syntax = "proto2";
  message TestMessage {
    enum EnumValues {
      A = 1;
      B = 2;
    }

    optional double double_field = 1;
    optional float float_field = 2;
    optional int32 int32_field = 3;
    optional int64 int64_field = 4;
    optional uint32 uint32_field = 5;
    optional uint64 uint64_field = 6;
    optional sint32 sint32_field = 7;
    optional sint64 sint64_field = 8;
    optional fixed32 fixed32_field = 9;
    optional fixed64 fixed64_field = 10;
    optional sfixed32 sfixed32_field = 11;
    optional sfixed64 sfixed64_field = 12;
    optional bool bool_field = 13;
    optional string string_field = 14;
    optional bytes bytes_field = 15;
    optional EnumValues enum_field = 16;

    repeated double repeated_double_field = 101;
    repeated float repeated_float_field = 102;
    repeated int32 repeated_int32_field = 103;
    repeated int64 repeated_int64_field = 104;
    repeated uint32 repeated_uint32_field = 105;
    repeated uint64 repeated_uint64_field = 106;
    repeated sint32 repeated_sint32_field = 107;
    repeated sint64 repeated_sint64_field = 108;
    repeated fixed32 repeated_fixed32_field = 109;
    repeated fixed64 repeated_fixed64_field = 110;
    repeated sfixed32 repeated_sfixed32_field = 111;
    repeated sfixed64 repeated_sfixed64_field = 112;
    repeated bool repeated_bool_field = 113;
    repeated string repeated_string_field = 114;
    repeated bytes repeated_bytes_field = 115;
    repeated EnumValues repeated_enum_field = 116;

    repeated TestMessage children = 1000;
    optional TestMessage optional_child = 1001;
  }
  '''

  protobuf_to_sql_types = {
    'double': 'REAL',
    'float': 'REAL',
    'int32': 'INTEGER',
    'int64': 'INTEGER',
    'uint32': 'INTEGER',
    'uint64': 'INTEGER',
    'sint32': 'INTEGER',
    'sint64': 'INTEGER',
    'fixed32': 'INTEGER',
    'fixed64': 'INTEGER',
    'sfixed32': 'INTEGER',
    'sfixed64': 'INTEGER',
    'bool': 'INTEGER',
    'string': 'TEXT',
    'bytes': 'BLOB',
    'enum': 'INTEGER',  # handled specially
    'null': 'NULL',  # handled specially
  }
  
  sql_example_value = {
    'REAL': 3.14,
    'INTEGER': 1337,
    'TEXT': 'abcdef',
    'BLOB': b'\x01\x02\x03\x04',
    'NULL': None,
  }

  def assertCorrectSQLType(self, protobuf_type, sql_type):
    expected = self.protobuf_to_sql_types[protobuf_type.lower()]
    self.assertEqual(expected, sql_type,
      msg='SQL type for %s field default value is %s, expected %s' %
      (protobuf_type, sql_type, expected))

  def test_extract_root(self):
    msg = self.proto.TestMessage()
    msg.int32_field = 1337
    self.assertEqual(
      msg.SerializeToString(),
      self.protobuf_extract(msg, 'TestMessage', '$')
    )

  def test_extract_bad_root(self):
    msg = self.proto.TestMessage()
    with self.assertRaisesRegex(sqlite3.OperationalError, 'Invalid path'):
      self.protobuf_extract(msg, 'TestMessage', '#')
  
  def test_extract_child_field(self):
    msg = self.proto.TestMessage()
    for i in range(100):
      child = msg.children.add()
      child.int32_field = i
    self.assertEqual(
      20,  # some index, must match path below
      self.protobuf_extract(msg, 'TestMessage', '$.children[20].int32_field')
    )
    self.assertEqual(
      97,  # negative index, must correspond to path below
      self.protobuf_extract(msg, 'TestMessage', '$.children[-3].int32_field')
    )
  
  def test_extract_child_message(self):
    msg = self.proto.TestMessage()
    child = msg.children.add()
    child.int32_field = 1337
    self.assertEqual(
      child.SerializeToString(),
      self.protobuf_extract(msg, 'TestMessage', '$.children[0]')
    )

  def test_extract_default_type(self):
    types = set(self.protobuf_to_sql_types.keys()) - set(('null',))
    for t in types:
      msg = self.proto.TestMessage()
      self.assertCorrectSQLType(t, self.protobuf_extract_result_type(msg,
        'TestMessage', '$.%s_field' % t))
  
  def test_extract_child_of_missing_optional_message(self):
    msg = self.proto.TestMessage()
    self.assertEqual(
      None,
      self.protobuf_extract(msg, 'TestMessage', '$.optional_child.int32_field')
    )
  
  def test_extract_child_of_missing_optional_nonmessage(self):
    msg = self.proto.TestMessage()
    with self.assertRaises(sqlite3.OperationalError):
      self.protobuf_extract(msg, 'TestMessage', '$.int32_field.foobar')

  def test_extract_stored_type(self):
    types = set(self.protobuf_to_sql_types.keys()) - set(('enum', 'null'))
    for t in types:
      msg = self.proto.TestMessage()
      setattr(msg, '%s_field' % t,
        self.sql_example_value[self.protobuf_to_sql_types[t]])
      self.assertCorrectSQLType(t, self.protobuf_extract_result_type(msg,
        'TestMessage', '$.%s_field' % t))
  
  def test_extract_repeated_type(self):
    types = set(self.protobuf_to_sql_types.keys()) - set(('enum', 'null'))
    for t in types:
      msg = self.proto.TestMessage()
      getattr(msg, 'repeated_%s_field' % t).append(
        self.sql_example_value[self.protobuf_to_sql_types[t]])
      self.assertCorrectSQLType(t, self.protobuf_extract_result_type(msg,
        'TestMessage', '$.repeated_%s_field[0]' % t))
    
  def test_extract_stored_type_enum(self):
    msg = self.proto.TestMessage()
    msg.enum_field = self.proto.TestMessage.EnumValues.Value('B')
    self.assertCorrectSQLType('enum', self.protobuf_extract_result_type(msg,
      'TestMessage', '$.enum_field'))
  
  def test_extract_repeated_enum(self):
    msg = self.proto.TestMessage()
    msg.repeated_enum_field.append(self.proto.TestMessage.EnumValues.Value('B'))
    self.assertCorrectSQLType('enum', self.protobuf_extract_result_type(msg,
      'TestMessage', '$.repeated_enum_field[0]'))
  
  def test_extract_enum_special_children(self):
    msg = self.proto.TestMessage()
    msg.enum_field = self.proto.TestMessage.EnumValues.Value('B')
    self.assertEqual(
      self.proto.TestMessage.EnumValues.Value('B'),
      self.protobuf_extract(msg, 'TestMessage', '$.enum_field.number')
    )
    self.assertEqual(
      'B',
      self.protobuf_extract(msg, 'TestMessage', '$.enum_field.name')
    )
    with self.assertRaises(sqlite3.OperationalError):
      self.protobuf_extract(msg, 'TestMessage', '$.enum_field.buzz')
  
  def test_extract_bad_path_traversal_error(self):
    msg = self.proto.TestMessage()
    msg.int32_field = 1337
    with self.assertRaises(sqlite3.OperationalError) as cm:
      self.protobuf_extract(msg, 'TestMessage', '$.int32_field.buzz')
    err1 = str(cm.exception)
    with self.assertRaises(sqlite3.OperationalError) as cm:
      self.protobuf_extract(msg, 'TestMessage', '$.enum_field.buzz')
    err2 = str(cm.exception)
    self.assertEqual(err1, err2)


if __name__ == '__main__':
  unittest.main()
