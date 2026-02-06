#ifndef CACACHE_H
#define CACACHE_H

#include <stdbool.h>
#include "turing_threading.h"
#include <gmp.h>

typedef enum {
    CACACHE_ENTRY_MAX_DIGIT
} cacahe_item_t;

typedef struct{
    unsigned int maxDigitEntryHash;
    mpz_t maxEntryDigit;

} cacache_state_t;

void cacache_init();
void cacache_destroy();

bool cacache_check_item(cacahe_item_t itemType, int hash);
void cacache_set_item(cacahe_item_t itemType, int hash, void* item);
void* cacache_get_item(cacahe_item_t itemType);




#endif