#include "turing_tests.h"
#include "turing_sim.h"
#include "turing_mapping.h"
#include "turing_utils.h"
#include "turing_enumerate.h"

#include "hashmap.h"

#include <stdio.h>
#include <string.h>

int main(){
    printf("\nhello turing\n\n");

    printf("main pthread_self() %d\n", pthread_self());
    printf("sizeof(tm_slice_counter_t) %d\n", sizeof(tm_slice_counter_t));
    printf("sizeof(tape_slice_t) %d\n", sizeof(tape_slice_t));

    turing_threading_init_global();

    turing_threading_self_init();

    // printf("did init?");

    // printf("self turing thread id %d", turing_threading_self_index());

    test_opt_t testOptions;
    testOptions.onlyPrintFailingTests = false;

    test_all(&testOptions);

    // return 1;

    // tm_print_enumerate_performance_stats(2,500);

    const int selfid = turing_threading_self_index();
    tm_srand(selfid, 1337);

    int states = 2;
    enumerate_job_opt_t enumerateOpt = (enumerate_job_opt_t){
        .length=tm_max_num_of_machines(states)+1,
        .states=states,
        .max_steps=300,
        .randomIterations=100,
        .start=0,
        .workthreads=4
    };

    struct hashmap* slice_count_map = do_tm_enumerate_job(&enumerateOpt);

    size_t iterA = 0;
    void *itemA;
    uint64_t totalCount = 0;
    while (hashmap_iter(slice_count_map, &iterA, &itemA)) {
        const tm_slice_counter_t *sliceCounter = itemA;
        uint64_t count = sliceCounter->count;
        totalCount += count;
    }
    printf("total counted strings: %lu\n", totalCount);

    iterA = 0;
    // totalCount = indexesConsidered - 1;
    while (hashmap_iter(slice_count_map, &iterA, &itemA)) {
        const tm_slice_counter_t *sliceCounter = itemA;
        uint64_t count = sliceCounter->count;
        if(count <= 0)continue;
        int length = sliceCounter->slice.length;
        tm_slice_print(&sliceCounter->slice);
        printf("lengthstr %d, count %lu, freq %lf\n\n",
            length, count, (double)count/(double)totalCount);
    }

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