#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "gmp_mpzstr.h"

mpz_t* mpzstr_init_malloc(int n)
{
    mpz_t* mpzstr = (mpz_t*)malloc(sizeof(mpz_t)*(n+1));

    for(int i=0;i<n;i++){
        mpz_init(*(mpzstr+i));
    }
    // *(mpzstr+n) = NULL; // null terminate it
    mpz_t* lastMpz = mpzstr+n;
    (*lastMpz)->_mp_d = NULL; // not sure if this is a thing but... yea im using it

    return mpzstr;
}

void mpzstr_clear_free(mpz_t* mpzstr)
{
    const int n = mpzstr_len(mpzstr);

    for(int i=0; i<n; i++){
        mpz_clear(*(mpzstr+i));
    }

    free(mpzstr);
}

int mpzstr_len(mpz_t* mpzstr)
{
    mpz_t* p = mpzstr;
    while((*p)->_mp_d != NULL)p++;

    int length = (int)((size_t)p - (size_t)mpzstr)/sizeof(mpz_t);
    // printf("mpzstr_len = %d\n", length);
    return length;
}

void mpzstr_set_zero(mpz_t* mpzstr)
{
    const int n = mpzstr_len(mpzstr);
    for(int i=0; i<n; i++){
        mpz_set_ui(*(mpzstr+i), 0);
        // printf("zeroed mpzstr+%d\n", i);
    }
}

void mpzstr_print(mpz_t* mpzstr)
{
    const int n = mpzstr_len(mpzstr);
    for(int i=0; i<n; i++){
        mpz_t* theMpz = mpzstr+i;
        if(i==n-1) gmp_printf("%Zd\n", *theMpz);
        else gmp_printf("%Zd, ", *theMpz);
    }
}

void mpz_set_mpzstr(mpz_t rop, mpz_t* mpzstr, int base)
{
    mpz_set_ui(rop, 0);

    mpz_t poo1;
    mpz_inits(poo1,NULL);

    const int n = mpzstr_len(mpzstr);

    // rightside has least significant digit
    for(int i=0; i<n; i++){
        int power = n-i-1;
        mpz_t* digit = mpzstr+i;

        mpz_ui_pow_ui(poo1, base, power);
        mpz_mul(poo1, poo1, *digit);
        mpz_add(rop, rop, poo1);
    }

    mpz_clears(poo1,NULL);
}

void mpz_get_mpzstr(mpz_t* digits, int littlebase, mpz_t index)
{
    const int n = mpzstr_len(digits);
    mpz_t multiplier, count, temp;
    mpz_init_set(count, index);
    mpz_init(temp);
    mpz_init_set_ui(multiplier, littlebase);

    int offset = n-1;

    // gmp_printf("start index=%Zd, count=%Zd, multiplier=%Zd\n", index, count, multiplier);
    
    mpz_t base; mpz_init(base);
    while(true){
        offset = n-1;
        mpz_set_ui(base, 1);
        if(mpz_cmp(count, multiplier) < 0){
            // digits[offset] = mpz_get_ui(count);
            mpz_set(*(digits+offset), count);
            mpz_clears(multiplier, base, count, temp, NULL);

            return;
        }
        
        while(mpz_cmp(base, count) <= 0){
            offset--;
            mpz_mul(base, base, multiplier);
        }
        mpz_fdiv_q(base, base, multiplier);
        offset++;

        mpz_fdiv_q(temp, count, base);
        
        // digits[offset] = mpz_get_ui(temp);
        mpz_set(*(digits+offset), temp);

        // gmp_printf("added digit %d, offset=%d\n", digits[offset], offset);

        mpz_mul(temp, temp, base);

        mpz_sub(count, count, temp);

    }

}
