#ifndef TURING_ENUMERATE_H
#define TURING_ENUMERATE_H

#include "turing_mapping.h"
#include "hashmap.h"

//wowzahipitatious.

typedef struct{
    tape_slice_t slice;
    uint64_t count; //TODO make this an arbtritrary precision integer with the library
} tm_slice_counter_t;

void tm_slicecounter_hashmap_free(void *item);
uint64_t tm_slicecounter_hashmap_hash(const void *item, uint64_t seed0, uint64_t seed1);
int tm_slicecounter_hashmap_compare(const void *a, const void *b, void *udata);

void tm_print_enumerate_performance_stats(int states, int max_steps);

typedef struct{
    int states;
    tm_index_t start;
    int length;
    uint64_t max_steps;
    int randomIterations;
} enumerate_job_opt_t;

struct hashmap* do_tm_enumerate_job(enumerate_job_opt_t *opt);

void tm_enumerate_index_length_generic(
    int states,
    tm_index_t start,
    int length,
    uint64_t max_steps,
    void(*halt_receiver)(tm_t* tm),
    void(*before_stepping)(tm_t* tm)
);

void tm_enumerate_index_length_with_hashmap(
    int states,
    tm_index_t start,
    int length,
    uint64_t max_steps,
    struct hashmap* map,
    void(*before_stepping)(tm_t* tm)
);

#endif