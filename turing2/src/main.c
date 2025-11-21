#include "turing_tests.h"
#include "turing_sim.h"
#include "turing_mapping.h"
#include <stdio.h>
#include <string.h>

int main(){
    printf("\nhello turing\n\n");

    tm_t tm;
    tm_t* tm_point = &tm;

    test_opt_t testOptions;
    testOptions.onlyPrintFailingTests = false;

    printf("Running all tests.\n");
    test_all(&testOptions);
    printf("Testing complete.\n");

    //TODO make test for testing table increment?
    // also increment by one or something idk bruv...
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