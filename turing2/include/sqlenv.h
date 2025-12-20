#ifndef SQLENV_H
#define SQLENV_H

#include <sqlite3.h>

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

// debug just prints everything
int sql_callback_print(void *data, int argc, char **argv, char **azColName);
void sql_resultHandler_print(void *sqlenv_void, enum SQL_RT resultType);

// callback to specifically load a text 'COUNT' column value into an mpz_t variable.
int sql_callback_load_countCol_into_mpz(void *data, int argc, char **argv, char **azColName);

#endif