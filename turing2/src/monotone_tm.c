#include <stdlib.h>
#include <stdio.h>

#include <stdint.h>

#include <mpfr.h>

#include "monotone_tm.h"

#include "gmp_mpzstr.h"
#include "mpz_helpers.h"

#include <string.h>

#include "cacache.h"

#include <stdbool.h>

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
    for(int i=1;i<MTM_MAX_WORK_TAPES+2;i++){
        table->_D[i] = 2;
    }
    table->_D[0] = MTM_MAX_STATES;

    table->entryMap = CreateArray(MTM_MAX_WORK_TAPES+2, table->_D);

    table->states = states;
    table->workTapes = workTapes;

    mtm_table_zero(table);
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

// increment nth entry, if it overflows zero that entry and increment the next one, if they all overflow then do nothing and return true.
// else return false.
bool mtm_table_increment(mtm_transition_table_t* table)
{
    mtm_entry_index_t entryIndex;
    mtm_entry_index_zero(&entryIndex);

    mtm_transition_entry_t* entry = mtm_table_get_entry(table, &entryIndex);

    bool indexOverflowed = false;

    INCREMENT:
    if(!indexOverflowed){
        bool overflowed = mtm_entry_increment(entry, table->states, table->workTapes);

        //overflows, zero and increment next entry
        if(overflowed){
            mtm_entry_zero(entry);

            indexOverflowed = mtm_entry_index_increment(&entryIndex, table->states, table->workTapes);
            entry = mtm_table_get_entry(table, &entryIndex);
            goto INCREMENT;
        }
    }else{//if we go too far we overflowed past last entry
        return true;
    }

    return false;
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

void mtm_print_entry_short(mtm_transition_entry_t* entry, int workTapes)
{
    printf("(inputMove, outputWrite, outputMove, ");
    for(int i=0;i<workTapes;i++){
        printf("tape%dWrite, ", i);
        printf("tape%dMove, ", i);
    }
    printf("nextState)\n");

    printf("(%d, %d, %d, ", entry->inputTapeMove, entry->outputTapeWrite, entry->outputTapeMove);
    for(int i=0;i<workTapes;i++){
        printf("%d, ", entry->workTapeWrites[i]);
        printf("%d, ", entry->workTapeMoves[i]);
    }
    printf("%d)\n", entry->nextState);
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

void mtm_print_entry_index_short(mtm_entry_index_t* entryIndex, int workTapes)
{
    printf("(state, inputRead, ");
    for(int i=0;i<workTapes;i++){
        if(i == workTapes-1) printf("workTape%dRead)", i);
        else printf("workTape%dRead, ", i);
    }

    printf(" (%d, %d, ", entryIndex->state, entryIndex->inputRead);
    for(int i=0;i<workTapes;i++){
        if(i == workTapes-1) printf("%d):",entryIndex->workTapeReads[i]);
        else printf("%d, ",entryIndex->workTapeReads[i]);
    }
    printf("\n");
}

void mtm_print_table_header(mtm_transition_table_t* table)
{
    int workTapes = table->workTapes;
    printf("(state, inputRead, ");
    for(int i=0;i<workTapes;i++){
        if(i == workTapes-1) printf("workTape%dRead)", i);
        else printf("workTape%dRead, ", i);
    }
    printf(" -> ");

    printf("(inputMove, outputWrite, outputMove, ");
    for(int i=0;i<workTapes;i++){
        printf("tape%dWrite, ", i);
        printf("tape%dMove, ", i);
    }
    printf("nextState):\n");
}

void mtm_print_table_entry(mtm_transition_table_t* table, mtm_entry_index_t* entryIndex)
{
    const int workTapes = table->workTapes;
    printf("    (%d, %d, ", entryIndex->state, entryIndex->inputRead);
    for(int i=0;i<workTapes;i++){
        if(i == workTapes-1) printf("%d) -> ",entryIndex->workTapeReads[i]);
        else printf("%d, ",entryIndex->workTapeReads[i]);
    }

    mtm_transition_entry_t* entry = mtm_table_get_entry(table, entryIndex);

    printf("(%d, %d, %d, ", entry->inputTapeMove, entry->outputTapeWrite, entry->outputTapeMove);
    for(int i=0;i<workTapes;i++){
        printf("%d, ", entry->workTapeWrites[i]);
        printf("%d, ", entry->workTapeMoves[i]);
    }
    printf("%d)\n", entry->nextState);
}

void mtm_print_table(mtm_transition_table_t* table)
{
    const int states = table->states;
    const int workTapes = table->workTapes;

    mtm_entry_index_t entryIndex;
    mtm_entry_index_zero(&entryIndex);

    mtm_print_table_header(table);
    int conut = 0;
    do{
        mtm_print_table_entry(table, &entryIndex);
        // mtm_print_entry_index_short(&entryIndex, workTapes);
        // mtm_print_entry_short(mtm_table_get_entry(table, &entryIndex), workTapes);
        // printf("\n");
        conut++;
    }while(!mtm_entry_index_increment(&entryIndex, states, workTapes) && conut<100);

}

void mtm_print_table_summary(mtm_transition_table_t* table)
{
    printf("table: states %d, worktapes %d", table->states, table->workTapes);
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

// starts on right side and overflows leftwards.
bool rincrement_int(int n, int* sizes, int* digits)
{
    // start on last digit
    int digiti = n-1;
    INCREMENT:
    if(digiti >= 0){
        digits[digiti]++;

        //overflows, try increment next digit
        if(digits[digiti] >= sizes[digiti]){
            digits[digiti] = 0;
            digiti--;
            goto INCREMENT;
        }
    }else{//if we go too far we overflowed past last digit
        return true;
    }

    return false;
}

//TODO change this to use mpz maybe?
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
    for(int i=0; i<MTM_MAX_WORK_TAPES; i++){
        entry->workTapeWrites[i] = 0;
        entry->workTapeMoves[i] = 0;
    }
}

bool mtm_entry_equal(mtm_transition_entry_t* entry1, mtm_transition_entry_t* entry2, int states, int worktapes)
{
    bool returnVal = true;
    returnVal &= entry1->inputTapeMove == entry2->inputTapeMove;
    returnVal &= entry1->outputTapeWrite == entry2->outputTapeWrite;
    returnVal &= entry1->outputTapeMove == entry2->outputTapeMove;
    returnVal &= entry1->nextState == entry2->nextState;
    for(int i=0; i<worktapes; i++){
        returnVal &= entry1->workTapeWrites[i] == entry2->workTapeWrites[i];
        returnVal &= entry1->workTapeMoves[i] == entry2->workTapeMoves[i];
    }
    return returnVal;
}

bool mtm_entry_increment_fast(mtm_transition_entry_t* entry, int states, int worktapes)
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

    // i had a comment here but it cringed me to keep seeing it as i scrolled past each time so now
    // this is a less cringy replacement comment.
    int digits[4+MTM_MAX_WORK_TAPES*2] = {0};
    digits[0] = entry->inputTapeMove;
    digits[1] = entry->outputTapeWrite;
    digits[2] = entry->outputTapeMove;
    for(int i=0;i<worktapes;i++){
        digits[i+3] = entry->workTapeWrites[i];
        digits[i+3+worktapes] = entry->workTapeMoves[i];
    }
    digits[n-1] = entry->nextState;

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

bool mtm_entry_increment_temps(mtm_transition_entry_t* entry, int states, int worktapes, mpz_t var1, mpz_t var2, mpz_t var3, mpz_t var4)
{

    mtm_entry_get_digit_temps(var1, entry, states, worktapes, var2, var3);
    mpz_add_ui(var1, var1, 1);

    mpz_get_entry_max_digit(var2, states, worktapes);

    bool overflowed = mpz_cmp(var1, var2) >= 0;
    // bool overflowed = mpz_get_ui(var1) >= mpz_get_ui(var2);
    if(overflowed){
        // printf("overflowed ");
        mtm_entry_zero(entry);
    }else{
        // mtm_entry_from_digit(entry, var1, states, worktapes);
        mtm_entry_from_digit_temps(entry, var1, states, worktapes, var2, var3, var4);
    }

    return overflowed;
}

bool mtm_entry_increment(mtm_transition_entry_t* entry, int states, int worktapes)
{
    mpz_t* vars = mpzstr_init_malloc(4);
    mpz_t* var1 = (vars+0);
    mpz_t* var2 = (vars+1);
    mpz_t* var3 = (vars+2);
    mpz_t* var4 = (vars+3);

    int overflowed = mtm_entry_increment_temps(entry, states, worktapes, *var1, *var2, *var3, *var4);

    mpzstr_clear_free(vars);

    return overflowed;
}

/*
bool inputTapeMove; 1*2^0 +
bool outputTapeWrite; 0*2^1 + 
bool outputTapeMove;
bool workTapeWrites[MTM_MAX_WORK_TAPES];
bool workTapeMoves[MTM_MAX_WORK_TAPES];
int nextState; states
*/

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

void mpz_get_entry_max_digit(mpz_t maxEntryDigit, int states, int worktapes)
{
    const int cacheHash = states*3 + worktapes*5;
    // check cache
    if(cacache_check_item(CACACHE_ENTRY_MAX_DIGIT, cacheHash)){
        mpz_t* cachedResult = (mpz_t*)cacache_get_item(CACACHE_ENTRY_MAX_DIGIT);
        mpz_set(maxEntryDigit, *cachedResult);
    }else{
        // we got 2^3 * 2^(worktapes*2) * states
        mpz_set_ui(maxEntryDigit, 1);
        mpz_mul_2exp(maxEntryDigit, maxEntryDigit, 3); // 2^3
        mpz_mul_ui(maxEntryDigit, maxEntryDigit, states); // 2^3 * states
        mpz_mul_2exp(maxEntryDigit, maxEntryDigit, worktapes*2); // 2^3 * 2^(worktapes*2) * states

        //update cache for next time just incase
        cacache_set_item(CACACHE_ENTRY_MAX_DIGIT, cacheHash, maxEntryDigit);
        // gmp_printf("cached %Zd\n", maxEntryDigit);
    }
}

void mtm_entry_get_digit_temps(mpz_t digit, mtm_transition_entry_t* entry, int states, int worktapes, mpz_t temp, mpz_t bits)
{
    if(2*worktapes > 64){
        fprintf(stderr, __FILE__ ":%d, %d worktapes too high :skull:\n",__LINE__, worktapes);
        exit(1);
    }
    mpz_set_ui(digit, 0);

    mp_bitcnt_t bitsi = 0;

    uint64_t intbits = 0;

    intbits |= entry->inputTapeMove << 0;
    intbits |= entry->outputTapeWrite << 1;
    intbits |= entry->outputTapeMove << 2;

    mpz_set_ui(bits, intbits);
    mpz_ior_bits_lshift(digit, temp, bits, bitsi);
    bitsi += 3;

    intbits = 0;
    for(int i=0; i<worktapes; i++){
        intbits |= entry->workTapeWrites[i] << i;
        intbits |= entry->workTapeMoves[i] << i+worktapes;
    }
    mpz_set_ui(bits, intbits);
    mpz_ior_bits_lshift(digit, temp, bits, bitsi);
    bitsi+=2*worktapes;

    mpz_set_ui(bits, entry->nextState);
    mpz_ior_bits_lshift(digit, temp, bits, bitsi);
    // mpz_set_ui(temp, states-1);
    // bitsi += mpz_sizeinbase(temp, 2);
}

void mtm_entry_get_digit(mpz_t digit, mtm_transition_entry_t* entry, int states, int worktapes)
{
    mpz_t temp; mpz_init(temp);
    mpz_t bits; mpz_init(bits);

    mtm_entry_get_digit_temps(digit, entry, states, worktapes, temp, bits);

    mpz_clears(bits, temp, NULL);
}

void mtm_entry_from_digit_temps(mtm_transition_entry_t* entry, mpz_t digit, int states, int worktapes, mpz_t bits, mpz_t number, mpz_t temp)
{
    if(2*worktapes > 64){
        fprintf(stderr, __FILE__ ":%d, %d worktapes too high\n",__LINE__, worktapes);
        exit(1);
    }

    mpz_set(number, digit);

    unsigned long long intbits = 0;

    mpz_set_ui(bits, 0);
    mpz_set_ui(temp, 0);

    mpz_pop_nbits_right(bits, number, 3);
    intbits = mpz_get_ui(bits);
    entry->inputTapeMove = (intbits & 1<<0)>>0;
    entry->outputTapeWrite = (intbits & 1<<1)>>1;
    entry->outputTapeMove = (intbits & 1<<2)>>2;

    mpz_pop_nbits_right(bits, number, 2*worktapes);
    intbits = mpz_get_ui(bits);
    for(int i=0; i<worktapes; i++){
        entry->workTapeWrites[i] = (intbits & 1<<i)>>i;
        entry->workTapeMoves[i] = (intbits & 1<<(i+worktapes))>>(i+worktapes);
    }

    mpz_set_ui(temp, states-1);
    mpz_set_ui(bits, mpz_sizeinbase(temp, 2));
    mpz_pop_nbits_right(bits, number, mpz_get_ui(bits));
    entry->nextState = mpz_get_ui(bits);
}

void mtm_entry_from_digit(mtm_transition_entry_t* entry, mpz_t digit, int states, int worktapes)
{
    mpz_t bits; mpz_init(bits);
    mpz_t number; mpz_init(number);
    mpz_t temp; mpz_init(temp);

    mtm_entry_from_digit_temps(entry, digit, states, worktapes, bits, number, temp);

    mpz_clears(bits, number, temp, NULL);
}


int mpz_push_table_entry_map_temps(mpz_t number, mtm_transition_table_t* table,mpz_t temp,mpz_t temp2,mpz_t temp3)
{
    // mpz_t temp; mpz_init(temp);
    // mpz_t temp2; mpz_init(temp2);

    mtm_entry_index_t index;
    mtm_entry_index_zero(&index);

    const int states = table->states;
    const int worktapes = table->workTapes;

    const int bitsi = mtm_get_entry_bits(states, worktapes);

    mtm_transition_entry_t* entry;

    int totalBitsAdded = 0;

    // push the entryMap digits
    do{
        if(worktapes < 1)continue;
        entry = mtm_table_get_entry(table, &index);

        mtm_entry_get_digit_temps(temp2, entry, states, worktapes, temp, temp3);
        // mtm_entry_get_digit(temp2, entry, states, worktapes);

        mpz_lshift(number, number, bitsi);
        mpz_ior(number, number, temp2);

        totalBitsAdded += bitsi;

    }while(!mtm_entry_index_increment(&index, states, worktapes));

    // mpz_clears(temp, temp2, NULL);

    return totalBitsAdded;
}

int mpz_push_table_entry_map(mpz_t number, mtm_transition_table_t* table)
{
    mpz_t temp; mpz_init(temp);
    mpz_t temp2; mpz_init(temp2);
    mpz_t temp3; mpz_init(temp3);

    int result = mpz_push_table_entry_map_temps(number, table,temp,temp2,temp3);

    mpz_clears(temp, temp2,temp3, NULL);

    return result;
}

int mpz_pop_entry_left_temps(mpz_t number, mtm_transition_entry_t* entry, int states, int worktapes, mpz_t temp1, mpz_t temp2)
{
    // gmp_printf("number %Zd\n", number);

    if(mpz_cmp_ui(number, 0) == 0){
        mtm_entry_zero(entry);
        return 0;
    }

    if(mpz_cmp_ui(number, 1) == 0){
        mtm_entry_zero(entry);
        mpz_set_ui(number, 0);
        return 1;
    }

    const int numberLength = mpz_sizeinbase(number,2);
    const int bitsi = mtm_get_entry_bits(states, worktapes);

    // try to isolate bitsi bits from number leftside
    int entries = (numberLength-1) / bitsi;
    int entryRemainder = (numberLength-1) % bitsi;
    int entryBits = 0;
    if(entries > 0){
        entryBits = bitsi;
    }else if(entries == 0 && entryRemainder > 0){
        entryBits = entryRemainder;
    }

    // mpz_t temp1; mpz_init(temp1);
    // mpz_t temp2; mpz_init(temp2);

    // capture the bits and shift it right so its a proper digit
    mpz_load_number_of_n_ones(temp1, entryBits);
    mpz_rshift(temp2, number, numberLength-entryBits - 1);
    mpz_and(temp1, temp1, temp2);

    // load it into entry
    mtm_entry_from_digit(entry, temp1, states, worktapes);
    entry->nextState = entry->nextState % states; // please speed i need this, my bits are kinda overflowed

    // remove the digits we used
    mpz_load_number_of_n_ones(temp1, numberLength-entryBits - 1);
    mpz_and(number, number, temp1);

    // add the leading 1 back in because we expect it. yea
    // for the input string part to retain its length we need the leading 1
    mpz_set_ui(temp1, 1);
    mpz_mul_2exp(temp1, temp1, numberLength-entryBits - 1);
    mpz_add(number, number, temp1);

    // mpz_clears(temp1, temp2, NULL);

    return entryBits;
}

int mpz_pop_entry_left(mpz_t number, mtm_transition_entry_t* entry, int states, int worktapes)
{
    mpz_t temp1; mpz_init(temp1);
    mpz_t temp2; mpz_init(temp2);

    int result = mpz_pop_entry_left_temps(number, entry, states, worktapes, temp1, temp2);

    mpz_clears(temp1, temp2, NULL);

    return result;
}

int mpz_pop_table_entry_map_left_temps(mpz_t number, mtm_transition_table_t* table, mpz_t temp1, mpz_t temp2)
{
    // const int numberFullLength = mpz_sizeinbase(number,2);
    const int states = table->states;
    const int worktapes = table->workTapes;
    
    if(mpz_cmp_ui(number, 0) == 0){
        mtm_table_zero(table);
        return 0;
    }

    mtm_entry_index_t index;
    mtm_entry_index_zero(&index);

    mtm_transition_entry_t* entry;

    int totalBitsRemoved = 0;

    // mpz_set(temp, number);
    // pop the entryMap digits
    // mpz_t temp1,temp2;
    // mpz_inits(temp1,temp2,NULL);
    do{
        entry = mtm_table_get_entry(table, &index);

        int removed = mpz_pop_entry_left_temps(number, entry, states, worktapes, temp1, temp2);
        
        if(removed == 0)break;

        totalBitsRemoved += removed;

    }while(!mtm_entry_index_increment(&index, states, worktapes));

    // mpz_clears(temp1, temp2, NULL);

    return totalBitsRemoved;
}

int mpz_pop_table_entry_map_left(mpz_t number, mtm_transition_table_t* table)
{
    mpz_t temp1,temp2;
    mpz_inits(temp1,temp2,NULL);

    int result = mpz_pop_table_entry_map_left_temps(number, table, temp1, temp2);

    mpz_clears(temp1, temp2, NULL);

    return result;
}


/*
    typedef struct{
        int states;
        int workTapes;
        mtm_transition_entry_t* entryMap;

        size_t _D[MTM_MAX_WORK_TAPES+2];
    } mtm_transition_table_t;
*/
int mtm_get_table_index_temps(mpz_t tableIndex, mtm_transition_table_t* table,
    mpz_t temp, mpz_t temp2, mpz_t poo1, mpz_t poo2, mpz_t poo3, mpz_t poo4, mpz_t poo5, mpz_t poo6)
{
    mpz_set_ui(tableIndex, 0);

    // push cantorPair(states,workTapes)
    mpz_cantor_pair_ui_temps(temp2, table->states, table->workTapes, poo1, poo2, poo3);
    // gmp_printf("pushing cantorPair(%d:states, %d:worktapes) = %Zd \n",table->states, table->workTapes,  temp2);
    mpz_apos_encode_prefix_index_temps(temp, temp2, poo1,poo2,poo3,poo4,poo5,poo6);
    int pushed1 = mpz_sizeinbase(temp,2);
    mpz_lshift(tableIndex, tableIndex, pushed1);
    mpz_ior(tableIndex, tableIndex, temp);

    // push a 1 before the map
    mpz_lshift(tableIndex, tableIndex, 1);
    mpz_setbit(tableIndex, 0);
    // gmp_printf("size %d pushed %Zd, tableIndex %Zd\n", 1, temp, tableIndex);

    // push entry map
    int pushed3 = mpz_push_table_entry_map_temps(tableIndex, table, poo1, poo2, poo3);
    // gmp_printf("size %d pushed map, tableIndex %Zd\n", pushed3, tableIndex);

    // printf("pushed1 %d, pushed2 %d, pushed3 %d\n",pushed1, pushed2, pushed3);
    // return pushed1 + pushed2 + pushed3 + 1;
    return pushed1 + 1 + pushed3;
}

int mtm_get_table_index(mpz_t tableIndex, mtm_transition_table_t* table)
{
    mpz_t temp; mpz_init(temp);
    mpz_t temp2; mpz_init(temp2);

    mpz_t poo1,poo2,poo3,poo4,poo5,poo6;
    mpz_inits(poo1,poo2,poo3,poo4,poo5,poo6,NULL);

    int result = mtm_get_table_index_temps(tableIndex, table,
        temp, temp2, poo1, poo2, poo3, poo4, poo5, poo6);

    mpz_clears(temp,temp2, NULL);
    mpz_clears(poo1,poo2,poo3,poo4,poo5,poo6,NULL);

    return result;
}

int mpz_table_index_pop_states_worktape_temps(mpz_t tableIndex, int* states, int* worktapes,
    mpz_t temp, mpz_t temp2, mpz_t poo1, mpz_t poo2, mpz_t poo3, mpz_t poo4, mpz_t poo5)
{
    // zero the table if index is zero
    if(mpz_cmp_ui(tableIndex, 0) == 0){
        *states = 0;
        *worktapes = 0;
        return 0;
    }
    // mpz_set(temp2, tableIndex);

    // pop cantorPair(states,workTapes)
    int aposLength1 = mpz_apos_decode_prefix_index_left_temps(temp, tableIndex, poo1,poo2,poo3,poo4,poo5);
    
    mpz_cantor_unpair_temps(poo1, poo2, temp, poo3);
    // gmp_printf("popped cantor unpair %Zd -> (%Zd, %Zd)\n", temp, poo1, poo2);
    *states = mpz_get_ui(poo1);
    *worktapes = mpz_get_ui(poo2);

    mpz_load_number_of_n_ones(temp, mpz_sizeinbase(tableIndex,2)-aposLength1);
    mpz_and(tableIndex, tableIndex, temp);

    // return aposLength1 + aposLength2;
    return aposLength1;
}

int mpz_table_index_pop_states_worktape(mpz_t tableIndex, int* states, int* worktapes)
{
    mpz_t temp; mpz_init(temp);
    mpz_t temp2; mpz_init(temp2);

    mpz_t poo1,poo2,poo3,poo4,poo5;
    mpz_inits(poo1,poo2,poo3,poo4,poo5, NULL);

    int result = mpz_table_index_pop_states_worktape_temps(tableIndex, states, worktapes,
        temp, temp2, poo1, poo2, poo3, poo4, poo5);

    mpz_clears(temp,temp2, poo1,poo2,poo3,poo4,poo5, NULL);

    return result;
}

// void mpz_load_input_string_into_table(mtm_transition_table_t* table, mpz_t inputStrInt, int inputStrLength)
// {
//     gmp_printf("loaded input strInt %Zd, length: %d\n", inputStrInt, inputStrLength);

// }

int mtm_load_table_from_index(mtm_transition_table_t* table, mpz_t tableIndex)
{
    // zero the table if index is zero
    if(mpz_cmp_ui(tableIndex, 0) == 0){
        mtm_table_zero(table);
        return 0;
    }

    mpz_t temp; mpz_init(temp);
    mpz_t temp2; mpz_init(temp2);
    mpz_t poo1,poo2,poo3,poo4,poo5,poo6;
    mpz_inits(poo1,poo2,poo3,poo4,poo5,poo6,NULL);

    mpz_set(temp2, tableIndex);

    int aposLength1And2 = mpz_table_index_pop_states_worktape_temps(
        temp2, &table->states, &table->workTapes,
        temp,poo1,poo2,poo3,poo4,poo5,poo6);

    // pop entry map
    // gmp_printf("temp2 when map pop %Zd\n", temp2);
    int mapLength = mpz_pop_table_entry_map_left_temps(temp2, table, poo1, poo2);
    
    // int inputStringLength = mpz_pop_leading1_input_string_into_table(temp2, table);

    // int inputStrLength = mpz_sizeinbase(temp2,2)-1;

    // // remove leading 1 left over from the entry map part
    // gmp_printf("input with leading1: %Zd\n", temp2);
    // mpz_load_number_of_n_ones(temp, mpz_sizeinbase(temp2,2)-1);
    // mpz_and(temp2, temp2, temp);

    // mpz_load_input_string_into_table(table, temp2, inputStrLength);

    mpz_clears(temp,temp2, NULL);
    mpz_clears(poo1,poo2,poo3,poo4,poo5,poo6,NULL);

    return aposLength1And2 + mapLength;
}

void mtm_tape_print(mtm_tape_t* tape)
{
    gmp_printf("tape head: %d, tapeMemory: %Zd\n", tape->headBitIndex, tape->tapeMemory);
}

void mtm_tape_init(mtm_tape_t* tape)
{
    mpz_inits(tape->tapeMemory, tape->temp1, tape->temp2, NULL);
    mpz_set_ui(tape->tapeMemory, 2); // binary 1 0
    tape->headBitIndex = 0;
}

void mtm_tape_reset(mtm_tape_t* tape)
{
    mpz_set_ui(tape->tapeMemory, 2); // binary 1 0
    tape->headBitIndex = 0;
}

void mtm_tape_destroy(mtm_tape_t* tape)
{
    mpz_clears(tape->tapeMemory, tape->temp1, tape->temp2, NULL);
}

inline unsigned long mpz_tape_mem_size(mtm_tape_t* tape)
{
    return mpz_sizeinbase(tape->tapeMemory,2);
}

// 0 writes 0, anything else writes 1.
void mtm_tape_write(mtm_tape_t* tape, unsigned char symbol)
{
    const unsigned int headBitIndex = tape->headBitIndex;
    // write 0
    if(symbol == 0){
        mpz_clrbit(tape->tapeMemory, headBitIndex);
    }else{// write 1
        mpz_setbit(tape->tapeMemory, headBitIndex);
    }
}

// reads a 1 or 0 from under the head.
unsigned char mtm_tape_read(mtm_tape_t* tape)
{
    const unsigned int headBitIndex = tape->headBitIndex;

    return mpz_tstbit(tape->tapeMemory, headBitIndex);
}

void mtm_tape_move_right(mtm_tape_t* tape)
{
    const unsigned long tapeSize = mpz_tape_mem_size(tape);
    const unsigned int headBitIndex = tape->headBitIndex;

    // need to grow tape
    if(headBitIndex == 0){
        mpz_lshift(tape->tapeMemory, tape->tapeMemory, 1);
    }else{
        tape->headBitIndex--;
    }
}

void mtm_tape_move_left(mtm_tape_t* tape)
{
    const unsigned long tapeSize = mpz_tape_mem_size(tape);
    const unsigned int headBitIndex = tape->headBitIndex;

    // need to grow tape
    if(headBitIndex == tapeSize-2){
        // add another leading 1 and remove the previous one
        mpz_clrbit(tape->tapeMemory, tapeSize-1);
        mpz_setbit(tape->tapeMemory, tapeSize);
    }

    tape->headBitIndex++;
}

// TODO check behavior of empty tape
int mtm_tape_get_code_temps(mtm_tape_t* tape, mpz_t tapeCode,
    mpz_t poo1, mpz_t poo2, mpz_t poo3, mpz_t poo4
)
{
    mpz_set_ui(tapeCode, 0);

    int tapeLength = mpz_tape_mem_size(tape);
    mpz_set(tape->temp1, tape->tapeMemory);
    mpz_clrbit(tape->temp1, tapeLength-1); // remove the leading 1

    // mpz_t poo1,poo2,poo3,poo4; mpz_inits(poo1,poo2,poo3,poo4,NULL);

    // encode string using apos encoding
    mpz_apos_encode_temps(tapeCode, tape->temp1, tapeLength-1, poo1, poo2, poo3, poo4, tape->temp2);

    // // encode the headBitIndex with apos
    // mpz_set_ui(tape->temp1, tape->headBitIndex);
    // mpz_apos_encode_prefix_index(tape->temp2, tape->temp1);
    // mpz_lshift(tapeCode, tapeCode, mpz_sizeinbase(tape->temp2,2));
    // mpz_ior(tapeCode, tapeCode, tape->temp2);

    return mpz_sizeinbase(tapeCode,2);
}

int mtm_tape_get_code(mtm_tape_t* tape, mpz_t tapeCode)
{
    mpz_t poo1,poo2,poo3,poo4; mpz_inits(poo1,poo2,poo3,poo4,NULL);

    int result = mtm_tape_get_code_temps(tape, tapeCode,
    poo1, poo2, poo3, poo4);

    mpz_clears(poo1,poo2,poo3,poo4,NULL);
    return result;
}

// TODO check behavior of empty tape
int mtm_tape_load_from_code(mtm_tape_t* tape, mpz_t tapeCode)
{
    int tapeLength;
    // decode the apos encoded tapeMemory
    int aposLength1 = mpz_apos_decode_left(tape->tapeMemory, &tapeLength, tapeCode);

    //add the leading 1
    mpz_setbit(tape->tapeMemory, tapeLength);

    // // decode the apos prefix encoded headBitIndex
    // mpz_set(tape->temp1, tapeCode);
    // int fullTapeCodeLength = mpz_sizeinbase(tapeCode,2);
    // mpz_load_number_of_n_ones(tape->temp2, fullTapeCodeLength-aposLength1);
    // mpz_and(tape->temp1, tape->temp1, tape->temp2); // cut out the tapeMemory apos part

    // int aposLength2 = mpz_apos_decode_prefix_index_left(tape->temp2, tape->temp1);
    // tape->headBitIndex = mpz_get_ui(tape->temp2);
    mtm_tape_goto_leftmost(tape);

    return aposLength1;
}

void mtm_tape_goto_leftmost(mtm_tape_t* tape)
{
    int tapeMemSize = mpz_tape_mem_size(tape);
    tape->headBitIndex = tapeMemSize-2;
}

void mtm_tape_goto_rightmost(mtm_tape_t* tape)
{
    tape->headBitIndex = 0;
}

// str null terminated, or else.
void mtm_tape_load_str(mtm_tape_t* tape, char* str)
{
    // god, i pray for null terminated strings.
    int strLen = strlen(str);

    // load the base 2 string
    mpz_set_str(tape->tapeMemory, str, 2);

    // leading 1
    mpz_setbit(tape->tapeMemory, strLen);

    mtm_tape_goto_leftmost(tape);
}

void mtm_init(mtm_t* mtm, int states, int worktapes)
{
    pthread_mutex_init(&mtm->mutex, NULL);

    mtm_entry_index_zero(&mtm->tempIndex);

    mtm_table_init(&mtm->table, states, worktapes);

    mtm_tape_init(&mtm->inputTape);
    mtm_tape_init(&mtm->outputTape);
    for(int i=0;i<MTM_MAX_WORK_TAPES;i++)mtm_tape_init(&mtm->workTapesArray[i]);
}

void mtm_destroy(mtm_t* mtm)
{
    pthread_mutex_destroy(&mtm->mutex);

    mtm_table_free(&mtm->table);

    mtm_tape_destroy(&mtm->inputTape);
    mtm_tape_destroy(&mtm->outputTape);
    for(int i=0;i<MTM_MAX_WORK_TAPES;i++)mtm_tape_destroy(&mtm->workTapesArray[i]);
}

void mtm_lock(mtm_t* mtm)
{
    pthread_mutex_lock(&mtm->mutex);
}

void mtm_unlock(mtm_t* mtm)
{
    pthread_mutex_unlock(&mtm->mutex);
}

// universal_enumeration_code <apos turingindex, apos sideInfo  inputString>
// mtm code [table, inputTape]
int mtm_load_from_code(mtm_t* mtm, mpz_t mtmCode)
{
    mpz_t temp1; mpz_init(temp1);
    mpz_t temp2; mpz_init(temp2);

    mpz_set(temp1, mtmCode);

    // load table
    int tableBits = mtm_load_table_from_index(&mtm->table, temp1);
    mpz_load_number_of_n_ones(temp2, mpz_sizeinbase(temp1,2) - tableBits);
    mpz_and(temp1, temp1, temp2);

    // TODO i think tape needs a leading one which i need to check if that gets added
    // because i nede to add an extra thing to tape code where there is an apos encoded string on the left.
    // and this can be 0 sometimes which means i need to add a leading one so the 0 doesnt get deleted.

    // load inputTape 
    int inputTapeBits = mtm_tape_load_from_code(&mtm->inputTape, temp1);

    mpz_clears(temp1, temp2, NULL);

    return tableBits + inputTapeBits;
}

// mtm code [table, inputTape]
int mtm_get_code(mtm_t* mtm, mpz_t mtmCode)
{
    mpz_set_ui(mtmCode, 0);

    mpz_t temp1; mpz_init(temp1);

    // push table encoding
    int tableBits = mtm_get_table_index(mtmCode, &mtm->table);
    // mpz_lshift(mtmCode, mtmCode, tableBits);

    //push inputTape encoding
    int inputTapeBits = mtm_tape_get_code(&mtm->inputTape, temp1);
    gmp_printf("inputtape code %Zd\n", temp1);
    mpz_lshift(mtmCode, mtmCode, inputTapeBits);
    mpz_ior(mtmCode, mtmCode, temp1);

    mpz_clears(temp1, NULL);

    printf("tableBits %d, inputTapeBits %d\n", tableBits, inputTapeBits);

    return tableBits + inputTapeBits;
}

void mtm_print(mtm_t* mtm)
{
    printf("mtm:\ninputTape,outputTape:\n");
    mtm_tape_print(&mtm->inputTape);
    mtm_tape_print(&mtm->outputTape);
    mtm_print_table_summary(&mtm->table);
    printf("\n");
    mtm_print_table(&mtm->table);
}