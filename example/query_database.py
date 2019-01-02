#!/usr/bin/env python

import sqlite3

# Connect to the database
db = sqlite3.connect('addressbook.db')

# Load the extension
db.enable_load_extension(True)
db.load_extension('../src/libsqlite_protobuf')

# Run a query to load the Protobuf descriptors
db.execute('SELECT protobuf_load("./libaddressbook.dylib");')

# Run a query
query = '''
SELECT protobuf_extract(protobuf, "Person", "$.name"),
       protobuf_extract(protobuf, "Person", "$.phones[0].number") AS number
  FROM people
 WHERE number LIKE "%8";
'''
for row in db.execute(query):
  print(row)
