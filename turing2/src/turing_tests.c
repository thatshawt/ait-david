#include "turing_tests.h"
#include "turing_sim.h"
#include "turing_mapping.h"

#include "turing_utils.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

unittest_state_t unitstate;
tm_t tm;

void test_all(test_opt_t* testopt)
{
    test_turing_sim(testopt);
    test_turing_mapping(testopt);
}

tm_run_opt_t runopt_9999_nocheck = (tm_run_opt_t){
        .trivialNonhaltingCheck=false,
        .max_steps=9999999999L
    };

void test_turing_sim(test_opt_t* testopt)
{
    printf("Turing Sim Tests\n");

    
    {
        unittest_begin(&unitstate, "bb5 load&run", testopt);
        char bb5_table[] = BB5_TABLE_LITERAL;

        tm_init(&tm);
        tm.states = 5;
        tm_load_table(&tm, bb5_table);
        uint64_t steps = tm_step_until_halt_or_max(&tm, runopt_9999_nocheck);
        unittest_assert_int_equals(&unitstate, steps, 47176870);
        unittest_assert_int_equals(&unitstate, tm_get_written_tape_size(&tm), 12289);
        unittest_assert_int_equals(&unitstate, tm_count_written_symbol(&tm,1), 4098);

        unittest_finish(&unitstate);
    }


    {
        unittest_begin(&unitstate, "bb4 load&run", testopt);

        char bb4_table[] = BB4_TABLE_LITERAL;

        tm_init(&tm);
        tm.states = 4;
        tm_load_table(&tm, bb4_table);
        uint64_t steps = tm_step_until_halt_or_max(&tm, runopt_9999_nocheck);
        unittest_assert_int_equals(&unitstate, steps, 107);
        unittest_assert_int_equals(&unitstate, tm_get_written_tape_size(&tm), 14);
        unittest_assert_int_equals(&unitstate, tm_count_written_symbol(&tm,1), 13);

        unittest_finish(&unitstate);
    }

    {
        unittest_begin(&unitstate, "random tape test", testopt);

        tm_init(&tm);
        tm_fill_tape_with_random(&tm, 1337);
        int ones = tm_count_symbol_entire_tape(&tm, 1);
        // printf("ones %d\n", ones);
        unittest_assert_int_equals(&unitstate, ones, 19834);

        unittest_finish(&unitstate);
    }

    {
        unittest_begin(&unitstate, "counting 3 state machines bruteforce", testopt);

        tm_init(&tm);
        tm.states = 3;
        int i = 1;
        while(tm_next_table_lexico(&tm) == false){
            i++;
        }
        // printf("%d states, %d diff tables\n", tm.states, i);
        unittest_assert_int_equals(&unitstate, i, 16777216);

        unittest_finish(&unitstate);
    }

    {
        unittest_begin(&unitstate, "number of 3 state machines", testopt);

        tm_init(&tm);
        tm.states = 3;
        int number = tm_max_num_of_machines(tm.states);
        unittest_assert_int_equals(&unitstate, number, 16777216);

        unittest_finish(&unitstate);
    }
}

void test_turing_mapping(test_opt_t* testopt)
{
    printf("Turing Mapping Tests\n");

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
            unittest_assert_int_equals(&unitstate,
                bb4Digits1[i], bb4DigitsExpected1[i]);
        }
        unittest_finish(&unitstate);
    }



    {
        unittest_begin(&unitstate, "bb4 digits->index", testopt);
        const int bb4Digits[] = {11, 5, 3, 19, 9, 12, 17, 6};
        tm_index_t index = tm_get_table_index_from_digits(4, bb4Digits);
        unittest_assert_int_equals(&unitstate, (uint32_t)index, (uint32_t)8807993311);
        unittest_finish(&unitstate);
    }


    {
        unittest_begin(&unitstate, "bb4 index->digits", testopt);
        const int bb4DigitsExpected[] = {11, 5, 3, 19, 9, 12, 17, 6};
        tm_index_t index = 8807993311;

        int bb4Digits[8] = {0};

        tm_init(&tm);
        tm.states = 4;
        tm_extract_digits_from_index(&tm, bb4Digits, index);

        for(int i=0;i<8;i++){
            unittest_assert_int_equals(&unitstate,
                bb4Digits[i], bb4DigitsExpected[i]);
        }

        unittest_finish(&unitstate);
    }

    {
        unittest_begin(&unitstate, "bb4 digits->table", testopt);
        const int bb4Digits[] = {11, 5, 3, 19, 9, 12, 17, 6};

        tm_init(&tm);
        tm.states = 4;

        tm_load_table_from_digits(&tm, bb4Digits);

        int steps = tm_step_until_halt_or_max(&tm, runopt_9999_nocheck);

        unittest_assert_int_equals(&unitstate, steps, 107);

        unittest_finish(&unitstate);
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
            printf("Test '%s' PASSED\n", state->testname);
    }else{
        printf("Test '%s' FAILED\n", state->testname);
    }
}

void unittest_assert_int_equals(unittest_state_t* state, int a, int b)
{
    state->passing = state->passing && a == b;
}
void unittest_assert_true(unittest_state_t* state, bool a)
{
    state->passing = state->passing && a == true;
}