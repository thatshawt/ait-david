#include "turing_sim.h"

#include <string.h>
#include <stdio.h>

#define MIN(a,b) ((a)<(b) ? (a):(b))
#define MAX(a,b) ((a)>(b) ? (a):(b))

void tm_init(tm_t* tm)
{
    tm->max_tape_index = TAPE_SIZE;

    tm->tape_index = tm->max_tape_index/2;
    tm->low_visited_tape_index = tm->tape_index;
    tm->high_visited_tape_index = tm->tape_index;

    tm->state = 1;
    
    tm_fill_tape(tm, 0);
}

int tm_get_written_tape_size(tm_t* tm)
{
    return tm->high_visited_tape_index-tm->low_visited_tape_index + 1;
}

void tm_print_written_tape(tm_t* tm)
{
    for(int i=tm->low_visited_tape_index;i <= tm->high_visited_tape_index;i++){
        tm_symbol_t symbol_on_tape = tm->tape[i];
        putc(symbol_on_tape == 1 ? '1':'0', stdout);
    }
    putc('\n', stdout);
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
    for(int i=0;i<tm->max_tape_index;i++){
        tm->tape[i] = symbol;
    }
}

void tm_srand(unsigned s)
{
	tm_random_seed = s-1;
}

int tm_rand(void)
{
	tm_random_seed = 6364136223846793005ULL*tm_random_seed + 1;
	return tm_random_seed>>33;
}

void tm_fill_tape_with_random(tm_t* tm, int seed)
{
    tm_srand(seed);
    for(int i=0;i<tm->max_tape_index;i++){
        tm->tape[i] = (tm_symbol_t) (tm_rand()%SYMBOLS);
    }
}

void tm_print_entire_tape_symbol_frequencies(tm_t* tm)
{
    uint64_t total = 0;
    uint64_t symbols_freq[SYMBOLS] = {0};
    for(int i=0;i<TAPE_SIZE;i++){
        tm_symbol_t symbol = tm->tape[i];
        symbols_freq[symbol]++;
        total++;
    }
    printf("total symbols: %ld\n", total);
    for(int symbol=0;symbol<SYMBOLS;symbol++){
        uint64_t count = symbols_freq[symbol];
        float percent = (double)count/(double)total;
        printf("    symbol %d, percent %f, frequency%ld\n", 
            symbol, percent, count);
    }
}

void tm_debug_print_table_entry(tm_transition_table_entry_t entry)
{
    printf("{entry write=%d, move=%c, next_state=%d}\n", entry.write,
                entry.move==TM_MOVE_R ? 'R':'L',
                entry.next_state);
}

void tm_load_table(tm_t* tm, int states, char* table_string)
{
    for(int i=0;i<(3*SYMBOLS*states);i+=(3*SYMBOLS)){
        int z = 0;
        const int state = i/(3*SYMBOLS)+1;
        for(int symbol=0;symbol<SYMBOLS;symbol++){
            // printf("i %d, z %d\n", i, z);
            tm_transition_table_entry_t entry = (tm_transition_table_entry_t)
                {table_string[i+z++],table_string[i+z++],table_string[i+z++]};
        
            // printf("symbol %d, state %d\n",symbol,state);
            tm->transition_table[symbol][state] = entry;
            // tm_debug_print_table_entry(entry);
        }
        
        // tm.transition_table[1][i/6] = (struct tm_transition_table_entry_t)
        //     {table_str[i+3],table_str[i+4],table_str[i+5]};
    }
}

void tm_fancy_print_transitions(tm_t* tm, int states)
{
    for(int state=0;state<states+1;state++){
        printf("state %d:\n", state);
        for(int symbol=0;symbol<SYMBOLS;symbol++){
            tm_transition_table_entry_t entry =
                tm->transition_table[symbol][state];
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
    if(tm->state == 0)return;

    tm_symbol_t symbol = tm->tape[tm->tape_index];
    const tm_transition_table_entry_t entry =
        tm->transition_table[symbol][tm->state];
    
    //write
    tm->tape[tm->tape_index] = entry.write;
    tm->low_visited_tape_index = MIN(tm->low_visited_tape_index, tm->tape_index);
    tm->high_visited_tape_index = MAX(tm->high_visited_tape_index, tm->tape_index);

    //move
    if(entry.move == TM_MOVE_L)tm->tape_index--;
    else tm->tape_index++;
    if(tm->tape_index < 0 || tm->tape_index > tm->max_tape_index){
        printf("error: out of bounds tape index %d\n", tm->tape_index);
        tm->state = 0;
        return;
    }

    //state
    tm->state = entry.next_state;
}

uint64_t tm_step_until_halt_or_max(tm_t* tm, uint64_t max_steps)
{
    for(uint64_t i=0;i<max_steps;i++){
        // tm_print_state(tm);
        if(tm->state == 0){
            return i;
        }
        tm_step(tm);
    }
    return max_steps;
}

void tm_print_state(tm_t* tm)
{
    printf("tape_index: %d. symbol: %d. state: %d\n",
        tm->tape_index, tm->tape[tm->tape_index], tm->state);
}