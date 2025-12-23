#ifndef SQLENV_H
#define SQLENV_H

#include <sqlite3.h>
#include <gmp.h>

enum SQL_RT{
    SQL_RT_EXEC, SQL_RT_OPEN, SQL_RT_CLOSE
};

typedef struct{
    sqlite3 *db;    // sqlite3 database handle
    char *errMsg;   // error message
    int rc;         // return value
    char *sql;      // sql statement

    int(*exec_callback)(void *data, int argc, char **argv, char **columnName);
    void(*resultHandler)(void *sqlenv, enum SQL_RT resultType);

} sqlenv_t;


int sqlenv_open(sqlenv_t *sqlenv, char *databaseFile,
    int(*exec_callback)(void *data, int argc, char **argv, char **columnName),
    void(*resultHandler)(void *sqlenv, enum SQL_RT resultType));

void sqlenv_exec(sqlenv_t *sqlenv, char *sql_statement, void* data);
void sqlenv_close(sqlenv_t *sqlenv);

// simplest complete callbacks that print stuff
int sql_callback_print(void *data, int argc, char **argv, char **azColName);
void sql_resultHandler_print(void *sqlenv_void, enum SQL_RT resultType);

// callback to specifically load a text 'COUNT' column value into an mpz_t variable.
// used by sql_load_str_count_into_mpz
int sql_callback_load_countCol_into_mpz(void *data, int argc, char **argv, char **azColName);

// sql str_count functions
void sql_set_str_count(sqlenv_t *sqlenv, char *statementBuffer, char *stringID, char *countStr);
void sql_load_str_count_into_mpz(sqlenv_t *sqlenv, char *statementBuffer, char *stringID, mpz_t *theMpz);
void sql_create_str_count_table_ifnotexist(sqlenv_t *sqlenv);

#endif