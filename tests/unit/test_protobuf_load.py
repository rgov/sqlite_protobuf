#!/usr/bin/env python
import unittest

from utils import *


class TestProtobufLoad(SQLiteProtobufTestCase, unittest.TestCase):
  __PROTOBUF__ = '''
  syntax = "proto2";
  message Empty { }
  '''
  
  def test_load_enabled(self):
    self.db.enable_load_extension(True)
    self.protobuf_load(self.proto.protobuf_library)

  def test_load_disabled(self):
    self.db.enable_load_extension(False)
    with self.assertRaisesRegex(sqlite3.OperationalError, 'Extension loading'):
      self.protobuf_load(self.proto.protobuf_library)


if __name__ == '__main__':
  unittest.main()
