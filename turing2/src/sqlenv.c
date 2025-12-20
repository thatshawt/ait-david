#include "sqlenv.h"

#include <stdio.h>
#include <string.h>
#include <gmp.h>


int sqlenv_open(sqlenv_t *sqlenv, char *databaseFile,
    int(*exec_callback)(void *data, int argc, char **argv, char **columnName),
    void(*resultHandler)(void *sqlenv, enum SQL_RT resultType)
)
{
    sqlenv->exec_callback = exec_callback;
    sqlenv->resultHandler = resultHandler;

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

int sql_callback_print(void *data, int argc, char **argv, char **azColName){
    int i;
    if(data)fprintf(stderr, "%s:\n", (const char*)data);

    for(i = 0; i<argc; i++){
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }

    printf("\n");
    return 0;
}

void sql_resultHandler_print(void *sqlenv_void, enum SQL_RT resultType){
    sqlenv_t *sqlenv = (sqlenv_t*)sqlenv_void;
    const int rc = sqlenv->rc;
    const char* errmsg = sqlenv->errMsg;
    switch(resultType){
        case SQL_RT_OPEN:
            if(rc == SQLITE_OK){
                fprintf(stdout, "Opened database.\n");
            }else{
                fprintf(stderr, "Could not open database. error '%s'\n", errmsg);
            }
            break;
        case SQL_RT_EXEC:
            if(rc == SQLITE_OK){
                fprintf(stdout, "Did exec.\n");
            }else{
                fprintf(stderr, "Could not exec. error '%s'\n", errmsg);
            }
            break;
        case SQL_RT_CLOSE:
            if(rc == SQLITE_OK){
                fprintf(stdout, "Database closed.\n");
            }else{
                fprintf(stderr, "Could not close database. error '%s'\n", errmsg);
            }
            break;
    }
}

int sql_callback_load_countCol_into_mpz(void *data, int argc, char **argv, char **azColName){
    for(int i = 0; i<argc; i++){
        const char* colName = azColName[i];
        const char* val = argv[i];

        if(strcmp(colName, "COUNT") == 0){
            mpz_set_str(*(mpz_t*)data, val, 10);
            // printf("    loaded %s into data, in col %s\n", val, colName);
        }

        // printf("%s = %s\n", colName, val ? val : "NULL");
    }

    return 0;
}