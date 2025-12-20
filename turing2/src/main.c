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

#include "sqlenv.h"


#define TO_STRING(x) #x
#define COMPANY_TABLE_STRING_SANDWHICH(a, b) a TO_STRING(COMPANY(ID INT PRIMARY KEY NOT NULL,NAME TEXT NOT NULL,AGE INT NOT NULL,ADDRESS CHAR(50), SALARY REAL)) b

void sql_set_str_count(sqlenv_t *sqlenv, char *statementBuffer, char *stringID, char *countStr){
    void* temp_callback = sqlenv->exec_callback; sqlenv->exec_callback = 0;
    void* temp_resulthandler = sqlenv->resultHandler; sqlenv->resultHandler = 0;

    sprintf(statementBuffer, "INSERT INTO NumbersCount(STR_ID, COUNT) VALUES ('%s', '%s')",stringID,countStr);
    sqlenv_exec(sqlenv, statementBuffer, NULL);

    sprintf(statementBuffer, "UPDATE NumbersCount SET COUNT = '%s' WHERE STR_ID='%s';",countStr,stringID);
    sqlenv_exec(sqlenv, statementBuffer, NULL);
    
    sqlenv->exec_callback = temp_callback; sqlenv->resultHandler = temp_resulthandler;
}

void sql_load_str_count_into_mpz(sqlenv_t *sqlenv, char *statementBuffer, char *stringID, mpz_t *theMpz){
    void* temp_callback = sqlenv->exec_callback; sqlenv->exec_callback = 0;
    void* temp_resulthandler = sqlenv->resultHandler; sqlenv->resultHandler = 0;

    sqlenv->exec_callback = &sql_callback_load_countCol_into_mpz;
    sprintf(statementBuffer, "SELECT * FROM NumbersCount WHERE STR_ID='%s';", stringID);
    sqlenv_exec(sqlenv, statementBuffer, (void*)theMpz);
    
    sqlenv->exec_callback = temp_callback; sqlenv->resultHandler = temp_resulthandler;
}

void sql_create_str_count_table_ifnotexist(sqlenv_t *sqlenv){
    void* temp_callback = sqlenv->exec_callback; sqlenv->exec_callback = 0;
    void* temp_resulthandler = sqlenv->resultHandler; sqlenv->resultHandler = 0;

    sqlenv_exec(sqlenv, "CREATE TABLE NumbersCount(STR_ID TEXT PRIMARY KEY NOT NULL, COUNT TEXT NOT NULL);", NULL);

    sqlenv->exec_callback = temp_callback; sqlenv->resultHandler = temp_resulthandler;
}

int main(){
    // mpz_t one,two,sum;

    // mpz_init_set_ui(one, 10);
    // mpz_init_set_ui(two, 5);
    // mpz_init_set_ui(sum, 0);

    // mpz_fdiv_r(sum, one, two);

    // gmp_printf("%Zd / %Zd = %Zd\n", one, two, sum);

    // char *buff = mpz_get_str(NULL, 10, sum);
    // printf("from buffer. sum = '%s'\n", buff);
    // free(buff);

    // mpz_clears(one,two,sum,NULL);

    // return 0;

    {
        // start
        sqlenv_t sqlenv;
        // sqlenv.resultHandler = &sql_resultHandler_print;
        // sqlenv.exec_callback = &sql_callback_print;

        if(sqlenv_open(&sqlenv, "test.db", 0, 0))return 1;

        // create table if not exists
        sql_create_str_count_table_ifnotexist(&sqlenv);
        
        // increment count of a str
        char* stringID = "123";

        char statementBuffer[2000];

        mpz_t count; mpz_init_set_ui(count, 0);

        sql_load_str_count_into_mpz(&sqlenv, statementBuffer, stringID, &count);
        
        mpz_add_ui(count,count,1);

        char* countStr = mpz_get_str(NULL, 10, count);

        sql_set_str_count(&sqlenv, statementBuffer, stringID, countStr);

        mpz_clear(count);
        free(countStr);

        // print all
        sqlenv.exec_callback = &sql_callback_print;
        sqlenv_exec(&sqlenv, "SELECT * FROM NumbersCount;", NULL);

        // done
        sqlenv_close(&sqlenv);

        return 0;
    }





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