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
void fill_tm_with_symbol(tm_t* tm)
{
    const int selfid = turing_threading_self_index();
    if(fillRandom[selfid]){
        tm_fill_tape_with_random(tm, fillSeed[selfid]);
    }else{
        tm_fill_tape(tm, fillSymbol[selfid]);
    }
}

void tm_slicecounter_hashmap_merge(struct hashmap* mapA, struct hashmap* mapB)
{
    size_t iter = 0;
    void *item;
    while (hashmap_iter(mapB, &iter, &item)) {
        const tm_slice_counter_t *sliceFromB = item;
        uint64_t count = sliceFromB->count;

        tm_slice_counter_t *sliceFromA = hashmap_get(mapA, sliceFromB);

        if(sliceFromA != NULL){
            sliceFromA->count += sliceFromB->count;
        }else{
            tm_slice_counter_t newSliceForA;
            newSliceForA.count = sliceFromB->count;
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

    printf("whole lotta nothing\n");

    
    turing_threading_self_init();
    const int selfid = turing_threading_self_index();
    
    enumerate_job_args_t* args = (enumerate_job_args_t*)a;
    
    
    int max_steps = args->opt.max_steps;
    int states = args->opt.states;
    int startIndex = args->opt.start;
    int indexesConsidered = args->opt.length;
    int randomIterations = args->opt.randomIterations;
    
    printf("thread job %d:\n", selfid);
    printf("    states %d\n", args->opt.states);
    printf("    max_steps %d\n", args->opt.max_steps);
    printf("    randomIters %d\n", args->opt.randomIterations);
    printf("    startIndex %d\n", args->opt.start);
    printf("    indexesConsidered %d\n", args->opt.length);
    printf("\n");
    
    fillSymbol[selfid] = 0;
    tm_enumerate_index_length_with_hashmap(states, startIndex, indexesConsidered, max_steps,
        args->slice_count_map,
        fill_tm_with_symbol
    );
    
    // turing_threading_self_remove();return NULL;

    fillSymbol[selfid] = 1;
    tm_enumerate_index_length_with_hashmap(states, startIndex, indexesConsidered, max_steps,
        args->slice_count_map,
        fill_tm_with_symbol
    );

    fillSeed[selfid] = 1;
    fillRandom[selfid] = true;
    for(int i=0;i<randomIterations;i++){
        printf("on i %d. selfid %d\n", i, selfid);
        tm_enumerate_index_length_with_hashmap(states, startIndex, indexesConsidered, max_steps,
            args->slice_count_map,
            fill_tm_with_symbol
        );
        fillSeed[selfid]++;
        // printf("completed i %d, selfid %d\n", i, selfid);
    }

    turing_threading_self_remove();
    
    printf("thread id %d, exiting thread...\n", selfid);
    // pthread_exit(NULL);

    return NULL;
}

struct hashmap* do_tm_enumerate_job(enumerate_job_opt_t *opt)
{
    const int selfid = turing_threading_self_index();

    struct hashmap* slice_count_map[TM_MAX_THREADS];

    // const int workthreads = turing_threading_get_workthreads_count();
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

    int max_steps = opt->max_steps;
    int states = opt->states;
    int startIndex = opt->start;
    int indexesConsidered = opt->length;
    int randomIterations = opt->randomIterations;

    // printf("Running enumeration states %d, max_steps %d, randomIters %d, startIndex %d, indexesConsidered %d,   ",
        // states, max_steps, randomIterations, startIndex, indexesConsidered);

    printf("running enumeration\n");
    printf("    states %d\n", opt->states);
    printf("    max_steps %d\n", opt->max_steps);
    printf("    randomIters %d\n", opt->randomIterations);
    printf("    startIndex %d\n", opt->start);
    printf("    indexesConsidered %d\n", opt->length);
    printf("    workthreads %d\n", opt->workthreads);
    printf("\n");
    
    pthread_t workthread_handles[TM_MAX_THREADS];
    enumerate_job_args_t* thread_args[TM_MAX_THREADS];

    int startIndexCounter = startIndex;
    const int indexesPerThread = indexesConsidered/workthreads;

    //spawn threads
    for(int i=0;i<workthreads;i++){
        thread_args[i] = malloc(sizeof(enumerate_job_args_t));
        enumerate_job_args_t* args = thread_args[i];

        args->opt = *opt;
        args->slice_count_map = slice_count_map[i];

        args->opt.start = startIndexCounter;
        // args->opt.start = 999;
        args->opt.length = indexesPerThread;

        startIndexCounter += indexesPerThread;

        pthread_create(&workthread_handles[i], NULL, enumerate_job_per_thread, (void*)args);
    }

    //wait for all the threads to finish
    for(int i=0;i<workthreads;i++){
        // sleep(10);
        pthread_join(workthread_handles[i], NULL);
    }

    printf("merge and destroy\n");

    //merge and destroy all the maps into the 0th one
    for(int i=1;i<workthreads;i++){
        tm_slicecounter_hashmap_merge(slice_count_map[0], slice_count_map[i]);
        hashmap_free(slice_count_map[i]);
    }

    //free thread args that we malloc'ed
    for(int i=0;i<workthreads;i++){
        free(thread_args[i]);
    }

    return slice_count_map[0];
}

void tm_print_enumerate_performance_stats(int states, int max_steps)
{
    uint64_t machines = tm_max_num_of_machines(states);

    uint64_t milliStart = current_timestamp();
    tm_enumerate_index_length_generic(states, 0, machines, max_steps, NULL, NULL);
    uint64_t milliEnd = current_timestamp();

    uint64_t duration = milliEnd-milliStart;

    printf("Performance: took %llu milliseconds to simulate %llu %d state machines\n",
        duration, machines, states);
    printf("Performance: thats like %llu machines per millisecond\n", machines/duration);
    printf("Performance: thats like %llu machines per second\n", machines*1000/duration);
    // printf("Performance: thats around %llu machine steps per second. maybe...\n", machines*1000*max_steps/duration);
}

void tm_enumerate_index_length_generic(
    int states,
    tm_index_t start,
    int length,
    uint64_t max_steps,
    void(*halt_receiver)(tm_t* tm),
    void(*before_stepping)(tm_t* tm)
)
{
    int halters = 0;

    tm_t tm;
    tm_init(&tm);
    tm.states = states;
    tm_load_table_by_index(&tm, start);
    // tm_print_table_short(&tm);
    for(int i=0;i<length;i++){
        tm_reset_keep_table_and_states(&tm);

        if(before_stepping)before_stepping(&tm);

        tm_run_opt_t runopt = (tm_run_opt_t){
            .trivialNonhaltingCheck=true, //we might not need this but eh
            .max_steps=max_steps
        };

        tm_step_until_halt_or_max(&tm, runopt);

        // printf("stepped until halt");

        if(tm.halted == true && tm.haltReason == HALT_NATURAL){
            if(halt_receiver)halt_receiver(&tm);
            halters++;
        }
        // printf("i %d\n", i);
        // go to next machine. if there is no next machine it breaks.
        if(tm_next_table_lexico(&tm)){
            // printf("table overflowed at %d\n", i);
            break;
        }
    }
    tm_destroy(&tm);
    printf("states %d, halters %d\n", states, halters);
}

void tm_slicecounter_hashmap_free(void *item)
{
    const tm_slice_counter_t* slicecounter = item;
    tm_slice_free(&slicecounter->slice);
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
    sliceCounter.count = 1;
    
    tm_slice_init_from_written_tape(tm, &sliceCounter.slice);

    tm_slice_counter_t* result = hashmap_get(counter_map[selfid], &sliceCounter);
    if(result != NULL){
        if(result->slice.length == 0){
            printf("%d result has length zero?\n", conuter++);
            exit(1);
        }
        result->count++;
        sliceCounter.count = result->count;
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
    tm_index_t start,
    int length,
    uint64_t max_steps,
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