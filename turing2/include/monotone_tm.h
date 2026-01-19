#ifndef MONOTONE_TM_H
#define MONOTONE_TM_H

#include <pthread.h>

/*
https://www.hutter1.net/ai/sintro2kc.pdf, slide 11.
Monotone Turing Machine
For technical reasons we need the following variants of a Turing machine
Definition 7 (Monotone Turing machine T (mTM))
• one unidirectional read-only input tape,
• one unidirectional write-only output tape,
• some bidirectional work tapes, initially filled with zeros.
• all tapes are binary (no blank symbol!),
• T outputs/computes a string starting with x (or a sequence ω)
on input p
    :⇐⇒ T (p) = x∗ (or T (p) = ω)
    :⇐⇒ p is to the left of the input head when the last bit of x is output.
• T may continue operation and need not to halt.
• For given x, {p : T (p) = x∗} forms a prefix code.
• We call such codes p minimal programs.
*/

/*
Uptm(mpz_t machine_index, char* input, unsigned long max_steps);

mtm table:
    state 0 = halt.
        state (states),
        input tape read (2),
        work tape1 read (2)
    ->
        input tape move (2), // if true, input moves right
        output tape write (2),
        output tape move (2), // if true, output moves right
        work tape write (2),
        work tape move (2),
        next state (states)
    ...

mtm_load_table_from_machine_index(mtm_t* mtm, mpz_t machine_index);
*/


#define MTM_MAX_WORK_TAPES 5
#define MTM_MAX_STATES 10
#define MTM_MAX_TAPE_SIZE 1000

typedef struct{
    char inputTapeMove;
    char outputTapeWrite;
    char outputTapeMove;
    char workTapeWrites[MTM_MAX_WORK_TAPES];
    char workTapeMoves[MTM_MAX_WORK_TAPES];
    unsigned int nextState;
} mtm_transition_entry_t;

typedef struct{
    char state;
    char inputRead;
    char workTapeReads[MTM_MAX_WORK_TAPES];
} mtm_entry_index_t;

typedef struct{
    int states;
    int workTapes;
    mtm_transition_entry_t* entryMap;

    size_t _D[MTM_MAX_WORK_TAPES+2];
} mtm_transition_table_t;

void mtm_trans_table_init(mtm_transition_table_t* table, int states, int workTapes);
mtm_transition_entry_t* mtm_trans_table_get_entry(mtm_transition_table_t* table, mtm_entry_index_t* entryIndex);
void mtm_trans_table_free(mtm_transition_table_t* table);

void mtm_print_entry(mtm_transition_entry_t* entry, int workTapes);
void mtm_print_entry_index(mtm_entry_index_t* entryIndex, int workTapes);
void mtm_print_table(mtm_transition_table_t* table);

// mtm tapes are binary and go left and right.
typedef struct{
    char tapeMemory[MTM_MAX_TAPE_SIZE];
    unsigned int tapeMemoryMinIndex;
    unsigned int tapeMemoryMaxIndex;

    unsigned int head_index;
} mtm_tape_t;

void mtm_tape_init(mtm_tape_t* tape);
void mtm_tape_destroy(mtm_tape_t* tape);

// 0 writes 0, anything else writes 1.
void mtm_tape_write(mtm_tape_t* tape, unsigned char symbol);
unsigned int mtm_tape_read(mtm_tape_t* tape);

void mtm_tape_move_right(mtm_tape_t* tape);
void mtm_tape_move_left(mtm_tape_t* tape);

void mtm_tape_ranged_fill_with_symbol(mtm_tape_t* tape, unsigned char fillSymbol, unsigned int startIndex, unsigned int endIndex);
void mtm_tape_fill_with_symbol(mtm_tape_t* tape, unsigned char fillSymbol);
void mtm_tape_fill_with_zeros(mtm_tape_t* tape);
void mtm_tape_fill_with_ones(mtm_tape_t* tape);
void mtm_tape_fill_with_callback(mtm_tape_t* tape, void* data, unsigned char(*fillCallback)(void* data, unsigned int index));

typedef struct{
    pthread_mutex_t mutex;

    int state;
    // supposed to be unidirectional, read-only
    mtm_tape_t input_tape;
    // supposed to be unidirectional, read-only
    mtm_tape_t output_tape;
    // supposed to be bidirectional, read-write
    mtm_tape_t work_tapes_array[MTM_MAX_WORK_TAPES];

    mtm_transition_table_t transition_table;

    mtm_entry_index_t _tableEntry;

} mtm_t;

// lock whenever reading/writing a mtm_t's state.
// unlock when your done.
void mtm_lock();
void mtm_unlock();



#endif