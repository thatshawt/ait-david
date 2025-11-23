#include "turing_threading.h"

turing_thread_info_t turing_global_thread_info;

void turing_threading_init_global()
{
    for(int i=0;i<TM_MAX_THREADS;i++){
        turing_global_thread_info.index_is_null[i] = true;
    }
    pthread_mutex_init(&turing_global_thread_info.mutex, NULL);
}

void turing_threading_destroy()
{
    pthread_mutex_destroy(&turing_global_thread_info.mutex);
}

void turing_threading_self_remove()
{
    pthread_mutex_lock(&turing_global_thread_info.mutex);
    const int i = turing_threading_self_index();
    turing_global_thread_info.index_is_null[i] = true;
    pthread_mutex_unlock(&turing_global_thread_info.mutex);
}

void turing_threading_self_init()
{
    pthread_t selfID = pthread_self();
    pthread_mutex_lock(&turing_global_thread_info.mutex);
    for(int i=0;i<TM_MAX_THREADS;i++){
        if(turing_global_thread_info.index_is_null[i]){
            turing_global_thread_info.pthread_index_map[i] = selfID;
            turing_global_thread_info.index_is_null[i] = false;
            break;
        }
    }
    pthread_mutex_unlock(&turing_global_thread_info.mutex);
}

int turing_threading_self_index()
{
    pthread_t selfID = pthread_self();
    pthread_mutex_lock(&turing_global_thread_info.mutex);
    for(int i=0;i<TM_MAX_THREADS;i++){
        if(turing_global_thread_info.index_is_null[i])continue;
        
        pthread_t ithThread = turing_global_thread_info.pthread_index_map[i];
        if(pthread_equal(ithThread, selfID)){
            pthread_mutex_unlock(&turing_global_thread_info.mutex);
            return i;
        }
    }
    pthread_mutex_unlock(&turing_global_thread_info.mutex);
    return -1;
}

