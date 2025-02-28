[![Build](https://github.com/rgov/sqlite_protobuf/actions/workflows/ci.yml/badge.svg)](https://github.com/rgov/sqlite_protobuf/actions/workflows/ci.yml)

# Protobuf Extension for SQLite

This project implements a [run-time loadable extension][ext] for [SQLite][]. It
allows SQLite to perform queries that can extract field values out of stored
Protobuf messages.

[ext]: https://www.sqlite.org/loadext.html
[SQLite]: https://www.sqlite.org/

See also the [JSON1 Extension][json1], which is a similar extension for querying
JSON-encoded database content.

[json1]: https://www.sqlite.org/json1.html


## Setup Instructions

This project is built with [CMake][]:

[CMake]: https://cmake.org

    mkdir build && cd build
    cmake .. && cmake --build . && ctest

The minimum supported SQLite version is probably 3.13.0. Running the test suite
requires Python and [Pipenv][].

[Pipenv]: https://github.com/pypa/pipenv

Once the extension is built, you can load into SQLite at runtime. For the
`sqlite3` tool, use:

    $ sqlite3 addressbook.db
    SQLite version 3.26.0 2018-12-01 12:34:55
    Enter ".help" for usage hints.
    sqlite> .load libsqlite_protobuf

Note that on macOS, the built-in `sqlite3` binary does not support extensions.
Install SQLite with [Homebrew][] and use `$(brew --prefix sqlite3)/bin/sqlite3`.

[Homebrew]: https://brew.sh/

In Python,

    import sqlite3
    db = sqlite3.connect('addressbook.db')
    db.enable_load_extension(True)
    db.load_extension('libsqlite_protobuf')

See the [documentation][ext] on run-time loadable extensions for more
information.


## Example

A simple Python example can be found in the `example/` directory.

    cd build/example
    pipenv run python create_database.py
    pipenv run python query_database.py

**Note:** The example currently hardcodes the macOS shared library extension
`.dylib`, which may need to be changed for other operating systems (e.g., `.so`
on Linux).


## API

### protobuf\_enum(_enum\_type_)

Returns a table with values from the specified enum type, with `number` and
`name` columns.

    SELECT number
      FROM protobuf_enum("Person.PhoneType")
     WHERE name = "MOBILE";

Note that an enum with the `allow_alias` option set can have multiple values
with the same number.


### protobuf\_extract(_protobuf_, _type\_name_, _path_)

This deserializes `protobuf` as a message of type `type_name`. The `path`
specifies the specific field to extract. The `path` must begin with `$`, which
refers to the root object, followed by zero or more field designations
`.field_name` or `.field_name[index]`.

    SELECT protobuf_extract(protobuf, "Person", "$.name") AS name,
           protobuf_extract(protobuf, "Person", "$.phones[0]") AS number,
      FROM people
     WHERE number LIKE "%8";

The return type of this function depends on the underlying field type. Messages
are returned serialized.

Enum values are returned as integers. The virtual child `.name` will return the
name of the enum value. If multiple aliases exist for the value, the first one
will be returned. The virtual child `.number` also exists but is the same as
omitting it.

Negative indexes are allowed. If an index is out of bounds, the function
returns `null` rather than throwing an error.

If a field is optional and not present, the default value is returned. For an
optional message field that is not present, `null` is returned regardless of the
subpath; an optional child's default value is not considered.


### protobuf\_load(_lib\_path_)

Before a serialized message can be parsed, the message type descriptor must be
loaded. This function accepts a path to a shared library that contains message
type descriptors.

    SELECT protobuf_load("./libaddressbook.dylib");

For security reasons, you must first [enable extension loading][ext-load]. It is
strongly recommended that you disable extension loading afterwards. Otherwise,
if untrusted users can send arbitrary queries (such as through SQL injection),
this function could be leveraged to gain code execution on your database host.

[ext-load]: https://www.sqlite.org/c3ref/enable_load_extension.html


## API Wishlist

**These functions are not yet implemented.**

### protobuf\_debug\_string(_protobuf_, _type\_name_)

This function would deserialize the message and call `Message::DebugString()` on
it.

### protobuf\_each(_protobuf_, _type\_name_, _path_)

Ideally this would be a table-valued function that is similar to
[`json_each()`][json1_each] from the JSON1 extension:

    SELECT protobuf_extract(protobuf, "Person", "$.name")
      FROM people, protobuf_each(people, "Person", "$.phones")
     WHERE protobuf_each.value LIKE "607-%";

[json1_each]: https://www.sqlite.org/json1.html#jeach


### protobuf\_has\_field(_protobuf_, _type\_name_, _path_)

Since `protobuf_extract` returns the default value for an unpopulated optional
field, it would be useful to be able to know whether the field exists or not.
