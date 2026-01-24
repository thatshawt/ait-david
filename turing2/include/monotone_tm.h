#ifndef MONOTONE_TM_H
#define MONOTONE_TM_H

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

#include <pthread.h>
#include <gmp.h>
#include <stdbool.h>

#ifndef MTM_MAX_WORK_TAPES
#define MTM_MAX_WORK_TAPES 5
#endif

#ifndef MTM_MAX_STATES
#define MTM_MAX_STATES 10
#endif

#ifndef MTM_MAX_TAPE_SIZE
#define MTM_MAX_TAPE_SIZE 1000
#endif

typedef struct{
    char inputTapeMove;
    char outputTapeWrite;
    char outputTapeMove;
    char workTapeWrites[MTM_MAX_WORK_TAPES];
    char workTapeMoves[MTM_MAX_WORK_TAPES];
    unsigned int nextState;
} mtm_transition_entry_t;

void mtm_entry_zero(mtm_transition_entry_t* entry);
bool mtm_entry_increment(mtm_transition_entry_t* entry, int states, int worktapes);
void mtm_print_entry(mtm_transition_entry_t* entry, int workTapes);

void mtm_entry_get_digit(mpz_t digit, mtm_transition_entry_t* entry, int states, int worktapes);
void mtm_entry_from_digit(mtm_transition_entry_t* entry, mpz_t digit, int states, int worktapes);
int mtm_get_entry_bits(int states, int worktapes);

typedef struct{
    char state;
    char inputRead;
    char workTapeReads[MTM_MAX_WORK_TAPES];
} mtm_entry_index_t;

void mtm_entry_index_zero(mtm_entry_index_t* entryIndex);
bool mtm_entry_index_increment(mtm_entry_index_t* entryIndex, int states, int worktapes);
void mtm_print_entry_index(mtm_entry_index_t* entryIndex, int workTapes);

typedef struct{
    int states;
    int workTapes;
    mtm_transition_entry_t* entryMap;

    size_t _D[MTM_MAX_WORK_TAPES+2];
} mtm_transition_table_t;

void mtm_table_init(mtm_transition_table_t* table, int states, int workTapes);
mtm_transition_entry_t* mtm_table_get_entry(mtm_transition_table_t* table, mtm_entry_index_t* entryIndex);
void mtm_table_free(mtm_transition_table_t* table);
bool mtm_table_increment(mtm_transition_table_t* table);

void mtm_print_table(mtm_transition_table_t* table);


void mpz_prefix_index_get_bit_length(mpz_t x, mpz_t bitlength);
void mpz_prefix_index_get_bit_integer(mpz_t x, mpz_t bitinteger);
void mpz_get_prefix_index_from_int_and_length(mpz_t x, mpz_t bitinteger, mpz_t bitlength);

// various mpz bit manipulating funcs
void mpz_lshift(mpz_t rop, mpz_t number, int n);
void mpz_rshift(mpz_t rop, mpz_t number, int n);
void mpz_load_number_of_n_ones(mpz_t rop, int n);
void mpz_set_or_lshifted_bits(mpz_t rop, mpz_t temp, mpz_t bits, mp_bitcnt_t biti);
void mpz_pop_nbits_right(mpz_t bits, mpz_t number, mp_bitcnt_t bitsN);
int mpz_count_leading_ones(mpz_t numberWithLeadingOnes);

// bar encoding
void mpz_bar_decode_left(mpz_t x, int* lengthx, mpz_t number_with_bar_encoded_left);
void mpz_bar_decode_left_pop(mpz_t x, int* lengthx, mpz_t number_with_bar_encoded_left);
void mpz_bar_encode_from_x_bits(mpz_t bar_encoded, mpz_t x, int lengthX);

// apos encoding
void mpz_apos_decode_left(mpz_t x, int* lengthx, mpz_t number_with_apos_encoded_left);
void mpz_apos_encode_from_x_bits(mpz_t apos_encoded, mpz_t x, int lengthX);

void mtm_get_table_code(mpz_t tableCode, mtm_transition_table_t* table);
void mtm_load_table_from_code(mtm_transition_table_t* table, mpz_t tableCode);


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
    // unidirectional, read-only
    mtm_tape_t input_tape;
    // unidirectional, write-only
    mtm_tape_t output_tape;
    // bidirectional, read-write
    mtm_tape_t work_tapes_array[MTM_MAX_WORK_TAPES];

    mtm_transition_table_t transition_table;

    mtm_entry_index_t _entryIndex;

} mtm_t;

// lock whenever reading/writing a mtm_t's state.
// unlock when your done.
void mtm_lock();
void mtm_unlock();



#endif