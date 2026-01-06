#ifndef TURING_ENUMERATE_H
#define TURING_ENUMERATE_H

#include <stdbool.h>
#include "turing_mapping.h"
#include "hashmap.h"
#include "sqlenv.h"

//wowzahipitatious. skib skib?
// skib SKIB SKIB

typedef struct{
    tape_slice_t slice;
    mpz_t count;
} tm_slice_counter_t;

// merge b into a.
void tm_slicecounter_hashmap_merge(struct hashmap* mapA, struct hashmap* mapB);
void tm_slicecounter_hashmap_free(void *item);
uint64_t tm_slicecounter_hashmap_hash(const void *item, uint64_t seed0, uint64_t seed1);
int tm_slicecounter_hashmap_compare(const void *a, const void *b, void *udata);

void tm_print_enumerate_performance_stats(int states, mpz_t max_steps);

typedef struct{
    int states;

    mpz_t start;
    mpz_t length;

    mpz_t max_steps;

    bool doOnesTape;
    bool doZerosTape;

    mpz_t randomIterations;
    mpz_t randomStartSeed;

    int workthreads;
} enumerate_job_opt_t;

void tm_enumerate_job_opt_init(enumerate_job_opt_t* opt);
void tm_enumerate_job_opt_destroy(enumerate_job_opt_t* opt);

typedef struct {
    int states;
    char *max_steps;
    char *randomIterations;
    char *randomStartSeed;
    bool doOnesTape;
    bool doZerosTape;
    char *startIndex;
    char *indexesConsidered; 
    int workers;
} tm_enumerate_options_simple_t;

struct hashmap* do_tm_enumerate_hashmap_job_wrapped(
    tm_enumerate_options_simple_t enumerate_simple_opt
);

bool do_tm_enumerate_sql_merge_job_wrapped(
    tm_enumerate_options_simple_t enumerate_simple_opt,

    sqlenv_t *sqlenv,
    char statementBuffer[],
    char *tablename
);
    
struct hashmap* do_tm_enumerate_job(enumerate_job_opt_t *opt);

void tm_enumerate_index_length_generic(
    int states,
    mpz_t start,
    mpz_t length,
    mpz_t max_steps,
    void(*halt_receiver)(tm_t* tm),
    void(*before_stepping)(tm_t* tm, void* data),
    void* data
);

void tm_enumerate_index_length_with_hashmap(
    int states,
    mpz_t start,
    mpz_t length,
    mpz_t max_steps,
    struct hashmap* map,
    void(*before_stepping)(tm_t* tm, void* data),
    void* data
);

#endif