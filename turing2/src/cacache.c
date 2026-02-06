#include "cacache.h"

cacache_state_t cacheState;

void cacache_init()
{
    mpz_init(cacheState.maxEntryDigit);
    cacheState.maxDigitEntryHash = -1;
}

void cacache_destroy()
{
    mpz_clears(cacheState.maxEntryDigit, 
        NULL);
}

bool cacache_check_item(cacahe_item_t itemType, int hash)
{
    switch(itemType){
        case CACACHE_ENTRY_MAX_DIGIT:
            return hash == cacheState.maxDigitEntryHash;
    }
}

void cacache_set_item(cacahe_item_t itemType, int hash, void* item)
{
    switch(itemType){
        case CACACHE_ENTRY_MAX_DIGIT:
            // gmp_printf("set as %Zd\n", *(mpz_t*)item);
            mpz_set(cacheState.maxEntryDigit, *((mpz_t*)item));
            cacheState.maxDigitEntryHash = hash;
            break;
    }
}

void* cacache_get_item(cacahe_item_t itemType)
{
    switch(itemType){
        case CACACHE_ENTRY_MAX_DIGIT:
            return &cacheState.maxEntryDigit;
    }
}