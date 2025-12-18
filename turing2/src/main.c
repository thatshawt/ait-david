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

int main(){
    // mpz_t one,two,sum;

    // mpz_init_set_ui(one, 10);
    // mpz_init_set_ui(two, 5);
    // mpz_init_set_ui(sum, 0);

    // mpz_fdiv_r(sum, one, two);

    // gmp_printf("%Zd / %Zd = %Zd\n", one, two, sum);

    // mpz_clears(one,two,sum,NULL);

    // return 0;

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
        char *randomIterations = "1000";
        char *startIndex = "0";
        char *indexesConsidered = NULL;
        int workers = 4;

        bool mustFree_indexesConsidered = false;
        if(indexesConsidered == NULL){
            mpz_t indexesConsidered_z; mpz_init(indexesConsidered_z);

            tm_max_num_of_machines(states, indexesConsidered_z);
            mpz_add_ui(indexesConsidered_z, indexesConsidered_z, 1);

            indexesConsidered = mpz_get_str(NULL, 10, indexesConsidered_z);

            mpz_clear(indexesConsidered_z);
            mustFree_indexesConsidered = true;
        }

        enumerate_job_opt_t enumerateOpt = (enumerate_job_opt_t){
            // .length=tm_max_num_of_machines(states)+1,
            // .max_steps=300,
            // .randomIterations=10,
            // .start=0,
            .states=states,
            .workthreads=workers
        };
        tm_enumerate_job_opt_init(&enumerateOpt);

        mpz_set_str(enumerateOpt.length, indexesConsidered, 10);
        mpz_set_str(enumerateOpt.max_steps, max_steps, 10);
        mpz_set_str(enumerateOpt.randomIterations, randomIterations, 10);
        mpz_set_str(enumerateOpt.start, startIndex, 10);

        slice_count_map = do_tm_enumerate_job(&enumerateOpt);

        tm_enumerate_job_opt_destroy(&enumerateOpt);
        if(mustFree_indexesConsidered)free(indexesConsidered);
    }

    size_t iterA = 0;
    void *itemA;
    mpz_t totalCount; mpz_init_set_ui(totalCount, 0);
    while (hashmap_iter(slice_count_map, &iterA, &itemA)) {
        const tm_slice_counter_t *sliceCounter = itemA;
        mpz_add(totalCount, totalCount, sliceCounter->count);// totalCount += count;
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