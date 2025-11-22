#include "turing_tests.h"
#include "turing_sim.h"
#include "turing_mapping.h"
#include "turing_utils.h"
#include "turing_enumerate.h"

#include "hashmap.h"

#include <stdio.h>
#include <string.h>

struct user {
    char *name;
    int age;
};

int user_compare(const void *a, const void *b, void *udata) {
    const struct user *ua = a;
    const struct user *ub = b;
    return strcmp(ua->name, ub->name);
}

bool user_iter(const void *item, void *udata) {
    const struct user *user = item;
    printf("%s (age=%d)\n", user->name, user->age);
    return true;
}

uint64_t user_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const struct user *user = item;
    return hashmap_sip(user->name, strlen(user->name), seed0, seed1);
}

tm_symbol_t fillSymbol = 0;
bool fillRandom = false;
int fillSeed = 100;
void fill_tm_with_symbol(tm_t* tm)
{
    if(fillRandom){
        tm_fill_tape_with_random(tm, fillSeed);
    }else{
        tm_fill_tape(tm, fillSymbol);
    }
}

int main(){
    printf("\nhello turing\n\n");

    tm_t tm;
    tm_t* tm_point = &tm;

    // test_opt_t testOptions;
    // testOptions.onlyPrintFailingTests = false;

    // printf("Running all tests.\n");
    // test_all(&testOptions);
    // printf("Testing complete.\n");


    //run bb4 and print tape slice

    tm_init(&tm);
    tm.states=4;

    char bb4Literal[] = BB4_TABLE_LITERAL;

    tm_load_table(&tm, bb4Literal);
    tm_step_until_halt_or_max(&tm, 99999999999L);

    tape_slice_t slice;
    tm_slice_init_from_written_tape(&tm, &slice);
    tm_slice_print(&slice);
    tm_slice_free(&slice);

    tm_srand(1337);

    struct hashmap* slice_count_map = hashmap_new(
            sizeof(tm_slice_counter_t), 0, tm_rand(), tm_rand(), 
            tm_slicecounter_hashmap_hash,
            tm_slicecounter_hashmap_compare,
            tm_slicecounter_hashmap_free,
            NULL
        );

    int max_steps = 300;
    int states = 2;
    int startIndex = 0;
    int indexesConsidered = tm_max_num_of_machines(states)+1;
    int randomIterations = 500;

    fillSymbol = 0;
    tm_enumerate_index_length_with_hashmap(states, startIndex, indexesConsidered, max_steps,
        slice_count_map,
        fill_tm_with_symbol
    );

    fillSymbol = 1;
    tm_enumerate_index_length_with_hashmap(states, startIndex, indexesConsidered, max_steps,
        slice_count_map,
        fill_tm_with_symbol
    );

    fillSeed = 1;
    fillRandom = true;
    for(int i=0;i< randomIterations ;i++){
        printf("on i %d. \n", i);
        tm_enumerate_index_length_with_hashmap(states, startIndex, indexesConsidered, max_steps,
            slice_count_map,
            fill_tm_with_symbol
        );
        fillSeed++;
    }

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
    while (hashmap_iter(slice_count_map, &iterA, &itemA)) {
        const tm_slice_counter_t *sliceCounter = itemA;
        uint64_t count = sliceCounter->count;
        if(count <= 0)continue;
        int length = sliceCounter->slice.length;
        tm_slice_print(&sliceCounter->slice);
        printf("lengthstr %d, count %lu, freq %lf\n\n", length, count, (double)count/(double)totalCount);
    }

    hashmap_free(slice_count_map);
    

    tm_print_enumerate_performance_stats(2,500);
    // tm_print_enumerate_performance_stats(3,500);


    return 1;





    // create a new hash map where each item is a `struct user`. The second
    // argument is the initial capacity. The third and fourth arguments are 
    // optional seeds that are passed to the following hash function.
    struct hashmap *map = hashmap_new(sizeof(struct user), 0, 0, 0, 
                                     user_hash, user_compare, NULL, NULL);

    // Here we'll load some users into the hash map. Each set operation
    // performs a copy of the data that is pointed to in the second argument.
    hashmap_set(map, &(struct user){ .name="Dale", .age=44 });
    hashmap_set(map, &(struct user){ .name="Roger", .age=68 });
    hashmap_set(map, &(struct user){ .name="Jane", .age=47 });

    struct user *user; 
    
    printf("\n-- get some users --\n");
    user = hashmap_get(map, &(struct user){ .name="Jane" });
    printf("%s age=%d\n", user->name, user->age);

    user = hashmap_get(map, &(struct user){ .name="Roger" });
    printf("%s age=%d\n", user->name, user->age);

    user = hashmap_get(map, &(struct user){ .name="Dale" });
    printf("%s age=%d\n", user->name, user->age);

    user = hashmap_get(map, &(struct user){ .name="Tom" });
    printf("%s\n", user?"exists":"not exists");

    printf("\n-- iterate over all users (hashmap_scan) --\n");
    hashmap_scan(map, user_iter, NULL);

    printf("\n-- iterate over all users (hashmap_iter) --\n");
    size_t iter = 0;
    void *item;
    while (hashmap_iter(map, &iter, &item)) {
        const struct user *user = item;
        printf("%s (age=%d)\n", user->name, user->age);
    }

    hashmap_free(map);
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
    random nth permutation via the factorial algorithm thing.
    n+1th permutation by simple algorithm.
    nth_permutation()
    next_permutation()

turing index enumeration:
    enumerate the turing index language for halting machines and their tape results.
    put each resulting string into the hashmap
*/