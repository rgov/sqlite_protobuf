#ifndef UTILITIES_H
#define UTILITIES_H

#include <string>

#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT3


/// Convenience method for constructing a std::string from sqlite3_value
const std::string string_from_sqlite3_value(sqlite3_value *value);



#endif
