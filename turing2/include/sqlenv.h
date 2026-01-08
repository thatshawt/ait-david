#ifndef SQLENV_H
#define SQLENV_H

#include "hashmap.h"

#include <sqlite3.h>
#include <gmp.h>
#include <mpfr.h>

enum SQL_RT{
    SQL_RT_EXEC, SQL_RT_OPEN, SQL_RT_CLOSE
};

#define SQL_CALLBACK_FUNC_ARG_PROTO void *data, int count, char **values, char **columnNames

typedef struct{
    sqlite3 *db;    // sqlite3 database handle
    char *errMsg;   // error message
    int rc;         // return value
    char *sql;      // sql statement

    int(*exec_callback)(SQL_CALLBACK_FUNC_ARG_PROTO);
    void(*resultHandler)(void *sqlenv, enum SQL_RT resultType);
} sqlenv_t;


int sqlenv_open(sqlenv_t *sqlenv, char *databaseFile,
    int(*exec_callback)(void *data, int argc, char **argv, char **columnName),
    void(*resultHandler)(void *sqlenv, enum SQL_RT resultType));

void sqlenv_exec(sqlenv_t *sqlenv, char *sql_statement, void* data);
void sqlenv_exec_with_callback(sqlenv_t *sqlenv, char *sql_statement, void* data, int(*exec_callback)(SQL_CALLBACK_FUNC_ARG_PROTO));
void sqlenv_close(sqlenv_t *sqlenv);

// simplest complete callbacks that print stuff
int sql_callback_print(SQL_CALLBACK_FUNC_ARG_PROTO);
void sql_resultHandler_print(void *sqlenv_void, enum SQL_RT resultType);

// callback to specifically load a text 'COUNT' column value into an mpz_t variable.
// used by sql_load_str_count_into_mpz
int sql_callback_load_countCol_into_mpz(SQL_CALLBACK_FUNC_ARG_PROTO);

// loads first result into (mpz_t*)data
int sql_callback_load_firstcol_into_mpz(SQL_CALLBACK_FUNC_ARG_PROTO);

unsigned long sqlenv_get_last_insert_rowid(sqlenv_t* sqlenv);

// sql str_count functions
void sql_set_str_count(sqlenv_t *sqlenv, char *statementBuffer, char *tableName, char *stringID, char *countStr);
void sql_load_str_count_into_mpz(sqlenv_t *sqlenv, char *statementBuffer, char *tablename, char *stringID, mpz_t *theMpz);
void sql_str_count_get_freq(sqlenv_t *sqlenv, char *statementBuffer, char *tablename, char *stringID, mpq_t freq);
void sql_str_count_get_freq_negativelog2(sqlenv_t *sqlenv, char *statementBuffer, char *tablename, char *stringID, mpfr_t resultMpfr);

void sql_create_str_count_table_ifnotexist(sqlenv_t *sqlenv, char *statementBuffer, char *tablename);

void sql_drop_table_if_exists(sqlenv_t *sqlenv, char *statementBuffer, char *tablename);

int sql_callback_str_count_sum_all_count_columns_into_mpz(SQL_CALLBACK_FUNC_ARG_PROTO);
void sql_str_count_sum_all_count(sqlenv_t *sqlenv, char *statementBuffer, char *tableName, mpz_t *zval);

int sql_callback_loadIntoSliceCountMap(SQL_CALLBACK_FUNC_ARG_PROTO);
struct hashmap* sql_str_count_get_map(sqlenv_t *sqlenv, char *statementBuffer, char *tableName);

void print_freq_sql_str_count_table_string(
    sqlenv_t *sqlenv,
    char *statementBuffer,
    char *tablename,
    char *strID
);

void sql_store_slicecount_map_as_sql_table(
    sqlenv_t *sqlenv,
    char *statementBuffer,
    char *tableName,
    struct hashmap *map
);

void sql_merge_slicecountmap_into_str_count_table(
    sqlenv_t *sqlenv,
    char *statementBuffer,
    char *tableName,
    struct hashmap *map
);

#endif