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

void mtm_trans_table_init(mtm_transition_table_t* table, int states, int workTapes)
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
mtm_transition_entry_t* mtm_trans_table_get_entry(mtm_transition_table_t* table, mtm_entry_index_t* entryIndex)
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
void mtm_trans_table_free(mtm_transition_table_t* table)
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

                mtm_print_entry_index(&entryIndex, workTapes);
                // mtm_print_entry(mtm_trans_table_get_entry(table, &entryIndex), workTapes);
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