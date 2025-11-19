#ifndef TURING_SIM_H
#define TURING_SIM_H

//47176870
//47176869

#include <stdint.h>

#define MAX_TRANSITIONS 256
#define TAPE_SIZE 40000
#define SYMBOLS 2

typedef uint8_t tm_symbol_t;
typedef uint8_t tm_state_t;

enum TM_Move{
    TM_MOVE_L,TM_MOVE_R
};

typedef struct {
    tm_symbol_t write;
    enum TM_Move move;
    tm_state_t next_state;
} tm_transition_table_entry_t;

typedef struct {
    uint32_t tape_index;
    uint32_t max_tape_index;
    uint32_t low_visited_tape_index;
    uint32_t high_visited_tape_index;

    tm_state_t state;
    
    tm_transition_table_entry_t transition_table[SYMBOLS][MAX_TRANSITIONS];
    tm_symbol_t tape[TAPE_SIZE];
} tm_t;

void tm_init(tm_t* tm);
void tm_load_table(tm_t* tm, int states, char* table_str);
void tm_step(tm_t* tm);
uint64_t tm_step_until_halt_or_max(tm_t* tm, uint64_t max_steps);
void tm_fill_tape(tm_t* tm, tm_symbol_t symbol);
int tm_get_written_tape_size(tm_t* tm);
int tm_count_written_symbol(tm_t* tm, tm_symbol_t symbol);

void tm_print_written_tape(tm_t* tm);
void tm_print_state(tm_t* tm);
void tm_debug_print_table_entry(tm_transition_table_entry_t entry);
void tm_fancy_print_transitions(tm_t* tm, int states);

#endif