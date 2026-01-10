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

#include "sqlenv.h"

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

    // tm_print_enumerate_performance_stats(2,500);

    const int selfid = turing_threading_self_index();
    tm_srand(selfid, 1337);

    char *originalTableName = "NumbersCount";
    char *halfATableName = "NumbersCountHalfA";
    char *halfBTableName = "NumbersCountHalfB";

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

        enumerate_simple_opt.startIndex = "0";
        enumerate_simple_opt.indexesConsidered = "10369";

        sql_drop_table_if_exists(&sqlenv, statementBuffer, halfATableName);
        do_tm_enumerate_sql_merge_job_wrapped(enumerate_simple_opt,
            &sqlenv, statementBuffer, halfATableName
        );

        enumerate_simple_opt.startIndex = "10369";
        enumerate_simple_opt.indexesConsidered = NULL;

        sql_drop_table_if_exists(&sqlenv, statementBuffer, halfBTableName);
        do_tm_enumerate_sql_merge_job_wrapped(enumerate_simple_opt,
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

        unsigned long jobId;

        jobId = tj_create_job_simple_args(&sqlenv, enumerate_simple_opt);
        printf("jobId is %ld\n\n", jobId);

        printf("called tj_delete_job, rc=%d\n\n", tj_delete_job(&sqlenv, jobId));

        jobId = tj_create_job_simple_args(&sqlenv, enumerate_simple_opt);
        printf("jobId is %ld\n\n", jobId);

        jobId = tj_get_job_id_from_simple_args(&sqlenv, enumerate_simple_opt);
        printf("jobId is %ld\n\n", jobId);

        sqlenv_exec_with_callback(&sqlenv, "SELECT * from jobs;", NULL, &sql_callback_print);

        enumerate_job_opt_t enumOpt;
        tm_enumerate_job_opt_init(&enumOpt);

        tj_get_job_args(&sqlenv, jobId, &enumOpt);
        tm_enumerate_print_opt(&enumOpt);

        tm_enumerate_job_opt_destroy(&enumOpt);

        enumerate_simple_opt.indexesConsidered = "123";
        unsigned long newJobId = tj_create_job_simple_args(&sqlenv, enumerate_simple_opt);
        printf("new jobId is %ld\n\n", newJobId);

        tj_map_enumeration_to_children_jobs(&sqlenv, jobId, 1, &newJobId);

        printf("print all enum maps\n");
        sqlenv_exec_with_callback(&sqlenv, "SELECT * from enumeration_job_mapping;", NULL, &sql_callback_print);

        printf("print all jobs\n");
        sqlenv_exec_with_callback(&sqlenv, "SELECT * from jobs;", NULL, &sql_callback_print);

        printf("deleted jobs.job_id=1;\n");
        sqlenv_exec_with_callback(&sqlenv, "DELETE FROM jobs WHERE job_id=1;", NULL, NULL);

        printf("print all enum maps\n");
        sqlenv_exec_with_callback(&sqlenv, "SELECT * from enumeration_job_mapping;", NULL, &sql_callback_print);

        printf("print all jobs\n");
        sqlenv_exec_with_callback(&sqlenv, "SELECT * from jobs;", NULL, &sql_callback_print);
    }

    // "full index" enumeration = enumerating every machine with the desired tapes
    // split an enumeration into "jobs".
    // each job has their parameters specified so the
    // runner knows what to do with it.
    // there is a job queue...
    // runners go in there and pick a job.
    // after a job is finished the runner submits their job results.
    // there should be a verification thing where two runners submit the same job result
    // just to make sure they are both the same.

    sqlenv_close(&sqlenv);

    turing_threading_destroy();

    printf("done\n");

    return 0;
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
    n+1th permutation by simple algorithm.
    nth_permutation()
    next_permutation()

turing index enumeration:
    enumerate the turing index language for halting machines and their tape results.
    put each resulting string into the hashmap
*/