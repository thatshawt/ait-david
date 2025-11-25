#include "turing_threading.h"

turing_thread_info_t turing_global_thread_info;

void ttlock(){
    ttlock(&turing_global_thread_info.mutex);
}

void ttunlock(){
    ttunlock(&turing_global_thread_info.mutex);
}

void turing_threading_init_global()
{
    for(int i=0;i<TM_MAX_THREADS;i++){
        turing_global_thread_info.index_is_null[i] = true;
    }
    pthread_mutex_init(&turing_global_thread_info.mutex, NULL);
    turing_global_thread_info.config_workThreadsCount = 1;
}

void turing_threading_set_workthreads_count(int workthreads)
{
    ttlock();

    turing_global_thread_info.config_workThreadsCount = workthreads;

    ttunlock();
}

int turing_threading_get_workthreads_count()
{
    return turing_global_thread_info.config_workThreadsCount;
}

void turing_threading_destroy()
{
    pthread_mutex_destroy(&turing_global_thread_info.mutex);
}

void turing_threading_self_remove()
{
    ttlock();
    const int i = turing_threading_self_index();
    turing_global_thread_info.index_is_null[i] = true;
    ttunlock();
}

void turing_threading_self_init()
{
    pthread_t selfID = pthread_self();
    ttlock();
    for(int i=0;i<TM_MAX_THREADS;i++){
        if(turing_global_thread_info.index_is_null[i]){
            turing_global_thread_info.pthread_index_map[i] = selfID;
            turing_global_thread_info.index_is_null[i] = false;
            break;
        }
    }
    ttunlock();
}

int turing_threading_self_index()
{
    pthread_t selfID = pthread_self();
    ttlock();
    for(int i=0;i<TM_MAX_THREADS;i++){
        if(turing_global_thread_info.index_is_null[i])continue;
        
        pthread_t ithThread = turing_global_thread_info.pthread_index_map[i];
        if(pthread_equal(ithThread, selfID)){
            ttunlock();
            return i;
        }
    }
    ttunlock();
    return -1;
}

