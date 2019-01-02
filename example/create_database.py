#!/usr/bin/env python

import random
import sqlite3

from AddressBook_pb2 import Person


# Create the database
db = sqlite3.connect('addressbook.db')
db.execute('CREATE TABLE IF NOT EXISTS people (protobuf BLOB)')


# Populate it with some random people
names = '''
    Kaila Dutton
    Collin Felps
    Lou Ohearn
    Hilde Charters
    Raymond Mangione
    Kasandra La
    Anette Poore
    Gilberte Strzelecki
    Lilli Glassman
    Lakiesha Peavy
    Billye Boelter
    Joan Claire
    Monika Seo
    Vicki Campoverde
    Destiny Eilers
    Alfonzo Hotz
    Daine Wyllie
    Yu Leer
    Shanel Yip
    Emiko Morvant
    Joe Esses
    Nathaniel Malcolm
    Cherry Robidoux
    Beulah Harshaw
    Alvina Granier
    Gerald Faries
    Lorri Elsass
    Janella Spray
    Shantelle Enger
    Chia Leite
'''

for name in names.splitlines():
  name = name.strip()
  if not name: continue
  
  # Generate a new person with a few phone numbers
  person = Person()
  person.name = name
  count = random.randint(1, len(Person.PhoneType.items()))
  for k, v in random.sample(Person.PhoneType.items(), count):
    area_code = random.randint(200, 999)
    exchange = random.randint(200, 999)
    line_number = random.randint(0, 9999)
    number = person.phones.add()
    number.type = v
    number.number = '({}) {}-{:04d}'.format(area_code, exchange, line_number)
  
  # Add them to the database
  data = person.SerializeToString()
  db.execute('INSERT INTO people VALUES (?)', (data,))
  print('Inserted', name)

db.commit()
db.close()
