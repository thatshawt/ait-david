#include "sqlenv.h"

int sqlenv_open(sqlenv_t *sqlenv, char *databaseFile)
{
    sqlenv->errMsg = 0;
    /* Open database */
    sqlenv->rc = sqlite3_open("test.db", &sqlenv->db);

    if( sqlenv->rc ) {
        // fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(sqlenv->db));
        sqlenv->errMsg = sqlite3_errmsg(sqlenv->db);
        if(sqlenv->resultHandler) sqlenv->resultHandler((void*)sqlenv, SQL_RT_OPEN);
        sqlenv->errMsg = 0;
        return 1;
    } else {
        // fprintf(stderr, "Opened database successfully\n");
        if(sqlenv->resultHandler) sqlenv->resultHandler((void*)sqlenv, SQL_RT_OPEN);
        return 0;
    }
}

void sqlenv_exec(sqlenv_t *sqlenv, char *sql_statement, void* data)
{
    sqlenv->sql = sql_statement;

    /* Execute SQL statement */
    sqlenv->rc = sqlite3_exec(sqlenv->db, sqlenv->sql, sqlenv->exec_callback, data, &sqlenv->errMsg);

    if( sqlenv->rc != SQLITE_OK ){
        // fprintf(stderr, "SQL error: %s\n", sqlenv->errMsg);
        if(sqlenv->resultHandler)sqlenv->resultHandler(sqlenv, SQL_RT_EXEC);

        sqlite3_free(sqlenv->errMsg);
        sqlenv->errMsg = 0;
    } else {
        if(sqlenv->resultHandler)sqlenv->resultHandler(sqlenv, SQL_RT_EXEC);
        // fprintf(stdout, "Exec successful.\n");
    }
}

void sqlenv_close(sqlenv_t *sqlenv)
{
    sqlenv->rc = sqlite3_close(sqlenv->db);
    if(sqlenv->resultHandler)sqlenv->resultHandler(sqlenv, SQL_RT_CLOSE);
}