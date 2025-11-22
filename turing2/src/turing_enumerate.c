#include "turing_enumerate.h"
#include "hashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#define MIN(a,b) ((a)<(b) ? (a):(b))
#define MAX(a,b) ((a)>(b) ? (a):(b))

uint64_t current_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return (uint64_t)milliseconds;
}

void tm_print_enumerate_performance_stats(int states, int max_steps)
{
    uint64_t machines = tm_max_num_of_machines(states);

    uint64_t milliStart = current_timestamp();
    tm_enumerate_index_length_generic(states, 0, machines, max_steps, NULL, NULL);
    uint64_t milliEnd = current_timestamp();

    uint64_t duration = milliEnd-milliStart;

    printf("took %llu milliseconds to simulate %llu %d state machines\n",
        duration, machines, states);
    printf("thats like %llu machines per millisecond\n", machines/duration);
    printf("thats like %llu machines per second\n", machines*1000/duration);
    printf("thats around %llu machine steps per second. maybe...\n", machines*1000*max_steps/duration);
}

bool atErrorNum = false;
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

        tm_step_until_halt_or_max(&tm, max_steps);

        if(tm.halted == true && tm.haltReason == HALT_NATURAL){
            // tm_print_table_short(&tm);
            if(i == 20039){ //error happens here on the goddaaamnnn yea...
                atErrorNum = true;
            }else{
                atErrorNum = false;
            }
            if(halt_receiver)halt_receiver(&tm);
            halters++;
        }
        // printf("i %d\n", i);
        //if we hit the last machine then break.
        //but still go to the next one though.
        if(tm_next_table_lexico(&tm))break;
    }
    printf("states %d halters %d\n", states, halters);
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

    int length = MIN(sa->length, sb->length);
    for(int i=0;i<length;i++){
        tm_symbol_t symbolA = sa->tapeslice[i];
        tm_symbol_t symbolB = sb->tapeslice[i];
        if(symbolA > symbolB)return 1;
        else if(symbolA < symbolB) return -1;
    }

    const int length1 = sa->length;
    const int length2 = sb->length;

    if(length1 < length2){
        return 1;
    }else if(length1 > length2){
        return -1;
    }

    return 0;
    // return memcmp(sa->tapeslice, sb->tapeslice,);
}

struct{
    char lottaPoo[100];
    struct hashmap* counter_map;
    char lottaPoopoo[100];
} bigBadStruct;
int conuter = 0;
void halt_receiver_hashmap(tm_t* tm)
{
    // printf("hash received start\n");
    tm_slice_counter_t sliceCounter;
    sliceCounter.count = 1;
    // if(atErrorNum)printf("before slice init, sliceCounter.count %llu\n", sliceCounter.count);
    
    tm_slice_init_from_written_tape(tm, &sliceCounter.slice);
    // if(atErrorNum)printf("after slice init, sliceCounter.count %llu\n", sliceCounter.count);

    // printf("    hash get\n");
    tm_slice_counter_t* result = hashmap_get(bigBadStruct.counter_map, &sliceCounter);
    if(result != NULL){
        if(result->slice.length == 0){
            printf("%d result has length zero?\n", conuter++);
            exit(1);
        }
        result->count++;
        sliceCounter.count = result->count;
        tm_slice_free(&sliceCounter.slice);
        sliceCounter.slice = result->slice;
        // if(atErrorNum){
        //     printf("sliceCounter.count %llu, sliceCounter.slice.length %d, slice @ 20039: \n",
        //          sliceCounter.count, sliceCounter.slice.length);
        //     printf("result->count %llu, result->slice.length %d, slice @ 20039: \n",
        //         result->count, result->slice.length);
        //     tm_slice_print(&sliceCounter.slice);
        //     // printf(" ");
        //     tm_slice_print(&result->slice);
        // }
        
        if(hashmap_oom(bigBadStruct.counter_map)){
            printf("hashmap_oom out of memory :skull:\n");
            exit(1);
        }
    }else{
        // result = &sliceCounter;
        // sliceCounter.count = 1;
        // if(sliceCounter.slice.length == 0)printf("slicecounter has length zero?\n");
        
        
        // tm_slice_counter_t* littletest = hashmap_get(halt_receiver_hashmap_map, &sliceCounter);
        // if(littletest->slice.length == 0)printf("littletest has length zero?\n");
    }
    // printf("    hash set\n");


    hashmap_set(bigBadStruct.counter_map, &sliceCounter);
    
    // printf("finish hash receive\n");
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
    bigBadStruct.counter_map = map;
    tm_enumerate_index_length_generic(states, start, length, max_steps,
        halt_receiver_hashmap,
        before_stepping
    );
}