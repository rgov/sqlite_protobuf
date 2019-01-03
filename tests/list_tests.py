#!/usr/bin/env python
'''
This tool discovers tests in a directory and outputs a list of

    test_module.TestClass.test_method
'''

import argparse
import re
import unittest


def print_suite(suite):
  if isinstance(suite, unittest.suite.TestSuite):
    for x in suite:
      yield from print_suite(x)
  else:
    m = re.match(r'^(.+) \((.*)\)$', str(suite))
    yield "{1}.{0}".format(*m.groups())


if __name__ == '__main__':
  argparser = argparse.ArgumentParser(description=__doc__)
  argparser.add_argument('dir')
  args = argparser.parse_args()

  for test in print_suite(unittest.defaultTestLoader.discover(args.dir)):
    if test.startswith('unittest.loader.'):
      raise Exception('Failed to load %s' % test)
    print(test)
