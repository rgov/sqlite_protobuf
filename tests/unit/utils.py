import atexit
import glob
import importlib.util
import os
import shutil
import sqlite3
import subprocess
import sys
import tempfile
import unittest


def get_sqlite_protobuf_library():
  return next(glob.iglob(os.path.join(
    glob.escape(os.environ['CMAKE_CURRENT_BINARY_DIR']),
    '..', 'src', 'libsqlite_protobuf.*')))


def get_compiler():
  if 'CXX' in os.environ:
    return os.environ['CXX']
  for compiler in ('clang++', 'g++'):
    try:
      path = subprocess.check_output(['which', compiler])
      return path.rstrip()
    except subprocess.CalledProcessError:
      continue
  raise Exception('No C++ compiler found')


def compile_proto(proto):
  workdir = tempfile.mkdtemp()
  with open(os.path.join(workdir, 'definitions.proto'), 'w') as f:
    f.write(proto)
  subprocess.check_call(['protoc', '--cpp_out=.', '--python_out=.',
    'definitions.proto'], cwd=workdir)

  pypath = next(glob.iglob(os.path.join(glob.escape(workdir), '*_pb2.py')))
  cxxpath = next(glob.iglob(os.path.join(glob.escape(workdir), '*.pb.cc')))

  spec = importlib.util.spec_from_file_location('protobuf', pypath)
  module = importlib.util.module_from_spec(spec)
  spec.loader.exec_module(module)

  args = ['-std=c++11', '-dynamiclib', '-lprotobuf']
  if sys.platform == 'darwin':
    library = os.path.join(workdir, 'definitions.dylib')
    args.append('-dynamiclib')
  else:
    library = os.path.join(workdir, 'definitions.so')
    args.append('-shared')
  args.extend(['-o', library, cxxpath])
  subprocess.check_call([get_compiler()] + args)
  module.protobuf_library = library

  atexit.register(shutil.rmtree, workdir)
  return module


class SQLiteProtobufTestCase(unittest.TestCase):
  def setUp(self):
    self.db = sqlite3.connect(':memory:')
    self.db.enable_load_extension(True)
    self.db.load_extension(get_sqlite_protobuf_library())
    
    if hasattr(self, '__PROTOBUF__'):
      self.proto = compile_proto(self.__PROTOBUF__)
      self.protobuf_load(self.proto.protobuf_library)
  
  def tearDown(self):
    self.db.close()
  
  def protobuf_extract(self, data, message_type, path):
    if hasattr(data, 'SerializeToString'):
      data = data.SerializeToString()
    c = self.db.cursor()
    c.execute('SELECT protobuf_extract(?, ?, ?)', (data, message_type, path))
    return c.fetchone()[0]
  
  def protobuf_load(self, path):
    self.db.execute('SELECT protobuf_load(?)', (path,))



