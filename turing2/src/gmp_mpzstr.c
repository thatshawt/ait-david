#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "gmp_mpzstr.h"

#define MIN(a,b) ((a)<(b) ? (a):(b))
#define MAX(a,b) ((a)>(b) ? (a):(b))

// the mpz should not be initialized by mpz for this to work properly.
inline void mpz_null_terminate(mpz_t* mpz)
{
    (*mpz)->_mp_d = NULL;
}

inline bool mpz_is_null_terminated(mpz_t* mpz)
{
    return (*mpz)->_mp_d == NULL;
}

mpz_t* mpzstr_clone(mpz_t* mpzstr)
{
    const int n = mpzstr_len(mpzstr);
    mpz_t* result = mpzstr_init_malloc(n);

    for(int i=0;i<n;i++){
        mpz_set(*(result+i), *(mpzstr+i));
    }

    return result;
}

// copies right-to-left respecting the index of digits.
void mpzstr_copy(mpz_t* mpzstrDest, mpz_t* mpzstrSrc)
{
    const int destLen = mpzstr_len(mpzstrDest);
    const int srcLen = mpzstr_len(mpzstrSrc);
    for(int i=0; i<MIN(destLen,srcLen); i++){
        mpz_set(*(mpzstrDest+destLen-i-1), *(mpzstrSrc+srcLen-i-1));
    }
}

mpz_t* mpzstr_get_i_left(mpz_t* mpzstr, int i)
{
    return (mpzstr+i);
}

mpz_t* mpzstr_get_i_right(mpz_t* mpzstr, int i)
{
    const int len = mpzstr_len(mpzstr);
    return (mpzstr+(len-i-1));
}

mpz_t* mpzstr_init_malloc(int n)
{
    mpz_t* mpzstr = (mpz_t*)malloc(sizeof(mpz_t)*(n+1));

    for(int i=0;i<n;i++){
        mpz_init(*(mpzstr+i));
    }
    // *(mpzstr+n) = NULL; // null terminate it
    mpz_t* lastMpz = mpzstr+n;
    mpz_null_terminate(lastMpz); // not sure if this is a thing but... yea im using it

    return mpzstr;
}

mpz_t* mpzstr_init2_malloc(int n, int bits)
{
    mpz_t* mpzstr = (mpz_t*)malloc(sizeof(mpz_t)*(n+1));

    for(int i=0;i<n;i++){
        mpz_init2(*(mpzstr+i), bits);
    }
    // *(mpzstr+n) = NULL; // null terminate it
    mpz_t* lastMpz = mpzstr+n;
    mpz_null_terminate(lastMpz); // not sure if this is a thing but... yea im using it

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
    while(!mpz_is_null_terminated(p))p++;

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

void mpzstr_set_ints_right(mpz_t* mpzstr, int* ints, int n)
{
    const int len = mpzstr_len(mpzstr);
    for(int i=0; i<n; i++){
        mpz_set_si(*(mpzstr+len-i-1), ints[n-i-1]);
    }
}

void mpzstr_print(mpz_t* mpzstr)
{
    const int n = mpzstr_len(mpzstr);
    printf("(");
    for(int i=0; i<n; i++){
        mpz_t* theMpz = mpzstr+i;
        if(i==n-1) gmp_printf("%Zd)\n", *theMpz);
        else gmp_printf("%Zd, ", *theMpz);
    }
}

void mpzstr_sprint(char* buff, mpz_t* mpzstr)
{
    const int n = mpzstr_len(mpzstr);
    for(int i=0; i<n; i++){
        char tempBuff[300] = {0};
        mpz_t* theMpz = mpzstr+i;
        if(i==n-1) gmp_sprintf(tempBuff, "%Zd", *theMpz);
        else gmp_sprintf(tempBuff, "%Zd, ", *theMpz);

        strcat(buff, tempBuff);
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

bool mpzstr_basenorm_temps(mpz_t* opmpzstr, int digits, int base, mpz_t* carry, mpz_t poo1, mpz_t poo2)
{
    // mpz_t* tempStr = mpzstr_clone(opmpzstr);
    mpz_t* tempStr = opmpzstr;

    const int mpzstrLen = mpzstr_len(tempStr);
    const int n = MIN(digits, mpzstrLen);

    for(int i=0; i<n; i++){
        const int currentIndex = n-i-1;
        const int nextIndex = n-i-2;

        mpz_t* currentDigit = tempStr+currentIndex;

        // gmp_printf("currentdigit %Zd\n", *currentDigit);
        
        // digit overflow
        if(mpz_cmp_ui(*currentDigit, base) >= 0){
            // printf("overflow\n");
            mpz_t* nextDigit = tempStr+nextIndex;
            if(nextIndex < 0)nextDigit = carry; // nextIndex<0 == overflowed

            mpz_fdiv_q_ui(poo1, *currentDigit, base);
            mpz_fdiv_r_ui(*currentDigit, *currentDigit, base);
            if(nextDigit != NULL)mpz_add(*nextDigit, *nextDigit, poo1);
            if(nextIndex < 0)return true;

            // gmp_printf("poo1 %Zd. nextDigit overflowed to %Zd\n", poo1, *nextDigit);
        
        // digit underflow
        }else if(mpz_cmp_si(*currentDigit, -1) <= 0){
            // printf("underflow\n");
            mpz_t* nextDigit = tempStr+nextIndex;
            if(nextIndex < 0)nextDigit = carry; // nextIndex<0 == underflowed

            mpz_set_si(poo2, -base);
            mpz_cdiv_q(poo1, *currentDigit, poo2);
            mpz_cdiv_r(*currentDigit, *currentDigit, poo2);
            if(nextDigit != NULL)mpz_sub(*nextDigit, *nextDigit, poo1);
            if(nextIndex < 0)return true;

            // gmp_printf("poo1 %Zd. current %Zd. nextDigit %Zd\n", poo1, *currentDigit, *nextDigit);
        }
    }

    // mpzstr_copy(rmpzstr, tempStr);

    // mpzstr_clear_free(tempStr);
    // mpzstr_clear_free(poos);

    return false;
}

bool mpzstr_basenorm(mpz_t* opmpzstr, int digits, int base, mpz_t* carry)
{
    mpz_t* poos = mpzstr_init_malloc(2);
    mpz_t* poo1 = poos+0;
    mpz_t* poo2 = poos+1;
    
    bool result = mpzstr_basenorm_temps(opmpzstr, digits, base, carry, *poo1, *poo2);

    mpzstr_clear_free(poos);

    return result;
}