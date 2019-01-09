#include <string>

#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT3


const std::string string_from_sqlite3_value(sqlite3_value *value)
{
    return std::string(reinterpret_cast<const char*>(sqlite3_value_text(value)),
                        static_cast<size_t>(sqlite3_value_bytes(value)));
}
