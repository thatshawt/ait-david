#include "turing_sim.h"
#include <stdio.h>

int main(){
    printf("hello turing\n");

    tm_t tm;

    tm_t* tm_point = &tm;
    /*
    bb5:
     	    0	1
        A 	1RB	1LC
        B 	1RC	1RB
        C 	1RD	0LE
        D 	1LA	1LD
        E 	1RZ	0LA
    */

    char bb5_table[] = {1,TM_MOVE_R,2, 1,TM_MOVE_L,3, 1,TM_MOVE_R,3, 1,TM_MOVE_R,2, 1,TM_MOVE_R,4, 0,TM_MOVE_L,5, 1,TM_MOVE_L,1, 1,TM_MOVE_L,4, 1,TM_MOVE_R,0, 0,TM_MOVE_L,1};

    tm_init(tm_point);
    
    tm_load_table(tm_point, 5, bb5_table);

    // tm_fancy_print_transitions(tm_point, 5);

    uint64_t steps = tm_step_until_halt_or_max(tm_point, 9999999999L);
    printf("bb5 steps %ld, tapesize:%d, 1's: %d\n", steps, tm_get_written_tape_size(tm_point), tm_count_written_symbol(tm_point,1));


    /*
    bb4
        A 	    B       C 	    D
    0 	1RB 	1LA 	1RH 	1RD
    1 	1LB 	0LC 	1LD 	0RA
    */
   char bb4_table[] = {1,TM_MOVE_R,2,1,TM_MOVE_L,2,1,TM_MOVE_L,1,0,TM_MOVE_L,3,1,TM_MOVE_R,0,1,TM_MOVE_L,4,1,TM_MOVE_R,4,0,TM_MOVE_R,1};

    tm_init(tm_point);
    
    tm_load_table(tm_point, 4, bb4_table);

    // tm_fancy_print_transitions(tm_point, 4);

    steps = tm_step_until_halt_or_max(tm_point, 9999999999L);
    printf("bb4 steps %ld, tapesize:%d, 1's: %d\n", steps, tm_get_written_tape_size(tm_point), tm_count_written_symbol(tm_point,1));
    tm_print_written_tape(tm_point);
}

// [1,TM_MOVE_R,2, 1,TM_MOVE_L,3, 1,TM_MOVE_R,3, 1,TM_MOVE_R,2, 1,TM_MOVE_R,4, 0,TM_MOVE_L,5, 1,TM_MOVE_L,1, 1,TM_MOVE_L,4, 1,TM_MOVE_R,0, 0,TM_MOVE_L,1, 0]

/*

turing simulation:
    variable tape.
    variable states/alphabet/transitions.
    step function.
    halting check.
    error results.

turing index language:
    map integers to turing machines.

turing index enumeration:
    enumerate the turing index language for halting machines and their tape results.

*/