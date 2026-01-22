#include <stdlib.h>
#include <stdio.h>

#include "monotone_tm.h"

// https://stackoverflow.com/questions/19883518/how-can-i-create-an-n-dimensional-array-in-c
//  Create an array with N dimensions with sizes specified in D.
mtm_transition_entry_t* CreateArray(size_t N, size_t D[])
{
    //  Calculate size needed.
    size_t s = sizeof(mtm_transition_entry_t);
    for (size_t n = 0; n < N; ++n)
        s *= D[n];

    //  Allocate space.
    printf("created mtm_transition_entry_t with size %d\n", s);
    return malloc(s);
}

/*
typedef struct{
    char state; // max states
    char inputRead; // max 2
    char workTapeReads[MTM_MAX_WORK_TAPES]; // each has max 2
} mtm_entry_index_t;
*/

void mtm_table_init(mtm_transition_table_t* table, int states, int workTapes)
{
    table->states = states;
    table->workTapes = workTapes;

    for(int i=1;i<MTM_MAX_WORK_TAPES+2;i++){
        table->_D[i] = 2;
    }
    table->_D[0] = MTM_MAX_STATES;

    table->entryMap = CreateArray(MTM_MAX_WORK_TAPES+2, table->_D);
}

/*  Return a pointer to an element in an N-dimensional A array with sizes
    specified in D and indices to the particular element specified in I.
*/
mtm_transition_entry_t* Element(mtm_transition_entry_t* A, size_t N, size_t D[], size_t I[])
{
    //  Handle degenerate case.
    if (N == 0)
        return A;

    //  Map N-dimensional indices to one dimension.
    int index = I[0];
    for (size_t n = 1; n < N; ++n)
        index = index * D[n] + I[n];

    //  Return address of element.
    return &A[index];
}


// typedef struct{
//     char state; // max states
//     char inputRead; // max 2
//     char workTapeReads[MTM_MAX_WORK_TAPES]; // each has max 2
// } mtm_entry_index_t;
mtm_transition_entry_t* mtm_table_get_entry(mtm_transition_table_t* table, mtm_entry_index_t* entryIndex)
{
    size_t I[MTM_MAX_WORK_TAPES+2] = {0};
    // get the indices for reading the work tape
    for(int i=2;i<MTM_MAX_WORK_TAPES+2;i++){
        I[i] = entryIndex->workTapeReads[i-2];
    }
    I[0] = entryIndex->state; // indice for current state
    I[1] = entryIndex->inputRead; // indice for input tape read
    return Element(table->entryMap, MTM_MAX_WORK_TAPES+2, table->_D, I);
}

void mtm_table_free(mtm_transition_table_t* table)
{
    free(table->entryMap);
}

// typedef struct{
//     char inputTapeMove;
//     char outputTapeWrite;
//     char outputTapeMove;
//     char workTapeWrites[MTM_MAX_WORK_TAPES];
//     char workTapeMoves[MTM_MAX_WORK_TAPES];
//     unsigned int nextState;
// } mtm_transition_entry_t;
void mtm_print_entry(mtm_transition_entry_t* entry, int workTapes)
{
    printf("mtm_transition_entry_t:\n");
    printf("    inputTapeMove: %d\n", entry->inputTapeMove);
    printf("    outputTapeWrite: %d\n", entry->outputTapeWrite);
    printf("    outputTapeMove: %d\n", entry->outputTapeMove);
    for(int i=0;i<workTapes;i++){
        printf("    workTape%dWrite: %d\n", i, entry->workTapeWrites[i]);
        printf("    workTape%dMove: %d\n", i, entry->workTapeMoves[i]);
    }
    printf("    nextState: %d\n", entry->nextState);
}

// typedef struct{
//     char state;
//     char inputRead;
//     char workTapeReads[MTM_MAX_WORK_TAPES];
// } mtm_entry_index_t;
void mtm_print_entry_index(mtm_entry_index_t* entryIndex, int workTapes)
{
    printf("mtm_entry_index_t:\n");
    printf("    state: %d\n", entryIndex->state);
    printf("    inputRead: %d\n", entryIndex->inputRead);
    for(int i=0;i<workTapes;i++){
        printf("    workTape%dRead: %d\n", i, entryIndex->workTapeReads[i]);
    }
}

// typedef struct{
//     int states;
//     int workTapes;
//     mtm_transition_entry_t* entryMap;

//     size_t _D[MTM_MAX_WORK_TAPES+2];
// } mtm_transition_table_t;

// typedef struct{
//     char state; // max states
//     char inputRead; // max 2
//     char workTapeReads[MTM_MAX_WORK_TAPES]; // each has max 2
// } mtm_entry_index_t;
void mtm_print_table(mtm_transition_table_t* table)
{
    const int states = table->states;
    const int workTapes = table->workTapes;

    mtm_entry_index_t entryIndex;

    for(int state=1;state<states;state++){
        entryIndex.state = state;

        for(int inputRead=0;inputRead<2;inputRead++){
            entryIndex.inputRead = inputRead;

            int workTapeReads[MTM_MAX_WORK_TAPES+2] = {0};
            int workTapeCounter = 0;
            while(workTapeCounter < workTapes){
                for(int i=0;i<workTapes;i++)
                    entryIndex.workTapeReads[i] = workTapeReads[i];

                // mtm_print_entry_index(&entryIndex, workTapes);
                // mtm_print_entry(mtm_table_get_entry(table, &entryIndex), workTapes);
                printf("\n");

                int overflows = 0;
                INCREMENT:
                if(workTapeCounter < workTapes){
                    workTapeReads[workTapeCounter]++;
                    if(workTapeReads[workTapeCounter]>=2){
                        workTapeReads[workTapeCounter]=0;
                        workTapeCounter++;
                        overflows++;
                        goto INCREMENT;
                    }
                }
                if(overflows < workTapes)workTapeCounter = 0;
                // printf("end of workTapeCounter forloop\n");
            }
            // printf("end of inputRead forloop\n");
        }
        // printf("end of state forloop\n");
    }
    // printf("end of func\n");
}

void mtm_entry_index_zero(mtm_entry_index_t* entryIndex)
{
    entryIndex->state = 0;
    entryIndex->inputRead = 0;
    for(int i=0;i<MTM_MAX_WORK_TAPES;i++) entryIndex->workTapeReads[i] = 0;
}

// increments leftmost digit and overflows rightwards.
// digits goes to 0 when overflowed
// returns true if overflowed on every digit.
bool lincrement_int(int n, int* sizes, int* digits)
{
    // start on first digit
    int digiti = 0;
    INCREMENT:
    if(digiti < n){
        digits[digiti]++;

        //overflows, try increment next digit
        if(digits[digiti] >= sizes[digiti]){
            digits[digiti] = 0;
            digiti++;
            goto INCREMENT;
        }
    }else{//if we go too far we overflowed past last digit
        return true;
    }

    return false;
}

bool mtm_entry_index_increment(mtm_entry_index_t* entryIndex, int states, int worktapes)
{
    // entry index encoding
    // domain {0..states-1},{0,1},{0,1}^worktapes.
    // sizes    states,         2,  2^worktapes.
    // encoding <state, inputread, worktapereads>
    // each worktaperead has size 2 and there are table->worktapes number of work tapes.

    // n is this much cus yea
    int n = worktapes+2;
    
    // set the sizes
    int sizes[2+MTM_MAX_WORK_TAPES] = {0};
    sizes[0] = states;
    sizes[1] = 2;
    for(int i=0;i<worktapes;i++) sizes[i+2] = 2;
    
    // set the digits
    int digits[2+MTM_MAX_WORK_TAPES] = {0};
    digits[0] = entryIndex->state;
    digits[1] = entryIndex->inputRead;
    for(int i=0;i<worktapes;i++) digits[i+2] = entryIndex->workTapeReads[i];

    // do the increment on the digits
    bool returnval = lincrement_int(n, sizes, digits);

    // load the digits back into the entryIndex so we did something
    entryIndex->state = digits[0];
    entryIndex->inputRead = digits[1];
    for(int i=0;i<worktapes;i++)  entryIndex->workTapeReads[i] = digits[i+2];
    
    return returnval;
}

void mtm_entry_zero(mtm_transition_entry_t* entry)
{
    entry->inputTapeMove = 0;
    entry->outputTapeWrite = 0;
    entry->outputTapeMove = 0;
    entry->nextState = 0;
    for(int i=0;i<MTM_MAX_WORK_TAPES;i++){
        entry->workTapeWrites[i] = 0;
        entry->workTapeMoves[i] = 0;
    }
}

bool mtm_entry_increment(mtm_transition_entry_t* entry, int states, int worktapes)
{

    /*
    char inputTapeMove;
    char outputTapeWrite;
    char outputTapeMove;
    char workTapeWrites[MTM_MAX_WORK_TAPES];
    char workTapeMoves[MTM_MAX_WORK_TAPES];
    unsigned int nextState;
    */
    // domain        {0,1},       {0,1},      {0,1},{0,1}^worktapes,{0,1}^worktapes,{0..states-1}.
    // size              2,           2,          2,    2^worktapes,   2^worktapes,    states.
    // encoding <inputmove, outputwrite, outputmove, worktapewrites, worktapemoves, nextstate>
    // index          0          1          2           3...            4???            lastone...

    int n = 4+(worktapes*2);

    int sizes[4+MTM_MAX_WORK_TAPES*2] = {0};
    sizes[0] = 2; // inputmove
    sizes[1] = 2; // outputwrite
    sizes[2] = 2; // outputmove

    // set both tape sizes...
    for(int i=0;i<worktapes*2;i++){
        sizes[i+3] = 2;
    }

    sizes[n-1] = states; // nextstate

    // everybody now, digits! digits!
    int digits[4+MTM_MAX_WORK_TAPES*2] = {0};
    digits[0] = entry->inputTapeMove;
    digits[1] = entry->outputTapeWrite;
    digits[2] = entry->outputTapeMove;
    digits[n-1] = entry->nextState;
    for(int i=0;i<worktapes;i++){
        digits[i+3] = entry->workTapeWrites[i];
        digits[i+3+worktapes] = entry->workTapeMoves[i];
    }


    bool returnval = lincrement_int(n, sizes, digits);

    entry->inputTapeMove = digits[0];
    entry->outputTapeWrite = digits[1];
    entry->outputTapeMove = digits[2];
    entry->nextState = digits[n-1];
    for(int i=0;i<worktapes;i++){
        entry->workTapeWrites[i] = digits[i+3];
        entry->workTapeMoves[i] = digits[i+3+worktapes];
    }

    // an llm couldnt have written this no shot

    return returnval;
}

/*
char inputTapeMove; 1*2^0 +
char outputTapeWrite; 0*2^1 + 
char outputTapeMove;
char workTapeWrites[MTM_MAX_WORK_TAPES];
char workTapeMoves[MTM_MAX_WORK_TAPES];
unsigned int nextState; states
*/
int mtm_entry_get_digit(mtm_transition_entry_t* entry, int states, int worktapes)
{
    int digit = 0;

    int n = 4+(worktapes*2);

    digit |= (entry->inputTapeMove)<<0;
    digit |= (entry->outputTapeWrite)<<1;
    digit |= (entry->outputTapeMove)<<2;
    digit |= (entry->nextState)<<(n-1);
    for(int i=0; i<worktapes; i++){
        // digits[i+3] = entry->workTapeWrites[i];
        // digits[i+3+worktapes] = entry->workTapeMoves[i];
        digit |= (entry->workTapeWrites[i])<<(i+3);
        digit |= (entry->workTapeMoves[i])<<(i+3+worktapes);
    }

    return digit;
}

int mtm_entry_max_digit(int states, int worktapes);