#include "turing_tests.h"
#include "turing_sim.h"
#include "turing_mapping.h"
#include "turing_utils.h"
#include "turing_enumerate.h"
#include "turing_jobs.h"

#include "hashmap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gmp.h>
#include <mpfr.h>

#include <unistd.h>

#include "sqlenv.h"
#include "monotone_tm.h"

// int trivialNonHalters = 0;

// how do we catch more non halters o.O

int main(){

    // print various messages.
    // run tests.
    {
        printf("\nhello turing\n\n");
        printf("SQLITE_OK %d\n", SQLITE_OK);

        turing_threading_init_global();

        turing_threading_self_init();

        test_opt_t testOptions;
        testOptions.onlyPrintFailingTests = false;
        test_all(&testOptions);

        // return 0;
    }

    // mtm testing

    int states = 2;
    int workTapes = 3;
    mtm_transition_table_t table;
    mtm_trans_table_init(&table, states, workTapes);
    mtm_print_table(&table);

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