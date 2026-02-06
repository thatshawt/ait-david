#ifndef GMP_MPZSTR
#define GMP_MPZSTR

#include <gmp.h>

// mpzstr is an string of mpz_t's followed by one mpz_t at the end with mpz->_mp_d == NULL.
// the last mpz_t is like the null terminator.
mpz_t* mpzstr_init_malloc(int n);
mpz_t* mpzstr_init2_malloc(int n, int bits);
void mpzstr_clear_free(mpz_t* mpzstr);
int mpzstr_len(mpz_t* mpzstr);
mpz_t* mpzstr_clone(mpz_t* mpzstr);
void mpzstr_copy(mpz_t* mpzstrDest, mpz_t* mpzstrSrc);

// i am assuming the mpz has not been initialized yet. or at least cleared.
void mpz_null_terminate(mpz_t* mpz);
bool mpz_is_null_terminated(mpz_t* mpz);

void mpzstr_set_zero(mpz_t* mpzstr);

void mpzstr_print(mpz_t* mpzstr);


// this treats an mpzstr as a string of digits whose values can be any integer but should but in the specified base.
// for example, if you choose base 7 and your string is 3,5,10,20 the digits will overflow/underflow into their proper places until every digit is within 7. the result should be 3,6,5,6.
// this also works with negative digits and represents subtraction for example 3,5,10,-100 in base 7 turns into 3,4,2,5.
// this is useful for when you want to add/subtract to the digits and then make sure the digits still represents valid digits in a certain base.
// returns true if overflowed/underflowed past most significant digit of rmpzstr.
bool mpzstr_basenorm(mpz_t* opmpzstr, int digits, int base, mpz_t* carry);

// this converts an mpzstr in base base into an integer rop
void mpz_set_mpzstr(mpz_t rop, mpz_t* mpzstr, int base);

// this converts an integer op into an mpzstr of digits in base base
void mpz_get_mpzstr(mpz_t* rmpzstr, int base, mpz_t op);

#endif