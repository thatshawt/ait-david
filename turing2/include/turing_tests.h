#ifndef TURING_TESTS_H
#define TURING_TESTS_H

#include <stdbool.h>

typedef struct{
    bool onlyPrintFailingTests;
} test_opt_t;

void test_opt_init(test_opt_t* testopt);

// void test_init();
void test_all(test_opt_t* testopt);
void test_turing_sim(test_opt_t* testopt);
void test_turing_mapping(test_opt_t* testopt);

typedef struct{
    bool passing;
    char* testname;
    test_opt_t* opt;
} unittest_state_t;
void unittest_begin(unittest_state_t* state, char* name, test_opt_t* testopt);
void unittest_finish(unittest_state_t* state);
void unittest_assert_int_equals(unittest_state_t* state, int a, int b);
void unittest_assert_true(unittest_state_t* state, bool a);


#endif