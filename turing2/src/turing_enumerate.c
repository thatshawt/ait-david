#include "turing_enumerate.h"
#include "turing_sim.h"
#include "hashmap.h"
#include "turing_threading.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#define MIN(a,b) ((a)<(b) ? (a):(b))
#define MAX(a,b) ((a)>(b) ? (a):(b))

uint64_t current_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return (uint64_t)milliseconds;
}

tm_symbol_t fillSymbol[TM_MAX_THREADS] = {0};
bool fillRandom[TM_MAX_THREADS] = {false};
int fillSeed[TM_MAX_THREADS] = {0};
int fillMaxSteps[TM_MAX_THREADS] = {0};

void resetFillTapeAlgorithm(){
    for(int i=0;i<TM_MAX_THREADS;i++){
        fillSymbol[i] = 0;
        fillRandom[i] = false;
        fillSeed[i] = 0;
        fillMaxSteps[i] = 0;
    }
}

void fill_tm_with_symbol(tm_t* tm)
{
    const int selfid = turing_threading_self_index();

    const int maxSteps = fillMaxSteps[selfid];
    const int startTapeI = TM_TAPE_SIZE/2 - maxSteps - 1;
    const int endTapeI = TM_TAPE_SIZE/2 + maxSteps + 1;
    if(fillRandom[selfid]){
        tm_fill_tape_with_random_range(tm, fillSeed[selfid], startTapeI, endTapeI);
    }else{
        tm_fill_tape_range(tm, fillSymbol[selfid], startTapeI, endTapeI);
    }
}

void tm_slicecounter_hashmap_merge(struct hashmap* mapA, struct hashmap* mapB)
{
    size_t iter = 0;
    void *item;
    while (hashmap_iter(mapB, &iter, &item)) {
        const tm_slice_counter_t *sliceFromB = item;
        tm_slice_counter_t *sliceFromA = hashmap_get(mapA, sliceFromB);

        if(sliceFromA != NULL){
            mpz_add(sliceFromA->count, sliceFromA->count, sliceFromB->count);//sliceFromA->count += sliceFromB->count;
        }else{
            tm_slice_counter_t newSliceForA;
            mpz_init_set(newSliceForA.count, sliceFromB->count); // newSliceForA.count = sliceFromB->count;
            newSliceForA.slice = tm_slice_clone(&sliceFromB->slice);
            hashmap_set(mapA, &newSliceForA);
        }
    }
}

typedef struct{
    enumerate_job_opt_t opt;
    struct hashmap* slice_count_map;
} enumerate_job_args_t;

void* enumerate_job_per_thread(void* a){
    // printf("whole lotta nothing\n");
    
    turing_threading_self_init();
    const int selfid = turing_threading_self_index();
    
    enumerate_job_args_t* args = (enumerate_job_args_t*)a;
    
    int states = args->opt.states;
    mpz_t max_steps; mpz_init_set(max_steps, args->opt.max_steps); // max_steps = args->opt.max_steps;
    mpz_t startIndex; mpz_init_set(startIndex,args->opt.start);// startIndex = args->opt.start;
    mpz_t indexesConsidered; mpz_init_set(indexesConsidered,args->opt.length);// indexesConsidered = args->opt.length;
    mpz_t randomIterations; mpz_init_set(randomIterations,args->opt.randomIterations);// randomIterations = args->opt.randomIterations;

    fillMaxSteps[selfid] = mpz_get_ui(max_steps);
    
    // printf("thread job %d:\n", selfid);
    // gmp_printf("    states %d\n", args->opt.states);
    // gmp_printf("    doZerosTape %d\n", args->opt.doZerosTape);
    // gmp_printf("    doOnesTape %d\n", args->opt.doOnesTape);
    // gmp_printf("    max_steps %Zd\n", args->opt.max_steps);
    // gmp_printf("    randomIters %Zd\n", args->opt.randomIterations);
    // gmp_printf("    randomStartSeed %Zd\n", args->opt.randomStartSeed);
    // gmp_printf("    startIndex %Zd\n", args->opt.start);
    // gmp_printf("    indexesConsidered %Zd\n", args->opt.length);
    // printf("\n");
    
    // 1 enumeration with 0s on the tape
    if(args->opt.doZerosTape){
        fillSymbol[selfid] = 0;
        tm_enumerate_index_length_with_hashmap(states, startIndex, indexesConsidered, max_steps,
            args->slice_count_map,
            fill_tm_with_symbol
        );
    }

    // 1 enumeration with 1s on the tape
    if(args->opt.doOnesTape){
        fillSymbol[selfid] = 1;
        tm_enumerate_index_length_with_hashmap(states, startIndex, indexesConsidered, max_steps,
            args->slice_count_map,
            fill_tm_with_symbol
        );
    }

    // randomIterations enumerations with random 1s and 0s on tape 
    fillSeed[selfid] = mpz_get_ui(args->opt.randomStartSeed);
    fillRandom[selfid] = true;
    mpz_t i; mpz_init_set_ui(i, 0);
    for( ; mpz_cmp(i,randomIterations)<0; mpz_add_ui(i,i,1)){
        // gmp_printf("on i %Zd. selfid %d\n", i, selfid);
        tm_enumerate_index_length_with_hashmap(states, startIndex, indexesConsidered, max_steps,
            args->slice_count_map,
            fill_tm_with_symbol
        );
        fillSeed[selfid]++;
        // printf("completed i %d, selfid %d\n", i, selfid);
    }

    // gotta call this when the thread stops so we can relinquish its turing_thread_id.
    turing_threading_self_remove();

    mpz_clears(max_steps,startIndex,indexesConsidered,randomIterations,i,
        NULL);
    
    // printf("thread id %d, exiting thread...\n", selfid);
    // pthread_exit(NULL);

    return NULL;
}

void tm_enumerate_job_opt_init(enumerate_job_opt_t* opt)
{
    // opt->doOnesTape = true;
    // opt->doZerosTape = true;
    mpz_init(opt->start);
    mpz_init(opt->length);
    mpz_init(opt->max_steps);
    mpz_init(opt->randomIterations);
    mpz_init(opt->randomStartSeed);
}

void tm_enumerate_job_opt_destroy(enumerate_job_opt_t* opt)
{
    mpz_clears(opt->start, opt->length, opt->max_steps, opt->randomIterations, opt->randomStartSeed, NULL);
}

struct hashmap* do_tm_enumerate_job(enumerate_job_opt_t *opt)
{
    const int selfid = turing_threading_self_index();

    struct hashmap* slice_count_map[TM_MAX_THREADS] = {0};

    const int workthreads = opt->workthreads;
    for(int i=0;i<workthreads;i++){
        slice_count_map[i] = hashmap_new(
            sizeof(tm_slice_counter_t), 0, tm_rand(selfid), tm_rand(selfid), 
            tm_slicecounter_hashmap_hash,
            tm_slicecounter_hashmap_compare,
            tm_slicecounter_hashmap_free,
            NULL
        );
    }

    int states = opt->states;

    mpz_t max_steps,startIndex,indexesConsidered,randomIterations,randomStartSeed;

    mpz_init_set(max_steps,opt->max_steps);
    mpz_init_set(startIndex,opt->start);
    mpz_init_set(indexesConsidered,opt->length);
    mpz_init_set(randomIterations,opt->randomIterations);
    mpz_init_set(randomStartSeed, opt->randomStartSeed);

    // clip indexesConsidered to make sure we save resources if it goes over.
    mpz_t max_num_machines, end_index;
    mpz_inits(max_num_machines, end_index, NULL);

    tm_max_num_of_machines(states, max_num_machines);
 
    mpz_add(end_index, startIndex, indexesConsidered);
    // if end index is greater than the last valid index, clip it
    if(mpz_cmp(end_index, max_num_machines) > 0){
        mpz_sub(indexesConsidered, max_num_machines, startIndex);
        mpz_add_ui(indexesConsidered, indexesConsidered, 1); //add 1 for good measure though
    }
    // mpz_clears(max_num_machines, end_index, NULL);

    //start running enumeration
    printf("\nrunning enumeration\n");
    gmp_printf("    states %d\n", states);
    gmp_printf("    max_steps %Zd\n", max_steps);
    gmp_printf("    randomIters %Zd\n", randomIterations);
    gmp_printf("    randomStartSeed %Zd\n", randomStartSeed);
    gmp_printf("    startIndex %Zd\n", startIndex);
    gmp_printf("    indexesConsidered %Zd\n", indexesConsidered);
    gmp_printf("    workthreads %d\n", opt->workthreads);
    // printf("\n");
    
    pthread_t workthread_handles[TM_MAX_THREADS];
    enumerate_job_args_t* thread_args[TM_MAX_THREADS];

    mpz_t startIndexCounter, indexesPerThread, indexRemainder;
    mpz_inits(indexesPerThread, indexRemainder, NULL);
    mpz_init_set(startIndexCounter,startIndex); // startIndexCounter = startIndex;

    mpz_fdiv_q_ui(indexesPerThread, indexesConsidered, workthreads); // indexesPerThread = indexesConsidered/workthreads;
    mpz_fdiv_r_ui(indexRemainder, indexesConsidered, workthreads);

    //spawn threads
    for(int i=0;i<workthreads;i++){
        thread_args[i] = malloc(sizeof(enumerate_job_args_t));
        enumerate_job_args_t* args = thread_args[i];
        
        tm_enumerate_job_opt_init(&args->opt);
        // args->opt = *opt;
        args->slice_count_map = slice_count_map[i];

        args->opt.doOnesTape = opt->doOnesTape;
        args->opt.doZerosTape = opt->doZerosTape;
        args->opt.states = opt->states;
        mpz_set(args->opt.max_steps,opt->max_steps);
        mpz_set(args->opt.randomIterations,opt->randomIterations);
        mpz_set(args->opt.randomStartSeed, randomStartSeed);
        mpz_set(args->opt.start,startIndexCounter); //args->opt.start = startIndexCounter;
        mpz_set(args->opt.length,indexesPerThread); //args->opt.length = indexesPerThread;

        // if we are on the last one check if we need the last thread to pick up the slack
        if(i==workthreads-1 && mpz_cmp_ui(indexRemainder,0) != 0){
            // gmp_printf("added slack %Zd\n", indexRemainder);
            mpz_add(args->opt.length,args->opt.length,indexRemainder);
        }

        mpz_add(startIndexCounter,startIndexCounter,indexesPerThread);//startIndexCounter += indexesPerThread;

        pthread_create(&workthread_handles[i], NULL, enumerate_job_per_thread, (void*)args);
    }

    //wait for all the threads to finish
    for(int i=0;i<workthreads;i++){
        // sleep(10);
        pthread_join(workthread_handles[i], NULL);
    }

    // gotta do this :skull: i think.
    resetFillTapeAlgorithm();

    // printf("merge and destroy\n");

    //merge and destroy all the maps into the 0th one
    for(int i=1;i<workthreads;i++){
        tm_slicecounter_hashmap_merge(slice_count_map[0], slice_count_map[i]);
        hashmap_free(slice_count_map[i]);
    }

    //free stuff for each thread
    for(int i=0;i<workthreads;i++){
        enumerate_job_args_t* args = thread_args[i];
        tm_enumerate_job_opt_destroy(&args->opt);
        
        free(thread_args[i]);
    }

    // free all the mpz_t's we made here
    mpz_clears(max_steps,startIndex,indexesConsidered,randomIterations,randomStartSeed,
        startIndexCounter, indexesPerThread,
        max_num_machines, end_index,
        indexRemainder, 
        NULL);

    return slice_count_map[0];
}

//TODO, fix this up
void tm_print_enumerate_performance_stats(int states, mpz_t max_steps)
{
    // uint64_t machines = tm_max_num_of_machines(states);
    mpz_t num_machines;
    mpz_init(num_machines);
    tm_max_num_of_machines(states, num_machines);

    mpz_t start;
    mpz_init_set_ui(start, 0);

    uint64_t milliStart = current_timestamp();
    tm_enumerate_index_length_generic(states, start, num_machines, max_steps, NULL, NULL);
    uint64_t milliEnd = current_timestamp();

    mpz_clears(start, NULL);


    uint64_t duration = milliEnd-milliStart;

    gmp_printf("Performance: took %llu milliseconds to simulate %Zd %d state machines\n",
        duration, num_machines, states);

    mpz_cdiv_q_ui(num_machines,num_machines,duration);// num_machines = num_machines/duration
    gmp_printf("Performance: thats like %Zd machines per millisecond\n", num_machines);

    mpz_mul_ui(num_machines,num_machines,1000); // num_machines = num_machines*1000/duration
    gmp_printf("Performance: thats like %Zd machines per second\n", num_machines);
    // printf("Performance: thats around %llu machine steps per second. maybe...\n", machines*1000*max_steps/duration);

    mpz_clear(num_machines);
}

void tm_enumerate_index_length_generic(
    int states,
    mpz_t start,
    mpz_t length,
    mpz_t max_steps,
    void(*halt_receiver)(tm_t* tm),
    void(*before_stepping)(tm_t* tm)
)
{
    // gmp_printf("called enumerate with args: (states, %d), (start, %Zd), (length, %Zd), (max_steps, %Zd)\n", states, start, length, max_steps);

    mpz_t halters; mpz_init_set_ui(halters,0);

    tm_t tm;
    tm_init(&tm);
    tm.states = states;
    tm_load_table_by_index(&tm, start);
    // tm_print_table_short(&tm);
    

    tm_run_opt_t runopt = (tm_run_opt_t){
        .trivialNonhaltingCheck=true, //we might not need this but eh
        // .max_steps=max_steps
    };
    mpz_init_set(runopt.max_steps, max_steps);

    mpz_t i; mpz_init_set_ui(i, 0);
    for(;mpz_cmp(i,length)<0;mpz_add_ui(i,i,1)){
        tm_reset_keep_table_and_states(&tm);

        if(before_stepping)before_stepping(&tm);

        tm_step_until_halt_or_max(&tm, runopt, NULL);

        // printf("stepped until halt");

        if(tm.halted == true && tm.haltReason == HALT_NATURAL){
            if(halt_receiver)halt_receiver(&tm);
            mpz_add_ui(halters,halters,1); // halters++;
        }
        // printf("i %d\n", i);
        // go to next machine. if there is no next machine it breaks.
        if(tm_next_table_lexico(&tm)){
            // printf("table overflowed at %d\n", i);
            break;
            // tm_load_table_by_index(&tm, start);
        }
    }
    tm_destroy(&tm);
    // gmp_printf("states %d, halters %Zd\n", states, halters);
    mpz_clears(halters, i, runopt.max_steps, NULL);
}

void tm_slicecounter_hashmap_free(void *item)
{
    const tm_slice_counter_t* slicecounter = item;
    tm_slice_free(&slicecounter->slice);
    mpz_clear(slicecounter->count);
}

uint64_t tm_slicecounter_hashmap_hash(const void *item, uint64_t seed0, uint64_t seed1)
{
    const tm_slice_counter_t* slicecounter = item;
    const tape_slice_t* slice = &slicecounter->slice;
    return hashmap_sip(slice->tapeslice, slice->length, seed0, seed1);
}

int tm_slicecounter_hashmap_compare(const void *a, const void *b, void *udata)
{
    const tm_slice_counter_t* slicecounterA = a;
    const tm_slice_counter_t* slicecounterB = b;
    
    const tape_slice_t* sa = &slicecounterA->slice;
    const tape_slice_t* sb = &slicecounterB->slice;
    
    return tm_slice_compare(sa, sb);
}

struct hashmap* counter_map[TM_MAX_THREADS];
int conuter = 0;
void halt_receiver_hashmap(tm_t* tm)
{
    const int selfid = turing_threading_self_index();

    tm_slice_counter_t sliceCounter;
    mpz_init_set_ui(sliceCounter.count, 1); // sliceCounter.count = 1;
    tm_slice_init_from_written_tape(tm, &sliceCounter.slice);

    tm_slice_counter_t* result = hashmap_get(counter_map[selfid], &sliceCounter);
    if(result != NULL){
        if(result->slice.length == 0){
            printf("%d result has length zero?\n", conuter++);
            exit(1);
        }
        
        mpz_add_ui(result->count, result->count, 1); // result->count++;
        mpz_set(sliceCounter.count, result->count); // sliceCounter.count = result->count;
        mpz_clear(result->count);

        tm_slice_free(&sliceCounter.slice);
        sliceCounter.slice = result->slice;
        
        if(hashmap_oom(counter_map[selfid])){
            printf("hashmap_oom out of memory :skull:\n");
            exit(1);
        }
    }

    hashmap_set(counter_map[selfid], &sliceCounter);
    // printf("hashmap set\n");
}

void tm_enumerate_index_length_with_hashmap(
    int states,
    mpz_t start,
    mpz_t length,
    mpz_t max_steps,
    struct hashmap* map,
    void(*before_stepping)(tm_t* tm)
)
{
    const int selfid = turing_threading_self_index();
    counter_map[selfid] = map;
    tm_enumerate_index_length_generic(states, start, length, max_steps,
        halt_receiver_hashmap,
        before_stepping
    );
}

struct hashmap* do_tm_enumerate_hashmap_job_wrapped(
    int states, 
    char *max_steps,
    char *randomIterations,
    char *randomStartSeed, 
    bool doOnesTape,
    bool doZerosTape,
    char *startIndex, 
    char *indexesConsidered, 
    int workers)
{
    // int states = 2;
    // char *max_steps = "300";
    // char *randomIterations = "10";
    // char *startIndex = "0";
    // char *indexesConsidered = NULL;
    // int workers = 4;

    struct hashmap* slice_count_map;

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
        .states=states,
        .workthreads=workers,
        .doOnesTape=doOnesTape,
        .doZerosTape=doZerosTape
    };
    tm_enumerate_job_opt_init(&enumerateOpt);

    mpz_set_str(enumerateOpt.length, indexesConsidered, 10);
    mpz_set_str(enumerateOpt.max_steps, max_steps, 10);
    mpz_set_str(enumerateOpt.randomIterations, randomIterations, 10);
    mpz_set_str(enumerateOpt.randomStartSeed, randomStartSeed, 10);
    mpz_set_str(enumerateOpt.start, startIndex, 10);

    slice_count_map = do_tm_enumerate_job(&enumerateOpt);

    tm_enumerate_job_opt_destroy(&enumerateOpt);
    if(mustFree_indexesConsidered)free(indexesConsidered);

    return slice_count_map;
}

bool do_tm_enumerate_sql_merge_job_wrapped(
    int states, 
    char *max_steps,
    char *randomIterations,
    char *randomStartSeed, 
    bool doOnesTape,
    bool doZerosTape,
    char *startIndex, 
    char *indexesConsidered, 
    int workers,
    
    sqlenv_t *sqlenv,
    char statementBuffer[],
    char *tablename
)
{
    struct hashmap *slicecount_map = do_tm_enumerate_hashmap_job_wrapped(states, max_steps, randomIterations, randomStartSeed, doOnesTape, doZerosTape, startIndex, indexesConsidered, workers);

    sql_merge_slicecountmap_into_str_count_table(sqlenv, statementBuffer, tablename, slicecount_map);

    hashmap_free(slicecount_map);
}