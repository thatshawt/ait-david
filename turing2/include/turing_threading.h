#ifndef TURING_THREADING_H
#define TURING_THREADING_H

#include <pthread.h>
#include <stdbool.h>

#define TM_MAX_THREADS 256

typedef struct {
    pthread_mutex_t mutex;
    pthread_t pthread_index_map[TM_MAX_THREADS];
    bool index_is_null[TM_MAX_THREADS];

    int config_workThreadsCount;
} turing_thread_info_t;

void turing_threading_init_global();
void turing_threading_destroy();
void turing_threading_self_init();
int turing_threading_self_index();
void turing_threading_self_remove();

void turing_threading_set_workthreads_count(int workthreads);
int turing_threading_get_workthreads_count();

#endif