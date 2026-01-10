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

// lock whenever calling sqlenv function or reading/writing tjState.
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

void tj_create_tables(sqlenv_t* sqlenv)
{
    tj_create_jobs_table(sqlenv);
    tj_create_enumeration_job_mapping_table(sqlenv);
}

void tj_DANGEROUS_delete_tables(sqlenv_t* sqlenv)
{
    char* tables[] = {"jobs", "enumeration_job_mapping"};

    for(int i=0;i< sizeof(tables)/sizeof(tables[0]);i++){
        char* table = tables[i];
        tj_lock();
        sprintf(tjState.statementBuffer,
            "DROP TABLE %s;",
            table
        );
        sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
            NULL, NULL,
            // NULL
            &sql_resultHandler_print
        );
        tj_unlock();
    }

}

void tj_create_jobs_table(sqlenv_t* sqlenv)
{
    tj_lock();
    sprintf(tjState.statementBuffer,
        "CREATE TABLE jobs("
            "job_id INTEGER PRIMARY KEY," // sqlite assigns a value automagically to 'INTEGER PRIMARY KEY's
            "states INTEGER NOT NULL,"
            "start TEXT NOT NULL,"
            "length TEXT NOT NULL,"
            "max_steps TEXT NOT NULL,"
            "doOnesTape INTEGER NOT NULL," // this is a boolean
            "doZerosTape INTEGER NOT NULL," // this is a boolean
            "randomIterations TEXT NOT NULL,"
            "randomStartSeed TEXT NOT NULL,"
            "unixepoch_timestamp INTEGER NOT NULL,"
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
        NULL);

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
            "enumeration_job_map_id INTEGER PRIMARY KEY," // sqlite assigns a value automagically to 'INTEGER PRIMARY KEY's
            "parent_id INTEGER REFERENCES jobs(job_id) ON DELETE CASCADE,"
            "child_id INTEGER REFERENCES jobs(job_id) ON DELETE CASCADE,"
            "UNIQUE(parent_id, child_id)"
        ");"
    );
    sqlenv_exec_with_callback_resulthandler(sqlenv, tjState.statementBuffer,
        NULL,
        NULL,
        &sql_resultHandler_print
        // NULL
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
        // &sql_resultHandler_print
        NULL
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
        // &sql_resultHandler_print
        NULL
    );
    tj_unlock();

    mpz_clear(state.anMpz);

    *jobCount = state.counter;

    // return true;
}

