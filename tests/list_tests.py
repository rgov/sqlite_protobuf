#!/usr/bin/env python
'''
This tool discovers tests in a directory and outputs a list of

    test_module.TestClass.test_method
'''

import argparse
import unittest
import os
import sys


def iter_tests(suite):
  if isinstance(suite, unittest.suite.TestSuite):
    for x in suite:
      yield from iter_tests(x)
  elif isinstance(suite, unittest.case.TestCase):
    yield suite  # actually a test case, not a suite
  else:
    raise Exception('Unexpected object in test suite hierarchy: %s' %
      (suite.__class__.__mro__))


if __name__ == '__main__':
  argparser = argparse.ArgumentParser(description=__doc__)
  argparser.add_argument('dir')
  args = argparser.parse_args()

  exit_code = 0

  # This is a signal to the modules we load that we are discovering, and not to
  # expect any built resources to be present at load time
  os.environ['IN_TEST_DISCOVERY'] = 'YES'
  
  for test in iter_tests(unittest.defaultTestLoader.discover(args.dir)):
    if isinstance(test, unittest.loader._FailedTest):
      print(test._exception, file=sys.stderr)
      exit_code = 1
    elif test.__class__.__module__ == 'unittest.loader':
      print('Unexpected failure:', file=sys.stderr)
      print('MRO:', test.__class__.__mro__, file=sys.stderr)
      __import__('pprint').pprint(test.__dict__, stream=sys.stderr)
      exit_code = 0
    else:
      if test._testMethodName == 'runTest':
        name = '%s.%s' % \
          (test.__module__, test.__class__.__name__)
      else:
        name = '%s.%s.%s' % \
          (test.__module__, test.__class__.__name__, test._testMethodName)
      print(name)

  sys.exit(exit_code)
