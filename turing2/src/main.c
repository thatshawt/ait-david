#include "turing_tests.h"
#include "turing_sim.h"
#include "turing_mapping.h"
#include "turing_utils.h"
#include "turing_enumerate.h"
#include "turing_jobs.h"
#include "cacache.h"

#include "hashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gmp.h>
#include <mpfr.h>

#include <unistd.h>
#include <sys/time.h>

#include "sqlenv.h"
#include "monotone_tm.h"
#include "gmp_mpzstr.h"

extern uint64_t current_timestamp();
// uint64_t current_timestamp() {
//     struct timeval te;
//     gettimeofday(&te, NULL); // get current time
//     long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
//     // printf("milliseconds: %lld\n", milliseconds);
//     return (uint64_t)milliseconds;
// }

int main(){

    // print various messages.
    // run tests.
    {
        printf("\nhello turing\n\n");
        printf("SQLITE_OK %d\n", SQLITE_OK);

        turing_threading_init_global();

        cacache_init();

        turing_threading_self_init();

        // test_opt_t testOptions;
        // testOptions.onlyPrintFailingTests = false;
        // test_all(&testOptions);

        // return 0;
    }

    // mpzstr testing

    int digits = 16;
    int baseConvert = 27;
    mpz_t* mpzstr = mpzstr_init_malloc(digits);
    // gmp_printf("%Zd\n", *mpzstr);
    // mpzstr_set_zero(mpzstr);
    
    mpz_set_si(*(mpzstr+(digits-4)), 3);
    mpz_set_si(*(mpzstr+(digits-3)), 5);
    mpz_set_si(*(mpzstr+(digits-2)), 10);
    mpz_set_si(*(mpzstr+(digits-1)), -100000);

    mpz_t shartPoop; mpz_init(shartPoop);

    mpzstr_basenorm(mpzstr+1, mpzstr_len(mpzstr), 7, &shartPoop);
    gmp_printf("carry %Zd, in base 7: ", shartPoop);
    mpzstr_print(mpzstr);


    mpz_set_mpzstr(shartPoop, mpzstr, 7);
    gmp_printf("as base 10: %Zd\n", shartPoop);

    mpzstr_set_zero(mpzstr);

    printf("as base 7: "); // what the sigma
    mpz_get_mpzstr(mpzstr, 7, shartPoop);
    mpzstr_print(mpzstr);
    mpz_t* mpzstr2 = mpzstr_init_malloc(mpzstr_len(mpzstr));
    mpzstr_copy(mpzstr2, mpzstr);

    printf("heres a copy i hope, ");
    mpzstr_print(mpzstr2);


    mpzstr_clear_free(mpzstr);
    mpzstr_clear_free(mpzstr2);
    mpz_clear(shartPoop);

    // return 1;

    // mtm testing

    int states = 2;
    int worktapes = 2;

    const int bitsi = mtm_get_entry_bits(states, worktapes);

    printf("entry bits %d\n", bitsi);
    printf("MPFR_PREC_MAX %ld\n", MPFR_PREC_MAX);

    mtm_transition_table_t table;
    mtm_table_init(&table, states, worktapes);
    mpz_t tableIndexHEEHEE; mpz_init(tableIndexHEEHEE);
    mpz_t temp123123; mpz_init(temp123123);

    // 2 states, 2 worktapes -> 153
    // 153 << (16*4), 16 cus each entry is 16 bits long when using 2 states 2 worktapes.
    // 4260087681 encodes 4 entries
    // mpz_set_ui(tableIndexHEEHEE, 153);
    // mpz_lshift(tableIndexHEEHEE, tableIndexHEEHEE, 16*4);
    // mpz_set_str(temp123123, "4260087681", 10);
    // mpz_add(tableIndexHEEHEE,tableIndexHEEHEE,temp123123);

    mpz_set_str(tableIndexHEEHEE, "661390083969", 10);

    int loadedBitsFromIndex123123 = mtm_load_table_from_index(&table, tableIndexHEEHEE);
    gmp_printf("loaded %d bits from table index %Zd\n",
        loadedBitsFromIndex123123, tableIndexHEEHEE);
    mtm_print_table_summary(&table);
    printf("\n");
    mtm_print_table(&table);

    mtm_table_increment(&table);
    mtm_print_table_summary(&table);
    printf("\n");
    mtm_print_table(&table);
    printf("\n");

    mtm_table_increment(&table);
    mtm_print_table_summary(&table);
    printf("\n");
    mtm_print_table(&table);
    printf("\n");

    mtm_table_increment(&table);
    mtm_print_table_summary(&table);
    printf("\n");
    mtm_print_table(&table);
    printf("\n");

    mtm_table_increment(&table);
    mtm_print_table_summary(&table);
    printf("\n");
    mtm_print_table(&table);
    printf("\n");

    mtm_table_increment(&table);
    mtm_print_table_summary(&table);
    printf("\n");
    mtm_print_table(&table);
    printf("\n");

    mtm_table_free(&table);

    mpz_clears(tableIndexHEEHEE, temp123123, NULL);

    return 1;

    int barencoded = 863;
    mpz_t thing1; mpz_init_set_ui(thing1, barencoded);
    mpz_t decodedX; mpz_init(decodedX);
    int lengthx;

    mpz_bar_decode_left_pop(decodedX, &lengthx, thing1);

    gmp_printf("bar encode %d -> x: %Zd, lengthX: %d, thingAfter: %Zd\n",
        barencoded, decodedX, lengthx, thing1);

    mpz_clears(thing1, decodedX, NULL);

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
    for(int i=0;i<25;i++){
        mpz_set_ui(x,i);

        mpz_prefix_index_get_bit_length(x, bitlength);
        mpz_prefix_index_get_bit_integer(x, bitinteger);

        mpz_get_prefix_index_from_int_and_length(xFromIntAndLength, bitinteger, bitlength);

        gmp_printf("(%Zd    -> length:%Zd, int:%Zd -> %Zd)\n",
                x, bitlength, bitinteger, xFromIntAndLength
        );

        mpz_bar_encode(bar_encoded, bitinteger, mpz_get_ui(bitlength));

        mpz_apos_encode_prefix_index(apos_encoded, x);

        int decodedBarLengthX;
        mpz_bar_decode_left(decodedBarX, &decodedBarLengthX, bar_encoded);

        gmp_printf("(bar:%Zd -> length:%d, int:%Zd) \n",
            bar_encoded, decodedBarLengthX, decodedBarX
        );

        int decodedAposLengthX;
        mpz_apos_decode_left(decodedAposX, &decodedAposLengthX, apos_encoded);

        mpz_apos_decode_prefix_index_left(xFromApos, apos_encoded);

        mpz_apos_decode_prefix_index_left(prefixXFromAposX, x);

        gmp_printf(
            "(apos:%Zd -> length: %d, int:%Zd -> %Zd) (xprefixApos: %Zd)\n",
            apos_encoded, decodedAposLengthX, decodedAposX, xFromApos, prefixXFromAposX
        );

        printf("\n");

    }
    mpz_clears(bitlength,bitinteger,x,bar_encoded,
        apos_encoded,decodedBarX,decodedAposX,xFromIntAndLength,xFromApos,
        prefixXFromAposX,
        NULL);

    printf("\n\n");



    mtm_table_init(&table, states, worktapes);
    mtm_table_zero(&table);

    mpz_t tableIndex; mpz_init(tableIndex);

    mpz_set_str(tableIndex, "36773560824", 10);
    mpz_t temp1; mpz_init(temp1);
    for(int i=0; i<10; i++){
        int loadedBitsFromIndex = mtm_load_table_from_index(&table, tableIndex);
        
        if(table.states < 1 || table.states > MTM_MAX_STATES
            || table.workTapes < 1 || table.workTapes > MTM_MAX_WORK_TAPES)continue;
            
        gmp_printf("table %Zd:\n", tableIndex);
        gmp_printf("entry bits for %Zd is %d\n", tableIndex, mtm_get_entry_bits(table.states, table.workTapes));

        printf(" %d bits from index\n", loadedBitsFromIndex);
        mtm_print_table_summary(&table);
        printf("\n");

        int tableGotLength = mtm_get_table_index(temp1, &table);

        gmp_printf(" (%d bits from get_index %Zd)\n", tableGotLength, temp1);

        mtm_print_table(&table);

        mpz_add_ui(tableIndex, tableIndex, 1);
        printf("\n");
    }

    mpz_clears(tableIndex, temp1, NULL);

    mtm_tape_t tape;
    mtm_tape_init(&tape);
    mpz_t tapeCode; mpz_init(tapeCode);

    mtm_tape_print(&tape);
    mtm_tape_get_code(&tape, tapeCode);
    gmp_printf("tape code %Zd\n", tapeCode);
    mtm_tape_load_from_code(&tape, tapeCode);
    printf("under head: %d\n\n", mtm_tape_read(&tape));

    mtm_tape_move_right(&tape);
    mtm_tape_print(&tape);
    mtm_tape_get_code(&tape, tapeCode);
    gmp_printf("tape code %Zd\n", tapeCode);
    mtm_tape_load_from_code(&tape, tapeCode);
    printf("under head: %d\n\n", mtm_tape_read(&tape));

    mtm_tape_write(&tape, 1);
    mtm_tape_move_left(&tape);
    mtm_tape_print(&tape);
    mtm_tape_get_code(&tape, tapeCode);
    gmp_printf("tape code %Zd\n", tapeCode);
    mtm_tape_load_from_code(&tape, tapeCode);
    printf("under head: %d\n\n", mtm_tape_read(&tape));

    mtm_tape_move_left(&tape);
    mtm_tape_write(&tape, 1);
    mtm_tape_print(&tape);
    mtm_tape_get_code(&tape, tapeCode);
    gmp_printf("tape code %Zd\n", tapeCode);
    mtm_tape_load_from_code(&tape, tapeCode);
    printf("under head: %d\n\n", mtm_tape_read(&tape));

    mtm_tape_move_left(&tape);
    mtm_tape_print(&tape);
    mtm_tape_get_code(&tape, tapeCode);
    gmp_printf("tape code %Zd\n", tapeCode);
    mtm_tape_load_from_code(&tape, tapeCode);
    printf("under head: %d\n\n", mtm_tape_read(&tape));

    mtm_tape_move_left(&tape);
    mtm_tape_print(&tape);
    mtm_tape_get_code(&tape, tapeCode);
    gmp_printf("tape code %Zd\n", tapeCode);
    mtm_tape_load_from_code(&tape, tapeCode);
    printf("under head: %d\n\n", mtm_tape_read(&tape));

    mtm_tape_destroy(&tape);
    mpz_clear(tapeCode);

    mtm_t mtm;
    mtm_init(&mtm, 3, 1);

    mtm.tempIndex.inputRead = 1;

    mtm_transition_entry_t* entryIdk = mtm_table_get_entry(&mtm.table, &mtm.tempIndex);
    entryIdk->inputTapeMove = 1;
    entryIdk->outputTapeWrite = 1;
    entryIdk->nextState = 3;

    // mtm_tape_load_str(&mtm.inputTape, "1000000000101");

    mpz_t mtmCode; mpz_init(mtmCode);
    mtm_print(&mtm);
    int mtmcodebits = mtm_get_code(&mtm, mtmCode);
    char poopooBuffer[1000] = {0};

    gmp_printf("\nbits %d, mtmCode %Zd\n", mtmcodebits, mtmCode);

    mtm_load_from_code(&mtm, mtmCode);
    mtmcodebits = mtm_get_code(&mtm, mtmCode);
    mpz_get_str(poopooBuffer, 62, mtmCode);
    gmp_printf("\nbits %d, mtmCode %Zd, base62: %s\n", mtmcodebits, mtmCode, poopooBuffer);
    mtm_print(&mtm);

    for(int i=0;i<5;i++){
        printf("on i %d\n", i);
        // mpz_add_ui(mtmCode, mtmCode, 1);
        // mtm_load_from_code(&mtm, mtmCode);
        mtm_table_increment(&mtm.table);
        mtmcodebits = mtm_get_code(&mtm, mtmCode);
        mpz_get_str(poopooBuffer, 62, mtmCode);
        gmp_printf("\n%d bits mtmCode %Zd, base62: %s\n", mtmcodebits, mtmCode, poopooBuffer);
        mtm_print(&mtm);
    }

    mpz_clears(mtmCode, NULL);
    mtm_destroy(&mtm);

    // return 1;

    // printf("entryindex increment test\n");
    // mtm_entry_index_t entryIndex;
    // mtm_entry_index_zero(&entryIndex);
    // do{
    //     mtm_print_entry_index(&entryIndex, worktapes);
    // }while(!mtm_entry_index_increment(&entryIndex, states, worktapes));

    printf("%d states, %d worktapes = %d bits\n", states, worktapes, mtm_get_entry_bits(states, worktapes));

    mpz_t number, bits;
    mpz_init(bits);
    mpz_init_set_ui(number, 1023);
    // inline void mpz_pop_nbits(mpz_t bits, mpz_t number, mp_bitcnt_t bitsN)
    gmp_printf("before: number %Zd, bits %Zd\n", number, bits);
    mpz_pop_nbits_right(bits, number, 5);
    gmp_printf("after: number %Zd, bits %Zd\n", number, bits);

    mpz_clears(number, bits, NULL);

    int counter = 0;
    mtm_transition_entry_t entry;
    mtm_transition_entry_t entry2;
    mtm_entry_zero(&entry);
    mpz_t digit; mpz_init(digit);
    mpz_t digit2; mpz_init(digit2);
    do{
        // mpz_get_entry_max_digit(digit, states, worktapes);
        // if(mpz_cmp_ui(digit, counter) == 0)break;

        mtm_entry_get_digit(digit, &entry, states, worktapes);

        mtm_entry_from_digit(&entry2, digit, states, worktapes);
        mtm_entry_get_digit(digit2, &entry2, states, worktapes);

        gmp_printf("counter: %d, entry digit: %Zd, digit2 %Zd\n", counter, digit, digit2);
        mtm_print_entry_short(&entry, worktapes);

        if(mpz_get_ui(digit) != counter || mpz_get_ui(digit2) != counter) return 1;
        // if(counter == 10)return 1;

        counter++;
    }while(!mtm_entry_increment(&entry, states, worktapes));

    printf("counted %d entries\n", counter);

    // performance test i suppose entry increment

    int testStates = 5;
    int testWorktapes = 5;

    printf("full enumeration test:\n");
    uint64_t startMilli = current_timestamp();
    uint64_t count = 0;
    mpz_t* poopoos = mpzstr_init_malloc(4);
    // mpz_t* poopoos = mpzstr_init2_malloc(4, 100);
    for(int i=0; i<500; i++){
        mtm_entry_zero(&entry);
        while(
            // !mtm_entry_increment_temps(&entry, testStates, testWorktapes,
            // *(poopoos), *(poopoos+1), *(poopoos+2), *(poopoos+3))
            !mtm_entry_increment_fast(&entry, testStates, testWorktapes)
            )
            {
                if(entry.inputTapeMove)count++;
            }
    }
    uint64_t endMilli = current_timestamp();
    printf("%ld counted, enumeration test done. took %f seconds\n", count, (float)(endMilli-startMilli)/((float)1000.0));

    mpzstr_clear_free(poopoos);

    mpz_clears(digit, digit2, NULL);



    return 0;







    // tm_print_enumerate_performance_stats(2,500);

    const int selfid = turing_threading_self_index();
    tm_srand(selfid, 1337);

    sqlenv_t sqlenv;
    if(sqlenv_open(&sqlenv, "artifacts/test.db", 0, 0))return 1;

    char statementBuffer[2000];

    tm_enumerate_options_simple_t enumerate_simple_opt = {
        .states = 2,
        .max_steps = "100",
        .startIndex = "0",
        .indexesConsidered = NULL,

        .randomStartSeed = "1",
        .randomIterations = "0",

        .doZerosTape = true,
        .doOnesTape = true,

        .workers = 4
    };

    tm_enumerate_options_simple_t enumerate_simple_opt_halfA = enumerate_simple_opt;
    enumerate_simple_opt_halfA.startIndex = "0";
    enumerate_simple_opt_halfA.indexesConsidered = "10369";

    tm_enumerate_options_simple_t enumerate_simple_opt_halfB = enumerate_simple_opt;
    enumerate_simple_opt_halfB.startIndex = "10369";
    enumerate_simple_opt_halfB.indexesConsidered = NULL;
    
    
    char *originalTableName = "NumbersCount";
    char *halfATableName = "NumbersCountHalfA";
    char *halfBTableName = "NumbersCountHalfB";

    // populate tables with enumeration results
    {
        // int states = 2;
        // TODO:
        // try to capture up to 99.999% of the theoretical number of predicted
        // halting machines. you can try sampling 20% of the machines randomly to
        // check how many are captured.
        
        sql_drop_table_if_exists(&sqlenv, statementBuffer, originalTableName);
        do_tm_enumerate_sql_merge_job_wrapped(enumerate_simple_opt,
            &sqlenv, statementBuffer, originalTableName
        );

        sql_drop_table_if_exists(&sqlenv, statementBuffer, halfATableName);
        do_tm_enumerate_sql_merge_job_wrapped(enumerate_simple_opt_halfA,
            &sqlenv, statementBuffer, halfATableName
        );

        sql_drop_table_if_exists(&sqlenv, statementBuffer, halfBTableName);
        do_tm_enumerate_sql_merge_job_wrapped(enumerate_simple_opt_halfB,
            &sqlenv, statementBuffer, halfBTableName
        );
    }

    // do stuff for original hashmap
    {
        printf("Summing all counts for original...\n");
        mpz_t zval; mpz_init_set_ui(zval, 0);

        sql_str_count_sum_all_count(&sqlenv, statementBuffer, originalTableName, &zval);

        gmp_printf("Total strings from sql: %Zd\n", zval);

        char *strID = "1";
        print_freq_sql_str_count_table_string(&sqlenv, statementBuffer, originalTableName, strID);

        mpz_clear(zval);
    }

    // do stuff for original hashmap half A
    {
        printf("Summing all counts...\n");
        mpz_t zval; mpz_init_set_ui(zval, 0);

        sql_str_count_sum_all_count(&sqlenv, statementBuffer, halfATableName, &zval);

        gmp_printf("Total strings from sql: %Zd\n", zval);

        char *strID = "1";
        print_freq_sql_str_count_table_string(&sqlenv, statementBuffer, halfATableName, strID);

        mpz_clear(zval);
    }

    // do stuff for original hashmap half B
    {
        printf("Summing all counts...\n");
        mpz_t zval; mpz_init_set_ui(zval, 0);

        sql_str_count_sum_all_count(&sqlenv, statementBuffer, halfBTableName, &zval);

        gmp_printf("Total strings from sql: %Zd\n", zval);

        char *strID = "1";
        print_freq_sql_str_count_table_string(&sqlenv, statementBuffer, halfBTableName, strID);

        mpz_clear(zval);
    }

    // merge B into A, then print stuff for A.
    {
        struct hashmap* slice_count_map_halfB = sql_str_count_get_map(&sqlenv, statementBuffer, halfBTableName);
        printf("\nmerging hashmap half B into sql half A...\n");
        sql_merge_slicecountmap_into_str_count_table(&sqlenv, statementBuffer, halfATableName, slice_count_map_halfB);

        hashmap_free(slice_count_map_halfB);

        printf("Summing all counts...\n");
        mpz_t zval; mpz_init_set_ui(zval, 0);
        sql_str_count_sum_all_count(&sqlenv, statementBuffer, halfATableName, &zval);
        gmp_printf("Total strings from sql: %Zd\n", zval);

        char *strID = "1";
        print_freq_sql_str_count_table_string(&sqlenv, statementBuffer, halfATableName, strID);

        mpz_clear(zval);

        // sprintf(statementBuffer, "SELECT * FROM %s;", halfATableName);
        // sqlenv_exec_with_callback(&sqlenv, statementBuffer, NULL, &sql_callback_print);
    }

    // some jobs tests
    {
        tj_DANGEROUS_delete_tables(&sqlenv);

        tj_create_tables(&sqlenv);

        //TODO make this split by into the children with a function by itself
        unsigned long jobIdOriginal = tj_create_job_simple_args(&sqlenv, enumerate_simple_opt);
        // sleep(1);
        unsigned long jobIdHalfA = tj_create_job_simple_args(&sqlenv, enumerate_simple_opt_halfA);
        // sleep(1);
        unsigned long jobIdHalfB = tj_create_job_simple_args(&sqlenv, enumerate_simple_opt_halfB);


        unsigned long childrenIds[] = { jobIdHalfA, jobIdHalfB };

        sqlenv_exec_with_callback(&sqlenv, "SELECT * from jobs;", NULL, &sql_callback_print);

        enumerate_job_opt_t enumOpt;
        tm_enumerate_job_opt_init(&enumOpt);

        // print it out cus yea
        tj_get_job_args(&sqlenv, jobIdOriginal, &enumOpt);
        tm_enumerate_print_opt(&enumOpt);

        tm_enumerate_job_opt_destroy(&enumOpt);

        // map parent jobs to children jobs first time
        tj_map_enumeration_to_children_jobs(&sqlenv, jobIdOriginal, 2, childrenIds);

        printf("jobId %ld has %d children.\n", jobIdOriginal, tj_number_of_children(&sqlenv, jobIdOriginal));
        printf("jobId %ld has %d children.\n", jobIdHalfA, tj_number_of_children(&sqlenv, jobIdHalfA));
        printf("jobId %ld has %d children.\n", jobIdHalfB, tj_number_of_children(&sqlenv, jobIdHalfB));

        printf("delete all job mappings\n");
        tj_delete_all_enumeration_mapping(&sqlenv, 1);

        
        printf("print all enum maps\n");
        sqlenv_exec_with_callback(&sqlenv, "SELECT * from enumeration_job_mapping;", NULL, &sql_callback_print);

        printf("print all jobs\n");
        sqlenv_exec_with_callback(&sqlenv, "SELECT * from jobs;", NULL, &sql_callback_print);


        // map parent jobs to children jobs second time? but zero it right after O.o
        tj_map_enumeration_to_children_jobs(&sqlenv, jobIdOriginal, 2, childrenIds);
        childrenIds[0] = 0;
        childrenIds[1] = 0;
        printf("childrenIds array: ");for(int i=0;i<2;i++){printf("%ld, ",childrenIds[i]);}printf("\n");
        int jobCountLoaded = 0;

        tj_get_enumeration_parents(&sqlenv, jobIdHalfA, &jobCountLoaded, childrenIds);

        printf("%d parent jobs of %ld: ", jobCountLoaded, jobIdHalfA);
        for(int i=0;i<jobCountLoaded;i++){printf("%ld, ",childrenIds[i]);}printf("\n");

        // mark some children has results
        tj_mark_job_completed_results(&sqlenv, jobIdHalfA);
        // tj_mark_job_completed_results(&sqlenv, jobIdHalfB);

        printf("print all merged_jobs\n");
        sqlenv_exec_with_callback(&sqlenv, "SELECT * from merged_jobs;", NULL, &sql_callback_print);

        // print merged children
        childrenIds[0] = -1;
        childrenIds[1] = -1;
        tj_get_children_with_results(&sqlenv, jobIdOriginal, &jobCountLoaded, childrenIds);
        printf("%d merged jobs into %d: ", jobCountLoaded, jobIdOriginal);
        for(int i=0;i<jobCountLoaded;i++){printf("%ld, ",childrenIds[i]);}printf("\n");

        // get unmerged children
        childrenIds[0] = -1;
        childrenIds[1] = -1;
        tj_get_children_without_results(&sqlenv, jobIdOriginal, &jobCountLoaded, childrenIds);
        printf("%d unmerged jobs into %d: ", jobCountLoaded, jobIdOriginal);
        for(int i=0;i<jobCountLoaded;i++){printf("%ld, ",childrenIds[i]);}printf("\n");

        printf("jobid %ld has %d merged children\n", jobIdOriginal, tj_number_children_with_results(&sqlenv, jobIdOriginal));
        printf("jobid %ld has %d merged children\n", jobIdHalfA, tj_number_children_with_results(&sqlenv, jobIdHalfA));
        printf("jobid %ld has %d merged children\n", jobIdHalfB, tj_number_children_with_results(&sqlenv, jobIdHalfB));

        printf("jobid %ld has %d unmerged children\n", jobIdOriginal, tj_number_children_without_results(&sqlenv, jobIdOriginal));
        printf("jobid %ld has %d unmerged children\n", jobIdHalfA, tj_number_children_without_results(&sqlenv, jobIdHalfA));
        printf("jobid %ld has %d unmerged children\n", jobIdHalfB, tj_number_children_without_results(&sqlenv, jobIdHalfB));

        printf("oldest unmerged child job: %ld\n", tj_get_oldest_child_job_without_results(&sqlenv));
        printf("oldest unmerged parent job: %ld\n", tj_get_oldest_parent_job_without_results(&sqlenv));
        
        printf("print job_reuslts\n");
        sqlenv_exec_with_callback(&sqlenv, "SELECT * from job_results;", NULL, &sql_callback_print);
        
        // doing a job
        tj_jobresults_do_tm_enumerate_job(&sqlenv, jobIdOriginal, 4);

        printf("print job_reuslts\n");
        sqlenv_exec_with_callback(&sqlenv, "SELECT * from job_results;", NULL, &sql_callback_print);

        tm_enumerate_options_simple_t twoStateFull100Random_simpleopt = {
            .states = 2,
            .max_steps = "100",
            .startIndex = "0",
            .indexesConsidered = NULL,

            .randomStartSeed = "1",
            .randomIterations = "100",

            .doZerosTape = true,
            .doOnesTape = true,

            // this doesnt affect the enumeration but it needs to be set. woopssies :P
            .workers = 0
        };

        unsigned long twoStateFull100Random_jobId = tj_create_job_simple_args(&sqlenv, twoStateFull100Random_simpleopt);

        tj_jobresults_do_tm_enumerate_job(&sqlenv, twoStateFull100Random_jobId, 4);

        printf("print job_reuslts\n");
        sqlenv_exec_with_callback(&sqlenv, "SELECT * from job_results;", NULL, &sql_callback_print);


        // job result specifics tests
        unsigned long chosenJob = twoStateFull100Random_jobId;
        char* lstring = "0";

        mpz_t tempMpz; mpz_init(tempMpz);
        tj_jobresults_get_rcount(&sqlenv, chosenJob, lstring, tempMpz);
        gmp_printf("%s|%Zd\n", lstring, tempMpz);

        printf("has 0: %d, has 123: %d\n", tj_jobresults_has_lstring(&sqlenv, chosenJob, "0"),tj_jobresults_has_lstring(&sqlenv, chosenJob, "123"));

        tj_jobresults_sum_counts(&sqlenv, chosenJob, tempMpz);
        gmp_printf("job %ld total count=%Zd\n", chosenJob, tempMpz);

        mpq_t tempMpq; mpq_init(tempMpq);
        tj_jobresults_get_freq(&sqlenv, chosenJob, lstring, tempMpq);
        printf("job %ld, lstring %s, frequency %f\n",chosenJob, lstring, mpq_get_d(tempMpq));

        mpfr_t tempMpfr; mpfr_init(tempMpfr);
        tj_jobresults_get_freq_neglog2(&sqlenv, chosenJob, lstring, tempMpfr);
        printf("job %ld, lstring %s, -log2(freq)=%f\n",chosenJob, lstring, mpfr_get_d(tempMpfr, MPFR_RNDZ));

        // loading into hashmap
        struct hashmap* slicecountMap = new_slicecounthashmap();
        tj_jobresults_load_into_slicecount_hashmap(&sqlenv, chosenJob, slicecountMap);

        tm_slice_counter_t sliceCounter;
        tm_slice_from_cstring(&sliceCounter.slice, "0");

        tm_slice_counter_t* result = hashmap_get(slicecountMap, &sliceCounter);

        char buff[40] = {0};
        tm_slice_sprint(&sliceCounter.slice, buff);
        gmp_printf("from hashmap %s:%Zd\n", buff, result->count);

        mpfr_clear(tempMpfr);
        mpz_clears(tempMpz, NULL);
        mpq_clears(tempMpq, NULL);
        // tj_DANGEROUS_delete_tables(&sqlenv);
    }

    sqlenv_close(&sqlenv);

    turing_threading_destroy();

    printf("done\n");

    return 0;
}