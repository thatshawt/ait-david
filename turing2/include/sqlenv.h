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


int sqlenv_open(sqlenv_t *sqlenv, char *databaseFile);
void sqlenv_exec(sqlenv_t *sqlenv, char *sql_statement, void* data);
void sqlenv_close(sqlenv_t *sqlenv);



#endif