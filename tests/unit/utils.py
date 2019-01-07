import atexit
import glob
import importlib.util
import os
import shlex
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
  for compiler in ('clang++', 'g++', 'c++'):
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

  # Build up the compiler command
  cmd = [get_compiler(), '-std=c++11', '-fPIC']

  # Ask pkg-config what flags we need to link to libprotobuf
  pkgcfg = subprocess.check_output(['pkg-config', '--cflags', '--libs',
    'protobuf'])
  cmd.extend(shlex.split(pkgcfg.decode('utf-8')))

  # Add platform-specific flags
  if sys.platform == 'darwin':
    library = os.path.join(workdir, 'definitions.dylib')
    cmd.append('-dynamiclib')
  else:
    library = os.path.join(workdir, 'definitions.so')
    cmd.append('-shared')
  cmd.extend(['-o', library])
  module.protobuf_library = library

  # Invoke the compiler
  cmd.append(cxxpath)
  subprocess.check_call(cmd)

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



