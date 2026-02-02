#ifndef MPZ_DIGITS_H
#define MPZ_DIGITS_H

// int* digits is -1 terminated.
// this is ok because digits are supposed to be 0 or positive anyways.

int* digits_malloc(int n);
void digits_free(int* digits);
void digits_print(int* digits);
int digits_len(int* digits);
void digits_zero(int* digits);

// this converts an mpzstr in base base into an integer rop
// void mpz_set_digits(mpz_t rop, int* digits, int base);

// // this converts an integer op into an mpzstr of digits in base base
// void mpz_get_digits(int* rdigits, int base, mpz_t op);

#endif