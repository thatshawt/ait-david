#ifndef TURING_JOBS_H
#define TURING_JOBS_H

#include "sqlenv.h"
#include "turing_enumerate.h"

// "exported" user friendly api:
    void turing_jobs_start();
    void turing_jobs_end();

    // max steps allowed per job
    //TODO: implement simple formula in turing_enumerate.c/h that calculates steps of an enumerate_job_opt_t
    void turing_jobs_set_maximum_steps_per_job(char* maxSteps);

    // the "enumeration" job gets split into children jobs that are under or equal to
    // the max steps allowed per job.
    // returns the job_id of the enumeration job.
    int turing_jobs_queue_enumeration(sqlenv_t *sqlenv, enumerate_job_opt_t jobArgs);

    // picks the oldest job from the "queue".
    // returns true if worked, false otherwise.
    // 'int* jobId' is loaded with the job's id.
    // 'enumerate_job_opt_t *jobArgs' is loaded with the job's enumeration arguments.
    bool turing_jobs_accept_oldest_job(sqlenv_t *sqlenv, int* jobId, enumerate_job_opt_t *jobArgs);

    // submit results for a job
    void turing_jobs_submit_job_results(sqlenv_t *sqlenv, int jobId, struct hashmap* slicecounter_hashmap);

    // returns new slicecounter hashmap from job results from jobId or jobArgs.
    // returns null pointer if it cant find the results or there are none.
    struct hashmap* turing_jobs_get_job_results(sqlenv_t *sqlenv, int jobId, enumerate_job_opt_t jobArgs);

    // returns true if worked, false otherwise.
    // 'int* jobId' is loaded with the jobId that matches the supplied 'enumerate_job_opt_t jobArgs'.
    // this can be used to check if an enumeration has already been added to the database.
    bool turing_jobs_get_jobid_from_args(sqlenv_t *sqlenv, enumerate_job_opt_t jobArgs, int* jobId);


// "low level" functions:
    // these are crucial to ensure thread-safety.
    void tj_lock();
    void tj_unlock();

    //sql stuff below here

    // this creates the tables below if they arent already created.
    void tj_create_tables(sqlenv_t* sqlenv);

    // this deletes the below tables...
    // dangerous...
    void tj_DANGEROUS_delete_tables(sqlenv_t* sqlenv);

    // jobs
    // AUTO UNIQUE INT job_id | PRIMARY(arg1 | arg2 | arg3 | ...)
        // returns job_id, -1 otherwise
        unsigned long tj_create_job_args(sqlenv_t* sqlenv, enumerate_job_opt_t jobArgs);
        unsigned long tj_create_job_simple_args(sqlenv_t* sqlenv, tm_enumerate_options_simple_t jobArgs);

        // returns job_id, -1 otherwise
        unsigned long tj_get_job_id_from_args(sqlenv_t* sqlenv, enumerate_job_opt_t jobArgs);
        unsigned long tj_get_job_id_from_simple_args(sqlenv_t* sqlenv, tm_enumerate_options_simple_t jobArgs);

        // loads job's args into 'enumerate_job_opt_t* jobArgs'.
        // returns true if worked, false otherwise.
        bool tj_get_job_args(sqlenv_t* sqlenv, unsigned long jobId, enumerate_job_opt_t* jobArgs);

    // enumeration_job_mapping
    // AUTO UNIQUE INT enumeration_job_map_id | PRIMARY(FOREIGN INT jobs.job_id as enumeration_id(one) | FOREIGN INT jobs.job_id(many))
        // maps one 'enumerationId' to array of job ids 'jobIds'.
        // 'jobCount' needs to be set to how many job ids are in the 'jobIds' variable.
        void tj_map_enumeration_to_children_jobs(sqlenv_t* sqlenv, unsigned long enumerationId, unsigned long jobCount, unsigned long* jobIds);

        // returns true if it worked false otherwise.
        // jobs loaded into the 'int** jobIds' variable.
        // amount of jobs loaded into 'int* jobCount' variable.
        bool tj_get_enumeration_jobs(sqlenv_t* sqlenv, unsigned long enumerationId, unsigned long* jobCount, unsigned long** jobIds);

        // returns a job's parent job id or -1.
        unsigned long tj_get_job_parent_enumeration(sqlenv_t* sqlenv, unsigned long jobId);

    // enumeration_job_merge_tracker
    // PRIMARY(FOREIGN INT jobs.job_id as enumeration_id(one) | FOREIGN INT jobs.job_id(many))
    // this is supposed to be incremental and keep track of which job results have 
    // been merged into the parent enumeration job.
        // if job is not merged, merges job into its parent enumeration.
        // returns true if it did the merge.
        bool tj_try_merge_job_into_parent_job(sqlenv_t* sqlenv, unsigned long jobId);

        // returns true if all the enumeration's children have been merged into the enumeration.
        bool tj_is_enumeration_fully_merged(sqlenv_t* sqlenv, unsigned long enumerationId);

        // returns true if the job has been merged into its parent enumeration.
        bool tj_has_job_been_merged_into_parent_enumeration(sqlenv_t* sqlenv, unsigned long jobId);

    // job_results
    // PRIMARY(FOREIGN INT jobs.job_id | NOT_NULL STR lstring) | NOT_NULL STR rcount

        // calls tj_try_merge_job_into_parent_job(jobId) after job result is added.
        void tj_add_single_job_result(sqlenv_t *sqlenv, unsigned long jobId, char* lstring, char* rcount);
        void tj_add_job_results_from_hashmap(sqlenv_t *sqlenv, unsigned long jobId, struct hashmap* slicecounter_hashmap);

        // true if succeed, false otherwise.
        // puts results of job_id into 'slicecounter_hashmap'.
        bool tj_load_job_results_into_slicecount_map(sqlenv_t *sqlenv, unsigned long jobId, struct hashmap* slicecounter_hashmap);

#endif