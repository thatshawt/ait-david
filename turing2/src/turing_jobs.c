#include "turing_jobs.h"

#include <stdio.h>
#include <string.h>
#include <pthread.h>

struct {
    pthread_mutex_t mutex;

    char statementBuffer[4000]; // increase if its not enough
} tjState;

void turing_jobs_start()
{
    pthread_mutex_init(&tjState.mutex, NULL);
}

void turing_jobs_end()
{
    pthread_mutex_destroy(&tjState.mutex);
}

// lock whenever reading/writing tjState.
void tj_lock()
{
    pthread_mutex_lock(&tjState.mutex);
}

void tj_unlock()
{
    pthread_mutex_unlock(&tjState.mutex);
}

void tj_create_jobs_table(sqlenv_t* sqlenv);
void tj_create_enumeration_job_mapping_table(sqlenv_t* sqlenv);
// void tj_create_merged_jobs_table(sqlenv_t* sqlenv);
void create_job_results_table(sqlenv_t* sqlenv);

void tj_create_tables(sqlenv_t* sqlenv)
{
    tj_create_jobs_table(sqlenv);
    tj_create_enumeration_job_mapping_table(sqlenv);
    // tj_create_merged_jobs_table(sqlenv);
    create_job_results_table(sqlenv);
}

void tj_DANGEROUS_delete_tables(sqlenv_t* sqlenv)
{
    const char* tables[] = {
        "jobs",
        "enumeration_job_mapping",
        // "merged_jobs",
        "job_results"
    };

    for(int i=0;i< sizeof(tables)/sizeof(tables[0]);i++){
        char* table = tables[i];
        tj_lock();
        sprintf(tjState.statementBuffer,
            "DROP TABLE %s;",
            table
        );
        // printf("%s\n",tjState.statementBuffer);
        sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
            NULL, NULL,
            NULL
            // &sql_resultHandler_print
        );
        if(sqlenv->rc == SQLITE_OK)printf("dropped table %s\n", table);
        else printf("failed to drop table %s\n", table);
        tj_unlock();
    }

}

void tj_create_jobs_table(sqlenv_t* sqlenv)
{
    tj_lock();
    sprintf(tjState.statementBuffer,
        "CREATE TABLE jobs("
            // sqlite assigns a value automagically to 'INTEGER PRIMARY KEY's
            "job_id INTEGER NOT NULL PRIMARY KEY," 
            
            // enumeration parameters
            "states INTEGER NOT NULL,"
            "start TEXT NOT NULL,"
            "length TEXT NOT NULL,"
            "max_steps TEXT NOT NULL,"

            // what kind of enumeration is done
            "doOnesTape INTEGER NOT NULL," // boolean
            "doZerosTape INTEGER NOT NULL," // boolean
            "randomIterations TEXT NOT NULL,"
            "randomStartSeed TEXT NOT NULL,"
            
            "unixepoch_timestamp INTEGER NOT NULL," // timestamp when it was created
            "has_job_results INTEGER NOT NULL DEFAULT 0," // boolean
            
            "UNIQUE(states,start,length,max_steps,doOnesTape,doZerosTape,randomIterations,randomStartSeed)"
        ");"
    );
    // printf("did '%s'", tjState.statementBuffer);
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        NULL, NULL,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();
    // printf("created table?\n");
}

// returns job_id, -1 otherwise
unsigned long tj_create_job_args(sqlenv_t* sqlenv, enumerate_job_opt_t jobArgs)
{
    unsigned long result = tj_get_job_id_from_args(sqlenv, jobArgs);

    if(result != -1){
        // printf("already exists\n");
        return result;
    };

    // tm_enumerate_print_opt(&jobArgs);
    tj_lock();
    // the curly braces are purely visual.
    { // the two sql calls need to execute one after another, hence the locking.
        gmp_sprintf(tjState.statementBuffer,
            "INSERT INTO"
            " jobs(states,start,length,max_steps,doOnesTape,doZerosTape,randomIterations,randomStartSeed,unixepoch_timestamp)"
            " VALUES (%d, %Zd, %Zd, %Zd, %d, %d, %Zd, %Zd, unixepoch());",
            jobArgs.states, jobArgs.start, jobArgs.length,
            jobArgs.max_steps, jobArgs.doOnesTape, jobArgs.doZerosTape,
            jobArgs.randomIterations, jobArgs.randomStartSeed
        );
        sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer, 
            NULL, NULL,
            NULL
        );

        // printf("sqlenv.rc %d\n", sqlenv->rc);

        result = sqlenv_get_last_insert_rowid(sqlenv);
    }
    tj_unlock();

    return result;
}

unsigned long tj_create_job_simple_args(sqlenv_t* sqlenv, tm_enumerate_options_simple_t jobArgs)
{   
    enumerate_job_opt_t resolvedArgs;
    tm_enumerate_job_opt_init(&resolvedArgs);

    // make sure jobArgs is properly "resolved".
    tm_resolve_simple_args_to_hard_args(jobArgs, &resolvedArgs);

    unsigned long result = tj_create_job_args(sqlenv, resolvedArgs);
    
    tm_enumerate_job_opt_destroy(&resolvedArgs);
    
    return result;
}

// returns job_id, -1 otherwise
unsigned long tj_get_job_id_from_args(sqlenv_t* sqlenv, enumerate_job_opt_t jobArgs)
{
    mpz_t theMpz; mpz_init_set_ui(theMpz, -1);

    tj_lock();
    gmp_sprintf(tjState.statementBuffer,
        "SELECT job_id FROM jobs WHERE"
        " states=%d AND start=%Zd AND length=%Zd AND max_steps=%Zd AND doOnesTape=%d AND doZerosTape=%d AND randomIterations=%Zd AND randomStartSeed=%Zd;",
        jobArgs.states, jobArgs.start, jobArgs.length,
        jobArgs.max_steps, jobArgs.doOnesTape, jobArgs.doZerosTape,
        jobArgs.randomIterations, jobArgs.randomStartSeed
    );
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        &theMpz, &sql_callback_load_firstcol_into_mpz,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();

    unsigned long result = mpz_get_ui(theMpz);

    mpz_clear(theMpz);

    return sqlenv->rc == SQLITE_OK ? result : -1;
}

unsigned long tj_get_job_id_from_simple_args(sqlenv_t* sqlenv, tm_enumerate_options_simple_t jobArgs)
{
    enumerate_job_opt_t resolvedArgs;
    tm_enumerate_job_opt_init(&resolvedArgs);

    // make sure jobArgs is properly "resolved".
    tm_resolve_simple_args_to_hard_args(jobArgs, &resolvedArgs);

    unsigned long result = tj_get_job_id_from_args(sqlenv, resolvedArgs);
    
    tm_enumerate_job_opt_destroy(&resolvedArgs);
    
    return result;
}

int tj_sql_callback_load_into_jobargs(void *data, int count, char **values, char **columnNames)
{
    enumerate_job_opt_t* jobArgs = (enumerate_job_opt_t*)data;

    mpz_t anMpz; mpz_init(anMpz);

    for(int i=0;i<count;i++){
        const char* value = values[i];
        const char* column = columnNames[i];

        if(strcmp(column,"states") == 0){
            mpz_set_str(anMpz, value, 10);
            jobArgs->states = mpz_get_ui(anMpz);

        }else if(strcmp(column,"start") == 0){
            mpz_set_str(jobArgs->start, value, 10);

        }else if(strcmp(column,"length") == 0){
            mpz_set_str(jobArgs->length, value, 10);

        }else if(strcmp(column,"max_steps") == 0){
            mpz_set_str(jobArgs->max_steps, value, 10);

        }else if(strcmp(column,"doOnesTape") == 0){
            mpz_set_str(anMpz, value, 10);
            jobArgs->doOnesTape = mpz_get_ui(anMpz);

        }else if(strcmp(column,"doZerosTape") == 0){
            mpz_set_str(anMpz, value, 10);
            jobArgs->doZerosTape = mpz_get_ui(anMpz);

        }else if(strcmp(column,"randomIterations") == 0){
            mpz_set_str(jobArgs->randomIterations, value, 10);

        }else if(strcmp(column,"randomStartSeed") == 0){
            mpz_set_str(jobArgs->randomStartSeed, value, 10);

        }
    }

    mpz_clear(anMpz);
    return 0;
}

bool tj_get_job_args(sqlenv_t* sqlenv, unsigned long jobId, enumerate_job_opt_t* jobArgs)
{
    bool jobIdExists = tj_get_job_id_from_args(sqlenv, *jobArgs) != -1;

    if(!jobIdExists){
        tj_lock();

        sprintf(tjState.statementBuffer, "SELECT * FROM jobs WHERE job_id=%ld;", jobId);

        sqlenv_exec_with_callback(sqlenv, tjState.statementBuffer,
            (void*)jobArgs, &tj_sql_callback_load_into_jobargs);

        tj_unlock();

        return true;
    }else{
        return false;
    }
}

// bool tj_get_job_simple_args(sqlenv_t* sqlenv, unsigned long jobId, enumerate_job_opt_t* jobArgs)
// {

// }

int tj_delete_job(sqlenv_t* sqlenv, unsigned long jobId)
{
    tj_lock();

    sprintf(tjState.statementBuffer,
        "DELETE FROM jobs WHERE job_id=%ld;",
        jobId
    );
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        NULL, NULL,
        // &sql_resultHandler_print
        NULL
    );

    int rc = sqlenv->rc;

    tj_unlock();

    return rc;
}

void tj_create_enumeration_job_mapping_table(sqlenv_t* sqlenv)
{
    tj_lock();
    sprintf(tjState.statementBuffer,
        "CREATE TABLE enumeration_job_mapping("
            "parent_id INTEGER NOT NULL REFERENCES jobs(job_id) ON DELETE CASCADE,"
            "child_id INTEGER NOT NULL REFERENCES jobs(job_id) ON DELETE CASCADE,"
            "is_merged INTEGER NOT NULL DEFAULT 0,"
            // "being_worked_on INTEGER NOT NULL DEFAULT 0,"
            // "datetime_started_work TEXT,"
            "PRIMARY KEY(parent_id, child_id)"
        ");"
    );
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        NULL, NULL,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();
    // printf("created table?\n");
}

//TODO needs testing.
// maps one 'enumerationId' to array of job ids 'jobIds'.
// 'jobCount' needs to be set to how many job ids are in the 'jobIds' variable.
void tj_map_enumeration_to_children_jobs(sqlenv_t* sqlenv, unsigned long enumerationId, int jobCount, unsigned long* jobIds)
{
    for(int i=0;i<jobCount;i++){
        const unsigned long childId = jobIds[i];
        tj_lock();

        sprintf(tjState.statementBuffer,
            "INSERT INTO enumeration_job_mapping(parent_id, child_id)"
            "VALUES (%ld, %ld);",
            enumerationId, childId
        );
        sqlenv_exec_with_callback(sqlenv, tjState.statementBuffer,
            NULL,
            &sql_callback_print
            // NULL
        );

        tj_unlock();
    }
}

// deletes the single mapping of the parentId and childId
int tj_delete_single_enumeration_job_mapping(sqlenv_t* sqlenv, unsigned long parentId, unsigned long childId)
{
    tj_lock();

    sprintf(tjState.statementBuffer,
        "DELETE FROM enumeration_job_mapping WHERE parent_id=%ld AND child_id=%ld;",
        parentId,childId
    );
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        NULL, NULL,
        // &sql_resultHandler_print
        NULL
    );

    int rc = sqlenv->rc;

    tj_unlock();

    return rc;
}

// deletes all mapping with the enumerationId as the parent.
int tj_delete_all_enumeration_mapping(sqlenv_t* sqlenv, unsigned long enumerationId)
{
    tj_lock();

    sprintf(tjState.statementBuffer,
        "DELETE FROM enumeration_job_mapping WHERE parent_id=%ld;",
        enumerationId
    );
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        NULL, NULL,
        // &sql_resultHandler_print
        NULL
    );

    int rc = sqlenv->rc;

    tj_unlock();

    return rc;
}

int tj_number_of_children(sqlenv_t* sqlenv, unsigned long enumerationId)
{
    mpz_t anMpz; mpz_init_set_ui(anMpz, 0);
    tj_lock();
    sprintf(tjState.statementBuffer,
        "SELECT COUNT(*) FROM enumeration_job_mapping"
        " WHERE parent_id=%ld;"
        ,enumerationId
    );
    // printf("%s\n", tjState.statementBuffer);
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        &anMpz, &sql_callback_load_firstcol_into_mpz,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();

    int result = (int)mpz_get_ui(anMpz);

    mpz_clear(anMpz);

    return result;
}

typedef struct{
    int counter;
    unsigned long* currentJobPointer;
    mpz_t anMpz;
} tj_get_enumeration_jobs_state_t;
int sql_callback_tj_load_id_into_state_thing_array(void *data, int count, char **values, char **columnNames)
{
    tj_get_enumeration_jobs_state_t* state = (tj_get_enumeration_jobs_state_t*)data;

    // printf("child_id=%s\n", values[0]);

    mpz_set_str(state->anMpz, values[0], 10);

    *(state->currentJobPointer) = mpz_get_ui(state->anMpz);
    state->currentJobPointer++;
    state->counter++;

    return 0;
}
// returns true if it worked false otherwise.
// jobs loaded into the 'int** jobIds' variable.
// amount of jobs loaded into 'int* jobCount' variable.
bool tj_get_enumeration_children(sqlenv_t* sqlenv, unsigned long enumerationId, int* jobCount, unsigned long* jobIds)
{
    tj_get_enumeration_jobs_state_t state = {
        .counter=0,
        .currentJobPointer=jobIds
    };
    mpz_init(state.anMpz);

    tj_lock();
    sprintf(tjState.statementBuffer,
        "SELECT child_id FROM enumeration_job_mapping"
        " WHERE parent_id=%ld;",
        enumerationId
    );
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        &state, &sql_callback_tj_load_id_into_state_thing_array,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();

    mpz_clear(state.anMpz);

    *jobCount = state.counter;

    return true;
}

// supply the child job id as childId.
// loads all the parents of a child job into parentIds.
// loads the number of parents into jobCount.
void tj_get_enumeration_parents(sqlenv_t* sqlenv, unsigned long childId, int* jobCount, unsigned long* parentIds)
{
    tj_get_enumeration_jobs_state_t state = {
        .counter=0,
        .currentJobPointer=parentIds
    };
    mpz_init(state.anMpz);

    tj_lock();
    sprintf(tjState.statementBuffer,
        "SELECT parent_id FROM enumeration_job_mapping"
        " WHERE child_id=%ld;",
        childId
    );
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        &state, &sql_callback_tj_load_id_into_state_thing_array,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();

    mpz_clear(state.anMpz);

    *jobCount = state.counter;

    // return true;
}


// not used
void tj_create_merged_jobs_table(sqlenv_t* sqlenv)
{
    tj_lock();
    sprintf(tjState.statementBuffer,
        "CREATE TABLE merged_jobs("
            "the_parent_id INTEGER NOT NULL REFERENCES jobs(job_id) ON DELETE CASCADE,"
            "the_child_id INTEGER NOT NULL REFERENCES jobs(job_id) ON DELETE CASCADE,"
            "PRIMARY KEY(the_parent_id, the_child_id)"
        ");"
    );
    // printf("did '%s'\n", tjState.statementBuffer);
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        NULL, NULL,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();
    // printf("created table?\n");
}

void tj_mark_job_completed_results(sqlenv_t* sqlenv, unsigned long jobId)
{
    tj_lock();
    sprintf(tjState.statementBuffer,
        // "INSERT INTO merged_jobs(the_parent_id,the_child_id)"
        // " VALUES (%ld, %ld);"

        "UPDATE jobs"
        " SET has_job_results=1"
        " WHERE job_id=%ld;"
        , jobId
    );
    // printf("did '%s'\n", tjState.statementBuffer);
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        NULL, NULL,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();
}

int tj_number_children_with_results(sqlenv_t* sqlenv, unsigned long enumerationId)
{
    mpz_t anMpz; mpz_init_set_ui(anMpz, 0);
    tj_lock();
    sprintf(tjState.statementBuffer,
        "SELECT COUNT(*) FROM ("
            "SELECT child_id FROM enumeration_job_mapping"
            " INNER JOIN jobs"
            " ON job_id=child_id AND has_job_results=1 AND parent_id=%ld"
        ");"
        ,enumerationId
    );
    // printf("%s\n", tjState.statementBuffer);
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        &anMpz, &sql_callback_load_firstcol_into_mpz,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();

    int result = (int)mpz_get_ui(anMpz);

    mpz_clear(anMpz);

    return result;
}

void tj_get_children_with_results(sqlenv_t* sqlenv, unsigned long enumerationId, int* jobCount, unsigned long* jobIds)
{
    tj_get_enumeration_jobs_state_t state = {
        .counter=0,
        .currentJobPointer=jobIds
    };
    mpz_init(state.anMpz);

    tj_lock();
    sprintf(tjState.statementBuffer,
        // "SELECT the_child_id FROM merged_jobs"
        // " WHERE the_parent_id=%ld;"
        "SELECT child_id FROM enumeration_job_mapping"
        " INNER JOIN jobs"
        " ON job_id=child_id AND has_job_results=1 AND parent_id=%ld;"
        // "SELECT child_id FROM enumeration_job_mapping WHERE parent_id=%ld AND is_merged=1;"
        ,enumerationId
    );
    // printf("%s\n", tjState.statementBuffer);
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        &state, &sql_callback_tj_load_id_into_state_thing_array,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();

    mpz_clear(state.anMpz);

    *jobCount = state.counter;
}

void tj_get_children_without_results(sqlenv_t* sqlenv, unsigned long enumerationId, int* jobCount, unsigned long* jobIds)
{
    tj_get_enumeration_jobs_state_t state = {
        .counter=0,
        .currentJobPointer=jobIds
    };
    mpz_init(state.anMpz);

    tj_lock();
    sprintf(tjState.statementBuffer,
        "SELECT child_id FROM enumeration_job_mapping"
        " INNER JOIN jobs"
        " ON job_id=child_id AND has_job_results=0 AND parent_id=%ld;"
        // "SELECT child_id FROM enumeration_job_mapping WHERE parent_id=%ld AND is_merged=0;"
       ,enumerationId
    );
    // printf("%s\n", tjState.statementBuffer);
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        &state, &sql_callback_tj_load_id_into_state_thing_array,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();

    mpz_clear(state.anMpz);

    *jobCount = state.counter;
}

int tj_number_children_without_results(sqlenv_t* sqlenv, unsigned long enumerationId)
{
    mpz_t anMpz; mpz_init_set_ui(anMpz, 0);
    tj_lock();
    sprintf(tjState.statementBuffer,
        "SELECT COUNT(*) FROM ("
            "SELECT child_id FROM enumeration_job_mapping"
            " INNER JOIN jobs"
            " ON job_id=child_id AND has_job_results=0 AND parent_id=%ld"
        ");"
        // "SELECT COUNT(*) FROM enumeration_job_mapping WHERE parent_id=%ld AND is_merged=0;"
        , enumerationId
    );
    // printf("%s\n", tjState.statementBuffer);
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        &anMpz, &sql_callback_load_firstcol_into_mpz,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();

    int result = (int)mpz_get_ui(anMpz);

    mpz_clear(anMpz);

    return result;
}

unsigned long tj_get_oldest_child_job_without_results(sqlenv_t* sqlenv)
{
    mpz_t anMpz; mpz_init_set_ui(anMpz, -1);
    tj_lock();
    sprintf(tjState.statementBuffer,
        "SELECT job_id, MIN(unixepoch_timestamp) FROM jobs"
        " INNER JOIN enumeration_job_mapping"
        " ON job_id=child_id AND has_job_results=0;"
    );
    // printf("%s\n", tjState.statementBuffer);
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        &anMpz, &sql_callback_load_firstcol_into_mpz,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();

    unsigned long result = mpz_get_ui(anMpz);

    mpz_clear(anMpz);

    return result;
}

unsigned long tj_get_oldest_parent_job_without_results(sqlenv_t* sqlenv)
{
    mpz_t anMpz; mpz_init_set_ui(anMpz, -1);
    tj_lock();
    sprintf(tjState.statementBuffer,
        "SELECT job_id, MIN(unixepoch_timestamp) FROM jobs"
        " INNER JOIN enumeration_job_mapping"
        " ON job_id=parent_id AND has_job_results=0;"
    );
    // printf("%s\n", tjState.statementBuffer);
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        &anMpz, &sql_callback_load_firstcol_into_mpz,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();

    unsigned long result = mpz_get_ui(anMpz);

    mpz_clear(anMpz);

    return result;
}

void create_job_results_table(sqlenv_t* sqlenv)
{
    tj_lock();
    sprintf(tjState.statementBuffer,
        "CREATE TABLE job_results("
            "job_id INTEGER NOT NULL REFERENCES jobs(job_id) ON DELETE CASCADE,"
            "lstring TEXT NOT NULL,"
            "rcount TEXT NOT NULL,"
            "PRIMARY KEY (job_id, lstring)"
        ");"
    );
    // printf("%s\n", tjState.statementBuffer);
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        NULL, NULL,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();
}

void tj_jobresults_add_single_row(sqlenv_t *sqlenv, unsigned long jobId, char* lstring, char* rcount)
{
    if(lstring == NULL || rcount == NULL)return;
    tj_lock();
    sprintf(tjState.statementBuffer,
        "INSERT INTO job_results(job_id, lstring, rcount)"
        " VALUES (%ld, '%s', '%s');"
        , jobId, lstring, rcount
    );
    // printf("%s\n", tjState.statementBuffer);
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        NULL, NULL,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();
}

void tj_jobresults_add_rows_slicecount_hashmap(sqlenv_t *sqlenv, unsigned long jobId, struct hashmap* map)
{
    size_t iterA = 0;
    void *itemA;
    char rcount[1000];
    char lstring[1000];
    mpz_t count; mpz_init(count);
    while (hashmap_iter(map, &iterA, &itemA)) {
        const tm_slice_counter_t *sliceCounter = itemA;

        tm_slice_sprint(&sliceCounter->slice, lstring);
        
        // mpz_set_ui(count, 0);
        // sql_load_str_count_into_mpz(sqlenv, statementBuffer, tableName, lstring, &count);
        // mpz_add(count, count, sliceCounter->count);
        mpz_set(count, sliceCounter->count);

        mpz_get_str(rcount, 10, count);

        // if(strcmp(lstring, "0") == 0)printf("           %s:%s\n", lstring, rcount);
        
        tj_jobresults_add_single_row(sqlenv, jobId, lstring, rcount);
    }
    mpz_clear(count);
}

void tj_jobresults_clear_job(sqlenv_t *sqlenv, unsigned long jobId)
{
    tj_lock();
    sprintf(tjState.statementBuffer,
        "DELETE FROM job_results WHERE job_id=%ld;"
        , jobId
    );
    // printf("%s\n", tjState.statementBuffer);
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        NULL, NULL,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();
}

bool tj_jobresults_do_tm_enumerate_job(sqlenv_t *sqlenv, unsigned long jobId, int workers)
{
    enumerate_job_opt_t enumerateOpt;
    tm_enumerate_job_opt_init(&enumerateOpt);
    tj_get_job_args(sqlenv, jobId, &enumerateOpt);
    
    enumerateOpt.workthreads = workers;

    // tm_enumerate_print_opt(&enumerateOpt);

    struct hashmap* slicecount_map = do_tm_enumerate_job(&enumerateOpt);

    tm_enumerate_job_opt_destroy(&enumerateOpt);

    tj_jobresults_clear_job(sqlenv, jobId);
    tj_jobresults_add_rows_slicecount_hashmap(sqlenv, jobId, slicecount_map);
    
    tj_mark_job_completed_results(sqlenv, jobId);

    hashmap_free(slicecount_map);
}

void tj_jobresults_get_rcount(sqlenv_t *sqlenv, unsigned long jobId, char* lstring, mpz_t rcountMpz)
{
    mpz_set_si(rcountMpz, -1);
    tj_lock();
    sprintf(tjState.statementBuffer,
        "SELECT rcount FROM job_results"
        " WHERE job_id=%ld AND lstring='%s';"
        , jobId, lstring
    );
    // printf("%s\n", tjState.statementBuffer);
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        (void*)rcountMpz, &sql_callback_load_firstcol_into_mpz,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();
}

bool tj_jobresults_has_lstring(sqlenv_t *sqlenv, unsigned long jobId, char* lstring)
{
    mpz_t rcountMpz; mpz_init(rcountMpz);

    tj_jobresults_get_rcount(sqlenv, jobId, lstring, rcountMpz);

    bool result = mpz_cmp_si(rcountMpz, -1) != 0;

    mpz_clear(rcountMpz);

    return result;
}

int sql_callback_add_firstcol_into_mpz(void *data, int count, char **values, char **columnNames)
{
    // mpz_t dataMpz = data;

    if(count == 0 || values[0] == NULL)return 0;

    mpz_t tempMpz; mpz_init(tempMpz);

    mpz_set_str(tempMpz, values[0], 10);

    mpz_add(*(mpz_t*)data, *(mpz_t*)data, tempMpz);

    mpz_clear(tempMpz);

    return 0;
}

void tj_jobresults_sum_counts(sqlenv_t *sqlenv, unsigned long jobId, mpz_t sumMpz)
{
    mpz_set_ui(sumMpz, 0);

    tj_lock();
    sprintf(tjState.statementBuffer,
        "SELECT rcount FROM job_results"
        " WHERE job_id=%ld;"
        , jobId
    );
    // printf("%s\n", tjState.statementBuffer);
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        (void*)sumMpz, &sql_callback_add_firstcol_into_mpz,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();
}

void tj_jobresults_get_freq(sqlenv_t *sqlenv, unsigned long jobId, char* lstring, mpq_t freqMpq)
{
    mpz_t count, totalCount; mpz_inits(count, totalCount, NULL);
    mpq_t tempq1;mpq_init(tempq1);

    tj_jobresults_sum_counts(sqlenv, jobId, totalCount);
    tj_jobresults_get_rcount(sqlenv, jobId, lstring, count);

    mpq_set_z(freqMpq, count); // freq = count
    mpq_set_z(tempq1, totalCount); // tempq1 = totalCount

    mpq_canonicalize(freqMpq);
    mpq_canonicalize(tempq1);

    mpq_div(freqMpq, freqMpq, tempq1); // freq = count/totalCount;

    mpq_canonicalize(freqMpq);

    mpz_clears(count, totalCount, NULL);
    mpq_clear(tempq1);
}

void tj_jobresults_get_freq_neglog2(sqlenv_t *sqlenv, unsigned long jobId, char* lstring, mpfr_t negLog2Mpfr)
{
    mpq_t freq; mpq_init(freq);
    
    tj_jobresults_get_freq(sqlenv, jobId, lstring, freq);

    mpfr_set_q(negLog2Mpfr, freq, MPFR_RNDZ);
    mpfr_log2(negLog2Mpfr, negLog2Mpfr, MPFR_RNDZ);
    mpfr_mul_si(negLog2Mpfr, negLog2Mpfr, -1, MPFR_RNDZ);

    mpq_clear(freq);
}

int sql_callback_load_lstring_rcount_into_hashmap(void *data, int count, char **values, char **columnNames)
{
    tape_slice_t slice;
    tm_slice_counter_t slicecounter;
    // printf("hi there");
    for(int i = 0; i<count; i++){
        const char* colName = columnNames[i];
        const char* val = values[i];

        if(strcmp(colName, "rcount") == 0){
            mpz_init(slicecounter.count);
            mpz_set_str(slicecounter.count, val, 10);
        }else if(strcmp(colName, "lstring") == 0){
            tm_slice_from_cstring(&slice, val);
            slicecounter.slice = slice;
        }
        
    }
    
    // gmp_printf("count: %Zd, slice: '%s', lemgth %d\n", slicecounter.count ,slice.tapeslice, slice.length);

    struct hashmap* slicecounter_map = (struct hashmap*)data;

    hashmap_set(slicecounter_map, &slicecounter);

    // mpz_clear(count);

    return 0;
}

void tj_jobresults_load_into_slicecount_hashmap(sqlenv_t *sqlenv, unsigned long jobId, struct hashmap* slicecounter_hashmap)
{
    tj_lock();
    sprintf(tjState.statementBuffer,
        "SELECT lstring, rcount FROM job_results"
        " WHERE job_id=%ld;"
        , jobId
    );
    // printf("%s\n", tjState.statementBuffer);
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        (void*)slicecounter_hashmap, &sql_callback_load_lstring_rcount_into_hashmap,
        NULL
        // &sql_resultHandler_print
    );
    tj_unlock();
}

void tj_jobresults_merge_B_into_A(sqlenv_t *sqlenv, unsigned long jobIdA, unsigned long jobIdB)
{
    // make function to merge two hashmaps from code at turing_enumerate.c:441
}


