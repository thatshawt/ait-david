#include "turing_tests.h"
#include "turing_sim.h"
#include "turing_mapping.h"

#include "turing_utils.h"

#include "gmp_mpzstr.h"
#include "mpz_helpers.h"
#include "monotone_tm.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

unittest_state_t unitstate;
tm_t tm;

tm_run_opt_t runopt_9999_nocheck = (tm_run_opt_t){
    .trivialNonhaltingCheck=false,
    // .max_steps=9999999999L
};
void test_all(test_opt_t* testopt)
{
    printf("\nRunning all tests.\n");
    
    mpz_init_set_ui(runopt_9999_nocheck.max_steps, 9999999999);
    
    test_turing_sim(testopt);
    test_turing_mapping(testopt);
    test_turing_enumerate(testopt);
    test_monotone_tm(testopt);
    test_mpz_helpers(testopt);

    mpz_clears(runopt_9999_nocheck.max_steps,NULL);

    printf("Testing complete.\n\n");
}

void test_monotone_tm(test_opt_t* testopt)
{
    printf("    Monotone Turing Machine Tests\n");
    {
        unittest_begin(&unitstate, "dummy", testopt);
        unittest_finish(&unitstate);
    }

    // mtm entry increment tests
    {
        mpz_t counter,temp;
        mpz_inits(counter,temp,NULL);
        char testTitle[200] = {0};

        const int statesMax = 3;
        const int worktapesMax = 3;
        for(int states=1; states<=statesMax; states++){
            for(int worktapes=1; worktapes<=worktapesMax; worktapes++){
                // check if incrementing follows the from digit thing
                sprintf(testTitle, "mtm entry increment == mtm entry from digit++ == get_digit (%d states, %d worktapes)", states, worktapes);

                unittest_begin(&unitstate, testTitle, testopt);
                
                mtm_transition_entry_t entry1;
                mtm_transition_entry_t entry2;
                
                mtm_entry_zero(&entry1);
                mpz_set_ui(counter, 0);

                bool overflow = false;
                
                do{
                    
                    mtm_entry_from_digit(&entry2, counter, states, worktapes);

                    //check if entries are equal
                    bool equalEntries = mtm_entry_equal(&entry1, &entry2, states, worktapes);

                    unittest_assert_true(&unitstate, equalEntries);

                    if(!equalEntries){
                        gmp_printf("counter %Zd\n", counter);
                        mtm_print_entry_short(&entry1, worktapes);
                        mtm_print_entry_short(&entry2, worktapes);
                        printf("\n");
                    }

                    //check if entry get_digit works too
                    mtm_entry_get_digit(temp, &entry1, states, worktapes);

                    bool equalGetDigit = mpz_cmp(temp, counter) == 0;

                    unittest_assert_true(&unitstate, equalGetDigit);

                    overflow = mtm_entry_increment(&entry1, states, worktapes);
                    mpz_add_ui(counter, counter, 1);
                }while(!overflow);

                unittest_finish(&unitstate);
                
                // check if it went to the max digit
                sprintf(testTitle, "    did it reach max digit?");
                unittest_begin(&unitstate, testTitle, testopt);
                
                mpz_get_entry_max_digit(temp, states, worktapes);

                unittest_assert_true(&unitstate, mpz_cmp(temp, counter) == 0);

                unittest_finish(&unitstate);
                
            }
        }
        
        mpz_clears(counter,temp,NULL);
    }
    
    // mtm table increment tests
    {
        mpz_t counter,temp;
        mpz_inits(counter,temp,NULL);
        char testTitle[200] = {0};

        const int statesMax = 1;
        const int worktapesMax = 1;
        for(int states=1; states<=statesMax; states++){
            for(int worktapes=1; worktapes<=worktapesMax; worktapes++){
                // check if incrementing follows the from digit thing
                sprintf(testTitle, "mtm table increment == mtm from index++ == mtm get index (%d states, %d worktapes)", states, worktapes);

                unittest_begin(&unitstate, testTitle, testopt);

                // while true
                // load table2 from index
                // check if they are the same table
                
                // load index of table1 into temp
                // check if index == temp
                
                // increment table1
                // increment table2 index
                // break if table1 overflows
                
                unittest_finish(&unitstate);
                
            }
        }
        
        mpz_clears(counter,temp,NULL);
    }

    // do{
    //     entry = mtm_table_get_entry(table, &index);
    // }while(!mtm_entry_index_increment(&index, states, worktapes));
}

void test_mpz_helpers(test_opt_t* testopt)
{
    printf("    Mpz Helpers Tests\n");
    {
        unittest_begin(&unitstate, "dummy", testopt);
        unittest_finish(&unitstate);
    }

    // Cantor Pair == Cantor Unpair
    {
        const int n = 100;
        unittest_begin(&unitstate, "Cantor Pair == Cantor Unpair from (0,0) to (99,99)", testopt);

        mpz_t* poopooooos = mpzstr_init_malloc(3);
        mpz_t* pooo1 = (poopooooos+0);
        mpz_t* pooo2 = (poopooooos+1);
        mpz_t* pooo3 = (poopooooos+2);

        for(int a=0;a<n;a++){
            for(int b=0;b<n;b++){
                mpz_set_ui(*pooo1, a);
                mpz_set_ui(*pooo2, b);

                mpz_cantor_pair(*pooo3, *pooo1, *pooo2);
                // gmp_printf("pair   (%Zd,%Zd) -> %Zd\n", *pooo1,*pooo2,*pooo3);

                mpz_cantor_unpair(*pooo1, *pooo2, *pooo3);
                // gmp_printf("unpair (%Zd,%Zd) -> %Zd\n\n", *pooo1,*pooo2,*pooo3);

                unittest_assert_true(&unitstate, mpz_get_ui(*pooo1) == a & mpz_get_ui(*pooo2) == b);
            }
        }

        mpzstr_clear_free(poopooooos);

        unittest_finish(&unitstate);
    }

    {
        unittest_begin(&unitstate, "dummy", testopt);
        unittest_finish(&unitstate);
    }

}

void test_turing_sim(test_opt_t* testopt)
{
    printf("    Turing Sim Tests\n");

    
    {
        unittest_begin(&unitstate, "bb5 load&run", testopt);
        char bb5_table[] = BB5_TABLE_LITERAL;

        tm_init(&tm);
        tm.states = 5;
        tm_load_table(&tm, bb5_table);

        mpz_t steps; mpz_init(steps);
        tm_step_until_halt_or_max(&tm, runopt_9999_nocheck, &steps);

        unittest_assert_int_equals(&unitstate, mpz_get_ui(steps), 47176870);
        unittest_assert_int_equals(&unitstate, tm_get_written_tape_size(&tm), 12289);
        unittest_assert_int_equals(&unitstate, tm_count_written_symbol(&tm,1), 4098);
        
        tm_destroy(&tm);
        mpz_clears(steps,NULL);
        unittest_finish(&unitstate);
    }


    {
        unittest_begin(&unitstate, "bb4 load&run", testopt);

        char bb4_table[] = BB4_TABLE_LITERAL;

        tm_init(&tm);
        tm.states = 4;
        tm_load_table(&tm, bb4_table);
        mpz_t steps; mpz_init(steps);
        tm_step_until_halt_or_max(&tm, runopt_9999_nocheck, &steps);
        unittest_assert_int_equals(&unitstate, mpz_get_ui(steps), 107);
        unittest_assert_int_equals(&unitstate, tm_get_written_tape_size(&tm), 14);
        unittest_assert_int_equals(&unitstate, tm_count_written_symbol(&tm,1), 13);
        tm_destroy(&tm);
        mpz_clears(steps,NULL);
        unittest_finish(&unitstate);
    }

    {
        unittest_begin(&unitstate, "bb4 tape slice compare", testopt);

        tm_init(&tm);
        tm.states=4;

        char bb4Literal[] = BB4_TABLE_LITERAL;

        tm_load_table(&tm, bb4Literal);
        tm_step_until_halt_or_max(&tm, runopt_9999_nocheck, NULL);

        tape_slice_t slice;
        tm_slice_init_from_written_tape(&tm, &slice);
        // tm_slice_print(&slice);
        
        // 10111111111111

        tm_symbol_t* bb4TapeSliceData = (tm_symbol_t[]){1,0,1,1,1,1,1,1,1,1,1,1,1,1};

        tape_slice_t bb4ExpectedSlice = (tape_slice_t){
            .tapeslice = bb4TapeSliceData,
            .length = 14
        };

        int sliceCompare = tm_slice_compare(&slice, &bb4ExpectedSlice);

        unittest_assert_int_equals(&unitstate, sliceCompare, 0);
    
        tm_slice_free(&slice);
        tm_destroy(&tm);
        unittest_finish(&unitstate);
    }

    // random tape test
    // {
    //     unittest_begin(&unitstate, "random tape test", testopt);

    //     tm_init(&tm);
    //     tm_fill_tape_with_random(&tm, 1337);
    //     int ones = tm_count_symbol_entire_tape(&tm, 1);
    //     printf("ones %d\n", ones);
    //     unittest_assert_int_equals(&unitstate, ones, 19833);
    //     tm_destroy(&tm);
    //     unittest_finish(&unitstate);
    // }

    {
        unittest_begin(&unitstate, "counting 2 state machines bruteforce", testopt);

        tm_init(&tm);
        tm.states = 2;
        int i = 1;
        while(tm_next_table_lexico(&tm) == false){
            i++;
        }
        // printf("%d states, %d diff tables\n", tm.states, i);
        unittest_assert_int_equals(&unitstate, i, 20736);
        tm_destroy(&tm);
        unittest_finish(&unitstate);
    }

    {
        unittest_begin(&unitstate, "number of 3 state machines", testopt);

        tm_init(&tm);
        tm.states = 3;
        mpz_t number; mpz_init(number);
        tm_max_num_of_machines(tm.states, number);
        unittest_assert_int_equals(&unitstate, mpz_get_ui(number), 16777216);
        tm_destroy(&tm);
        mpz_clear(number);
        unittest_finish(&unitstate);
    }
}

void test_turing_mapping(test_opt_t* testopt)
{
    printf("    Turing Mapping Tests\n");

    {
        unittest_begin(&unitstate, "bb4 table->digits", testopt);
        int bb4Digits1[8] = {0};
        int bb4DigitsExpected1[] = {11, 5, 3, 19, 9, 12, 17, 6};
        char bb4Table[] = BB4_TABLE_LITERAL;
        tm_init(&tm);
        tm.states = 4;
        tm_load_table(&tm, bb4Table);
        tm_extract_digits_from_table(&tm, bb4Digits1);

        for(int i=0;i<8;i++){
            printf("%d ", bb4Digits1[i]);
        }
        printf("\n");
        tm_print_table_short(&tm);

        for(int i=0;i<8;i++){
            unittest_assert_int_equals(&unitstate,
                bb4Digits1[i], bb4DigitsExpected1[i]);
        }
        tm_destroy(&tm);
        unittest_finish(&unitstate);
    }



    {
        unittest_begin(&unitstate, "bb4 digits->index", testopt);
        const int bb4Digits[] = {11, 5, 3, 19, 9, 12, 17, 6};
        mpz_t index; mpz_init(index);
        tm_get_table_index_from_digits(4, bb4Digits, index);
        unittest_assert_int_equals(&unitstate, mpz_get_ui(index), 8807993311);
        tm_destroy(&tm);
        mpz_clear(index);
        unittest_finish(&unitstate);
    }


    {
        unittest_begin(&unitstate, "bb4 index->digits", testopt);
        const int bb4DigitsExpected[] = {11, 5, 3, 19, 9, 12, 17, 6};
        mpz_t index; mpz_init_set_ui(index, 8807993311);

        int bb4Digits[8] = {0};

        tm_init(&tm);
        tm.states = 4;
        tm_extract_digits_from_index(&tm, bb4Digits, index);

        // int i=0;
        // printf("resultDigits: %d %d %d %d %d %d %d %d\n", bb4Digits[i++],bb4Digits[i++],bb4Digits[i++],bb4Digits[i++],bb4Digits[i++],bb4Digits[i++],bb4Digits[i++],bb4Digits[i++]);
        // i=0;
        // printf("expected: %d %d %d %d %d %d %d %d\n", bb4DigitsExpected[i++],bb4DigitsExpected[i++],bb4DigitsExpected[i++],bb4DigitsExpected[i++],bb4DigitsExpected[i++],bb4DigitsExpected[i++],bb4DigitsExpected[i++],bb4DigitsExpected[i++]);

        for(int i=0;i<8;i++){
            unittest_assert_int_equals(&unitstate,
                bb4Digits[i], bb4DigitsExpected[i]);
        }
        tm_destroy(&tm);
        mpz_clear(index);
        unittest_finish(&unitstate);
    }

    {
        unittest_begin(&unitstate, "bb4 digits->table", testopt);
        const int bb4Digits[] = {11, 5, 3, 19, 9, 12, 17, 6};

        tm_init(&tm);
        tm.states = 4;

        tm_load_table_from_digits(&tm, bb4Digits);

        // int steps = tm_step_until_halt_or_max(&tm, runopt_9999_nocheck);
        mpz_t steps; mpz_init(steps);
        tm_step_until_halt_or_max(&tm, runopt_9999_nocheck, &steps);
        // printf("steps: %d\n", steps);
        unittest_assert_int_equals(&unitstate, mpz_get_ui(steps), 107);
        tm_destroy(&tm);
        mpz_clear(steps);
        unittest_finish(&unitstate);
    }

    {
        unittest_begin(&unitstate, "2 states: table incrementing == index incrementing", testopt);
        // int state2Digits[4] = {0};

        tm_t tm2;
        tm_init(&tm2);
        tm2.states = 2;

        tm_init(&tm);
        tm.states = 2;

        mpz_t temp; mpz_init(temp);
        tm_max_num_of_machines(tm.states, temp);
        int maxNumberMachine = mpz_get_ui(temp);
        for(int i=0;i<maxNumberMachine;i++){
            // printf("i %d\n", i);
            mpz_set_ui(temp, i);
            tm_load_table_by_index(&tm2, temp);

            // int digits[256] = {0};
            // tm_extract_digits_from_table(&tm2, digits);
            // // tm_extract_digits_from_index(&tm2, digits, temp);
            // // tm_load_table_from_digits(&tm2, digits);
            // printf("indexed digits %d %d %d %d\n", digits[0],digits[1],digits[2],digits[3]);
            // tm_print_table_short(&tm2);
            
            // printf("\n");
            // tm_extract_digits_from_table(&tm, digits);
            // printf("incremented digits %d %d %d %d\n", digits[0],digits[1],digits[2],digits[3]);
            // tm_print_table_short(&tm);
            // printf("\n\n");

            //assert equivalent tm2 table and tm table.
            unittest_assert_true(&unitstate, tm_eq_tables(&tm, &tm2));

            if(!unitstate.passing){
                gmp_printf("failed i=%d, temp=%Zd\n", i, temp);
                tm_print_table_short(&tm);
                tm_print_table_short(&tm2);
                break;
            }

            tm_next_table_lexico(&tm);
        }
        
        mpz_clear(temp);
        tm_destroy(&tm2);
        tm_destroy(&tm);
        unittest_finish(&unitstate);
    }

    {
        unittest_begin(&unitstate, "1 states: table incrementing == index incrementing", testopt);
        // int state2Digits[4] = {0};

        tm_t tm2;
        tm_init(&tm2);
        tm2.states = 1;

        tm_init(&tm);
        tm.states = 1;

        mpz_t temp; mpz_init(temp);
        tm_max_num_of_machines(tm.states, temp);
        int maxNumberMachine = mpz_get_ui(temp);
        for(int i=0;i<maxNumberMachine;i++){
            // printf("i %d\n", i);
            mpz_set_ui(temp, i);
            tm_load_table_by_index(&tm2, temp);

            // int digits[256] = {0};
            // tm_extract_digits_from_table(&tm2, digits);
            // // tm_extract_digits_from_index(&tm2, digits, temp);
            // // tm_load_table_from_digits(&tm2, digits);
            // printf("indexed digits %d %d %d %d\n", digits[0],digits[1],digits[2],digits[3]);
            // tm_print_table_short(&tm2);
            
            // printf("\n");
            // tm_extract_digits_from_table(&tm, digits);
            // printf("incremented digits %d %d %d %d\n", digits[0],digits[1],digits[2],digits[3]);
            // tm_print_table_short(&tm);
            // printf("\n\n");

            //assert equivalent tm2 table and tm table.
            unittest_assert_true(&unitstate, tm_eq_tables(&tm, &tm2));

            if(!unitstate.passing){
                gmp_printf("failed i=%d, temp=%Zd\n", i, temp);
                tm_print_table_short(&tm);
                tm_print_table_short(&tm2);
                break;
            }

            tm_next_table_lexico(&tm);
        }
        
        mpz_clear(temp);
        tm_destroy(&tm2);
        tm_destroy(&tm);
        unittest_finish(&unitstate);
    }
}

void test_turing_enumerate(test_opt_t* testopt)
{
    printf("    Turing Enumeration Tests\n");
    
    {
        //hashmap enumeration test.

    }

}

/////////////////////////////////////////////

void test_opt_init(test_opt_t* testopt)
{
    testopt->onlyPrintFailingTests = true;
}

void unittest_begin(unittest_state_t* state, char* name, test_opt_t* testopt)
{
    state->passing = true;
    state->testname = name;
    state->opt = testopt;
}
void unittest_finish(unittest_state_t* state)
{
    if(state->passing == true){
        if(state->opt->onlyPrintFailingTests == false)
            printf("        PASS '%s'\n", state->testname);
    }else{
        printf("        FAILED '%s'\n", state->testname);
    }
}

void unittest_assert_int_equals(unittest_state_t* state, long a, long b)
{
    state->passing = state->passing && a == b;
}

void unittest_assert_true(unittest_state_t* state, bool a)
{
    state->passing = state->passing && a == true;
}