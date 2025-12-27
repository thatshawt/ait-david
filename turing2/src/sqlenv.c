#include "sqlenv.h"

#include <stdio.h>
#include <string.h>

#define SQLENV_TEMP_ZERO void* temp_callback = sqlenv->exec_callback; sqlenv->exec_callback = 0; \
    void* temp_resulthandler = sqlenv->resultHandler; sqlenv->resultHandler = 0;

#define SQLENV_TEMP_REVERT sqlenv->exec_callback = temp_callback; sqlenv->resultHandler = temp_resulthandler;

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


void sql_set_str_count(sqlenv_t *sqlenv, char *statementBuffer, char *tableName, char *stringID, char *countStr)
{
    SQLENV_TEMP_ZERO;

    sprintf(statementBuffer, "INSERT INTO %s(STR_ID, COUNT) VALUES ('%s', '%s')",tableName,stringID,countStr);
    sqlenv_exec(sqlenv, statementBuffer, NULL);

    sprintf(statementBuffer, "UPDATE %s SET COUNT = '%s' WHERE STR_ID='%s';",tableName,countStr,stringID);
    sqlenv_exec(sqlenv, statementBuffer, NULL);
    
    SQLENV_TEMP_REVERT;
}

void sql_load_str_count_into_mpz(sqlenv_t *sqlenv, char *statementBuffer, char *tablename, char *stringID, mpz_t *theMpz)
{
    SQLENV_TEMP_ZERO;

    sqlenv->exec_callback = &sql_callback_load_countCol_into_mpz;
    sprintf(statementBuffer, "SELECT * FROM %s WHERE STR_ID='%s';", tablename, stringID);
    sqlenv_exec(sqlenv, statementBuffer, (void*)theMpz);
    
    SQLENV_TEMP_REVERT;
}

void sql_str_count_get_freq(sqlenv_t *sqlenv, char *statementBuffer, char *tablename, char *stringID, mpq_t freq)
{
    // SQLENV_TEMP_ZERO;

    // sqlenv->exec_callback = &sql_callback_load_countCol_into_mpz;
    // sprintf(statementBuffer, "SELECT * FROM %s WHERE STR_ID='%s';", tablename, stringID);
    // sqlenv_exec(sqlenv, statementBuffer, (void*)theMpz);

    mpz_t count, totalCount; mpz_inits(count, totalCount, NULL);
    mpq_t tempq1;mpq_init(tempq1);

    sql_str_count_sum_all_count(sqlenv, statementBuffer, tablename, &totalCount);
    sql_load_str_count_into_mpz(sqlenv, statementBuffer, tablename, stringID, &count);

    mpq_set_z(freq, count); // freq = count
    mpq_set_z(tempq1, totalCount); // tempq1 = totalCount

    mpq_canonicalize(freq);
    mpq_canonicalize(tempq1);

    mpq_div(freq, freq, tempq1); // freq = count/totalCount;

    mpq_canonicalize(freq);

    // gmp_printf("totals was %Zd, count was %Zd, freq of '%s' is %lf\n", totalCount, count, strID, mpq_get_d(freq));

    mpz_clears(count, totalCount, NULL);
    mpq_clear(tempq1);
    
    // SQLENV_TEMP_REVERT;
}

int sql_callback_str_count_sum_all_count_columns_into_mpz(void *data, int argc, char **argv, char **columnName)
{
    char numberBuffer[1000];
    mpz_t zval; mpz_init(zval);
    for(int i=0;i<argc;i++){
        const char *colName = columnName[i];
        const char *colValue = argv[i];
        
        if(strcmp(colName,"COUNT") == 0){
            mpz_set_str(zval, colValue, 10);
            mpz_add(*(mpz_t*)data, *(mpz_t*)data, zval);
        }

    }
    mpz_clear(zval);
}

void sql_str_count_sum_all_count(sqlenv_t *sqlenv, char *statementBuffer, char *tableName, mpz_t *zval)
{
    SQLENV_TEMP_ZERO;

    sqlenv->exec_callback = &sql_callback_str_count_sum_all_count_columns_into_mpz;
    sprintf(statementBuffer, "SELECT * FROM %s;", tableName);
    sqlenv_exec(sqlenv, statementBuffer, (void*)zval);

    SQLENV_TEMP_REVERT;
}

void sql_create_str_count_table_ifnotexist(sqlenv_t *sqlenv, char *statementBuffer, char *tablename)
{
    SQLENV_TEMP_ZERO;

    sprintf(statementBuffer, "CREATE TABLE %s(STR_ID TEXT PRIMARY KEY NOT NULL, COUNT TEXT NOT NULL);", tablename);
    sqlenv_exec(sqlenv, statementBuffer, NULL);

    SQLENV_TEMP_REVERT;
}

void sql_drop_table_if_exists(sqlenv_t *sqlenv, char *statementBuffer, char *tablename)
{
    SQLENV_TEMP_ZERO;

    sprintf(statementBuffer, "DROP TABLE %s;", tablename);
    sqlenv_exec(sqlenv, statementBuffer, NULL);

    SQLENV_TEMP_REVERT;
}

