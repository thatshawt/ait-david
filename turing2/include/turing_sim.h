#ifndef TURING_SIM_H
#define TURING_SIM_H

#include "turing_threading.h"
#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>

#include <gmp.h>

#define TM_MAX_STATES 10
#define TM_TAPE_SIZE ((500*2)+10)
#define TM_SYMBOLS 2

typedef uint8_t tm_symbol_t;
typedef uint8_t tm_state_t;

enum TM_Move{
    TM_MOVE_L,TM_MOVE_R
};

enum HaltReason{
    HALT_NATURAL,               // halted from transition table
    HALT_TAPE_OUT_OF_BOUNDS,    // halted because went outside tape bounds
    HALT_MAX_STEPS,             // hit max steps from tm_step_until_halt_or_max
    HALT_TRIVIAL_NONHALTING,           // found to never halt i guess...
};

typedef struct {
    tm_symbol_t write;
    enum TM_Move move;
    tm_state_t next_state;
} tm_transition_table_entry_t;

typedef struct {
    pthread_mutex_t mutex;

    //current position on tape.
    uint32_t tape_index;
    //maximum prescribed tape index despite the TAPE_SIZE.
    uint32_t max_tape_index;

    bool halted;
    enum HaltReason haltReason;

    //inclusive
    uint32_t low_visited_tape_index;
    //inclusive
    uint32_t high_visited_tape_index;

    //current state number.
    tm_state_t state;
    //how many states this tm is supposed to have.
    //referenced by many other functions.
    uint32_t states;
    
    tm_transition_table_entry_t transition_table[TM_SYMBOLS][TM_MAX_STATES];
    tm_symbol_t tape[TM_TAPE_SIZE];
} tm_t;

void tm_init(tm_t* tm);
void tm_destroy(tm_t* tm);
void tm_reset(tm_t* tm);
void tm_mutex_lock(tm_t* tm);
void tm_mutex_unlock(tm_t* tm);
void tm_reset_keep_table_and_states(tm_t* tm);
void tm_load_table(tm_t* tm, char* table_string);

tm_transition_table_entry_t tm_get_entry(tm_t* tm, int symbol, int state);
void tm_set_entry(tm_t* tm, int symbol, int state, tm_transition_table_entry_t* entry);

void tm_step(tm_t* tm);

// just replace it all bruh idk
// typedef mpz_t tm_index_t;

typedef struct{
    bool trivialNonhaltingCheck;
    mpz_t max_steps;
} tm_run_opt_t;
void tm_step_until_halt_or_max(tm_t* tm, tm_run_opt_t opt, mpz_t* result);

void tm_fill_tape(tm_t* tm, tm_symbol_t symbol);
void tm_fill_tape_range(tm_t* tm, tm_symbol_t symbol, int tapeStartI, int tapeEndI);

void tm_fill_tape_with_random(tm_t* tm, int seed);
void tm_fill_tape_with_random_range(tm_t* tm, int seed, int tapeStartI, int tapeEndI);

int tm_get_written_tape_size(tm_t* tm);
int tm_count_written_symbol(tm_t* tm, tm_symbol_t symbol);

int tm_count_symbol_entire_tape(tm_t* tm, int symbol);

typedef struct{
    tm_symbol_t* tapeslice;
    int length;
} tape_slice_t;

int tm_slice_compare(tape_slice_t *slice1, tape_slice_t *slice2);
tape_slice_t tm_slice_clone(tape_slice_t *slice1);

void tm_slice_init_from_written_tape(tm_t* tm, tape_slice_t* slice);
void tm_slice_free(tape_slice_t* slice);
void tm_slice_print(tape_slice_t* slice);

void tm_print_entire_tape_symbol_frequencies(tm_t* tm);
void tm_print_written_tape(tm_t* tm);
void tm_print_state(tm_t* tm);
void tm_print_entry_short(tm_transition_table_entry_t* entry);
void tm_debug_print_table_entry(tm_transition_table_entry_t entry);
void tm_fancy_print_transitions(tm_t* tm);
void tm_print_table_short(tm_t* tm);

//got this from musl. thank you musl.
void tm_srand(int threadid, unsigned s);
int tm_rand(int threadid);

#endif