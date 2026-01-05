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

// int trivialNonHalters = 0;

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
    // struct hashmap* slice_count_map = 0;
    // struct hashmap* slice_count_map_halfA = 0;
    // struct hashmap* slice_count_map_halfB = 0;

    char *originalTableName = "NumbersCount";
    char *halfATableName = "NumbersCountHalfA";
    char *halfBTableName = "NumbersCountHalfB";

    sqlenv_t sqlenv;
    if(sqlenv_open(&sqlenv, "artifacts/test.db", 0, 0))return 1;
    char statementBuffer[2000];
    {
        int states = 2;

        // TODO:
        // try to capture up to 99.999% of the theoretical number of predicted
        // halting machines. you can try sampling 20% of the machines randomly to
        // check how many are captured.
        char *max_steps = "100";
        char *randomStartSeed = "1";
        char *randomIterations = "0";
        bool doZerosTape = true;
        bool doOnesTape = false;
        char *startIndex = "0";
        char *indexesConsidered = NULL;
        int workers = 4;
        
        sql_drop_table_if_exists(&sqlenv, statementBuffer, originalTableName);
        do_tm_enumerate_sql_merge_job_wrapped(states, max_steps, randomIterations, randomStartSeed, doOnesTape, doZerosTape, startIndex, indexesConsidered, workers,
            &sqlenv, statementBuffer, originalTableName
        );
        // printf("trivial nonHalters found: %d\n", trivialNonHalters);
        // trivialNonHalters = 0;
        // slice_count_map = sql_str_count_get_map(&sqlenv, statementBuffer, originalTableName);

        startIndex = "0";
        indexesConsidered = "10369";

        sql_drop_table_if_exists(&sqlenv, statementBuffer, halfATableName);
        do_tm_enumerate_sql_merge_job_wrapped(states, max_steps, randomIterations, randomStartSeed, doOnesTape, doZerosTape, startIndex, indexesConsidered, workers,
            &sqlenv, statementBuffer, halfATableName
        );
        // printf("trivial nonHalters found: %d\n", trivialNonHalters);
        // trivialNonHalters = 0;
        // slice_count_map_halfA = sql_str_count_get_map(&sqlenv, statementBuffer, halfATableName);

        startIndex = "10369";
        indexesConsidered = NULL;

        sql_drop_table_if_exists(&sqlenv, statementBuffer, halfBTableName);
        do_tm_enumerate_sql_merge_job_wrapped(states, max_steps, randomIterations, randomStartSeed, doOnesTape, doZerosTape, startIndex, indexesConsidered, workers,
            &sqlenv, statementBuffer, halfBTableName
        );
        // printf("trivial nonHalters found: %d\n", trivialNonHalters);
        // trivialNonHalters = 0;
        // slice_count_map_halfB = sql_str_count_get_map(&sqlenv, statementBuffer, halfBTableName);
    }

    // traverse slice count hashmaps
    {
        // original
        size_t iterA = 0;
        void *itemA;
        mpz_t totalCount; mpz_init_set_ui(totalCount, 0);

        // if(slice_count_map != 0){
        //     mpz_set_ui(totalCount, 0);
        //     while (hashmap_iter(slice_count_map, &iterA, &itemA)) {
        //         const tm_slice_counter_t *sliceCounter = itemA;
        //         mpz_add(totalCount, totalCount, sliceCounter->count); // totalCount += count;
        //         char buff[100];
        //         tm_slice_sprint(&sliceCounter->slice, buff);
        //         gmp_printf("'%s' %Zd\n", buff, sliceCounter->count);
        //     }
        //     gmp_printf("total counted strings original: %Zd\n", totalCount);
        // }

        // half A
        // if(slice_count_map_halfA != 0){
        //     iterA = 0;
        //     mpz_set_ui(totalCount, 0);
        //     while (hashmap_iter(slice_count_map_halfA, &iterA, &itemA)) {
        //         const tm_slice_counter_t *sliceCounter = itemA;
        //         mpz_add(totalCount, totalCount, sliceCounter->count); // totalCount += count;
        //         char buff[100];
        //         tm_slice_sprint(&sliceCounter->slice, buff);
        //         gmp_printf("'%s' %Zd\n", buff, sliceCounter->count);
        //     }
        //     gmp_printf("total counted strings half A: %Zd\n", totalCount);
        // }

        // half B
        // if(slice_count_map_halfB != 0){
        //     iterA = 0;
        //     mpz_set_ui(totalCount, 0);
        //     while (hashmap_iter(slice_count_map_halfB, &iterA, &itemA)) {
        //         const tm_slice_counter_t *sliceCounter = itemA;
        //         mpz_add(totalCount, totalCount, sliceCounter->count); // totalCount += count;
        //         char buff[100];
        //         tm_slice_sprint(&sliceCounter->slice, buff);
        //         gmp_printf("'%s' %Zd\n", buff, sliceCounter->count);
        //     }
        //     gmp_printf("total counted strings half B: %Zd\n", totalCount);
        // }

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

    // do stuff for original hashmap
    {
        printf("Summing all counts for original...\n");
        mpz_t zval; mpz_init_set_ui(zval, 0);

        sql_str_count_sum_all_count(&sqlenv, statementBuffer, originalTableName, &zval);

        gmp_printf("Total strings from sql: %Zd\n", zval);

        char *strID = "1";
        print_freq_sql_str_count_table_string(&sqlenv, statementBuffer, originalTableName, strID);

        mpz_clear(zval);
    }

    // do stuff for original hashmap half A
    {
        printf("Summing all counts...\n");
        mpz_t zval; mpz_init_set_ui(zval, 0);

        sql_str_count_sum_all_count(&sqlenv, statementBuffer, halfATableName, &zval);

        gmp_printf("Total strings from sql: %Zd\n", zval);

        char *strID = "1";
        print_freq_sql_str_count_table_string(&sqlenv, statementBuffer, halfATableName, strID);

        mpz_clear(zval);
    }

    // do stuff for original hashmap half B
    {
        printf("Summing all counts...\n");
        mpz_t zval; mpz_init_set_ui(zval, 0);

        sql_str_count_sum_all_count(&sqlenv, statementBuffer, halfBTableName, &zval);

        gmp_printf("Total strings from sql: %Zd\n", zval);

        char *strID = "1";
        print_freq_sql_str_count_table_string(&sqlenv, statementBuffer, halfBTableName, strID);

        mpz_clear(zval);
    }

    // merge B into A, then print stuff for A.
    {
        struct hashmap* slice_count_map_halfB = sql_str_count_get_map(&sqlenv, statementBuffer, halfBTableName);
        printf("\nmerging hashmap half B into sql half A...\n");
        sql_merge_slicecountmap_into_str_count_table(&sqlenv, statementBuffer, halfATableName, slice_count_map_halfB);

        hashmap_free(slice_count_map_halfB);

        printf("Summing all counts...\n");
        mpz_t zval; mpz_init_set_ui(zval, 0);
        sql_str_count_sum_all_count(&sqlenv, statementBuffer, halfATableName, &zval);
        gmp_printf("Total strings from sql: %Zd\n", zval);

        char *strID = "1";
        print_freq_sql_str_count_table_string(&sqlenv, statementBuffer, halfATableName, strID);

        mpz_clear(zval);

        // sprintf(statementBuffer, "SELECT * FROM %s;", halfATableName);
        // sqlenv_exec_with_callback(&sqlenv, statementBuffer, NULL, &sql_callback_print);
    }

    sqlenv_close(&sqlenv);

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