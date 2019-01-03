#!/usr/bin/env python

import glob
import os
import sqlite3
import unittest
import sys


sys.path.append(os.path.join(
  os.environ['CMAKE_CURRENT_BINARY_DIR'],
  '..', 'example'))

from AddressBook_pb2 import Person


def get_sqlite_protobuf_library():
  return next(glob.iglob(os.path.join(
    os.environ['CMAKE_CURRENT_BINARY_DIR'],
    '..', 'src', 'libsqlite_protobuf.*')))

def get_addressbook_library():
  return next(glob.iglob(os.path.join(
    os.environ['CMAKE_CURRENT_BINARY_DIR'],
    '..', 'example', 'libaddressbook.*')))


class TestProtobufExtract(unittest.TestCase):
  def setUp(self):
    self.db = sqlite3.connect(':memory:')
    
    self.db.enable_load_extension(True)
    self.db.load_extension(get_sqlite_protobuf_library())
    self.db.execute('SELECT protobuf_load(?)', (get_addressbook_library(),))
    self.db.enable_load_extension(False)
    
    self.person = Person()
    self.person.name = 'John Smith'

  def tearDown(self):
    self.db.close()
  
  def protobuf_extract(self, path):
    c = self.db.cursor()
    c.execute('SELECT protobuf_extract(?, "Person", ?)',
      (self.person.SerializeToString(), path))
    return c.fetchone()[0]

  def test_extract_root(self):
    self.assertEqual(
      self.person.SerializeToString(),
      self.protobuf_extract('$')
    )
  
  def test_extract_nonroot(self):
    with self.assertRaisesRegexp(sqlite3.OperationalError, 'Invalid path'):
      self.protobuf_extract('#')


if __name__ == '__main__':
  unittest.main()
