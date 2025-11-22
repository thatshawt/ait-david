#include "turing_sim.h"
#include "hashmap.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MIN(a,b) ((a)<(b) ? (a):(b))
#define MAX(a,b) ((a)>(b) ? (a):(b))

void tm_init(tm_t* tm)
{
    tm_reset_keep_table_and_states(tm);

    memset(tm->transition_table, 0, sizeof tm->transition_table);
}

void tm_reset_keep_table_and_states(tm_t* tm)
{
    tm->max_tape_index = TM_TAPE_SIZE;

    tm->tape_index = tm->max_tape_index/2;
    tm->low_visited_tape_index = tm->tape_index;
    tm->high_visited_tape_index = tm->tape_index;

    tm->state = 1;
    tm->halted = false;

    tm_fill_tape(tm, 0);
}

tm_transition_table_entry_t* tm_get_entry(tm_t* tm, int symbol, int state)
{
    return &tm->transition_table[symbol][state];
}

int tm_get_written_tape_size(tm_t* tm)
{
    int size = tm->high_visited_tape_index - tm->low_visited_tape_index + 1;
    // if(size == 0){
    //     printf("low %d high %d tape size was zero?\n",
    //         tm->low_visited_tape_index,
    //     tm->high_visited_tape_index);
    // }
    return size;
}

void tm_print_written_tape(tm_t* tm)
{
    for(int i=tm->low_visited_tape_index;i <= tm->high_visited_tape_index;i++){
        tm_symbol_t symbol_on_tape = tm->tape[i];
        putc(symbol_on_tape == 1 ? '1':'0', stdout);
    }
    putc('\n', stdout);
}


int tm_count_symbol_entire_tape(tm_t* tm, int symbol)
{
    int count = 0;
    for(int i=0;i <= TM_TAPE_SIZE;i++){
        tm_symbol_t symbol_on_tape = tm->tape[i];
        if(symbol == symbol_on_tape)count++;
    }
    return count;
}

int tm_count_written_symbol(tm_t* tm, tm_symbol_t symbol)
{
    int count = 0;
    for(int i=tm->low_visited_tape_index;i <= tm->high_visited_tape_index;i++){
        tm_symbol_t symbol_on_tape = tm->tape[i];
        if(symbol == symbol_on_tape)count++;
    }
    return count;
}

void tm_fill_tape(tm_t* tm, tm_symbol_t symbol)
{
    for(int i=0;i<TM_TAPE_SIZE;i++){
        tm->tape[i] = symbol;
    }
}

void tm_slice_init_from_written_tape(tm_t* tm, tape_slice_t* slice)
{
    slice->length = tm_get_written_tape_size(tm);
    slice->tapeslice = malloc(slice->length * sizeof(tm_symbol_t));
    // int j=0;
    // for(int i=tm->low_visited_tape_index;i<=tm->high_visited_tape_index;i++){
    //     slice->tapeslice[j++] = tm->tape[i];
    // }
    for(int i=0;i<slice->length;i++){
        slice->tapeslice[i] = tm->tape[tm->low_visited_tape_index+i];
    }
}

void tm_slice_free(tape_slice_t* slice)
{
    free(slice->tapeslice);
}

void tm_slice_print(tape_slice_t* slice)
{
    for(int i=0;i<slice->length;i++){
        char val = slice->tapeslice[i];
        putc(val == 1?'1':'0', stdout);
    }
    putc('\n', stdout);
}

// void tm_slice_hashmap_free(const void *item)
// {
//     tape_slice_t* slice = item;
//     tm_slice_free(slice);
// }

// uint64_t tm_slice_hashmap_hash(const void *item, uint64_t seed0, uint64_t seed1)
// {
//     tape_slice_t* slice = item;
//     return hashmap_sip(slice->tapeslice, slice->length, seed0, seed1);
// }

// uint64_t tm_slice_hashmap_compare(const void *a, const void *b, void *udata)
// {
//     const tape_slice_t* sa = a;
//     const tape_slice_t* sb = b;
//     int length = MIN(sa->length, sb->length);
//     for(int i=0;i<length;i++){
//         tm_symbol_t symbolA = sa->tapeslice[i];
//         tm_symbol_t symbolB = sb->tapeslice[i];
//         if(symbolA > symbolB)return 1;
//         else if(symbolA < symbolB) return -1;
//     }
//     return 0;
//     // return memcmp(sa->tapeslice, sb->tapeslice,);
// }

//got this from musl. thank you musl.
void tm_srand(unsigned s)
{
	tm_random_seed = s-1;
}

//got this from musl. thank you musl.
int tm_rand(void)
{
	tm_random_seed = 6364136223846793005ULL*tm_random_seed + 1;
	return tm_random_seed>>33;
}

void tm_fill_tape_with_random(tm_t* tm, int seed)
{
    tm_srand(seed);
    for(int i=0;i<TM_TAPE_SIZE;i++){
        tm->tape[i] = (tm_symbol_t) (tm_rand()%TM_SYMBOLS);
    }
}

void tm_print_entire_tape_symbol_frequencies(tm_t* tm)
{
    uint64_t total = 0;
    uint64_t symbols_freq[TM_SYMBOLS] = {0};
    for(int i=0;i<TM_TAPE_SIZE;i++){
        tm_symbol_t symbol = tm->tape[i];
        symbols_freq[symbol]++;
        total++;
    }
    printf("total symbols: %ld\n", total);
    for(int symbol=0;symbol<TM_SYMBOLS;symbol++){
        uint64_t count = symbols_freq[symbol];
        float percent = (double)count/(double)total;
        printf("    symbol %d, percent %f, frequency%ld\n", 
            symbol, percent, count);
    }
}

void tm_print_entry_short(tm_transition_table_entry_t* entry)
{
    printf("(%d %d %d)",entry->write,entry->move,entry->next_state);
}

void tm_print_table_short(tm_t* tm)
{
    int states = tm->states;
    for(int state=1;state<states+1;state++){
        for(int symbol=0;symbol<TM_SYMBOLS;symbol++){
            tm_transition_table_entry_t* entry =
                tm_get_entry(tm,symbol,state);
            tm_print_entry_short(entry);
        }
        if(state != states)
        printf("_");
    }
    printf("\n");
}


void tm_debug_print_table_entry(tm_transition_table_entry_t entry)
{
    printf("{entry write=%d, move=%c, next_state=%d}\n", entry.write,
                entry.move==TM_MOVE_R ? 'R':'L',
                entry.next_state);
}

void tm_load_table(tm_t* tm, char* table_string)
{
    int states = tm->states;
    for(int i=0;i<(3*TM_SYMBOLS*states);i+=(3*TM_SYMBOLS)){
        int z = 0;
        const int state = i/(3*TM_SYMBOLS)+1;
        for(int symbol=0;symbol<TM_SYMBOLS;symbol++){
            // printf("i %d, z %d\n", i, z);
            tm_transition_table_entry_t entry = (tm_transition_table_entry_t)
                {table_string[i+z++],table_string[i+z++],table_string[i+z++]};
        
            // printf("symbol %d, state %d\n",symbol,state);
            *tm_get_entry(tm, symbol, state) = entry;
            // tm_debug_print_table_entry(entry);
        }
        
        // tm.transition_table[1][i/6] = (struct tm_transition_table_entry_t)
        //     {table_str[i+3],table_str[i+4],table_str[i+5]};
    }
}

void tm_fancy_print_transitions(tm_t* tm)
{
    int states = tm->states;
    for(int state=0;state<states+1;state++){
        printf("state %d:\n", state);
        for(int symbol=0;symbol<TM_SYMBOLS;symbol++){
            tm_transition_table_entry_t entry =
                *tm_get_entry(tm, symbol, state);
            printf("    %d: write %d, move %c, next state %d\n",
                symbol, entry.write,
                entry.move==TM_MOVE_R ? 'R':'L',
                entry.next_state);
        }
        printf("\n");
    }
}

void tm_step(tm_t* tm)
{
    if(tm->state == 0){
        return;
    }

    tm_symbol_t symbol = tm->tape[tm->tape_index];
    const tm_transition_table_entry_t entry =
        *tm_get_entry(tm, symbol, tm->state);
    
    //write
    tm->tape[tm->tape_index] = entry.write;
    tm->low_visited_tape_index = MIN(tm->low_visited_tape_index, tm->tape_index);
    tm->high_visited_tape_index = MAX(tm->high_visited_tape_index, tm->tape_index);

    //move
    if(entry.move == TM_MOVE_L)tm->tape_index--;
    else tm->tape_index++;
    if(tm->tape_index < 0 || tm->tape_index > tm->max_tape_index){
        // printf("error: out of bounds tape index %d\n", tm->tape_index);
        tm->state = 0;
        tm->halted = true;
        tm->haltReason = HALT_TAPE_OUT_OF_BOUNDS;
        return;
    }

    //state
    tm->state = entry.next_state;

    if(tm->state == 0){
        tm->halted = true;
        tm->haltReason = HALT_NATURAL;
    }
}

uint64_t tm_step_until_halt_or_max(tm_t* tm, tm_run_opt_t opt)
{
    //check trivial nonhalting
    //it never works bruh idk why
    if(opt.trivialNonhaltingCheck){
        // printf("checked");
        bool hasHaltingTrans = false;
        for(int sym=0;sym<TM_SYMBOLS;sym++){
            for(int state=1;state<TM_MAX_STATES;state++){
                if(hasHaltingTrans)break;
                if(tm_get_entry(tm,sym,state)->next_state == 0){
                    hasHaltingTrans = true;
                    break;
                }
            }
        }
        if(hasHaltingTrans == false){
            tm->halted = true;
            tm->haltReason = HALT_TRIVIAL_NONHALTING;
            tm->state = 0;
            // printf("actually did it");
            return 0;
        }
    }
    const uint64_t max_steps = opt.max_steps;
    
    for(uint64_t i=0;i<max_steps;i++){
        // tm_print_state(tm);
        if(tm->state == 0){
            return i;
        }
        tm_step(tm);
    }
    //yea we just did that
    tm->state = 0;

    tm->halted = true;
    tm->haltReason = HALT_MAX_STEPS;
    return max_steps;
}

void tm_print_state(tm_t* tm)
{
    printf("tape_index: %d. symbol: %d. state: %d\n",
        tm->tape_index, tm->tape[tm->tape_index], tm->state);
}