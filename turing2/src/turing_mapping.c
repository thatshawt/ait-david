#include "turing_mapping.h"
#include <stdint.h>
#include <math.h>
#include <stdio.h>

//get tm from integer index.

// typedef uint64_t tm_index_t;

// typedef struct {

// } awd;

/*
typedef struct {
    tm_symbol_t write;
    enum TM_Move move;
    tm_state_t next_state;
} tm_transition_table_entry_t;
*/

// void tm_load_table_by_index(tm_t* tm, tm_index_t index)
// {
//     int digits[] = {0};
// }

void tm_load_table_by_index(tm_t* tm, tm_index_t index)
{
    //extract digits from index
    int digits[256] = {0};
    tm_extract_digits_from_index(tm, digits, index);
    
    //load from digits
    tm_load_table_from_digits(tm, digits);
}

//loads it little endian.
void tm_extract_digits_from_index(tm_t* tm, int* digits, tm_index_t index)
{
    const tm_index_t multiplier = tm_num_per_entry(tm->states);
    
    tm_index_t count = index;
    int* d = digits+tm_num_table_entries(tm->states) - 1;
    
    while(true){
        tm_index_t base = 1;
        if(count <= multiplier){
            // printf("before d[0] = count;\n");
            d[0] = (int)count;
            return;
        }
        
        while(base <= count){
            base *= multiplier;
        }
        base /= multiplier;
        
        // printf("i:%d, count %llu ,base %llu\n",i, count, base);
        d[0] = count/base;
        count -= base * (count/base);
        // printf("added digit %d\n", d[0]);
        d--;
    }

}

// the digits encodes the index in little endian form.
// the digits would represent a number in base (4*states + 4).
// the base can also be calculated with the function tm_num_per_entry(int).
tm_index_t tm_get_table_index_from_digits(int states, int* digits)
{
    tm_index_t index = 0;
    int* d = digits;
    tm_index_t base = 1;
    for(int sym=0;sym<TM_SYMBOLS;sym++){
        for(int state=0;state<states;state++){
            index += base * (*d);
            base *= tm_num_per_entry(states);
            d++;
        }
    }
    return index;
}

void tm_extract_digits_from_table(tm_t* tm, int* digits)
{
    int* d = digits;
    for(int sym=0;sym<TM_SYMBOLS;sym++){
        for(int state=1;state<=tm->states;state++){
            tm_transition_table_entry_t* entry = tm_get_entry(tm, sym, state);
            int digit = tm_get_entry_digit(tm->states, entry);
            d[0] = digit;
            d++;
        }
    }
}

void tm_print_table_entryDigitsForm(tm_t* tm)
{
    for(int sym=0;sym<TM_SYMBOLS;sym++){
        for(int state=1;state<=tm->states;state++){
            tm_transition_table_entry_t* entry = tm_get_entry(tm, sym, state);
            printf("%d ", tm_get_entry_digit(tm->states, entry));
        }
    }
    printf("\n");
}

int tm_get_entry_digit(int states, tm_transition_table_entry_t* entry)
{
    tm_transition_table_entry_t entryTemp;
    entryTemp.move = 0;
    entryTemp.next_state = 0;
    entryTemp.write = 0;
    for(int i=0;i<tm_num_per_entry(states);i++){
        if(tm_entry_equals(&entryTemp, entry))return i;
        tm_entry_increment(states, &entryTemp);
    }
    return -1; //bruh shouldnt happen
}

void tm_load_entry_from_digit(int states, int digit, tm_transition_table_entry_t* entry)
{
    tm_transition_table_entry_t entryTemp;
    entryTemp.move = 0;
    entryTemp.next_state = 0;
    entryTemp.write = 0;
    for(int i=0;i<tm_num_per_entry(states);i++){
        if(i == digit){
            entry->move = entryTemp.move;
            entry->write = entryTemp.write;
            entry->next_state = entryTemp.next_state;
            return;
        }
        tm_entry_increment(states, &entryTemp);
    }
}

void tm_load_table_from_digits(tm_t* tm, int* digits)
{
    int* d = digits;
    for(int j=0;j<TM_SYMBOLS;j++){
        for(int i=1;i<=tm->states;i++){
            tm_transition_table_entry_t* entry = tm_get_entry(tm, j, i);
            tm_load_entry_from_digit(tm->states, *d++, entry);
        }
    }
    
}

bool tm_entry_equals(tm_transition_table_entry_t* entry1, tm_transition_table_entry_t* entry2)
{
    return (entry1->move==entry2->move
        && entry1->write==entry2->write
        && entry1->next_state==entry2->next_state) ? true:false;
}

// returns true if overflow happened
bool tm_entry_increment(int states, tm_transition_table_entry_t* entry)
{
    /*
    try to increment write
        overflow:
            set to 0
            try to increment move
                overflow:
                    set to 0
                    try to increment next_state
                        overflow:
                            set to 0
                            return true 
    */

    tm_symbol_t write = entry->write;
    enum TM_Move move = entry->move;
    tm_state_t next_state = entry->next_state;
    if(++write >= TM_SYMBOLS){// write overflow
        entry->write = 0;
        if(++move >= 2){//move overflow
            entry->move = 0;
            if(++next_state > states){// next_state overflow
                entry->next_state = 0;
                return true;
            }else{
                entry->next_state = next_state;
            }
        }else{
            entry->move = move;
        }
    }else{
        entry->write = write;
    }
    return false;
}

//TODO account for TM_SYMBOLS
int tm_num_per_entry(int states)
{
    return (4*(states+1));
}

//TODO make this account for TM_SYMBOLS...
int tm_num_table_entries(int states)
{
    return 2*states;
}

tm_index_t tm_max_num_of_machines(int states)
{
    // (4*(n+1))^(2*n)
    // tm_index_t states = tm->states;
    return pow(tm_num_per_entry(states),tm_num_table_entries(states));
}

/*
advance a tm's transition table to the lexicographical next table.
returns true if overflowed.
try to increment first one
    overflow:
        try to increment next one.
            ...
                hit last state -> overflowed:
                    return true;
*/
bool tm_next_table_lexico(tm_t* tm)
{
    int symbol = 0;
    int state = 1; // dont start at 0, which is just for halting
    for(;;){
        bool tryIncrement = tm_entry_increment(
            tm->states,
            tm_get_entry(tm, symbol, state)
        );

        if(tryIncrement == true){//overflowed
            if(++symbol >= TM_SYMBOLS){//symbol overflow
                symbol = 0;
                if(++state > tm->states){//state overflow
                    state = 1; //dont need this, but for completeness sake whatever.
                    //they both overflowed we reached the end
                    return true;
                }
            }
            continue;
        }else{//didnt overflow
            return false;
        }

    }
}
