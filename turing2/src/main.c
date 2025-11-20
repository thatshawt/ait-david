#include "turing_sim.h"
#include "turing_mapping.h"
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
    //bb5 test
    char bb5_table[] = {1,TM_MOVE_R,2, 1,TM_MOVE_L,3, 1,TM_MOVE_R,3, 1,TM_MOVE_R,2, 1,TM_MOVE_R,4, 0,TM_MOVE_L,5, 1,TM_MOVE_L,1, 1,TM_MOVE_L,4, 1,TM_MOVE_R,0, 0,TM_MOVE_L,1};

    printf("bb5\n");
    tm_init(tm_point);
    tm.states = 5;
    tm_load_table(tm_point, bb5_table);
    tm_print_table_short(tm_point);
    tm_print_table_entryDigitsForm(tm_point);
    uint64_t steps = tm_step_until_halt_or_max(tm_point, 9999999999L);
    printf("bb5 steps %ld, tapesize:%d, 1's: %d\n", steps, tm_get_written_tape_size(tm_point), tm_count_written_symbol(tm_point,1));


    /*
    bb4
        A 	    B       C 	    D
    0 	1RB 	1LA 	1RH 	1RD
    1 	1LB 	0LC 	1LD 	0RA
    */
    //bb4 test 
    char bb4_table[] = {1,TM_MOVE_R,2,1,TM_MOVE_L,2,1,TM_MOVE_L,1,0,TM_MOVE_L,3,1,TM_MOVE_R,0,1,TM_MOVE_L,4,1,TM_MOVE_R,4,0,TM_MOVE_R,1};

    printf("\nbb4\n");
    tm_init(tm_point);
    tm.states = 4;
    tm_load_table(tm_point, bb4_table);
    tm_print_table_short(tm_point);
    tm_print_table_entryDigitsForm(tm_point);
    steps = tm_step_until_halt_or_max(tm_point, 9999999999L);
    printf("bb4 steps %ld, tapesize:%d, 1's: %d\n", steps, tm_get_written_tape_size(tm_point), tm_count_written_symbol(tm_point,1));

    int bb4Digits[8];
    tm_load_table_into_digits_array(tm_point, bb4Digits);

    //fill with random to see if random number gen is ok.
    tm_init(tm_point);
    tm_print_table_short(tm_point);
    tm_print_table_entryDigitsForm(tm_point);
    tm_fill_tape_with_random(tm_point, 1337);
    tm_print_entire_tape_symbol_frequencies(tm_point);

    // some turing machine counting.
    tm_init(tm_point);
    tm.states = 3;
    int i = 1;
    while(tm_next_table_lexico(tm_point) == false){
        i++;
    }
    printf("%d states, %d diff tables\n", tm.states, i);

    //another way to do it
    printf("%ld machines for 3 states\n", tm_max_num_of_machines(tm_point));

    // how many per single entry;
    printf("with %d states, %d per entry\n",
        tm_point->states, tm_num_per_entry(tm_point->states));

    tm_print_table_short(tm_point);
    tm_print_table_entryDigitsForm(tm_point);

    printf("\nbb4 from digits\n");
    tm_init(tm_point);
    tm.states = 4;
    tm_load_table_from_digits(tm_point, bb4Digits);
    tm_print_table_short(tm_point);

    tm_index_t index = tm_get_table_index_from_digits(tm.states, bb4Digits);
    printf("bb4 is index %llu i think\n", index);

    int bb4Digits2[8] = {0};
    tm_init(tm_point);
    tm.state = 4;
    tm_load_digits_from_index(tm_point,bb4Digits2,index);
    printf("bb4 from the index back to digits then back to index\n");
    // tm_print_table_short(tm_point);
    index = tm_get_table_index_from_digits(tm.states, bb4Digits);
    printf("index %llu again\n", index);
}

/*

turing simulation:
    variable tape.
    variable states/alphabet/transitions.
    step function.
    halting check.
    error results.

turing index language:
    dense mapping from integers to turing machines. 
    random nth permutation via the factorial algorithm thing.
    n+1th permutation by simple algorithm.
    nth_permutation()
    next_permutation()

turing index enumeration:
    enumerate the turing index language for halting machines and their tape results.

*/