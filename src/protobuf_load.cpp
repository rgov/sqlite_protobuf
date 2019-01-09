#include <string>

#include <dlfcn.h>

#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT3

#include "header.h"
#include "utilities.h"


/// Loads a shared library that presumably contains message descriptors. Returns
/// NULL on success or throws an error on failure.
///
///     SELECT protobuf_load("example/libaddressbook.dylib");
///
static void protobuf_load(sqlite3_context *context,
                          int argc,
                          sqlite3_value **argv)
{
    // Confirm that we have permission to load extensions
    int enabled, err;
    err = sqlite3_db_config(sqlite3_context_db_handle(context),
        SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, -1, &enabled);
    if (err != SQLITE_OK) {
        auto error_msg = std::string("Failed to get load_extension setting: ")
            + sqlite3_errmsg(sqlite3_context_db_handle(context));
        sqlite3_result_error(context, error_msg.c_str(), -1);
        return;
    } else if (!enabled) {
        sqlite3_result_error(context, "Extension loading is disabled", -1);
        return;
    }
    
    // Load the library
    const std::string path = string_from_sqlite3_value(argv[0]);
    void *handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL);
    if (!handle) {
        auto error_msg = std::string("Could not load library: ") + dlerror();
        sqlite3_result_error(context, error_msg.c_str(), -1);
        return;
    }

    sqlite3_result_null(context);
}


DECLARE_(protobuf_load)
{
    return sqlite3_create_function(db, "protobuf_load", 1, SQLITE_UTF8, 0,
        protobuf_load, 0, 0);
}
