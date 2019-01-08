#ifndef SQLITE_PROTOBUF_HEADER_H
#define SQLITE_PROTOBUF_HEADER_H

#include <sqlite3ext.h>

#define DECLARE_(X) \
    extern "C" \
    int register_##X(sqlite3 *db, \
                     char **pzErrMsg, \
                     const sqlite3_api_routines *pApi)


DECLARE_(protobuf_load);
DECLARE_(protobuf_extract);

#endif
