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

int main(){
    // mpz_t one,two,sum;

    // mpz_init_set_ui(one, 10);
    // mpz_init_set_ui(two, 5);
    // mpz_init_set_ui(sum, 0);

    // mpz_fdiv_r(sum, one, two);

    // gmp_printf("%Zd / %Zd = %Zd\n", one, two, sum);

    // mpz_clears(one,two,sum,NULL);

    sqlite3 *db;
    int err = sqlite3_open("test.db", &db);

    if(err){
        printf("%s\n", sqlite3_errmsg(db));
        return 1;
    }else{
        printf("opened database succesfully\n");
    }


    sqlite3_stmt *selectAllFromTbl1;
    // int sqlite3_prepare_v2(
    //     sqlite3 *db,            /* Database handle */
    //     const char *zSql,       /* SQL statement, UTF-8 encoded */
    //     int nByte,              /* Maximum length of zSql in bytes. */
    //     sqlite3_stmt **ppStmt,  /* OUT: Statement handle */
    //     const char **pzTail     /* OUT: Pointer to unused portion of zSql */
    // );
    err = sqlite3_prepare_v2(
        db, "SELECT * FROM tbl1;",
        -1, &selectAllFromTbl1,
        NULL
    );

    if(err){
        fprintf(stderr, "could not prepare statment\n");
        return 1;
    }else{
        printf("prepared statement\n");
    }
    
    err = sqlite3_step(selectAllFromTbl1);
    printf("row 0, col0 '%s', col1 '%d'\n",
        sqlite3_column_text(selectAllFromTbl1, 0), 
        sqlite3_column_int(selectAllFromTbl1, 1)
    );

    err = sqlite3_step(selectAllFromTbl1);
    printf("row 1, col0 '%s', col1 '%d'\n",
        sqlite3_column_text(selectAllFromTbl1, 0), 
        sqlite3_column_int(selectAllFromTbl1, 1)
    );

    sqlite3_finalize(selectAllFromTbl1);
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