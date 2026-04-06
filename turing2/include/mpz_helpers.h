#ifndef MPZ_HELPERS_H
#define MPZ_HELPERS_H

#include <gmp.h>
#include <stdbool.h>

bool lincrement_int(int n, int* sizes, int* digits);
bool ldecrement_int(int n, int* sizes, int* digits);
bool rincrement_int(int n, int* sizes, int* digits);

// various mpz bit manipulating funcs
void mpz_lshift(mpz_t rop, mpz_t number, int n);
void mpz_rshift(mpz_t rop, mpz_t number, int n);
void mpz_load_number_of_n_ones(mpz_t rop, int n);
void mpz_ior_bits_lshift(mpz_t rop, mpz_t temp, mpz_t bits, mp_bitcnt_t biti);
void mpz_pop_nbits_right(mpz_t bits, mpz_t number, mp_bitcnt_t bitsN);
int mpz_count_leading_ones(mpz_t numberWithLeadingOnes);

// cantor's [un]pairing function
// see readings/ElegantPairing.pdf
// bijective map between pairs of natural numbers and the natural numbers.
// (a,b) <-> c.
void mpz_cantor_pair(mpz_t rz, mpz_t x, mpz_t y);
void mpz_cantor_pair_temps(mpz_t rz, mpz_t x, mpz_t y, mpz_t t1);
void mpz_cantor_pair_ui_temps(mpz_t rz, int intX, int intY, mpz_t t1, mpz_t x, mpz_t y);
void mpz_cantor_unpair(mpz_t rx, mpz_t ry, mpz_t z);
void mpz_cantor_unpair_temps(mpz_t rx, mpz_t ry, mpz_t z, mpz_t i);

unsigned long long mpz_cantor_pair_ui_ui(unsigned long long x, unsigned long long y);
void mpz_cantor_unpair_ui_ui(unsigned long long* rx, unsigned long long* ry, unsigned long long z);

void mpz_cantor_pair_mpzstr(mpz_t rz, mpz_t* mpzstr);
void mpz_cantor_pair_mpzstr_temps(mpz_t rz, mpz_t* mpzstr, mpz_t poo1, mpz_t poo2);
void mpz_cantor_unpair_mpzstr(mpz_t* rmpzstr, mpz_t z, int n);
void mpz_cantor_unpair_mpzstr_temps(mpz_t* rmpzstr, mpz_t z, int n, mpz_t poo1, mpz_t poo2, mpz_t poo3);

// see readings/ElegantPairing.pdf
// bijective map between pairs of natural numbers and the natural numbers.
// (a,b) <-> c.
// i have this one alongside the cantor pairing because the numbers increase in a different
// pattern than cantor's. i think its better for enumerating the natural numbers.
void mpz_elegant_pair(mpz_t rz, mpz_t x, mpz_t y);
void mpz_elegant_pair_temps(mpz_t rz, mpz_t x, mpz_t y);
void mpz_elegant_unpair(mpz_t rx, mpz_t ry, mpz_t z);
void mpz_elegant_unpair_temps(mpz_t rx, mpz_t ry, mpz_t z, mpz_t left, mpz_t right);

unsigned long long mpz_elegant_pair_ui_ui(unsigned long long x, unsigned long long y);
void mpz_elegant_unpair_ui_ui(unsigned long long* rx, unsigned long long* ry, unsigned long long z);

void mpz_elegant_pair_mpzstr(mpz_t rz, mpz_t* mpzstr);
void mpz_elegant_pair_mpzstr_temps(mpz_t rz, mpz_t* mpzstr, mpz_t poo1);
void mpz_elegant_unpair_mpzstr(mpz_t* rmpzstr, mpz_t z, int n);
void mpz_elegant_unpair_mpzstr_temps(mpz_t* rmpzstr, mpz_t z, int n, mpz_t poo1, mpz_t poo2, mpz_t poo3, mpz_t poo4);


// bijective map between natural numbers and binary strings.
// see turing2/readings/hutter-sintro2kc.pdf page 7.
// these functions implement first three rows of that table
// describing the strings and natural numbers.
void mpz_prefix_index_get_bit_length(mpz_t prefixIndex, mpz_t bitlength);
void mpz_prefix_index_get_bit_length_temps(mpz_t x, mpz_t bitlength, mpz_t tempMpz);
void mpz_prefix_index_get_bit_integer(mpz_t prefixIndex, mpz_t bitinteger);
void mpz_prefix_index_get_bit_integer_temps(mpz_t x, mpz_t bitinteger, mpz_t tempMpz, mpz_t tempMpz2);
void mpz_get_prefix_index_from_int_and_length(mpz_t prefixIndex, mpz_t bitinteger, mpz_t bitlength);
void mpz_get_prefix_index_from_int_and_length_temps(mpz_t x, mpz_t bitinteger, mpz_t bitlength, mpz_t temp);

// bar encoding, see turing2/readings/hutter-sintro2kc.pdf
// page 7, its the x̄ code.
void mpz_bar_decode_left(mpz_t x, int* lengthx, mpz_t number_with_bar_encoded_left);
void mpz_bar_decode_left_pop(mpz_t x, int* lengthx, mpz_t number_with_bar_encoded_left);
void mpz_bar_encode(mpz_t bar_encoded, mpz_t x, int lengthX);
void mpz_bar_encode_temps(mpz_t bar_encoded, mpz_t x, int lengthX, mpz_t temp,mpz_t bits);

// apos encoding, see turing2/readings/hutter-sintro2kc.pdf
// page 7, its the x` code.
int mpz_apos_decode_left(mpz_t x, int* lengthx, mpz_t number_with_apos_encoded_left);
int mpz_apos_decode_prefix_index_left(mpz_t prefixIndex, mpz_t number_with_apos_encoded_left);
int mpz_apos_decode_prefix_index_left_temps(mpz_t prefixIndex, mpz_t number_with_apos_encoded_left,
    mpz_t xint, mpz_t temp, mpz_t temp2, mpz_t temp3, mpz_t temp4 
);
void mpz_apos_encode(mpz_t apos_encoded, mpz_t x, int lengthX);
void mpz_apos_encode_str(mpz_t apos_encoded, char* str);
void mpz_apos_encode_temps(mpz_t apos_encoded, mpz_t x, int lengthX, mpz_t temp, mpz_t temp2, mpz_t bits, mpz_t poo1, mpz_t poo2);
void mpz_apos_encode_temps_str(mpz_t apos_encoded, char* str, mpz_t temp, mpz_t temp2, mpz_t bits, mpz_t poo1, mpz_t poo2, mpz_t poo3);
void mpz_apos_encode_prefix_index(mpz_t apos_encoded, mpz_t prefixIndex);
void mpz_apos_encode_prefix_index_temps(mpz_t apos_encoded, mpz_t prefixIndex,
    mpz_t temp, mpz_t poo1, mpz_t poo2, mpz_t poo3, mpz_t poo4, mpz_t poo5
);


#endif