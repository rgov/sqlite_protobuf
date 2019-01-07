#!/usr/bin/env python
import unittest

from utils import *


class TestProtobufExtract(SQLiteProtobufTestCase):
  __PROTOBUF__ = '''
  syntax = "proto2";
  message Person {
    required string name = 1;
  }
  '''
  
  def setUp(self):
    super().setUp()
    self.person = self.proto.Person()
    self.person.name = 'John Smith'

  def test_extract_root(self):
    self.assertEqual(
      self.person.SerializeToString(),
      self.protobuf_extract(self.person, 'Person', '$')
    )
  
  def test_extract_nonroot(self):
    with self.assertRaisesRegex(sqlite3.OperationalError, 'Invalid path'):
      self.protobuf_extract(self.person, 'Person', '#')


if __name__ == '__main__':
  unittest.main()
