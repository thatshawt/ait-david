#ifndef TURING_JOBS_H
#define TURING_JOBS_H

#include "sqlenv.h"
#include "turing_enumerate.h"

// "exported" user friendly api:
    void turing_jobs_start();
    void turing_jobs_end();

    //TODO: implement simple formula in turing_enumerate.c/h that calculates steps of an enumerate_job_opt_t
    
    // the "enumeration" job gets split into children jobs that are under or equal to
    // the max steps allowed per job.
    // returns the job_id of the enumeration job.
    // CONDITION: always submits two jobs at a minimum. one for the "parent" and one at least for the "child".
    // the enumeration jobs are only used to get merged into, the children are like the "worker" jobs
    // that get enumerated. the original main parent enumeration job only gets merged the results from the children jobs.
    unsigned long turing_jobs_queue_enumeration(sqlenv_t *sqlenv, char* maxStepsPerJob, enumerate_job_opt_t jobArgs);

    // picks the oldest job from the "queue".
    // returns true if worked, false otherwise.
    // 'int* jobId' is loaded with the job's id.
    // 'enumerate_job_opt_t *jobArgs' is loaded with the job's enumeration arguments.
    bool turing_jobs_accept_oldest_job(sqlenv_t *sqlenv, int* jobId, enumerate_job_opt_t *jobArgs);

    // submit results for a job
    void turing_jobs_submit_job_results(sqlenv_t *sqlenv, unsigned long jobId, struct hashmap* slicecounter_hashmap);

    // returns new slicecounter hashmap from job results from jobId or jobArgs.
    // returns null pointer if it cant find the results or there are none.
    struct hashmap* turing_jobs_get_job_results(sqlenv_t *sqlenv, unsigned long jobId, enumerate_job_opt_t jobArgs);

    // returns jobId that matches the args or -1 if there is none
    // 'int* jobId' is loaded with the jobId that matches the supplied 'enumerate_job_opt_t jobArgs'.
    // this can be used to check if an enumeration has already been added to the database.
    unsigned long turing_jobs_get_jobid_from_args(sqlenv_t *sqlenv, enumerate_job_opt_t jobArgs);


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

    // jobs table
    // returns job_id, -1 otherwise
        unsigned long tj_create_job_args(sqlenv_t* sqlenv, enumerate_job_opt_t jobArgs);
        unsigned long tj_create_job_simple_args(sqlenv_t* sqlenv, tm_enumerate_options_simple_t jobArgs);

        // returns job_id, -1 otherwise
        unsigned long tj_get_job_id_from_args(sqlenv_t* sqlenv, enumerate_job_opt_t jobArgs);
        unsigned long tj_get_job_id_from_simple_args(sqlenv_t* sqlenv, tm_enumerate_options_simple_t jobArgs);

        // loads job's args into 'enumerate_job_opt_t* jobArgs'.
        // returns true if worked, false otherwise.
        bool tj_get_job_args(sqlenv_t* sqlenv, unsigned long jobId, enumerate_job_opt_t* jobArgs);
        // bool tj_get_job_simple_args(sqlenv_t* sqlenv, unsigned long jobId, enumerate_job_opt_t* jobArgs);

        // removes job from database as well as any mappings to it.
        // returns sql return code.
        int tj_delete_job(sqlenv_t* sqlenv, unsigned long jobId);

        // this will create a new child job
        // bool tj_split_child_job_into_workable_job();

    // enumeration_job_mapping
    // maps one 'enumerationId' to array of job ids 'jobIds'.
    // 'jobCount' needs to be set to how many job ids are in the 'jobIds' variable.
        void tj_map_enumeration_to_children_jobs(sqlenv_t* sqlenv, unsigned long enumerationId, int jobCount, unsigned long* jobIds);

        // deletes the single mapping of the parentId and childId
        int tj_delete_single_enumeration_job_mapping(sqlenv_t* sqlenv, unsigned long parentId, unsigned long childId);

        // deletes all mapping with the enumerationId as the parent.
        int tj_delete_all_enumeration_mapping(sqlenv_t* sqlenv, unsigned long enumerationId);

        int tj_number_of_children(sqlenv_t* sqlenv, unsigned long enumerationId);

        // returns true if it worked false otherwise.
        // jobs loaded into the 'int** jobIds' variable.
        // amount of jobs loaded into 'int* jobCount' variable.
        bool tj_get_enumeration_children(sqlenv_t* sqlenv, unsigned long enumerationId, int* jobCount, unsigned long* jobIds);

        // supply the child job id as childId.
        // loads all the parents of a child job into parentIds.
        // loads the number of parents into jobCount.
        void tj_get_enumeration_parents(sqlenv_t* sqlenv, unsigned long childId, int* jobCount, unsigned long* parentIds);

        // call this when you merge a child into a parent job so it gets tracked properly.
        void tj_mark_job_completed_results(sqlenv_t* sqlenv, unsigned long jobId);

        // returns number of merged children into parent job.
        int tj_number_children_with_results(sqlenv_t* sqlenv, unsigned long enumerationId);

        // gets the merged children
        void tj_get_children_with_results(sqlenv_t* sqlenv, unsigned long enumerationId, int* jobCount, unsigned long* jobIds);

        // gets the children of an enumeration job that are not merged yet.
        void tj_get_children_without_results(sqlenv_t* sqlenv, unsigned long enumerationId, int* jobCount, unsigned long* jobIds);

        int tj_number_children_without_results(sqlenv_t* sqlenv, unsigned long enumerationId);
        
        // returns id of job added earliest that still needs to be merged.
        // -1 if there are no more unmerged jobs.
        unsigned long tj_get_oldest_child_job_without_results(sqlenv_t* sqlenv);
        unsigned long tj_get_oldest_parent_job_without_results(sqlenv_t* sqlenv);

        // returns true if all the enumeration job's children have been merged,
        // false otherwise.
        // bool tj_is_enumeration_fully_merged(sqlenv_t* sqlenv, unsigned long enumerationId);


    /*
    CREATE TABLE job_results(
        job_id INTEGER NOT NULL REFERENCES jobs(job_id) ON DELETE CASCADE,
        lstring TEXT NOT NULL,
        rcount TEXT NOT NULL,
        PRIMARY KEY (job_id, lstring)
    );
    */


        void tj_jobresults_add_single_row(sqlenv_t *sqlenv, unsigned long jobId, char* lstring, char* rcount);

        void tj_jobresults_add_rows_slicecount_hashmap(sqlenv_t *sqlenv, unsigned long jobId, struct hashmap* slicecounter_hashmap);

        bool tj_do_tm_enumerate_job(sqlenv_t *sqlenv, unsigned long jobId, int workers);

        // true if succeed, false otherwise.
        // puts results of job_id into 'slicecounter_hashmap'.
        bool tj_jobresults_load_into_slicecount_hashmap(sqlenv_t *sqlenv, unsigned long jobId, struct hashmap* slicecounter_hashmap);

        void tj_jobresults_clear_job(sqlenv_t *sqlenv, unsigned long jobId);

        void tj_jobresults_merge_B_into_A(sqlenv_t *sqlenv, unsigned long jobIdA, unsigned long jobIdB);

        void tj_jobresults_get_rcount(sqlenv_t *sqlenv, unsigned long jobId, char* lstring, mpz_t rcountMpz);
        bool tj_jobresults_has_lstring(sqlenv_t *sqlenv, unsigned long jobId, char* lstring);
        void tj_jobresults_sum_counts(sqlenv_t *sqlenv, unsigned long jobId, mpz_t sumMpz);
        void tj_jobresults_get_freq(sqlenv_t *sqlenv, unsigned long jobId, char* lstring, mpq_t freqMpq);
        void tj_jobresults_get_freq_neglog2(sqlenv_t *sqlenv, unsigned long jobId, char* lstring, mpfr_t negLog2Mpfr);

#endif