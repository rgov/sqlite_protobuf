#!/usr/bin/env python
import unittest

from utils import *


class TestProtobufEnum(SQLiteProtobufTestCase, unittest.TestCase):
  __PROTOBUF__ = '''
  syntax = "proto2";
  enum TestEnum {
    A = 3;
    B = 2;
    C = 1;
  }
  
  enum TestEnumWithAliases {
    option allow_alias = true;
    A1 = 1;
    XY = 2;
    A2 = 1;
  }

  message TestMessage {
    enum EmbeddedTestEnum {
      D = 1;
      E = 2;
      F = 3;
    }

    optional EmbeddedTestEnum enum_field = 1;
  }
  '''
  
  def test_protobuf_enum(self):
    c = self.db.cursor()
    c.execute('SELECT * FROM protobuf_enum(?)', ('TestEnum',))
    self.assertEqual(c.fetchall(), [(3, 'A'), (2, 'B'), (1, 'C')])
  
  def test_protobuf_embedded_enum(self):
    c = self.db.cursor()
    c.execute('SELECT * FROM protobuf_enum(?)',
      ('TestMessage.EmbeddedTestEnum',))
    self.assertEqual(c.fetchall(), [(1, 'D'), (2, 'E'), (3, 'F')])
  
  def test_protobuf_enum_by_name(self):
    c = self.db.cursor()
    c.execute('SELECT * FROM protobuf_enum(?) WHERE name = ?',
      ('TestEnum', 'B'))
    self.assertEqual(c.fetchall(), [(2, 'B')])
  
  def test_protobuf_enum_by_number(self):
    c = self.db.cursor()
    c.execute('SELECT * FROM protobuf_enum(?) WHERE number = ?',
      ('TestEnum', 2))
    self.assertEqual(c.fetchall(), [(2, 'B')])
  
  def test_protobuf_enum_with_aliases(self):
    c = self.db.cursor()
    c.execute('SELECT * FROM protobuf_enum(?) WHERE number = ?',
      ('TestEnumWithAliases', 1))
    self.assertEqual(c.fetchall(), [(1, 'A1'), (1, 'A2')])
  
  def test_protobuf_bad_enum(self):
    c = self.db.cursor()
    with self.assertRaises(sqlite3.OperationalError):
      c.execute('SELECT * FROM protobuf_enum(?)', ('BadEnum',))


if __name__ == '__main__':
  unittest.main()
