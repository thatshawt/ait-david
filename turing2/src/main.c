#include "turing_tests.h"
#include "turing_sim.h"
#include "turing_mapping.h"
#include "turing_utils.h"
#include "turing_enumerate.h"

#include "hashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gmp.h>
#include <sqlite3.h>

static int sql_callback(void *data, int argc, char **argv, char **azColName){
   int i;
   if(data)fprintf(stderr, "%s: ", (const char*)data);
   
   for(i = 0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
   }
   
   printf("\n");
   return 0;
}

typedef struct{
    sqlite3 *db;    // sqlite3 database handle
    char *errMsg;   // error message
    int rc;         // return value
    char *sql;      // sql statement

    // sqlite3_exec callback
    int(*exec_callback)(void *data, int argc, char **argv, char **columnName);

} sqlenv_t;

void sqlenv_open(sqlenv_t *sqlenv, char *databaseFile){
    sqlenv->errMsg = 0;
    /* Open database */
    sqlenv->rc = sqlite3_open("test.db", &sqlenv->db);

    if( rc ) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(sqlenv->db));
        return(0);
    } else {
        fprintf(stderr, "Opened database successfully\n");
    }
}

void sqlenv_exec(sqlenv_t *sqlenv, char *sql_statement, void* data){
    sqlenv->sql = sql_statement;

    /* Execute SQL statement */
    sqlenv->rc = sqlite3_exec(sqlenv->db, sqlenv->sql, sql->exec_callback, data, &sqlenv->errMsg);

    if( sqlenv->rc != SQLITE_OK ){
        fprintf(stderr, "SQL error: %s\n", sqlenv->errMsg);
        sqlite3_free(sqlenv->errMsg);
    } else {
        fprintf(stdout, "Exec successful.\n");
    }

}

void sqlenv_close(sqlenv_t *sqlenv){
     sqlite3_close(sqlenv->db);
}


int main(){
    // mpz_t one,two,sum;

    // mpz_init_set_ui(one, 10);
    // mpz_init_set_ui(two, 5);
    // mpz_init_set_ui(sum, 0);

    // mpz_fdiv_r(sum, one, two);

    // gmp_printf("%Zd / %Zd = %Zd\n", one, two, sum);

    // mpz_clears(one,two,sum,NULL);

    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    char *sql;
    const char* data;

    {
        /* Open database */
        rc = sqlite3_open("test.db", &db);

        if( rc ) {
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
            return(0);
        } else {
            fprintf(stderr, "Opened database successfully\n");
        }
    }

    {
        /* Create SQL statement */
        sql = "CREATE TABLE COMPANY("  \
            "ID INT PRIMARY KEY     NOT NULL," \
            "NAME           TEXT    NOT NULL," \
            "AGE            INT     NOT NULL," \
            "ADDRESS        CHAR(50)," \
            "SALARY         REAL );";

        /* Execute SQL statement */
        rc = sqlite3_exec(db, sql, sql_callback, 0, &zErrMsg);

        if( rc != SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        } else {
            fprintf(stdout, "Table created successfully\n");
        }
    }

    {
        /* Create SQL statement */
        sql = "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "  \
                "VALUES (1, 'Paul', 32, 'California', 20000.00 ); " \
                "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY) "  \
                "VALUES (2, 'Allen', 25, 'Texas', 15000.00 ); "     \
                "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
                "VALUES (3, 'Teddy', 23, 'Norway', 20000.00 );" \
                "INSERT INTO COMPANY (ID,NAME,AGE,ADDRESS,SALARY)" \
                "VALUES (4, 'Mark', 25, 'Rich-Mond ', 65000.00 );";

        /* Execute SQL statement */
        rc = sqlite3_exec(db, sql, sql_callback, 0, &zErrMsg);

        if( rc != SQLITE_OK ){
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        } else {
            fprintf(stdout, "Records created successfully\n");
        }
    }

    {
        data = "Select All";
        /* Create SQL statement */
        sql = "SELECT * from COMPANY";

        /* Execute SQL statement */
        rc = sqlite3_exec(db, sql, sql_callback, (void*)data, &zErrMsg);

        if( rc != SQLITE_OK ) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        } else {
            fprintf(stdout, "Operation done successfully\n");
        }
    }

    {
        data = "Update Salary ID=1 then Select All";
        /* Create merged SQL statement */
        sql = "UPDATE COMPANY set SALARY = 25000.00 where ID=1; " \
                "SELECT * from COMPANY";

        /* Execute SQL statement */
        rc = sqlite3_exec(db, sql, sql_callback, (void*)data, &zErrMsg);

        if( rc != SQLITE_OK ) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        } else {
            fprintf(stdout, "Operation done successfully\n");
        }
    }

    {
        data = "Delete ID=2 then Select All";
        /* Create merged SQL statement */
        sql = "DELETE from COMPANY where ID=2; " \
                "SELECT * from COMPANY";

        /* Execute SQL statement */
        rc = sqlite3_exec(db, sql, sql_callback, (void*)data, &zErrMsg);

        if( rc != SQLITE_OK ) {
            fprintf(stderr, "SQL error: %s\n", zErrMsg);
            sqlite3_free(zErrMsg);
        } else {
            fprintf(stdout, "Operation done successfully\n");
        }
    }


    sqlite3_close(db);

    return 0;




    printf("\nhello turing\n\n");

    printf("main pthread_self() %d\n", pthread_self());
    printf("sizeof(tm_slice_counter_t) %d\n", sizeof(tm_slice_counter_t));
    printf("sizeof(tape_slice_t) %d\n", sizeof(tape_slice_t));

    turing_threading_init_global();

    turing_threading_self_init();

    test_opt_t testOptions;
    testOptions.onlyPrintFailingTests = false;
    test_all(&testOptions);

    // return 0;

    // tm_print_enumerate_performance_stats(2,500);

    const int selfid = turing_threading_self_index();
    tm_srand(selfid, 1337);

    struct hashmap* slice_count_map;
    {
        int states = 2;
        char *max_steps = "300";
        char *randomIterations = "10";
        char *startIndex = "0";
        char *indexesConsidered = NULL;
        int workers = 4;

        slice_count_map = do_tm_enumerate_hashmap_job_wrapped(states, max_steps, randomIterations, startIndex, indexesConsidered, workers);
    }

    size_t iterA = 0;
    void *itemA;
    mpz_t totalCount; mpz_init_set_ui(totalCount, 0);
    while (hashmap_iter(slice_count_map, &iterA, &itemA)) {
        const tm_slice_counter_t *sliceCounter = itemA;
        mpz_add(totalCount, totalCount, sliceCounter->count); // totalCount += count;
    }
    gmp_printf("total counted strings: %Zd\n", totalCount);

    iterA = 0;
    mpz_t count; mpz_init(count);
    mpq_t freq, tempq1; mpq_inits(freq, tempq1, NULL);

    while (hashmap_iter(slice_count_map, &iterA, &itemA)) {
        const tm_slice_counter_t *sliceCounter = itemA;
        mpz_set(count, sliceCounter->count); // count = sliceCounter->count;

        if(mpz_cmp_ui(count, 0) <= 0)continue;

        mpq_set_z(freq, count); // freq = count
        mpq_set_z(tempq1, totalCount); // tempq1 = totalCount

        mpq_canonicalize(freq);
        mpq_canonicalize(tempq1);
        mpq_div(freq, freq, tempq1); // freq = count/totalCount;

        mpq_canonicalize(freq);

        int length = sliceCounter->slice.length;
        tm_slice_print(&sliceCounter->slice);
        gmp_printf("lengthstr %d, count %Zd, freq %lf\n\n", length, count, mpq_get_d(freq));
    }

    mpz_clears(count, totalCount, NULL);
    mpq_clears(freq, tempq1, NULL);

    hashmap_free(slice_count_map);

    turing_threading_destroy();

    printf("done\n");

    return 0;
}

/*
turing simulation:
    variable tape.
    variable states/alphabet/transitions.
    step function.
    halting check.
    error results.

turing index language:
    dense mapping from integers to turing machines.
    n+1th permutation by simple algorithm.
    nth_permutation()
    next_permutation()

turing index enumeration:
    enumerate the turing index language for halting machines and their tape results.
    put each resulting string into the hashmap
*/