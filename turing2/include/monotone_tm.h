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

/*
Uptm(mpz_t machine_index, char* input, unsigned long max_steps);

ptm tables:
    state 0 = halt.
    state entry:
        ( input tape read (2) ,
            work tape read (2) )
        ->
        ( output tape write (2),
        work tape write (2),
        work tape move (2),
        next state (states) )
    ...

table[]

mtm_load_table_from_machine_index(mtm_t* ptm, mpz_t machine_index);

https://people.idsia.ch/~juergen/toesv2/node6.html
 Monotone TMs (MTMs).
 Most current theory of description size and inductive inference is based on MTMs (compare [#!LiVitanyi:97!#, p. 276 ff]) with:
    several tapes
each tape being a finite chain of adjacent squares with a scanning head initially pointing to the leftmost square.
There is:
    one output tape
and:
    at least two work tapes (sufficient to compute everything traditionally regarded as computable).

The MTM has a finite number of internal states, one of them being the initial state. MTM behavior is specified by a lookup table mapping current state and contents of the squares above work tape scanning heads to a new state and an instruction to be executed next.

There are instructions for shifting work tape scanning heads one square left or right (appending new squares when necessary), and for writing 0 or 1 on squares above work tape scanning heads.

The only input-related instruction requests an input bit determined by an external process and copies it onto the square above the first work tape scanning head.

There may or may not be a halt instruction to terminate a computation.

Sequences of requested input bits are called self-delimiting programs because they convey all information about their own length, possibly causing the MTM to halt [#!Levin:74!#,#!Gacs:74!#,#!Chaitin:75!#], or at least to cease requesting new input bits (the typical case in this paper).

MTMs are called monotone because they have a one-way write-only output tape -- they cannot edit their previous output, because the only ouput instructions are: append a new square at the right end of the output tape and fill it with 0/1.

*/

#include <pthread.h>

// mtm tapes are binary and go left and right.
typedef struct{
    char* tapeMemory;
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

typedef struct{
    mtm_tape_t* firstTapePointer;
    unsigned int numberOfTapes;
} mtm_tape_array_t;

void mtm_tape_array_init(mtm_tape_array_t* tapeArray, unsigned int numberOfTapes);
void mtm_tape_array_destroy(mtm_tape_array_t* tapeArray);

mtm_tape_t* mtm_tape_array_get_tape(mtm_tape_array_t* tapeArray, unsigned int tapeNum);
unsigned int mtm_tape_array_get_number_of_tapes(mtm_tape_array_t* tapeArray);

//TODO encode the transition table somehow...
// there can be a variable amount of work tapes...

typedef struct {
    // unsigned char 
} mtm_table_entry_t;

typedef struct{

} mtm_transition_table_t;


typedef struct{
    pthread_mutex_t mutex;

    // supposed to be unidirectional, read-only
    mtm_tape_t input_tape;
    // supposed to be unidirectional, read-only
    mtm_tape_t output_tape;
    // supposed to be bidirectional, read-write
    mtm_tape_array_t work_tapes_array;

} mtm_t;

// lock whenever reading/writing a mtm_t's state.
// unlock when your done.
void mtm_lock();
void mtm_unlock();



#endif