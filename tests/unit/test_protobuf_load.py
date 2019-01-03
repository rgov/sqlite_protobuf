#!/usr/bin/env python

import glob
import os
import sqlite3
import unittest


def get_sqlite_protobuf_library():
  return next(glob.iglob(os.path.join(
    os.environ['CMAKE_CURRENT_BINARY_DIR'],
    '..', 'src', 'libsqlite_protobuf.*')))

def get_addressbook_library():
  return next(glob.iglob(os.path.join(
    os.environ['CMAKE_CURRENT_BINARY_DIR'],
    '..', 'example', 'libaddressbook.*')))


class TestProtobufLoad(unittest.TestCase):
  def setUp(self):
    self.db = sqlite3.connect(':memory:')
    self.db.enable_load_extension(True)
    self.db.load_extension(get_sqlite_protobuf_library())

  def tearDown(self):
    self.db.close()

  def test_load_enabled(self):
    self.db.enable_load_extension(True)
    self.db.execute('SELECT protobuf_load(?)', (get_addressbook_library(),))

  def test_load_disabled(self):
    self.db.enable_load_extension(False)
    with self.assertRaisesRegexp(sqlite3.OperationalError, 'Extension loading'):
      self.db.execute('SELECT protobuf_load(?)', (get_addressbook_library(),))


if __name__ == '__main__':
  unittest.main()
