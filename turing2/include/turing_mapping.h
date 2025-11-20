#ifndef TURING_MAPPING_H
#define TURING_MAPPING_H

#include "linkedlist.h"
#include "turing_sim.h"
#include <stdbool.h>

typedef uint64_t tm_index_t;

// typedef struct{
//     const tm_index_t* singleDigitsEntryMap;
//     const int singleDigits;
//     const int states;
    
//     const tm_index_t** singleDigitsTimesTable;
// } entrynumber_t;

// void entrynumber_create(entrynumber_t* entrynum, int states);
// void entrynumber_free(entrynumber_t* entrynum);

void tm_load_table_by_index(tm_t* tm, int index);

int tm_get_entry_digit(int states, tm_transition_table_entry_t* entry);
void tm_load_entry_from_digit(int states, int digit, tm_transition_table_entry_t* entry);
void tm_load_table_from_digits(tm_t* tm, int* digits);
void tm_load_table_into_digits_array(tm_t* tm, int* digits);

tm_index_t tm_get_table_index_from_digits(int states, int* digits);
void tm_load_digits_from_index(tm_t* tm, int* digits, tm_index_t index);

void tm_print_table_entryDigitsForm(tm_t* tm);

// tm_transition_table_entry_t entrynumber_entry_from_index(entrynumber_t* entrynum, tm_index_t index);
bool tm_entry_equals(tm_transition_table_entry_t* entry1, tm_transition_table_entry_t* entry2);
bool tm_entry_increment(int states, tm_transition_table_entry_t* entry);
bool tm_entry_decrement(int states, tm_transition_table_entry_t* entry);
bool tm_next_table_lexico(tm_t* tm);

int tm_num_per_entry(int states);
int tm_num_table_entries(int states);
tm_index_t tm_max_num_of_machines(tm_t* tm);

#endif