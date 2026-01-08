#include "turing_jobs.h"
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

// jobs
// AUTO UNIQUE INT job_id | PRIMARY(arg1 | arg2 | arg3 | ...)
static void tj_create_jobs_table(sqlenv_t* sqlenv)
{
    sprintf(tjState.statementBuffer,
        "CREATE TABLE jobs("
            "job_id INTEGER PRIMARY KEY," // sqlite assigns a value automagically if we dont
            "states INTEGER NOT NULL,"
            "start TEXT NOT NULL,"
            "length TEXT," // if its NULL it gets resolved eventually i think
            "max_steps TEXT NOT NULL,"
            "doOnesTape INTEGER NOT NULL," // this is a boolean
            "doZerosTape INTEGER NOT NULL," // this is a boolean
            "randomIterations TEXT NOT NULL,"
            "randomStartSeed TEXT NOT NULL,"
            "UNIQUE(states,start,length,max_steps,doOnesTape,doZerosTape,randomIterations,randomStartSeed)"
        ");"
    );
    // sprintf(statementBuffer, "CREATE TABLE %s(STR_ID TEXT PRIMARY KEY NOT NULL, COUNT TEXT NOT NULL);", tablename);
    sqlenv_exec_with_callback(sqlenv, tjState.statementBuffer,
        NULL,
        NULL
    );
}

void tj_create_tables(sqlenv_t* sqlenv)
{
    tj_create_jobs_table(sqlenv);
}

void tj_DANGEROUS_delete_tables(sqlenv_t* sqlenv)
{
    sprintf(tjState.statementBuffer,
        "DROP TABLE jobs;"
        // "DROP TABLE otherTable;"
        // ...
    );
    // sprintf(statementBuffer, "CREATE TABLE %s(STR_ID TEXT PRIMARY KEY NOT NULL, COUNT TEXT NOT NULL);", tablename);
    sqlenv_exec_with_callback(sqlenv, tjState.statementBuffer,
        NULL,
        NULL
    );
}

// returns job_id, -1 otherwise
unsigned long tj_create_job_args(sqlenv_t* sqlenv, enumerate_job_opt_t jobArgs);
//TODO: change function so its like this: "return tj_create_job_args(sqlenv, change_simple_to_hard_args(userJobArgs));"
unsigned long tj_create_job_simple_args(sqlenv_t* sqlenv, tm_enumerate_options_simple_t userJobArgs)
{
    tm_enumerate_options_simple_t jobArgs = userJobArgs;
    
    // make sure jobArgs is properly "resolved". (no NULL for the length).
    {
        // intermediate hard args
        enumerate_job_opt_t resolvedArgs;
        tm_enumerate_job_opt_init(&resolvedArgs);
        tm_resolve_simple_args_to_hard_args(jobArgs, &resolvedArgs);

        // turn the resolved "hard args" back into "simple args" that we can easily parse.
        tm_revert_hard_args_to_simple_args(resolvedArgs, &jobArgs);
        
        // we dont need these anymore
        tm_enumerate_job_opt_destroy(&resolvedArgs);
    }

    unsigned long result;
    tj_lock();
    // the curly braces are purely visual.
    { // the two sql calls need to execute one after another, hence the locking.
        sprintf(tjState.statementBuffer,
            "INSERT INTO"
            "jobs(states,start,length,max_steps,doOnesTape,doZerosTape,randomIterations,randomStartSeed)"
            "VALUES (%d, %s, %s, %s, %d, %d, %s, %s);",
            jobArgs.states, jobArgs.startIndex,
            jobArgs.indexesConsidered == NULL ? "NULL":jobArgs.indexesConsidered,
            jobArgs.max_steps, jobArgs.doOnesTape, jobArgs.doZerosTape,
            jobArgs.randomIterations, jobArgs.randomStartSeed
        );
        sqlenv_exec_with_callback(sqlenv, tj.statementBuffer, NULL, NULL);

        result = sqlenv_get_last_insert_rowid(sqlenv);
    }
    tj_unlock();
    
    // free the jobArgs cus we used tm_revert_hard_args_to_simple_args.
    tm_free_simple_args_made_from_revert_thing(&jobArgs);
    
    return result;
}

// returns job_id, -1 otherwise
unsigned long tj_get_job_id_from_args(sqlenv_t* sqlenv, enumerate_job_opt_t jobArgs);

// TODO; change this functoin so its like this:
// return tj_get_job_id_from_args(sqlenv, convert_simple_to_hard_args(jobArgs));
unsigned long tj_get_job_id_from_simple_args(sqlenv_t* sqlenv, tm_enumerate_options_simple_t jobArgs)
{
    mpz_t theMpz; mpz_init_set_ui(theMpz, -1);

    sprintf(tjState.statementBuffer,
        "SELECT job_id FROM jobs WHERE"
        "states=%d AND start=%s AND length=%s AND max_steps=%s AND doOnesTape=%d AND doZerosTape=%d AND randomIterations=%s AND randomStartSeed=%s;",
        jobArgs.states, jobArgs.startIndex,
        jobArgs.indexesConsidered == NULL ? "NULL":jobArgs.indexesConsidered,
        jobArgs.max_steps, jobArgs.doOnesTape, jobArgs.doZerosTape,
        jobArgs.randomIterations, jobArgs.randomStartSeed
    );
    sqlenv_exec_with_callback(sqlenv, tj.statementBuffer,
        &theMpz, &sql_callback_load_firstcol_into_mpz);

    unsigned long result = mpz_get_ui(theMpz);

    mpz_clear(theMpz);

    return sqlenv->rq == SQLITE_OK ? result : -1;
}

int tj_sql_callback_load_into_jobargs(void *data, int count, char **values, char **columnNames){
    enumerate_job_opt_t* jobArgs = (enumerate_job_opt_t*)data;

    mpz_t anMpz; mpz_init(anMpz);

    for(int i=0;i<count;i++){
        const char* value = values[i];
        const char* column = columnNames[i];

        if(strcmp(column,"states") == 0){
            mpz_set_str(&anMpz, value, 10);
            jobArgs->states = mpz_get_ui(anMpz);

        }else if(strcmp(column,"start") == 0){
            mpz_set_str(&jobArgs->start, value, 10);

        }else if(strcmp(column,"length") == 0){
            mpz_set_str(&jobArgs->length, value, 10);

        }else if(strcmp(column,"max_steps") == 0){
            mpz_set_str(&jobArgs->max_steps, value, 10);

        }else if(strcmp(column,"doOnesTape") == 0){
            mpz_set_str(&anMpz, value, 10);
            jobArgs->doOnesTape = mpz_get_ui(anMpz);

        }else if(strcmp(column,"doZerosTape") == 0){
            mpz_set_str(&anMpz, value, 10);
            jobArgs->doZerosTape = mpz_get_ui(anMpz);

        }else if(strcmp(column,"randomIterations") == 0){
            mpz_set_str(&jobArgs->randomIterations, value, 10);

        }else if(strcmp(column,"randomStartSeed") == 0){
            mpz_set_str(&jobArgs->randomStartSeed, value, 10);

        }
    }

    mpz_clear(anMpz);
    return 0;
}

bool tj_get_job_args(sqlenv_t* sqlenv, unsigned long jobId, enumerate_job_opt_t* jobArgs)
{

}