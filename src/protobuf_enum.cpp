#include <strings.h>
#include <stdio.h>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/descriptor_database.h>

#include <sqlite3ext.h>
SQLITE_EXTENSION_INIT3

#include "header.h"
#include "utilities.h"

using google::protobuf::DescriptorPool;
using google::protobuf::EnumDescriptor;
using google::protobuf::EnumValueDescriptor;


// The column indexes, corresponding to the order of the columns in the CREATE
// TABLE statement in xConnect
enum {
    COLUMN_NUMBER,
    COLUMN_NAME,
    COLUMN_ENUM,
};

// The indexing strategies used by xBestIndex and xFilter
enum {
    LOOKUP_ALL,
    LOOKUP_BY_NUMBER,
    LOOKUP_BY_NAME,
};


/* enum_cursor is a subclass of sqlite3_vtab_cursor which will
** serve as the underlying representation of a cursor that scans
** over rows of the result
*/
typedef struct enum_cursor enum_cursor;
struct enum_cursor {
    sqlite3_vtab_cursor base;
    const EnumDescriptor *descriptor;
    sqlite3_int64 index;
    sqlite3_int64 stopIndex;
    bool isInvalid;
};


static int xConnect(
    sqlite3 *db,
    void *pAux,
    int argc, const char * const *argv,
    sqlite3_vtab **ppVtab,
    char **pzErr
) {
    int err = sqlite3_declare_vtab(db,
        "CREATE TABLE tbl("
        "    number INTEGER PRIMARY KEY,"
        "    name TEXT,"
        "    enum TEXT HIDDEN"
        ")");
    if (err != SQLITE_OK) return err;

    *ppVtab = (sqlite3_vtab *)sqlite3_malloc(sizeof(**ppVtab));
    if (!*ppVtab) return SQLITE_NOMEM;
    bzero(*ppVtab, sizeof(**ppVtab));

    return SQLITE_OK;
}


/*
** This method is the destructor for enum_cursor objects.
*/
static int xDisconnect(sqlite3_vtab *pVtab)
{
    sqlite3_free(pVtab);
    return SQLITE_OK;
}


/*
** Constructor for a new enum_cursor object.
*/
static int xOpen(sqlite3_vtab *p, sqlite3_vtab_cursor **ppCursor)
{
    enum_cursor *cursor = (enum_cursor *)sqlite3_malloc(sizeof(*cursor));
    if (!cursor)
        return SQLITE_NOMEM;
    bzero(cursor, sizeof(*cursor));
    *ppCursor = &cursor->base;
    return SQLITE_OK;
}

/*
** Destructor for a enum_cursor.
*/
static int xClose(sqlite3_vtab_cursor *cur)
{
    sqlite3_free(cur);
    return SQLITE_OK;
}

/*
** Advance a enum_cursor to its next row of output.
*/
static int xNext(sqlite3_vtab_cursor *cur)
{
    enum_cursor *cursor = (enum_cursor *)cur;
    cursor->index += 1;
    return SQLITE_OK;
}


/*
** Return the rowid for the current row. In this implementation, the
** first row returned is assigned rowid value 1, and each subsequent
** row a value 1 more than that of the previous.
*/
static int xRowid(sqlite3_vtab_cursor *cur, sqlite_int64 *pRowid)
{
    enum_cursor *cursor = (enum_cursor *)cur;
    *pRowid = cursor->index;
    return SQLITE_OK;
}


/*
** Return TRUE if the cursor has been moved off of the last
** row of output.
*/
static int xEof(sqlite3_vtab_cursor *cur)
{
    enum_cursor *cursor = (enum_cursor *)cur;
    return !cursor->descriptor || cursor->isInvalid ||
        cursor->index >= cursor->stopIndex;
}


/*
** Return values of columns for the row at which the enum_cursor
** is currently pointing.
*/
static int xColumn(sqlite3_vtab_cursor *cur, sqlite3_context *ctx, int i)
{
    enum_cursor *cursor = (enum_cursor *)cur;
    switch (i) {
    case COLUMN_NUMBER:
        sqlite3_result_int(ctx,
            cursor->descriptor->value(cursor->index)->number());
        break;
    case COLUMN_NAME:
        sqlite3_result_text(ctx, 
            cursor->descriptor->value(cursor->index)->name().c_str(), -1, 0);
        break;
    case COLUMN_ENUM:
        sqlite3_result_text(ctx,
            cursor->descriptor->full_name().c_str(), -1, 0);
        break;
    }
    return SQLITE_OK;
}

static int xBestIndex(sqlite3_vtab *tab, sqlite3_index_info *pIdxInfo)
{
    // Loop over the constraints to find useful ones -- namely, ones that pin
    // a column to a specific value
    int numberEqConstraintIdx = -1;
    int nameEqConstraintIdx = -1;
    int enumEqConstraintIdx = -1;
    
    const auto *constraint = pIdxInfo->aConstraint;
    for(int i = 0; i < pIdxInfo->nConstraint; i ++, constraint ++) {
        if (!constraint->usable) continue;
        if (constraint->op == SQLITE_INDEX_CONSTRAINT_EQ) {
            switch(constraint->iColumn) {
            case COLUMN_NUMBER:
                numberEqConstraintIdx = i;
                break;
            case COLUMN_NAME:
                nameEqConstraintIdx = i;
                break;
            case COLUMN_ENUM:
                enumEqConstraintIdx = i;
                break;
            }
        }
    }
    
    // If we did not get a constraint on the enum type name, we cannot continue
    if (enumEqConstraintIdx == -1) {
        return SQLITE_CONSTRAINT;
    }
    
    // If we have a constraint on the name, we can only return zero or one
    // matching rows. We can't say the same for a constraint on the number,
    // in case aliases are allowed. This allows SQLite to be more efficient.
    if (nameEqConstraintIdx >= 0) {
        pIdxInfo->idxFlags |= SQLITE_INDEX_SCAN_UNIQUE;
    }
    
    // Decide on the indexing strategy to use. Also, tell SQLite the cost of
    // different combinations of constraints, to help it compare multiple query
    // plans.
    if (numberEqConstraintIdx >= 0 && nameEqConstraintIdx < 0) {
        pIdxInfo->idxNum = LOOKUP_BY_NUMBER;
        pIdxInfo->estimatedCost = 1;
    } else if (nameEqConstraintIdx >= 0) {
        pIdxInfo->idxNum = LOOKUP_BY_NAME;
        pIdxInfo->estimatedCost = 5;
    } else {
        pIdxInfo->idxNum = LOOKUP_ALL;
        pIdxInfo->estimatedCost = 100;
    }
    
    // Copy the values of our constraints (i.e., the right-hand sides of the
    // equality assertions) into the arguments that will be passed to xFilter.
    //     argv[0] = enum type name
    //     argv[1] = number or name to lookup
    int argIdx = 1;
    pIdxInfo->aConstraintUsage[enumEqConstraintIdx].argvIndex = argIdx ++;
    switch (pIdxInfo->idxNum) {
    case LOOKUP_BY_NUMBER:
        pIdxInfo->aConstraintUsage[numberEqConstraintIdx].argvIndex = argIdx ++;
        break;
    case LOOKUP_BY_NAME:
        pIdxInfo->aConstraintUsage[nameEqConstraintIdx].argvIndex = argIdx ++;
        break;
    }

    // For some constraints, SQLite does not need to double check that our
    // output matches the constraints.
    for (int constraintIdx : { enumEqConstraintIdx, nameEqConstraintIdx }) {
        if (constraintIdx >= 0) {
            pIdxInfo->aConstraintUsage[constraintIdx].omit = 1;
        }
    }

    return SQLITE_OK;
}


/*
** This routine should initialize the cursor and position it so that it
** is pointing at the first row, or pointing off the end of the table
** (so that seriesEof() will return true) if the table is empty.
*/
static int xFilter(
    sqlite3_vtab_cursor *pVtabCursor, 
    int idxNum, const char *idxStr,
    int argc, sqlite3_value **argv
){
    enum_cursor *cursor = (enum_cursor *)pVtabCursor;
    
    // Find the descriptor for this enum type
    std::string enum_name = string_from_sqlite3_value(argv[0]);
    cursor->descriptor =
        DescriptorPool::generated_pool()->FindEnumTypeByName(enum_name);
    if (!cursor->descriptor) {
        // TODO: Better way to report this error message?
        sqlite3_log(SQLITE_WARNING, "Could not find enum type \"%s\"",
            enum_name.c_str());
        return SQLITE_ERROR;
    }
    
    // Set the cursor to iterate over the whole set of enum values, but this
    // may be overridden by a specific strategy.
    cursor->index = 0;
    cursor->stopIndex = cursor->descriptor->value_count();
    
    // If there's no particular index strategy, we don't need to do anything
    // further.
    if (idxNum == LOOKUP_ALL)
        return SQLITE_OK;
    
    // Otherwise, find the value by our constraint argument
    const EnumValueDescriptor *value_desc;
    switch (idxNum) {
    case LOOKUP_BY_NUMBER:
        value_desc = cursor->descriptor->FindValueByNumber(
            sqlite3_value_int(argv[1]));
        break;
    case LOOKUP_BY_NAME:
        value_desc = cursor->descriptor->FindValueByName(
            string_from_sqlite3_value(argv[1]));
        break;
    }
    
    if (!value_desc) {
        cursor->isInvalid = 1;
        return SQLITE_OK;
    }
    
    // Set the starting index to the index of the result
    cursor->index = value_desc->index();
    
    if (idxNum == LOOKUP_BY_NUMBER && 
        cursor->descriptor->options().allow_alias()) {
        // In this case, we cannot set an upper bound, because we need there
        // may be additional values with the same number.
    } else {
        // Set the upper bound to the next entry
        cursor->stopIndex = cursor->index + 1;
    }
    
    return SQLITE_OK;
}


static sqlite3_module module = {
  0,                         /* iVersion */
  0,                         /* xCreate */
  xConnect,                  /* xConnect - required */
  xBestIndex,                /* xBestIndex - required */
  xDisconnect,               /* xDisconnect - required */
  0,                         /* xDestroy */
  xOpen,                     /* xOpen - open a cursor - required */
  xClose,                    /* xClose - close a cursor - required */
  xFilter,                   /* xFilter - configure scan constraints - required */
  xNext,                     /* xNext - advance a cursor - required */
  xEof,                      /* xEof - check for end of scan - required */
  xColumn,                   /* xColumn - read data - required */
  xRowid,                    /* xRowid - read data - required */
  0,                         /* xUpdate */
  0,                         /* xBegin */
  0,                         /* xSync */
  0,                         /* xCommit */
  0,                         /* xRollback */
  0,                         /* xFindMethod */
  0,                         /* xRename */
};


DECLARE_(protobuf_enum)
{
    return sqlite3_create_module(db, "protobuf_enum", &module, 0);
}
