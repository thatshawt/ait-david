#include "turing_mapping.h"
#include <stdint.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

bool tm_eq_tables(tm_t* tm1, tm_t* tm2)
{
    for(int sym=0;sym<TM_SYMBOLS;sym++){
        for(int state=1;state<tm1->states;state++){
            tm_transition_table_entry_t entry1 = tm1->transition_table[sym][state];
            tm_transition_table_entry_t entry2 = tm2->transition_table[sym][state];
            if(entry1.move != entry2.move ||
                entry1.next_state != entry2.next_state ||
                entry1.write != entry2.write
            )
                return false;
        }
    }

    return true;
}

void tm_load_table_by_index(tm_t* tm, mpz_t index)
{
    //extract digits from index
    int digits[256] = {0};

    tm_extract_digits_from_index(tm, digits, index);
    // printf("extracted digits: ");
    // for(int i=0;i<8;i++){
    //     printf("%d ", digits[i]);
    // }
    // printf("\n");    
    //load from digits
    tm_load_table_from_digits(tm, digits);
}

//loads it little endian.
void tm_extract_digits_from_index(tm_t* tm, int* digits, mpz_t index)
{
    tm_mutex_lock(tm);
    const int states = tm->states;
    tm_mutex_unlock(tm);

    mpz_t multiplier, count, temp;
    mpz_init_set(count, index);
    mpz_init(temp);
    mpz_init_set_ui(multiplier, tm_num_per_entry(states));

    // tm_index_t count = index;
    // int* d = digits+tm_num_table_entries(states) - 1;
    // int* d = digits;
    // int offset = tm_num_table_entries(states) - 1;
    int offset = 0;

    // gmp_printf("start index=%Zd, count=%Zd, multiplier=%Zd\n", index, count, multiplier);
    
    mpz_t base; mpz_init(base);
    while(true){
        offset = 0;
        mpz_set_ui(base, 1);
        if(mpz_cmp(count, multiplier) < 0){
            // gmp_printf("final d[0] = %Zd;, d=%d\n", count, d-digits);
            // d[0] = mpz_get_ui(count);
            // offset = 0;
            // gmp_printf("final add digit %Zd\n", count, offset);
            digits[offset] = mpz_get_ui(count);
            mpz_clears(multiplier, base, count, temp, NULL);

            //flip the array cus its loaded backwards
            // for(int i=0;i<tm_num_table_entries(states) - 1;i++){
            //     int temp = digits[i];
            //     digits[i] = digits[tm_num_table_entries(states) - 1 - i];
            //     digits[tm_num_table_entries(states)-1-i] = temp;
            // }
            return;
        }
        
        while(mpz_cmp(base, count) <= 0){
            offset++;
            mpz_mul(base, base, multiplier);
        }
        mpz_fdiv_q(base, base, multiplier);
        offset--;

        mpz_fdiv_q(temp, count, base); // temp = count/base
        
        // d[0] = mpz_get_ui(temp); // d[0] = count/base;
        digits[offset] = mpz_get_ui(temp);

        // gmp_printf("added digit %d, offset=%d\n", digits[offset], offset);

        mpz_mul(temp, temp, base);

        // mpz_mul(base, base, temp);
        mpz_sub(count, count, temp); // count = count - (base * (count/base));
        // mpz_add_ui(count, count, 1);
        // offset--;

        // if(offset > 10){
        //     gmp_printf("index %Zd, count %Zd, offset %d\n",index, count, offset);
        //     exit(1);
        // }

        // d--;
    }

}

// the digits encodes the index in little endian form.
// the digits would represent a number in base (4*states + 4).
// the base can also be calculated with the function tm_num_per_entry(int).
void tm_get_table_index_from_digits(int states, int* digits, mpz_t result)
{
    mpz_t index,base,temp;
    mpz_init(temp);
    mpz_init_set_ui(index, 0);
    mpz_init_set_ui(base, 1);

    int* d = digits;

    for(int state=1;state<states;state++){
        for(int sym=0;sym<TM_SYMBOLS;sym++){
            mpz_mul_si(temp, base, *d);  //temp = base * (*d)
            mpz_add(index, index, temp); //index += base * (*d);
            mpz_mul_si(base, base, tm_num_per_entry(states));//base *= tm_num_per_entry(states);
            d++;
        }
    }
    if(result)mpz_set(result, index);
    mpz_clears(index, base, temp, NULL);
    // return index;
}

void tm_extract_digits_from_table(tm_t* tm, int* digits)
{
    tm_mutex_lock(tm);
    const int states = tm->states;
    tm_mutex_unlock(tm);
    int* d = digits;
    for(int state=1;state<=states;state++){
        for(int sym=0;sym<TM_SYMBOLS;sym++){
            tm_transition_table_entry_t entry = tm_get_entry(tm, sym, state);
            int digit = tm_get_entry_digit(states, &entry);
            d[0] = digit;
            d++;
        }
    }
}

void tm_print_table_entryDigitsForm(tm_t* tm)
{
    tm_mutex_lock(tm);
    const int states = tm->states;
    tm_mutex_unlock(tm);
    for(int state=1;state<=states;state++){
        for(int sym=0;sym<TM_SYMBOLS;sym++){
            tm_transition_table_entry_t entry = tm_get_entry(tm, sym, state);
            printf("%d ", tm_get_entry_digit(states, &entry));
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
    tm_mutex_lock(tm);
    const int states = tm->states;
    tm_mutex_unlock(tm);
    int* d = digits;
    for(int i=1;i<=states;i++){
        for(int j=0;j<TM_SYMBOLS;j++){
            tm_transition_table_entry_t entry = tm_get_entry(tm, j, i);
            tm_load_entry_from_digit(states, *d, &entry);
            tm_set_entry(tm,j,i,&entry);
            d++;
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

void tm_max_num_of_machines(int states, mpz_t result)
{
    // (4*(n+1))^(2*n)
    // (4n+4)^(2n)
    if (result)mpz_ui_pow_ui(result,tm_num_per_entry(states),tm_num_table_entries(states));
    // return pow(tm_num_per_entry(states),tm_num_table_entries(states));
}

// i dont know why or how but this seems wrong...
void tm_machines_considered_for_full_enumeration(int states, mpz_t result)
{
    // (4n + 2)^2n
    if (result)mpz_ui_pow_ui(result,4*states + 2, 2*states);
    // return pow(4*states + 2, 2*states);
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
        tm_transition_table_entry_t entry = tm_get_entry(tm, symbol, state);
        bool tryIncrement = tm_entry_increment(
            tm->states,
            &entry
        );
        tm_set_entry(tm, symbol, state, &entry);

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

// bool tm_next_table_enumeration(tm_t* tm)
// {

// }
