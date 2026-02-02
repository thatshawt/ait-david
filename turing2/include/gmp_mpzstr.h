#ifndef GMP_MPZSTR
#define GMP_MPZSTR

#include <gmp.h>

// mpzstr is an string of mpz_t's followed by one mpz_t at the end with mpz->_mp_d == NULL.
// the last mpz_t is like the null terminator.
mpz_t* mpzstr_init_malloc(int n);
void mpzstr_clear_free(mpz_t* mpzstr);
int mpzstr_len(mpz_t* mpzstr);

void mpzstr_set_zero(mpz_t* mpzstr);

void mpzstr_print(mpz_t* mpzstr);

// this converts an mpzstr in base base into an integer rop
void mpz_set_mpzstr(mpz_t rop, mpz_t* mpzstr, int base);

// this converts an integer op into an mpzstr of digits in base base
void mpz_get_mpzstr(mpz_t* rmpzstr, int base, mpz_t op);

#endif