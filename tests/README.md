# Tests

This directory contains tests. The test driver used by this project is
[CTest][]. Normally, you would run tests like this:

    cd build
    ctest --output-on-failure

To run tests matching a regular expression, use

    ctest -R test_protobuf_load

[CTest]: https://cmake.org/cmake/help/latest/manual/ctest.1.html


## Python Tests

Tests written in Python use the [`unittest`][pyunittest] unit testing framework.

[pyunittest]: https://docs.python.org/3/library/unittest.html

Normally, `unittest` performs its own test discovery. To inform CTest about each
unit test, CMake runs the `list_tests.py` tool and parses its output. It is
important that tests do not expect any resources to have been built at module
load time, or the initial test discovery will fail.

Tests are executed inside a [Pipenv][] virtual environment described by the
Pipfile. When a test is run, the working directory is the test's *source
directory*. The environment variable `CMAKE_CURRENT_BINARY_DIR` is set to the
corresponding CMake binary directory relative to the build directory.

[Pipenv]: https://github.com/pypa/pipenvv
