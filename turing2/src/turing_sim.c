#include "turing_sim.h"

#include <string.h>
#include <stdio.h>

void tm_init(tm_t* tm)
{
    tm->max_tape_index = TAPE_SIZE;

    tm->tape_index = tm->max_tape_index/2;

    tm->state = 1;
    
    tm_fill_tape(tm, 0);
}

void tm_fill_tape(tm_t* tm, tm_symbol_t symbol)
{
    for(int i=0;i<tm->max_tape_index;i++){
        tm->tape[i] = symbol;
    }
}

void tm_debug_print_table_entry(tm_transition_table_entry_t entry)
{
    printf("{entry write=%d, move=%c, next_state=%d}\n", entry.write,
                entry.move==TM_MOVE_R ? 'R':'L',
                entry.next_state);
}

// table_str has to be zero terminated
void tm_load_table(tm_t* tm, int states, char* table_str)
{
    for(int i=0;i<(3*SYMBOLS*states);i+=(3*SYMBOLS)){
        int z = 0;
        const int state = i/(3*SYMBOLS)+1;
        for(int symbol=0;symbol<SYMBOLS;symbol++){
            // printf("i %d, z %d\n", i, z);
            tm_transition_table_entry_t entry = (tm_transition_table_entry_t)
                {table_str[i+z++],table_str[i+z++],table_str[i+z++]};
        
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
    for(uint64_t i=1;i<max_steps;i++){
        // tm_print_state(tm);
        tm_step(tm);
        if(tm->state == 0){
            return i;
        }
    }
    return max_steps;
}

void tm_print_state(tm_t* tm)
{
    printf("tape_index: %d. symbol: %d. state: %d\n",
        tm->tape_index, tm->tape[tm->tape_index], tm->state);
}