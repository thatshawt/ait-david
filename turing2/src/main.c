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

/*
TODO: create this below.

    // this merges with existing table or creates a new one
    sql_str_count_merge_with_slicecount_hashmap
        sqlenv_t *sqlenv,
        char *statementBuffer,
        char *tableName
        struct hashmap *map, 
    );

*/ 

/*
TODO: sql tests.?
sum all state 2 strings test.?
frequency of certain strings test.?
*/

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

    // floating point -log2 test
    {
        mpfr_t a;
        mpfr_inits2(30, a, NULL);

        double op = 0.0123123;

        mpfr_set_d(a, op, MPFR_RNDN);
        mpfr_log2 (a, a, MPFR_RNDZ);
        mpfr_mul_si(a, a, -1, MPFR_RNDZ);

        mpfr_printf("log2(%lf) = %.10Rf\n", op, a);

        mpfr_clears(a, NULL);
        // return 1;
    }

    // incrementing into a str_count table experiment code
    {
        // start
        sqlenv_t sqlenv;
        // sqlenv.resultHandler = &sql_resultHandler_print;
        // sqlenv.exec_callback = &sql_callback_print;

        char statementBuffer[2000];

        if(sqlenv_open(&sqlenv, "test.db", 0, 0))return 1;

        // create table if not exists
        sql_create_str_count_table_ifnotexist(&sqlenv, statementBuffer, "NumbersCount");
        
        // increment count of a str
        char* stringID = "123";


        mpz_t count; mpz_init_set_ui(count, 0);

        sql_load_str_count_into_mpz(&sqlenv, statementBuffer, "NumbersCount", stringID, &count);
        
        mpz_add_ui(count,count,1);

        char *countStr = mpz_get_str(NULL, 10, count);

        sql_set_str_count(&sqlenv, statementBuffer, "NumbersCount", stringID, countStr);

        mpz_clear(count);
        free(countStr);

        // print all
        sqlenv.exec_callback = &sql_callback_print;
        sqlenv_exec(&sqlenv, "SELECT * FROM NumbersCount;", NULL);

        // drop the table i guess
        sql_drop_table_if_exists(&sqlenv, statementBuffer, "NumbersCount");

        // done
        sqlenv_close(&sqlenv);

        // return 0;
    }

    // print various messages and test all
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
    }

    // return 0;

    // tm_print_enumerate_performance_stats(2,500);

    const int selfid = turing_threading_self_index();
    tm_srand(selfid, 1337);

    // generate slice count hashmap from an enumeration
    struct hashmap* slice_count_map;
    {
        int states = 3;

        //TODO make a thing that samples a max_steps that captures 99.999% halting machines.
        // i figure sampling 20% of all machines is a confident way to measure that...
        // idea:  find_first_power_of_two_max_steps(desired_percent_halters:float) -> max_steps:
        //          # desired_percent is gonna be something like "0.9999"
        //          start at 2 max_steps.
        //          "sample 20% of randomly picked machine indexes"().
        //          if "number of halted machines" / "number of machines sampled" < desired_percent_halters:
        //              sample again with max_steps *= 2;
        //          else:
        //              return max_steps;
        char *max_steps = "21";
        char *randomIterations = "0";
        char *randomStartSeed = "1";
        bool doZerosTape = true;
        bool doOnesTape = true;
        char *startIndex = "0";
        char *indexesConsidered = NULL;
        int workers = 4;

        slice_count_map = do_tm_enumerate_hashmap_job_wrapped(
            states, max_steps,
            randomIterations, randomStartSeed,
            doOnesTape, doZerosTape,
            startIndex, indexesConsidered,
            workers);
    }

    // traverse slice conut hashmap
    {
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
    }

    //store into sql as table
    const char *tablename = "NumbersCount2";
    sqlenv_t sqlenv;
    if(sqlenv_open(&sqlenv, "test.db", 0, 0))return 1;
    {
        char statementBuffer[2000];

        printf("Storing hashmap into sqltable...\n");
        sql_store_slicecount_map_as_sql_table(&sqlenv, statementBuffer, tablename, slice_count_map);

    }

    // print out total counted strings from sql
    {
        char statementBuffer[2000];

        printf("Summing all counts...\n");
        mpz_t zval; mpz_init_set_ui(zval, 0);
        sql_str_count_sum_all_count(&sqlenv, statementBuffer, tablename, &zval);

        gmp_printf("Total strings from sql: %Zd\n", zval);

        mpz_clear(zval);

        // sqlenv.exec_callback = &sql_callback_print;
        // sqlenv_exec(&sqlenv, "SELECT * FROM NumbersCount2;", NULL);

    }

    // print frequency,-log2 of a string from sql
    {
        char statementBuffer[2000];
        char *strID = "1";
        mpq_t freq; mpq_init(freq);
        
        printf("getting frequency...\n");
        sql_str_count_get_freq(&sqlenv, statementBuffer, tablename, strID, freq);

        mpfr_t a; mpfr_inits2(256, a, NULL);

        mpfr_set_q(a, freq, MPFR_RNDZ);
        mpfr_log2(a, a, MPFR_RNDZ);
        mpfr_mul_si(a, a, -1, MPFR_RNDZ);

        mpfr_printf("freq of '%s' is %lf. -log2 is %.5Rf\n", strID, mpq_get_d(freq), a);

        mpfr_clear(a);
        mpq_clear(freq);
    }

    sqlenv_close(&sqlenv);

    // free things
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