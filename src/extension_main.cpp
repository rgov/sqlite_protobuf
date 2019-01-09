#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT1

#include "header.h"


extern "C"
int sqlite3_sqliteprotobuf_init(sqlite3 *db,
                                char **pzErrMsg,
                                const sqlite3_api_routines *pApi)
{
    int err;
    
    SQLITE_EXTENSION_INIT2(pApi);
    
    // Check that we are compatible with this version of SQLite
    // 3.13.0 - SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION added
    if (sqlite3_libversion_number() < 3013000) {
        *pzErrMsg = sqlite3_mprintf(
            "sqlite_protobuf requires SQLite 3.13.0 or later");
        return SQLITE_ERROR;
    }
    
    // Run each register_* function and abort if any of them fails
    int (*register_fns[])(sqlite3 *, char **, const sqlite3_api_routines *) = {
        register_protobuf_enum,
        register_protobuf_extract,
        register_protobuf_load,
    };
    
    int nfuncs = sizeof(register_fns) / sizeof(register_fns[0]);
    for (int i = 0; i < nfuncs; i ++) {
        int err = (register_fns[i])(db, pzErrMsg, pApi);
        if (err != SQLITE_OK) return err;
    }

    return SQLITE_OK;
}
