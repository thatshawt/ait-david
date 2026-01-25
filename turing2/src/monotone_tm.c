#include <stdlib.h>
#include <stdio.h>

#include <mpfr.h>

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
// mtm_transition_entry_t* mtm_entrymap_get_entry(mtm_transition_entry_t* entryMap, mtm_entry_index_t* entryIndex, int states, int workTapes)
// {

// }

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
    table->entryMap = NULL;
}

void mtm_table_zero(mtm_transition_table_t* table)
{
    mtm_transition_entry_t* entry;
    mtm_entry_index_t index;
    mtm_entry_index_zero(&index);
    int states = table->states;
    int worktapes = table->workTapes;
    do{
        entry = mtm_table_get_entry(table, &index);

        mtm_entry_zero(entry);

    }while(!mtm_entry_index_increment(&index, states, worktapes));
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
                mtm_print_entry(mtm_table_get_entry(table, &entryIndex), workTapes);
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
bool inputTapeMove; 1*2^0 +
bool outputTapeWrite; 0*2^1 + 
bool outputTapeMove;
bool workTapeWrites[MTM_MAX_WORK_TAPES];
bool workTapeMoves[MTM_MAX_WORK_TAPES];
int nextState; states
*/

inline void mpz_push_nbits_right(mpz_t rop, mpz_t temp, mpz_t bits, mp_bitcnt_t biti)
{
    // rop |= bits<<biti;
    mpz_lshift(temp, bits, biti);
    mpz_ior(rop, rop, temp);
}

int mtm_get_entry_bits(int states, int worktapes)
{
    mpz_t temp;
    mpz_init_set_ui(temp, states);
    mpz_sub_ui(temp,temp,1);
    int bits = 3+(worktapes*2) + mpz_sizeinbase(temp,2);
    // printf("%d sizeinbase(%d,2)\n", mpz_sizeinbase(temp,2),states);
    mpz_clear(temp);
    return bits;
}

void mtm_entry_get_digit(mpz_t digit, mtm_transition_entry_t* entry, int states, int worktapes)
{
    mpz_set_ui(digit, 0);

    int n = mtm_get_entry_bits(states, worktapes);
    mpz_t temp; mpz_init(temp);
    mpz_t bits; mpz_init(bits);
    mp_bitcnt_t bitsi = 0;

    mpz_set_ui(bits, entry->inputTapeMove);
    mpz_push_nbits_right(digit, temp, bits, bitsi);
    bitsi++;

    mpz_set_ui(bits, entry->outputTapeWrite);
    mpz_push_nbits_right(digit, temp, bits, bitsi);
    bitsi++;

    mpz_set_ui(bits, entry->outputTapeMove);
    mpz_push_nbits_right(digit, temp, bits, bitsi);
    bitsi++;
    
    for(int i=0; i<worktapes; i++){
        mpz_set_ui(bits, entry->workTapeWrites[i]);
        mpz_push_nbits_right(digit, temp, bits, bitsi);
        bitsi++;
    }

    for(int i=0; i<worktapes; i++){
        mpz_set_ui(bits, entry->workTapeMoves[i]);
        mpz_push_nbits_right(digit, temp, bits, bitsi);
        bitsi++;
    }

    mpz_set_ui(bits, entry->nextState);
    mpz_push_nbits_right(digit, temp, bits, bitsi);
    mpz_set_ui(temp, states-1);
    bitsi += mpz_sizeinbase(temp, 2);

    mpz_clears(bits, temp, NULL);

    // return digit;
}

inline void mpz_load_number_of_n_ones(mpz_t rop, int n)
{
    if(n == 0){
        mpz_set_ui(rop, 0);
    }else{
        mpz_set_ui(rop, 1);
        mpz_mul_2exp(rop, rop, n);
        mpz_sub_ui(rop, rop, 1);
    }
}

inline void mpz_lshift(mpz_t rop, mpz_t number, int n)
{
    mpz_mul_2exp(rop, number, n);
}

inline void mpz_rshift(mpz_t rop, mpz_t number, int n)
{
    mpz_fdiv_q_2exp(rop, number, n);
}

inline void mpz_pop_nbits_right(mpz_t bits, mpz_t number, mp_bitcnt_t bitsN)
{
    // bits = number & (2^bitsN - 1);
    mpz_load_number_of_n_ones(bits, bitsN);
    mpz_and(bits, bits, number);

    // number >> bitsN;
    mpz_rshift(number, number, bitsN);
}

void mtm_entry_from_digit(mtm_transition_entry_t* entry, mpz_t digit, int states, int worktapes)
{
    mpz_t bits; mpz_init(bits);
    mpz_t number; mpz_init_set(number, digit);
    mpz_t temp; mpz_init(temp);

    mpz_pop_nbits_right(bits, number, 1);
    entry->inputTapeMove = mpz_get_ui(bits);

    mpz_pop_nbits_right(bits, number, 1);
    entry->outputTapeWrite = mpz_get_ui(bits);

    mpz_pop_nbits_right(bits, number, 1);
    entry->outputTapeMove = mpz_get_ui(bits);
    
    for(int i=0; i<worktapes; i++){
        mpz_pop_nbits_right(bits, number, 1);
        entry->workTapeWrites[i] = mpz_get_ui(bits);
    }

    for(int i=0; i<worktapes; i++){
        mpz_pop_nbits_right(bits, number, 1);
        entry->workTapeMoves[i] = mpz_get_ui(bits);
    }

    mpz_set_ui(temp, states-1);
    mpz_set_ui(bits, mpz_sizeinbase(temp, 2));
    mpz_pop_nbits_right(bits, number, mpz_get_ui(bits));
    entry->nextState = mpz_get_ui(bits);

    mpz_clears(bits, number, temp, NULL);
}

// x is the prefix index
// bitlength = floor(log(x+1)/log(2))
void mpz_prefix_index_get_bit_length(mpz_t x, mpz_t bitlength)
{
    mpz_t tempMpz; mpz_init(tempMpz);
    mpfr_t tempMpfr; mpfr_init(tempMpfr);

    mpz_set(tempMpz, x);
    mpz_add_ui(tempMpz, tempMpz, 1);
    mpfr_set_z(tempMpfr, tempMpz, MPFR_RNDZ);
    mpfr_log2(tempMpfr, tempMpfr, MPFR_RNDZ);

    mpfr_get_z(bitlength, tempMpfr, MPFR_RNDZ);

    mpz_clear(tempMpz);
    mpfr_clear(tempMpfr);
}

// bitinteger = x - 2^bitlength - 1
// bitinteger = x - 2^floor(log(x+1)/log(2)) - 1
// x is the prefix index
void mpz_prefix_index_get_bit_integer(mpz_t x, mpz_t bitinteger)
{
    mpz_t tempMpz; mpz_init(tempMpz);

    mpz_prefix_index_get_bit_length(x, tempMpz);
    mpz_set_ui(bitinteger, 1);
    mpz_mul_2exp(bitinteger, bitinteger, mpz_get_ui(tempMpz));
    mpz_sub_ui(bitinteger, bitinteger, 1);

    mpz_sub(bitinteger, x, bitinteger);

    mpz_clear(tempMpz);
}

// x = 2^bitlength + bitinteger - 1
inline void mpz_get_prefix_index_from_int_and_length(mpz_t x, mpz_t bitinteger, mpz_t bitlength)
{
    mpz_t temp; mpz_init(temp);

    mpz_set_ui(temp, 1);
    mpz_mul_2exp(temp, temp, mpz_get_ui(bitlength));
    mpz_add(temp, temp, bitinteger);
    mpz_sub_ui(x, temp, 1);

    mpz_clear(temp);
}

// bar_encode(x) = 1^ℓ(x) 0 x
// x is treated as a binary string of length lengthX
void mpz_bar_encode(mpz_t bar_encoded, mpz_t x, int lengthX)
{
    mpz_t temp; mpz_init(temp);
    mpz_t bits; mpz_init(bits);
    mp_bitcnt_t bitsi = 0;
    
    // zero bar_encoded to start off.
    mpz_set_ui(bar_encoded, 0);

    // encode x
    mpz_set(bits, x);
    mpz_push_nbits_right(bar_encoded, temp, bits, bitsi);
    // bitsi += mpz_sizeinbase(bits, 2);
    bitsi += lengthX;
    
    // add the 0 before the x
    mpz_set_ui(bits, 0);
    mpz_push_nbits_right(bar_encoded, temp, bits, bitsi);
    bitsi += mpz_sizeinbase(bits, 2);
    
    // encode the 1^ℓ(x)
    // int lengthX = mpz_sizeinbase(x, 2); // save for later
    mpz_set_ui(temp, lengthX);
    mpz_set_ui(bits, 1);
    mpz_mul_2exp(bits, bits, lengthX);
    mpz_sub_ui(bits, bits, 1); // bits = 2^ℓ(x) - 1 -> bit string 1^ℓ(x)
    mpz_push_nbits_right(bar_encoded, temp, bits, bitsi);
    bitsi += mpz_sizeinbase(bits, 2);
    
    // finished.
    mpz_clears(temp, bits, NULL);
}

int mpz_count_leading_ones(mpz_t numberWithLeadingOnes)
{
    if(mpz_cmp_ui(numberWithLeadingOnes, 0) == 0)
        return 0;

    int numberLength = mpz_sizeinbase(numberWithLeadingOnes,2);
    int count = 0;
    mpz_t temp; mpz_init(temp);
    mpz_t temp2; mpz_init(temp2);
    CHECK:
        count++;
        mpz_load_number_of_n_ones(temp, count);
        mpz_rshift(temp2, numberWithLeadingOnes, numberLength-count);
        // gmp_printf("count %d, temp %Zd, temp2: %Zd\n", count, temp, temp2);
        if(count == numberLength){
            goto FINISH;
        }else if(mpz_cmp(temp, temp2) == 0){
            goto CHECK;
        }else{
            count--;
        }


    FINISH:
        mpz_clears(temp, temp2, NULL);
        return count;
}

void mpz_bar_decode_left(mpz_t x, int* lengthx, mpz_t number_with_bar_encoded_left)
{
    if(mpz_cmp_ui(number_with_bar_encoded_left,0) == 0){
        *lengthx = 0;
        mpz_set_ui(x, 0);
        return;
    }
    // gmp_printf("got %Zd at start\n", number_with_bar_encoded_left);
    int lengthOfBarEncodedFull = mpz_sizeinbase(number_with_bar_encoded_left, 2);

    *lengthx = mpz_count_leading_ones(number_with_bar_encoded_left);

    mpz_set_ui(x, 0);

    mpz_t temp; mpz_init(temp);

    const int shifted = lengthOfBarEncodedFull - *lengthx - *lengthx - 1;
    mpz_load_number_of_n_ones(temp, *lengthx);
    mpz_lshift(temp, temp, shifted);
    // gmp_printf("shifted %d left is %Zd\n", shifted, temp);
    mpz_and(temp, temp, number_with_bar_encoded_left);
    // gmp_printf("after and temp is %Zd\n", temp);
    mpz_rshift(x, temp, shifted);
    // gmp_printf("x at end %Zd\n", x);

    mpz_clear(temp);
}

void mpz_bar_decode_left_pop(mpz_t x, int* lengthx, mpz_t number_with_bar_encoded_left)
{
    int lengthOfBarEncodedFull = mpz_sizeinbase(number_with_bar_encoded_left, 2);
    mpz_bar_decode_left(x, lengthx, number_with_bar_encoded_left);

    if(mpz_cmp_ui(number_with_bar_encoded_left,0) == 0){
        *lengthx = 0;
        mpz_set_ui(x, 0);
        return;
    }

    mpz_t temp; mpz_init(temp);

    mpz_load_number_of_n_ones(temp, lengthOfBarEncodedFull - *lengthx - *lengthx - 1);

    mpz_and(number_with_bar_encoded_left, temp, number_with_bar_encoded_left);

    mpz_clear(temp);
}

void mpz_apos_encode(mpz_t apos_encoded, mpz_t x, int lengthX)
{
    mpz_t temp; mpz_init(temp);
    mpz_t temp2; mpz_init(temp2);
    mpz_t bits; mpz_init(bits);
    mp_bitcnt_t bitsi = 0;
    
    // zero apos_encoded to start off.
    mpz_set_ui(apos_encoded, 0);

    // encode x
    mpz_set(bits, x);
    mpz_push_nbits_right(apos_encoded, temp, bits, bitsi);
    bitsi += lengthX;

    // encode the bar_encoded(lengthX)
    mpz_set_ui(temp, lengthX);
    mpz_prefix_index_get_bit_length(temp, temp);

    mpz_set_ui(temp2, lengthX);
    mpz_prefix_index_get_bit_integer(temp2, bits);
    
    mpz_bar_encode(temp2, bits, mpz_get_ui(temp));
    mpz_set(bits, temp2);
    mpz_push_nbits_right(apos_encoded, temp, bits, bitsi);
    bitsi += mpz_sizeinbase(bits, 2);
    
    // finished.
    mpz_clears(temp, temp2, bits, NULL);
}

void mpz_apos_encode_prefix_index(mpz_t apos_encoded, mpz_t prefixIndex)
{
    mpz_t temp; mpz_init(temp);
    
    mpz_prefix_index_get_bit_length(prefixIndex, temp);

    int lengthX = mpz_get_ui(temp);

    mpz_prefix_index_get_bit_integer(prefixIndex, temp);

    mpz_apos_encode(apos_encoded, temp, lengthX);

    mpz_clear(temp);
}

int mpz_apos_decode_left(mpz_t x, int* lengthx, mpz_t number_with_apos_encoded_left)
{
    if(mpz_cmp_ui(number_with_apos_encoded_left,0) == 0){
        *lengthx = 0;
        mpz_set_ui(x, 0);
        return 0;
    }

    const int lengthOfFullNumber = mpz_sizeinbase(number_with_apos_encoded_left,2);

    int lengthOfLength;
    mpz_t temp; mpz_init(temp);
    mpz_t temp2; mpz_init(temp2);
    mpz_bar_decode_left(temp, &lengthOfLength, number_with_apos_encoded_left);

    mpz_set_ui(temp2, lengthOfLength);
    mpz_get_prefix_index_from_int_and_length(temp, temp, temp2);
    *lengthx = mpz_get_ui(temp);

    int shifted = lengthOfFullNumber - lengthOfLength - lengthOfLength - 1 - *lengthx;
    // gmp_printf("shifted %d\n", shifted);
    mpz_load_number_of_n_ones(temp, *lengthx);
    mpz_lshift(temp, temp, shifted);
    mpz_and(temp, temp, number_with_apos_encoded_left);
    mpz_rshift(x, temp, shifted);

    mpz_clears(temp, temp2, NULL);

    return 2*lengthOfLength + 1 + *lengthx;
}

int mpz_apos_decode_prefix_index_left(mpz_t prefixIndex, mpz_t number_with_apos_encoded_left)
{
    mpz_t xint; mpz_init(xint);
    mpz_t temp; mpz_init(temp);
    int lengthx;

    int aposLength = mpz_apos_decode_left(xint, &lengthx, number_with_apos_encoded_left);

    mpz_set_ui(temp, lengthx);

    mpz_get_prefix_index_from_int_and_length(prefixIndex, xint, temp);

    mpz_clears(xint,temp,
        NULL);

    return aposLength;
}


int mpz_push_table_entry_map(mpz_t number, mtm_transition_table_t* table)
{
    mpz_t temp; mpz_init(temp);
    mpz_t temp2; mpz_init(temp2);

    mtm_entry_index_t index;
    mtm_entry_index_zero(&index);

    const int states = table->states;
    const int worktapes = table->workTapes;

    const int bitsi = mtm_get_entry_bits(states, worktapes);

    mtm_transition_entry_t* entry;

    int totalBitsAdded = 0;

    // push the entryMap digits
    do{
        entry = mtm_table_get_entry(table, &index);

        mtm_entry_get_digit(temp2, entry, states, worktapes);

        mpz_push_nbits_right(number, temp, temp2, bitsi);

        totalBitsAdded += bitsi;

    }while(!mtm_entry_index_increment(&index, states, worktapes));

    mpz_clears(temp, temp2,
        NULL);

    return totalBitsAdded;
}

int mpz_pop_table_entry_map_left(mpz_t number, mtm_transition_table_t* table)
{
    const int numberFullLength = mpz_sizeinbase(number,2);

    mpz_t temp; mpz_init(temp);
    mpz_t temp2; mpz_init(temp2);

    mtm_entry_index_t index;
    mtm_entry_index_zero(&index);

    const int states = table->states;
    const int worktapes = table->workTapes;

    const int bitsi = mtm_get_entry_bits(states, worktapes);

    mtm_transition_entry_t* entry;

    int totalBitsRemoved = 0;

    // pop the entryMap digits
    do{
        entry = mtm_table_get_entry(table, &index);

        // get the entry digit
        mpz_load_number_of_n_ones(temp, bitsi);
        mpz_lshift(temp, temp, numberFullLength-totalBitsRemoved-bitsi);
        mpz_and(temp, temp, number);
        
        // load the entry into the table
        mtm_entry_from_digit(entry, temp2, states, worktapes);
        
        totalBitsRemoved += bitsi;
    }while(!mtm_entry_index_increment(&index, states, worktapes));

    mpz_clears(temp, temp2,
        NULL);

    // remove the entry digits
    mpz_load_number_of_n_ones(temp, totalBitsRemoved);
    mpz_lshift(temp, temp, numberFullLength-totalBitsRemoved);
    mpz_and(number, number, temp);

    return totalBitsRemoved;
}

/*
    typedef struct{
        int states;
        int workTapes;
        mtm_transition_entry_t* entryMap;

        size_t _D[MTM_MAX_WORK_TAPES+2];
    } mtm_transition_table_t;
*/
int mtm_get_table_index(mpz_t tableIndex, mtm_transition_table_t* table)
{
    mpz_set_ui(tableIndex, 0);

    mpz_t temp; mpz_init(temp);
    mpz_t temp2; mpz_init(temp2);

    // push states integer
    mpz_set_ui(temp2, table->states);
    mpz_apos_encode_prefix_index(temp, temp2);
    int pushed1 = mpz_sizeinbase(temp,2);
    mpz_push_nbits_right(tableIndex, temp2, temp, pushed1);

    // push workTapes integer
    mpz_set_ui(temp2, table->workTapes);
    mpz_apos_encode_prefix_index(temp, temp2);
    int pushed2 = mpz_sizeinbase(temp,2);
    mpz_push_nbits_right(tableIndex, temp2, temp, pushed2);

    // push entry map
    int pushed3 = mpz_push_table_entry_map(tableIndex, table);

    mpz_clears(temp,temp2,
        NULL);

    return pushed1 + pushed2 + pushed3;
}

int mtm_load_table_from_index(mtm_transition_table_t* table, mpz_t tableIndex)
{
    // zero the table if index is zero
    if(mpz_cmp_ui(tableIndex, 0) == 0){
        mtm_table_zero(table);
        return 0;
    }

    mpz_t temp; mpz_init(temp);
    mpz_t temp2; mpz_init(temp2);

    mpz_set(temp2, tableIndex);

    // pop states integer
    int aposLength1 = mpz_apos_decode_prefix_index_left(temp, temp2);
    table->states = mpz_get_ui(temp);
    mpz_load_number_of_n_ones(temp, mpz_sizeinbase(temp2,2)-aposLength1);
    mpz_and(temp2, temp2, temp);

    // pop workTapes integer
    int aposLength2 = mpz_apos_decode_prefix_index_left(temp, temp2);
    table->workTapes = mpz_get_ui(temp);
    mpz_load_number_of_n_ones(temp, mpz_sizeinbase(temp2,2)-aposLength2);
    mpz_and(temp2, temp2, temp);

    // pop entry map
    int mapLength = mpz_pop_table_entry_map_left(temp2, table);

    mpz_clears(temp,temp2,
        NULL);

    return aposLength1 + aposLength2 + mapLength;
}

