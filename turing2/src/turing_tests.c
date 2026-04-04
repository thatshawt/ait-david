#include "turing_tests.h"
#include "turing_sim.h"
#include "turing_mapping.h"

#include "turing_utils.h"

#include "gmp_mpzstr.h"
#include "mpz_helpers.h"
#include "monotone_tm.h"
#include "gmp_mpzstr.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define MIN(a,b) ((a)<(b) ? (a):(b))
#define MAX(a,b) ((a)>(b) ? (a):(b))

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
    
    // test_turing_sim(testopt);
    // test_turing_mapping(testopt);
    // test_turing_enumerate(testopt);
    test_monotone_tm(testopt);
    test_mpz_helpers(testopt);
    test_mpzstring(testopt);

    mpz_clears(runopt_9999_nocheck.max_steps,NULL);

    printf("Testing complete.\n\n");
}

void test_mpzstring(test_opt_t* testopt)
{
    printf("    MpzString Tests\n");
    {
        unittest_begin(&unitstate, "dummy", testopt);
        unittest_finish(&unitstate);
    }

    {
        unittest_begin(&unitstate, "(mpz 1234) == (mpz 1234 -> base 10 mpzstring 1234 -> mpz 1234)", testopt);

        mpz_t temp1; mpz_init(temp1);
        mpz_t* mpzstr = mpzstr_init_malloc(4);

        mpzstr_set_ints_right(mpzstr, (int*)(int[]){1,2,3,4}, 4);

        mpz_set_mpzstr(temp1, mpzstr, 10);

        char buff[12] = {0};
        char testStr[] = "1, 2, 3, 4";

        mpzstr_set_zero(mpzstr);

        mpz_get_mpzstr(mpzstr, 10, temp1);
        
        mpzstr_sprint(buff, mpzstr);

        int strDiff = strcmp(buff, testStr);
        // printf("str '%s', diff id %d\n", buff,strDiff);

        unittest_assert_true(&unitstate, strDiff == 0);

        mpzstr_clear_free(mpzstr);
        mpz_clear(temp1);

        unittest_finish(&unitstate);
    }

    {
        unittest_begin(&unitstate, "(mpzstr base 10 1234 -> mpz) == (mpzstr base 2 10011010010 -> mpz)", testopt);

        mpz_t temp1; mpz_init(temp1);
        mpz_t* mpzstr1 = mpzstr_init_malloc(11);

        mpzstr_set_ints_right(mpzstr1, (int*)(int[]){1,2,3,4}, 4);
        mpz_set_mpzstr(temp1, mpzstr1, 10);
        int mpz1ui = mpz_get_ui(temp1);
        // mpzstr_print(mpzstr1);
        // printf(". got %d\n", mpz1ui);

        mpzstr_set_ints_right(mpzstr1, (int*)(int[]){1,0,0,1,1,0,1,0,0,1,0}, 11);
        mpz_set_mpzstr(temp1, mpzstr1, 2);
        int mpz2ui = mpz_get_ui(temp1);
        // mpzstr_print(mpzstr1);
        // printf(". got %d\n", mpz2ui);

        unittest_assert_true(&unitstate, mpz1ui == mpz2ui);

        mpzstr_clear_free(mpzstr1);
        mpz_clear(temp1);

        unittest_finish(&unitstate);
    }


    {
        unittest_begin(&unitstate, "(mpzstr 1,2,3,0 base 10 -> mpz + 401) == (mpzstr 1,2,3,401 base 10 -> mpz) == (basenorm(mpzstr 1,2,3,401 base 10) -> mpz)", testopt);

        mpz_t temp1; mpz_init(temp1);
        mpz_t* mpzstr1 = mpzstr_init_malloc(4);

        mpzstr_set_ints_right(mpzstr1, (int*)(int[]){1,2,3,0}, 4);
        mpz_set_mpzstr(temp1, mpzstr1, 10);
        int mpz1ui = mpz_get_ui(temp1) + 401;
        // mpzstr_print(mpzstr1);
        // printf(". got %d\n", mpz1ui);

        mpzstr_set_ints_right(mpzstr1, (int*)(int[]){1,2,3,401}, 4);
        mpz_set_mpzstr(temp1, mpzstr1, 10);
        int mpz2ui = mpz_get_ui(temp1);
        // mpzstr_print(mpzstr1);
        // printf(". got %d\n", mpz2ui);

        mpzstr_set_ints_right(mpzstr1, (int*)(int[]){1,2,3,401}, 4);
        mpzstr_basenorm(mpzstr1, 4, 10, NULL);
        mpz_set_mpzstr(temp1, mpzstr1, 10);
        int mpz3ui = mpz_get_ui(temp1);
        // mpzstr_print(mpzstr1);
        // printf(". got %d\n", mpz2ui);

        unittest_assert_true(&unitstate, mpz1ui == mpz2ui && mpz1ui == mpz3ui);

        mpzstr_clear_free(mpzstr1);
        mpz_clear(temp1);

        unittest_finish(&unitstate);
    }
}

bool testDebugMode = false;

void test_monotone_tm(test_opt_t* testopt)
{
    printf("    Monotone Turing Machine Tests\n");
    // dummy
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

        sprintf(testTitle, "mtm entry increment == mtm entry from digit++ == get_digit (%d max states, %d max worktapes), and did it reach max entry digit.", statesMax, worktapesMax);

        unittest_begin(&unitstate, testTitle, testopt);

        for(int states=1; states<=statesMax; states++){
            for(int worktapes=1; worktapes<=worktapesMax; worktapes++){
                // check if incrementing follows the from digit thing
                
                
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

                // unittest_finish(&unitstate);
                
                // check if it went to the max digit
                // sprintf(testTitle, "    did it reach max digit?");
                // unittest_begin(&unitstate, testTitle, testopt);
                
                mpz_get_entry_max_digit(temp, states, worktapes);

                unittest_assert_true(&unitstate, mpz_cmp(temp, counter) == 0);

                
            }
        }

        unittest_finish(&unitstate);

        
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

    // 1 state 1 worktape "100001" input tape, mtm get code -> load code, until overflow
    {
        unittest_begin(&unitstate, "(mtm -> code -> mtm) == (mtm), 1 state 1 worktape '100001' input tape, from init machine to overflow machine", testopt);

        mtm_t mtm1, mtm2;
        mtm_init(&mtm1, 1, 1);
        mtm_init(&mtm2, 1, 1);

        mtm_tape_load_str(&mtm1.inputTape, "100001");

        mpz_t mtmCode; mpz_init(mtmCode);

        // mtm_print(&mtm);
        char poopooBuffer[1000] = {0};

        // stops at i = 1048575
        for(int i=0; i<1048575 ;i++){
            break; //TODO: remove this to enable the test
            if(i == 65535)testDebugMode = true;
            // printf("on i %d\n", i);
            
            int mtmcodebits = mtm_get_code(&mtm1, "", mtmCode);
            
            // mpz_add_ui(mtmCode, mtmCode, 1);
            int loadedbits = mtm_load_from_code(&mtm2, mtmCode);

            bool mtmEquals = mtm_equals(&mtm1, &mtm2);
            unittest_assert_true(&unitstate, mtmEquals);

            // gmp_printf("\n%d bits mtmCode %Zd, base62: %s\n", mtmcodebits, mtmCode, poopooBuffer);
            // mtm_print(&mtm1);
            // mtm_print(&mtm2);

            // if(i == 65535){
            //     testDebugMode = true;
            //     printf("one before fail %d\n", i);
            //     mtm_print(&mtm1);
            //     mpz_get_str(poopooBuffer, 62, mtmCode);
            //     gmp_printf("\n%d bits mtmCode %Zd, %d loaded, base62: %s\n", mtmcodebits, mtmCode, loadedbits, poopooBuffer);
            //     mtm_print(&mtm2);
            // }
            
            if(!mtmEquals || (i == -1)){ //65535
                printf("failed at i %d\n", i);
                mtm_print(&mtm1);
                mpz_get_str(poopooBuffer, 62, mtmCode);
                gmp_printf("\n%d bits mtmCode %Zd, %d loaded, base62: %s\n", mtmcodebits, mtmCode, loadedbits, poopooBuffer);
                mtm_print(&mtm2);
                break;
            }

            if(mtm_table_increment(&mtm1.table)){
                // printf("finished at %d\n", i);
                break;
            }
            testDebugMode = false;
        }
        testDebugMode = false;
        
        mpz_clears(mtmCode, NULL);
        mtm_destroy(&mtm1);
        mtm_destroy(&mtm2);

        unittest_finish(&unitstate);
    }

    // various tapes -> code -> back to tape
    {
        unittest_begin(&unitstate, "tape equality code validation", testopt);

        int thing = 0;

        mtm_tape_t tape1, tape2;
        mtm_tape_init(&tape1);
        mtm_tape_init(&tape2);

        mpz_t tapeCode; mpz_init(tapeCode);

        char buff[100] = {0};
        
        sprintf(buff, "");

        TAPE_CHECK:
        mtm_tape_reset(&tape1);
        mtm_tape_reset(&tape2);

        mtm_tape_load_str(&tape1, buff);
        mtm_tape_get_code(&tape1, tapeCode);
        mtm_tape_load_from_code(&tape2, tapeCode);

        bool tapeequals = mtm_tape_equals(&tape1, &tape2);

        unittest_assert_true(&unitstate, tapeequals);

        if(!tapeequals){
            printf("tape not equal buff:%s,\n", buff);
            mtm_tape_print(&tape1);
            mtm_tape_print(&tape2);
        }

        memset(buff, 0, 100);
        switch(thing++){
            case 0: sprintf(buff, "0"); goto TAPE_CHECK;
            case 1: sprintf(buff, "1"); goto TAPE_CHECK;
            case 2: sprintf(buff, "01"); goto TAPE_CHECK;
            case 3: sprintf(buff, "10"); goto TAPE_CHECK;
            case 4: sprintf(buff, "11"); goto TAPE_CHECK;
            case 5: sprintf(buff, "110101000101010"); goto TAPE_CHECK;
            case 6: sprintf(buff, "11001010101010100101101010"); goto TAPE_CHECK;
            default: goto END;
        }

        END:

        mtm_tape_destroy(&tape1);
        mtm_tape_destroy(&tape2);

        mpz_clear(tapeCode);

        unittest_finish(&unitstate);
    }

    // tape move write test
    {
        unittest_begin(&unitstate, "tape move write test", testopt);

        mtm_tape_t tape;
        mtm_tape_init(&tape);

        // mtm_tape_print(&tape);//2
        // printf("under head: %d\n\n", mtm_tape_read(&tape));

        mtm_tape_move_right(&tape);
        // mtm_tape_print(&tape);//4
        // printf("under head: %d\n\n", mtm_tape_read(&tape));

        mtm_tape_write(&tape, 1);
        mtm_tape_move_left(&tape);
        // mtm_tape_print(&tape);//5
        // printf("under head: %d\n\n", mtm_tape_read(&tape));

        mtm_tape_move_left(&tape);
        mtm_tape_write(&tape, 1);
        // mtm_tape_print(&tape);//13
        // printf("under head: %d\n\n", mtm_tape_read(&tape));

        mtm_tape_move_left(&tape);
        // mtm_tape_print(&tape);//21
        // printf("under head: %d\n\n", mtm_tape_read(&tape));

        mtm_tape_move_left(&tape);
        // mtm_tape_print(&tape);//37
        // printf("under head: %d\n\n", mtm_tape_read(&tape));

        for(int i=0;i<6;i++)mtm_tape_move_right(&tape);
        // mtm_tape_print(&tape);//148
        // printf("under head: %d\n\n", mtm_tape_read(&tape));

        unittest_assert_true(&unitstate, mpz_cmp_ui(tape.tapeMemory, 148) == 0);

        mtm_tape_destroy(&tape);

        unittest_finish(&unitstate);
    }


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
        unittest_begin(&unitstate, "(Cantor Pair == Cantor Unpair) from (0,0) to (99,99)", testopt);

        mpz_t* poopooooos = mpzstr_init_malloc(2);
        mpz_t* pooo1 = (poopooooos+0);
        mpz_t* pooo2 = (poopooooos+1);
        mpz_t z; mpz_init(z);

        for(int a=0;a<n;a++){
            for(int b=0;b<n;b++){
                mpz_set_ui(*pooo1, a);
                mpz_set_ui(*pooo2, b);

                mpz_cantor_pair(z, *pooo1, *pooo2);
                // gmp_printf("pair   (%Zd,%Zd) -> %Zd\n", *pooo1,*pooo2,z);

                mpz_cantor_unpair(*pooo1, *pooo2, z);
                // gmp_printf("unpair (%Zd,%Zd) <- %Zd\n\n", *pooo1,*pooo2,z);

                unittest_assert_true(&unitstate, mpz_get_ui(*pooo1) == a & mpz_get_ui(*pooo2) == b);
            }
        }
        mpzstr_clear_free(poopooooos);
        mpz_clear(z);

        unittest_finish(&unitstate);
    }

    // cantor ui ui pair == unpair
    {
        unittest_begin(&unitstate, "cantor ui ui pair == unpair from 0,0 to 99,99", testopt);
        const int aMax = 99;
        const int bMax = 99;

        int maxZ,maxA,maxB = -1;
        for(int a=0; a<=aMax; a++){for(int b=0; b<=bMax; b++){
            unsigned long long z = mpz_cantor_pair_ui_ui(a, b);
            unsigned long long rx, ry = -1;
            mpz_cantor_unpair_ui_ui(&rx, &ry, z);

            unittest_assert_true(&unitstate, a == rx && b == ry);

            if(z > maxZ){
                maxZ = z;
                maxA = a;
                maxB = b;
            }
            // if(ry==rx)
            // printf("(%d,%d) = %d\n", rx, ry, z);
        }}

        // printf("(%d,%d) = %d\n", maxA, maxB, maxZ);

        unittest_finish(&unitstate);
    }

    // mpzstr cantor pair test
    {
        unittest_begin(&unitstate, "Cantor MpzStr (Pair == Unpair) from (0,0,0) to (49,49,49)", testopt);

        int n = 50;
        mpz_t* poopooooos = mpzstr_init_malloc(3);
        mpz_t* pooo1 = (poopooooos+0);
        mpz_t* pooo2 = (poopooooos+1);
        mpz_t* pooo3 = (poopooooos+2);
        mpz_t z; mpz_init(z);

        int i = 0;
        for(int a=0;a<n;a++){
        for(int b=0;b<n;b++){
        for(int c=0;c<n;c++){
            mpz_set_ui(*pooo1, a);
            mpz_set_ui(*pooo2, b);
            mpz_set_ui(*pooo3, c);

            mpz_cantor_pair_mpzstr(z, poopooooos);
            // gmp_printf("pair   (%Zd,%Zd,%Zd) -> %Zd\n", *pooo1,*pooo2,*pooo3, z);

            mpz_cantor_unpair_mpzstr(poopooooos, z, 3);
            // gmp_printf("unpair (%Zd,%Zd,%Zd) <- %Zd\n\n", *pooo1,*pooo2,*pooo3, z);

            // gmp_printf("(%d, %Zd),", i++, z);

            unittest_assert_true(&unitstate, mpz_get_ui(*pooo1) == a && mpz_get_ui(*pooo2) == b && mpz_get_ui(*pooo3) == c);
        }
        }
        }
        // printf("\n");

        mpzstr_clear_free(poopooooos);
        mpz_clear(z);

        unittest_finish(&unitstate);
    }

    // elegant Pair == elegant Unpair
    {
        const int n = 100;
        unittest_begin(&unitstate, "(elegant Pair == elegant Unpair) from (0,0) to (99,99) && (4,4) == 24", testopt);

        mpz_t* poopooooos = mpzstr_init_malloc(2);
        mpz_t* pooo1 = (poopooooos+0);
        mpz_t* pooo2 = (poopooooos+1);
        mpz_t z; mpz_init(z);

        for(int a=0;a<n;a++){
            for(int b=0;b<n;b++){
                mpz_set_ui(*pooo1, a);
                mpz_set_ui(*pooo2, b);

                mpz_elegant_pair(z, *pooo1, *pooo2);
                // gmp_printf("pair   (%Zd,%Zd) -> %Zd\n", *pooo1,*pooo2,z);

                mpz_elegant_unpair(*pooo1, *pooo2, z);
                // gmp_printf("unpair (%Zd,%Zd) <- %Zd\n\n", *pooo1,*pooo2,z);

                unittest_assert_true(&unitstate, mpz_get_ui(*pooo1) == a & mpz_get_ui(*pooo2) == b);

                if(a == 4 && b == 4){
                    unittest_assert_true(&unitstate, mpz_get_ui(z) == 24);
                }
            }
        }
        mpzstr_clear_free(poopooooos);
        mpz_clear(z);

        unittest_finish(&unitstate);
    }

    // elegant ui ui pair == unpair
    {
        unittest_begin(&unitstate, "elegant ui ui pair == unpair from 0,0 to 99,99", testopt);
        const int aMax = 99;
        const int bMax = 99;

        int maxZ,maxA,maxB = -1;
        for(int a=0; a<=aMax; a++){for(int b=0; b<=bMax; b++){
            unsigned long long z = mpz_elegant_pair_ui_ui(a, b);
            unsigned long long rx, ry = -1;
            mpz_elegant_unpair_ui_ui(&rx, &ry, z);

            unittest_assert_true(&unitstate, a == rx && b == ry);

            if(z > maxZ){
                maxZ = z;
                maxA = a;
                maxB = b;
            }
            // if(ry==rx)
            // printf("(%d,%d) = %d\n", rx, ry, z);
        }}

        // printf("(%d,%d) = %d\n", maxA, maxB, maxZ);

        unittest_finish(&unitstate);
    }

    // mpzstr elegant pair test
    {
        unittest_begin(&unitstate, "elegant MpzStr (Pair == Unpair) from (0,0,0) to (49,49,49)", testopt);

        int n = 50;
        mpz_t* poopooooos = mpzstr_init_malloc(3);
        mpz_t* pooo1 = (poopooooos+0);
        mpz_t* pooo2 = (poopooooos+1);
        mpz_t* pooo3 = (poopooooos+2);
        mpz_t z; mpz_init(z);

        int i = 0;
        for(int a=0;a<n;a++){
        for(int b=0;b<n;b++){
        for(int c=0;c<n;c++){
            mpz_set_ui(*pooo1, a);
            mpz_set_ui(*pooo2, b);
            mpz_set_ui(*pooo3, c);

            mpz_elegant_pair_mpzstr(z, poopooooos);
            // if(a==0 && b==0)gmp_printf("pair   (%Zd,%Zd,%Zd) -> %Zd\n", *pooo1,*pooo2,*pooo3, z);

            mpz_elegant_unpair_mpzstr(poopooooos, z, 3);
            // if(a==0 && b==0)gmp_printf("unpair (%Zd,%Zd,%Zd) <- %Zd\n\n", *pooo1,*pooo2,*pooo3, z);

            // gmp_printf("(%d, %Zd),", i++, z);

            unittest_assert_true(&unitstate, mpz_get_ui(*pooo1) == a && mpz_get_ui(*pooo2) == b && mpz_get_ui(*pooo3) == c);
        }
        }
        }
        // printf("\n");

        mpzstr_clear_free(poopooooos);
        mpz_clear(z);

        unittest_finish(&unitstate);
    }

    // big encoding test of bar encoding and apos encoding
    {
        unittest_begin(&unitstate,
            "Validate bar/apos encode/decode from x=0 to x=10000: "
            "(x -> length,int -> x) == (x)"
            " && (bar x -> length,int) == (x -> length,int)"
            " && (apos x -> length,int) == (x -> length,int)"
            " && (apos x -> x) == (x)"
            , testopt);

        mpz_t bar_encoded; mpz_init(bar_encoded);
        mpz_t apos_encoded; mpz_init(apos_encoded);
        mpz_t x; mpz_init(x);
        mpz_t xFromApos; mpz_init(xFromApos);
        mpz_t bitlength; mpz_init(bitlength);
        mpz_t bitinteger; mpz_init(bitinteger);
        mpz_t xFromIntAndLength; mpz_init(xFromIntAndLength);

        mpz_t prefixXFromAposX; mpz_init(prefixXFromAposX);

        mpz_t decodedBarX; mpz_init(decodedBarX);
        mpz_t decodedAposX; mpz_init(decodedAposX);
        for(int i=0;i<10000;i++){
            mpz_set_ui(x,i);

            mpz_prefix_index_get_bit_length(x, bitlength);
            mpz_prefix_index_get_bit_integer(x, bitinteger);

            mpz_get_prefix_index_from_int_and_length(xFromIntAndLength, bitinteger, bitlength);

            // gmp_printf("(%Zd    -> length:%Zd, int:%Zd -> %Zd)\n",
            //         x, bitlength, bitinteger, xFromIntAndLength
            // );

            unittest_assert_true(&unitstate, mpz_cmp(x, xFromIntAndLength) == 0);

            mpz_bar_encode(bar_encoded, bitinteger, mpz_get_ui(bitlength));

            mpz_apos_encode_prefix_index(apos_encoded, x);

            int decodedBarLengthX;
            mpz_bar_decode_left(decodedBarX, &decodedBarLengthX, bar_encoded);

            // gmp_printf("(bar:%Zd -> length:%d, int:%Zd) \n",
            //     bar_encoded, decodedBarLengthX, decodedBarX
            // );

            unittest_assert_true(&unitstate, mpz_get_ui(bitlength) == decodedBarLengthX);
            unittest_assert_true(&unitstate, mpz_cmp(bitinteger, decodedBarX) == 0);

            int decodedAposLengthX;
            mpz_apos_decode_left(decodedAposX, &decodedAposLengthX, apos_encoded);

            mpz_apos_decode_prefix_index_left(xFromApos, apos_encoded);

            mpz_apos_decode_prefix_index_left(prefixXFromAposX, x);

            // gmp_printf(
            //     "(apos:%Zd -> length: %d, int:%Zd -> %Zd) (xprefixApos: %Zd)\n",
            //     apos_encoded, decodedAposLengthX, decodedAposX, xFromApos, prefixXFromAposX
            // );

            unittest_assert_true(&unitstate, mpz_get_ui(bitlength) == decodedAposLengthX);
            unittest_assert_true(&unitstate, mpz_cmp(bitinteger, decodedAposX) == 0);
            unittest_assert_true(&unitstate, mpz_cmp(x, xFromApos) == 0);

            // printf("\n");

        }
        mpz_clears(bitlength,bitinteger,x,bar_encoded,
            apos_encoded,decodedBarX,decodedAposX,xFromIntAndLength,xFromApos,
            prefixXFromAposX,
            NULL);

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