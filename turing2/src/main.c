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
#include <mpfr.h>

#include "sqlenv.h"

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
        
        // mpz_set(count, sliceCounter->count);
        mpz_set_ui(count, 0);
        sql_load_str_count_into_mpz(sqlenv, statementBuffer, tableName, stringID, &count);
        mpz_add(count, count, sliceCounter->count);

        mpz_get_str(countStr, 10, count);
        
        sql_set_str_count(sqlenv, statementBuffer, tableName, stringID, countStr);
        // mpz_add(totalCount, totalCount, sliceCounter->count); // totalCount += count;
    }
    mpz_clear(count);
}

void print_freq_sql_str_count_table_string(
    sqlenv_t *sqlenv,
    char *statementBuffer,
    char *tablename,
    char *strID
){
    mpq_t freq; mpq_init(freq);
    
    printf("getting frequency...\n");
    sql_str_count_get_freq(sqlenv, statementBuffer, tablename, strID, freq);

    mpfr_t a; mpfr_inits2(256, a, NULL);

    printf("getting -log2 of freq...\n");
    sql_str_count_get_freq_negativelog2(sqlenv, statementBuffer, tablename, strID, a);

    mpfr_printf("freq of '%s' is %lf. -log2 is %.5Rf\n", strID, mpq_get_d(freq), a);

    mpfr_clear(a);
    mpq_clear(freq);
}

int main(){

    // print various messages.
    // run tests.
    {
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
    }

    // tm_print_enumerate_performance_stats(2,500);

    const int selfid = turing_threading_self_index();
    tm_srand(selfid, 1337);

    // generate slice count hashmap from an enumeration
    struct hashmap* slice_count_map = 0;
    struct hashmap* slice_count_map_halfA = 0;
    struct hashmap* slice_count_map_halfB = 0;
    {
        int states = 2;

        // TODO:
        // try to capture up to 99.999% of the theoretical number of predicted
        // halting machines. you can try sampling 20% of the machines randomly to
        // check how many are captured.
        char *max_steps = "21";
        char *randomStartSeed = "1";
        char *randomIterations = "0";
        bool doZerosTape = true;
        bool doOnesTape = true;
        char *startIndex = "0";
        char *indexesConsidered = NULL;
        int workers = 1;

        slice_count_map = do_tm_enumerate_hashmap_job_wrapped(
            states, max_steps,
            randomIterations, randomStartSeed,
            doOnesTape, doZerosTape,
            startIndex, indexesConsidered,
            workers);


        startIndex = "0";
        indexesConsidered = "10369";

        slice_count_map_halfA = do_tm_enumerate_hashmap_job_wrapped(
            states, max_steps,
            randomIterations, randomStartSeed,
            doOnesTape, doZerosTape,
            startIndex, indexesConsidered,
            workers);

        startIndex = "10369";
        indexesConsidered = NULL;

        slice_count_map_halfB = do_tm_enumerate_hashmap_job_wrapped(
            states, max_steps,
            randomIterations, randomStartSeed,
            doOnesTape, doZerosTape,
            startIndex, indexesConsidered,
            workers);
        // TODO;
        // consolidate all the code below and create a function like this:
        // do_tm_enumerate_sql_merge_job_wrapped().
        // which still does the hashmap thing but returns nothing and instead
        // merges the result into an sql table of your choosing.
    }

    // traverse slice count hashmaps
    {
        // original
        size_t iterA = 0;
        void *itemA;
        mpz_t totalCount; mpz_init_set_ui(totalCount, 0);

        if(slice_count_map != 0){
            mpz_set_ui(totalCount, 0);
            while (hashmap_iter(slice_count_map, &iterA, &itemA)) {
                const tm_slice_counter_t *sliceCounter = itemA;
                mpz_add(totalCount, totalCount, sliceCounter->count); // totalCount += count;
            }
            gmp_printf("total counted strings original: %Zd\n", totalCount);
        }

        // half A
        if(slice_count_map_halfA != 0){
            iterA = 0;
            mpz_set_ui(totalCount, 0);
            while (hashmap_iter(slice_count_map_halfA, &iterA, &itemA)) {
                const tm_slice_counter_t *sliceCounter = itemA;
                mpz_add(totalCount, totalCount, sliceCounter->count); // totalCount += count;
            }
            gmp_printf("total counted strings half A: %Zd\n", totalCount);
        }

        // half B
        if(slice_count_map_halfB != 0){
            iterA = 0;
            mpz_set_ui(totalCount, 0);
            while (hashmap_iter(slice_count_map_halfB, &iterA, &itemA)) {
                const tm_slice_counter_t *sliceCounter = itemA;
                mpz_add(totalCount, totalCount, sliceCounter->count); // totalCount += count;
            }
            gmp_printf("total counted strings half B: %Zd\n", totalCount);
        }

        mpz_clear(totalCount);

        /* iterA = 0;
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
            // tm_slice_print(&sliceCounter->slice);
            // gmp_printf("lengthstr %d, count %Zd, freq %lf\n\n", length, count, mpq_get_d(freq));
        }
        mpz_clears(count, totalCount, NULL);
        mpq_clears(freq, tempq1, NULL);
        */
    }
    
    sqlenv_t sqlenv;
    if(sqlenv_open(&sqlenv, "artifacts/test.db", 0, 0))return 1;

    const char *tablename = "NumbersCount2";
    char statementBuffer[2000];

    // do stuff for original hashmap
    {
        //store into sql as table
        printf("\nStoring hashmap into sqltable...\n");
        sql_store_slicecount_map_as_sql_table(&sqlenv, statementBuffer, tablename, slice_count_map);

        printf("Summing all counts...\n");
        mpz_t zval; mpz_init_set_ui(zval, 0);

        sql_str_count_sum_all_count(&sqlenv, statementBuffer, tablename, &zval);

        gmp_printf("Total strings from sql: %Zd\n", zval);

        char *strID = "1";
        print_freq_sql_str_count_table_string(&sqlenv, statementBuffer, tablename, strID);

        mpz_clear(zval);
    }

    // do stuff for original hashmap half A
    {
        tablename = "NumbersHalfA";
        //store into sql as table
        printf("\nStoring hashmap half A into sqltable...\n");
        sql_store_slicecount_map_as_sql_table(&sqlenv, statementBuffer, tablename, slice_count_map_halfA);

        printf("Summing all counts...\n");
        mpz_t zval; mpz_init_set_ui(zval, 0);

        sql_str_count_sum_all_count(&sqlenv, statementBuffer, tablename, &zval);

        gmp_printf("Total strings from sql: %Zd\n", zval);

        char *strID = "1";
        print_freq_sql_str_count_table_string(&sqlenv, statementBuffer, tablename, strID);

        mpz_clear(zval);
    }

    // do stuff for original hashmap half B
    {
        tablename = "NumbersHalfB";
        //store into sql as table
        printf("\nStoring hashmap half B into sqltable...\n");
        sql_store_slicecount_map_as_sql_table(&sqlenv, statementBuffer, tablename, slice_count_map_halfB);

        printf("Summing all counts...\n");
        mpz_t zval; mpz_init_set_ui(zval, 0);

        sql_str_count_sum_all_count(&sqlenv, statementBuffer, tablename, &zval);

        gmp_printf("Total strings from sql: %Zd\n", zval);

        char *strID = "1";
        print_freq_sql_str_count_table_string(&sqlenv, statementBuffer, tablename, strID);

        mpz_clear(zval);
    }

    // merge B into A, then print stuff for A.
    {
        tablename = "NumbersHalfA";
        printf("\nmerging hashmap half B into sql half A...\n");
        sql_merge_slicecountmap_into_str_count_table(&sqlenv, statementBuffer, tablename, slice_count_map_halfB);

        //store into sql as table
        // printf("\nStoring hashmap into sqltable...\n");
        // sql_store_slicecount_map_as_sql_table(&sqlenv, statementBuffer, tablename, slice_count_map_halfA);

        printf("Summing all counts...\n");
        mpz_t zval; mpz_init_set_ui(zval, 0);

        sql_str_count_sum_all_count(&sqlenv, statementBuffer, tablename, &zval);

        gmp_printf("Total strings from sql: %Zd\n", zval);

        char *strID = "1";
        print_freq_sql_str_count_table_string(&sqlenv, statementBuffer, tablename, strID);

        mpz_clear(zval);
    }

    sqlenv_close(&sqlenv);

    // free things
    hashmap_free(slice_count_map);
    hashmap_free(slice_count_map_halfA);
    hashmap_free(slice_count_map_halfB);

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