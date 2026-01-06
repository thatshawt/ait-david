#ifndef TURING_JOBS_H
#define TURING_JOBS_H

#include "sqlenv.h"
#include "turing_enumerate.h"

void tj_delete_job_table(sqlenv_t *sqlenv, char *jobname);
void tj_queue_job(sqlenv_t *sqlenv, char * enumerationName, enumerate_job_opt_t jobArgs);

// job_results
// job_id | string_count_table_name | validation_value

//  job_mapping
//  job_mapping_id | job_id | enumeration_id | job_args_id

// enumerations
// enumeration_id | job_args_id

// jobs
// job_id | job_args_id

// job_args
// job_args_id | arg1 | arg2 | arg3 | ...

void tj_define_enumeration_jobs_mapping(sqlenv_t *sqlenv, char * enumerationName, char **jobNames);

void turing_jobs_submit_job_results(sqlenv_t *sqlenv, char *jobname, char *resultsStrCountTableName);
void turing_jobs_accept_most_recent_job(sqlenv_t *sqlenv, enumerate_job_opt_t *jobArgs);
void turing_jobs_queue_enumeration(sqlenv_t *sqlenv, char * enumerationName, enumerate_job_opt_t jobArgs);

void turing_jobs_start();
void turing_jobs_end();

#endif