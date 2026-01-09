#include "sqlenv.h"

#include "turing_enumerate.h"
#include "hashmap.h"

#include <stdio.h>
#include <string.h>

int sqlenv_open(sqlenv_t *sqlenv, char *databaseFile,
    int(*exec_callback)(void *data, int argc, char **argv, char **columnName),
    void(*resultHandler)(void *sqlenv, enum SQL_RT resultType)
)
{
    sqlenv->exec_callback = exec_callback;
    sqlenv->resultHandler = resultHandler;

    sqlenv->errMsg = 0;
    /* Open database */
    sqlenv->rc = sqlite3_open(databaseFile, &sqlenv->db);

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

void sqlenv_exec_with_callback(sqlenv_t *sqlenv, char *sql_statement, void* data, int(*exec_callback)(SQL_CALLBACK_FUNC_ARG_PROTO))
{
    void* temp_callback = sqlenv->exec_callback; sqlenv->exec_callback = 0;
    void* temp_resulthandler = sqlenv->resultHandler; sqlenv->resultHandler = 0;

    sqlenv->exec_callback = exec_callback;
    sqlenv_exec(sqlenv, sql_statement, data);
    
    sqlenv->exec_callback = temp_callback;
    sqlenv->resultHandler = temp_resulthandler;
}

void sqlenv_exec_with_callback_resulthandler(sqlenv_t *sqlenv, char *sql_statement, void* data, int(*exec_callback)(SQL_CALLBACK_FUNC_ARG_PROTO), void(*resultHandler)(void *sqlenv, enum SQL_RT resultType))
{
    void* temp_callback = sqlenv->exec_callback; sqlenv->exec_callback = 0;
    void* temp_resulthandler = sqlenv->resultHandler; sqlenv->resultHandler = 0;

    sqlenv->resultHandler = resultHandler;
    sqlenv->exec_callback = exec_callback;
    sqlenv_exec(sqlenv, sql_statement, data);
    
    sqlenv->exec_callback = temp_callback;
    sqlenv->resultHandler = temp_resulthandler;
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


int sql_callback_load_firstcol_into_mpz(void *data, int argc, char **argv, char **azColName)
{
    int i=0;
    const char* colName = azColName[i];
    const char* val = argv[i];

    mpz_set_str(*(mpz_t*)data, val, 10);

    return 0;
}

unsigned long sqlenv_get_last_insert_rowid(sqlenv_t* sqlenv)
{
    mpz_t theMpz; mpz_init(theMpz);

    sqlenv_exec_with_callback(sqlenv, "SELECT last_insert_rowid();",
        &theMpz,
        &sql_callback_load_firstcol_into_mpz
    );
    
    unsigned long result = mpz_get_ui(theMpz);
    
    mpz_clear(theMpz);

    return result;
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
    sprintf(statementBuffer, "INSERT INTO %s(STR_ID, COUNT) VALUES ('%s', '%s');",tableName,stringID,countStr);
    sqlenv_exec_with_callback(sqlenv, statementBuffer,
        NULL,
        NULL
    );

    sprintf(statementBuffer, "UPDATE %s SET COUNT = '%s' WHERE STR_ID='%s';",tableName,countStr,stringID);
    sqlenv_exec_with_callback(sqlenv, statementBuffer,
        NULL,
        NULL
    );
}

void sql_load_str_count_into_mpz(sqlenv_t *sqlenv, char *statementBuffer, char *tablename, char *stringID, mpz_t *theMpz)
{
    sprintf(statementBuffer, "SELECT * FROM %s WHERE STR_ID='%s';", tablename, stringID);

    sqlenv_exec_with_callback(sqlenv, statementBuffer,
        (void*)theMpz,
        sql_callback_load_countCol_into_mpz
    );
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

void sql_str_count_get_freq_negativelog2(sqlenv_t *sqlenv, char *statementBuffer, char *tablename, char *strID, mpfr_t a)
{
    mpq_t freq; mpq_init(freq);
    
    // printf("getting frequency...\n");
    sql_str_count_get_freq(sqlenv, statementBuffer, tablename, strID, freq);

    // mpfr_t a; mpfr_inits2(256, a, NULL);

    mpfr_set_q(a, freq, MPFR_RNDZ);
    mpfr_log2(a, a, MPFR_RNDZ);
    mpfr_mul_si(a, a, -1, MPFR_RNDZ);

    // mpfr_printf("freq of '%s' is %lf. -log2 is %.5Rf\n", strID, mpq_get_d(freq), a);

    // mpfr_clear(a);
    mpq_clear(freq);
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
    sprintf(statementBuffer, "SELECT * FROM %s;", tableName);

    sqlenv_exec_with_callback(sqlenv, statementBuffer,
        (void*)zval,
        sql_callback_str_count_sum_all_count_columns_into_mpz
    );
}

void sql_create_str_count_table_ifnotexist(sqlenv_t *sqlenv, char *statementBuffer, char *tablename)
{
    sprintf(statementBuffer, "CREATE TABLE %s(STR_ID TEXT PRIMARY KEY NOT NULL, COUNT TEXT NOT NULL);", tablename);

    sqlenv_exec_with_callback(sqlenv, statementBuffer,
        NULL,
        NULL
    );
}

void sql_drop_table_if_exists(sqlenv_t *sqlenv, char *statementBuffer, char *tablename)
{
    sprintf(statementBuffer, "DROP TABLE %s;", tablename);

    sqlenv_exec_with_callback(sqlenv, statementBuffer,
        NULL,
        NULL
    );

}

void print_freq_sql_str_count_table_string(
    sqlenv_t *sqlenv,
    char *statementBuffer,
    char *tablename,
    char *strID
)
{
    mpq_t freq; mpq_init(freq);
    
    // printf("getting frequency...\n");
    sql_str_count_get_freq(sqlenv, statementBuffer, tablename, strID, freq);

    mpfr_t a; mpfr_inits2(256, a, NULL);

    // printf("getting -log2 of freq...\n");
    sql_str_count_get_freq_negativelog2(sqlenv, statementBuffer, tablename, strID, a);

    mpfr_printf("freq of '%s' is %lf. -log2 is %.5Rf\n", strID, mpq_get_d(freq), a);

    mpfr_clear(a);
    mpq_clear(freq);
}

//this overwrites the table if exists.
void sql_store_slicecount_map_as_sql_table(
    sqlenv_t *sqlenv,
    char *statementBuffer,
    char *tableName,
    struct hashmap *map
)
{
    sql_drop_table_if_exists(sqlenv, statementBuffer, tableName);

    sql_create_str_count_table_ifnotexist(sqlenv, statementBuffer, tableName);

    size_t iterA = 0;
    void *itemA;
    char countStr[1000];
    char stringID[1000];
    while (hashmap_iter(map, &iterA, &itemA)) {
        const tm_slice_counter_t *sliceCounter = itemA;

        tm_slice_sprint(&sliceCounter->slice, stringID);
        mpz_get_str(countStr, 10, sliceCounter->count);
        
        sql_set_str_count(sqlenv, statementBuffer, tableName, stringID, countStr);
        // mpz_add(totalCount, totalCount, sliceCounter->count); // totalCount += count;
    }
}

// this merges an slicecount hashmap into a strcount sql table
//TODO call this function and try it out in main()
void sql_merge_slicecountmap_into_str_count_table
(
    sqlenv_t *sqlenv,
    char *statementBuffer,
    char *tableName,
    struct hashmap *map
)
{
    // sql_drop_table_if_exists(sqlenv, statementBuffer, tableName);

    sql_create_str_count_table_ifnotexist(sqlenv, statementBuffer, tableName);

    size_t iterA = 0;
    void *itemA;
    char countStr[1000];
    char stringID[1000];
    mpz_t count; mpz_init(count);
    while (hashmap_iter(map, &iterA, &itemA)) {
        const tm_slice_counter_t *sliceCounter = itemA;

        tm_slice_sprint(&sliceCounter->slice, stringID);
        // memcpy(stringID, sliceCounter->slice.tapeslice, sliceCounter->slice.length);
        
        // mpz_set(count, sliceCounter->count);
        mpz_set_ui(count, 0);
        sql_load_str_count_into_mpz(sqlenv, statementBuffer, tableName, stringID, &count);
        mpz_add(count, count, sliceCounter->count);

        mpz_get_str(countStr, 10, count);
        
        sql_set_str_count(sqlenv, statementBuffer, tableName, stringID, countStr);
        // printf("set %s to %s\n", stringID, countStr);
        // mpz_add(totalCount, totalCount, sliceCounter->count); // totalCount += count;
    }
    mpz_clear(count);
}

int sql_callback_loadIntoSliceCountMap(void *data, int argc, char **argv, char **columnName)
{
    tape_slice_t slice;
    tm_slice_counter_t slicecounter;
    // printf("hi there");
    for(int i = 0; i<argc; i++){
        const char* colName = columnName[i];
        const char* val = argv[i];

        if(strcmp(colName, "COUNT") == 0){
            mpz_init(slicecounter.count);
            mpz_set_str(slicecounter.count, val, 10);
        }else if(strcmp(colName, "STR_ID") == 0){
            tm_slice_from_cstring(&slice, val);
            slicecounter.slice = slice;
        }
        
    }
    
    // gmp_printf("count: %Zd, slice: '%s', lemgth %d\n", slicecounter.count ,slice.tapeslice, slice.length);

    struct hashmap* slicecounter_map = (struct hashmap*)data;

    hashmap_set(slicecounter_map, &slicecounter);

    // mpz_clear(count);

    return 0;
}

struct hashmap* sql_str_count_get_map(sqlenv_t *sqlenv, char *statementBuffer, char *tableName)
{
    struct hashmap* slicecounter_map = hashmap_new(
            sizeof(tm_slice_counter_t), 0,
            0, 0, 
            tm_slicecounter_hashmap_hash,
            tm_slicecounter_hashmap_compare,
            tm_slicecounter_hashmap_free,
            NULL
        );

    sprintf(statementBuffer, "SELECT * FROM %s;", tableName);

    sqlenv_exec_with_callback(sqlenv, statementBuffer,
        (void*)slicecounter_map,
        &sql_callback_loadIntoSliceCountMap
    );

    return slicecounter_map;
}