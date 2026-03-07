#include "mpz_helpers.h"
#include "gmp_mpzstr.h"
#include <mpfr.h>
#include <string.h>
#include <math.h>

inline void mpz_ior_bits_lshift(mpz_t rop, mpz_t temp, mpz_t bits, mp_bitcnt_t biti)
{
    // rop |= bits<<biti;
    mpz_lshift(temp, bits, biti);
    mpz_ior(rop, rop, temp);
}

inline void mpz_load_number_of_n_ones(mpz_t rop, int n)
{
    if(n < 0){
        mpz_set_ui(rop, 0);
    }else{
        mpz_set_ui(rop, 1);
        mpz_mul_2exp(rop, rop, n);
        mpz_sub_ui(rop, rop, 1);
    }
}

inline void mpz_lshift(mpz_t rop, mpz_t number, int n)
{
    mpz_mul_2exp(rop, number, n);
}

inline void mpz_rshift(mpz_t rop, mpz_t number, int n)
{
    mpz_fdiv_q_2exp(rop, number, n);
}

inline void mpz_pop_nbits_right(mpz_t bits, mpz_t number, mp_bitcnt_t bitsN)
{
    // bits = number & (2^bitsN - 1);
    mpz_load_number_of_n_ones(bits, bitsN);
    mpz_and(bits, bits, number);

    // number >> bitsN;
    mpz_rshift(number, number, bitsN);
}

int mpz_count_leading_ones(mpz_t numberWithLeadingOnes)
{
    if(mpz_cmp_ui(numberWithLeadingOnes, 0) == 0)
        return 0;

    int numberLength = mpz_sizeinbase(numberWithLeadingOnes,2);
    int count = 0;
    mpz_t temp; mpz_init(temp);
    mpz_t temp2; mpz_init(temp2);
    CHECK:
        count++;
        mpz_load_number_of_n_ones(temp, count);
        mpz_rshift(temp2, numberWithLeadingOnes, numberLength-count);
        // gmp_printf("count %d, temp %Zd, temp2: %Zd\n", count, temp, temp2);
        if(count == numberLength){
            goto FINISH;
        }else if(mpz_cmp(temp, temp2) == 0){
            goto CHECK;
        }else{
            count--;
        }


    FINISH:
        mpz_clears(temp, temp2, NULL);
        return count;
}

// got this from http://szudzik.com/ElegantPairing.pdf
// cantors pairing function
void mpz_cantor_pair_temps(mpz_t rz, mpz_t x, mpz_t y, mpz_t t1)
{
    // it fits into 64 bit unsigned long long or fits into 32 bits
    if((sizeof(unsigned long long) >= 64 && mpz_cmp_ui(x, 2600000000) < 0 && mpz_cmp_ui(y, 2600000000) < 0)
    || (sizeof(unsigned long long) >= 32 && mpz_cmp_ui(x, 42400) < 0 && mpz_cmp_ui(y, 42400) < 0)
    ){
        unsigned long long z = mpz_cantor_pair_ui_ui(mpz_get_ui(x),mpz_get_ui(y));
        mpz_set_ui(rz, z);
    // just do mpz otherwise
    }else
    {
        // rz = (x*x + 3x + 2xy + y + y*y)/2
        mpz_set_ui(rz, 0);

        mpz_set(t1, x);
        mpz_mul(t1, t1, t1);
        mpz_add(rz, rz, t1);

        mpz_mul_ui(t1, x, 3);
        mpz_add(rz, rz, t1);

        mpz_mul_ui(t1, x, 2);
        mpz_mul(t1, t1, y);
        mpz_add(rz, rz, t1);
        
        mpz_add(rz, rz, y);

        mpz_set(t1, y);
        mpz_mul(t1, t1, t1);
        mpz_add(rz, rz, t1);

        mpz_fdiv_q_ui(rz, rz, 2);
    }
}

void mpz_cantor_pair_ui_temps(mpz_t rz, int intX, int intY, mpz_t t1, mpz_t x, mpz_t y)
{
    mpz_set_ui(x, intX);
    mpz_set_ui(y, intY);

    mpz_cantor_pair_temps(rz, x, y, t1);
}

void mpz_cantor_pair(mpz_t rz, mpz_t x, mpz_t y)
{
    mpz_t t1; mpz_init(t1);

    mpz_cantor_pair_temps(rz, x, y, t1);

    mpz_clear(t1);
}

unsigned long long mpz_cantor_pair_ui_ui(unsigned long long x, unsigned long long y)
{
    // rz = (x*x + 3x + 2xy + y + y*y)/2
    return (x*x + 3*x + 2*x*y + y + y*y)/2;
}

void mpz_cantor_unpair_ui_ui(unsigned long long* rx, unsigned long long* ry, unsigned long long z)
{
    // i = fdiv_q_ui(-1 + sqrt(1 + 8z), 2);
    unsigned long long i = (-1 + (unsigned long long)sqrt(1 + 8*z))/2;
    
    // rx = z - (i*(1+i))/2
    *rx = z - (i*(1+i))/2;

    // ry = (i*(3+i))/2 - z
    *ry = (i*(3+i))/2 - z;
}

void mpz_cantor_unpair_temps(mpz_t rx, mpz_t ry, mpz_t z, mpz_t i)
{
    // if z fits into an unsigned long long
    if(mpz_cmp_ui(z, (unsigned long long)-1) < 0){
        unsigned long long rxInt, ryInt;
        mpz_cantor_unpair_ui_ui(&rxInt, &ryInt, mpz_get_ui(z));
        mpz_set_ui(rx, rxInt);
        mpz_set_ui(ry, ryInt);
    }else
    { //otherwise do the mpz version
        // i = fdiv_q_ui(-1 + sqrt(1 + 8z), 2);
        mpz_mul_ui(i, z, 8);
        mpz_add_ui(i, i, 1);
        mpz_sqrt(i, i);
        mpz_sub_ui(i, i, 1);
        mpz_fdiv_q_ui(i, i, 2);

        // rx = z - (i*(1+i))/2
        mpz_add_ui(rx, i, 1);
        mpz_mul(rx, rx, i);
        mpz_fdiv_q_ui(rx, rx, 2);
        mpz_sub(rx, z, rx);

        // ry = (i*(3+i))/2 - z
        mpz_add_ui(ry, i, 3);
        mpz_mul(ry, ry, i);
        mpz_fdiv_q_ui(ry, ry, 2);
        mpz_sub(ry, ry, z);
    }
}
void mpz_cantor_unpair(mpz_t rx, mpz_t ry, mpz_t z)
{
    mpz_t i; mpz_init(i);

    mpz_cantor_unpair_temps(rx, ry, z, i);

    mpz_clear(i);
}

void mpz_cantor_pair_mpzstr(mpz_t rz, mpz_t* mpzstr)
{

    mpz_t* poos = mpzstr_init_malloc(2);
    mpz_t* poo1 = (poos+0);
    mpz_t* poo2 = (poos+1);

    mpz_cantor_pair_mpzstr_temps(rz, mpzstr, *poo1, *poo2);

    mpzstr_clear_free(poos);

}
void mpz_cantor_pair_mpzstr_temps(mpz_t rz, mpz_t* mpzstr, mpz_t poo1, mpz_t poo2)
{
    const int n = mpzstr_len(mpzstr);

    if(n < 2){
        // we need 2 at least...
        mpz_set_si(rz, -1);
        return;
    }

    //pair the first two
    mpz_cantor_pair_temps(rz, *(mpzstr_get_i_left(mpzstr, 0)), *(mpzstr_get_i_left(mpzstr, 1)), poo1);

    //pair the rest
    for(int i=2;i<n;i++){
        // mpz_t* digit = mpzstr_get_i_left(mpzstr, i);
        mpz_cantor_pair_temps(poo1, rz, *(mpzstr_get_i_left(mpzstr, i)), poo2);
        mpz_set(rz, poo1);
    }
}

void mpz_cantor_unpair_mpzstr(mpz_t* rmpzstr, mpz_t z, int n)
{
    mpz_t* poos = mpzstr_init_malloc(3);
    mpz_t* poo1 = (poos+0);
    mpz_t* poo2 = (poos+1);
    mpz_t* poo3 = (poos+2);

    mpz_cantor_unpair_mpzstr_temps(rmpzstr, z, n, *poo1,*poo2,*poo3);

    mpzstr_clear_free(poos);
}
void mpz_cantor_unpair_mpzstr_temps(mpz_t* rmpzstr, mpz_t z, int n, mpz_t poo1, mpz_t poo2, mpz_t poo3)
{
    const int len = mpzstr_len(rmpzstr);

    if(len < n || n < 2){
        // the mpzstr cant fit the desired amount
        // or there are less than 2 
        mpzstr_set_zero(rmpzstr);
        return;
    }

    // start with z
    mpz_set(poo2, z);
    
    for(int i=n-3;i>=0;i--){
        mpz_cantor_unpair_temps(poo1, *(mpzstr_get_i_left(rmpzstr, i)), poo2, poo3);
        mpz_set(poo2, poo1);
    }
    //do the final pair
    mpz_cantor_unpair_temps(*(mpzstr_get_i_left(rmpzstr, 0)), *(mpzstr_get_i_left(rmpzstr, 1)), poo2, poo3);

}

void mpz_elegant_pair_temps(mpz_t rz, mpz_t x, mpz_t y)
{
    /*
    if x >= y:
        return x*x + x + y
    else:
        return y*y + x
    */

    // it fits into 64 bit unsigned long long or fits into 32 bits
    if((sizeof(unsigned long long) >= 64 && mpz_cmp_ui(x, 4000000000) < 0 && mpz_cmp_ui(y, 4000000000) < 0)
    || (sizeof(unsigned long long) >= 32 && mpz_cmp_ui(x, 60000) < 0 && mpz_cmp_ui(y, 60000) < 0)
    ){
        unsigned long long z = mpz_elegant_pair_ui_ui(mpz_get_ui(x),mpz_get_ui(y));
        mpz_set_ui(rz, z);
    // just do mpz otherwise
    }else{
        if(mpz_cmp(x,y) >= 0){
            mpz_set_ui(rz, 1);

            mpz_mul(rz, rz, x);
            mpz_mul(rz, rz, x);

            mpz_add(rz, rz, x);

            mpz_add(rz, rz, y);
        }else{
            mpz_set_ui(rz, 1);

            mpz_mul(rz, rz, y);
            mpz_mul(rz, rz, y);

            mpz_add(rz, rz, x);
        }
    }
}
void mpz_elegant_pair(mpz_t rz, mpz_t x, mpz_t y)
{
    mpz_elegant_pair_temps(rz, x, y);
}


void mpz_elegant_unpair_temps(mpz_t rx, mpz_t ry, mpz_t z, mpz_t left, mpz_t right)
{
    /*  
    left = z - int(math.sqrt(z))*int(math.sqrt(z))
    right = int(math.sqrt(z))
    
    if left < right:
        return (left,right)
    else:
        return (right, left-right)
    */

    // if z fits into an unsigned long long
    if(mpz_cmp_ui(z, (unsigned long long)-1) < 0){
        unsigned long long rxInt, ryInt;
        mpz_elegant_unpair_ui_ui(&rxInt, &ryInt, mpz_get_ui(z));
        mpz_set_ui(rx, rxInt);
        mpz_set_ui(ry, ryInt);
    }else{
        mpz_sqrt(left, z);
        mpz_mul(left, left, left);
        mpz_sub(left, z, left);

        mpz_sqrt(right, z);

        if(mpz_cmp(left, right) < 0){
            mpz_set(rx, left);
            mpz_set(ry, right);
        }else{
            mpz_set(rx, right);
            mpz_sub(ry, left, right);
        }
    }
}

void mpz_elegant_unpair(mpz_t rx, mpz_t ry, mpz_t z)
{
    mpz_t* poo = mpzstr_init_malloc(2);
    mpz_t* left = poo+0;
    mpz_t* right = poo+1;

    mpz_elegant_unpair_temps(rx, ry, z, *left, *right);

    mpzstr_clear_free(poo);
}

unsigned long long mpz_elegant_pair_ui_ui(unsigned long long x, unsigned long long y)
{
    /*
    if x >= y:
        return x*x + x + y
    else:
        return y*y + x
    */

    if(x >= y)
        return x*x + x + y;
    else
        return y*y + x;
}

void mpz_elegant_unpair_ui_ui(unsigned long long* rx, unsigned long long* ry, unsigned long long z)
{
    /*  
    left = z - int(math.sqrt(z))*int(math.sqrt(z))
    right = int(math.sqrt(z))
    
    if left < right:
        return (left,right)
    else:
        return (right, left-right)
    */
    unsigned long long left = z - (unsigned long long)(sqrt(z)) * (unsigned long long)(sqrt(z));
    unsigned long long right = (unsigned long long)(sqrt(z));

    if(left < right){
        *rx = left;
        *ry = right;
    }else{
        *rx = right;
        *ry = left-right;
    }
}

void mpz_elegant_pair_mpzstr_temps(mpz_t rz, mpz_t* mpzstr, mpz_t poo1)
{
    const int n = mpzstr_len(mpzstr);

    if(n < 2){
        // we need 2 at least...
        mpz_set_si(rz, -1);
        return;
    }

    //pair the first two
    mpz_elegant_pair_temps(rz, *(mpzstr_get_i_left(mpzstr, 0)), *(mpzstr_get_i_left(mpzstr, 1)));

    //pair the rest
    for(int i=2;i<n;i++){
        // mpz_t* digit = mpzstr_get_i_left(mpzstr, i);
        mpz_elegant_pair_temps(poo1, rz, *(mpzstr_get_i_left(mpzstr, i)));
        mpz_set(rz, poo1);
    }
}
void mpz_elegant_pair_mpzstr(mpz_t rz, mpz_t* mpzstr)
{
    mpz_t* temps = mpzstr_init_malloc(1);

    mpz_elegant_pair_mpzstr_temps(rz, mpzstr, *(temps+0));

    mpzstr_clear_free(temps);
}

void mpz_elegant_unpair_mpzstr_temps(mpz_t* rmpzstr, mpz_t z, int n, mpz_t poo1, mpz_t poo2, mpz_t poo3, mpz_t poo4)
{
    const int len = mpzstr_len(rmpzstr);

    if(len < n || n < 2){
        // the mpzstr cant fit the desired amount
        // or there are less than 2 
        mpzstr_set_zero(rmpzstr);
        return;
    }

    // start with z
    mpz_set(poo2, z);
    
    for(int i=n-3;i>=0;i--){
        mpz_elegant_unpair_temps(poo1, *(mpzstr_get_i_left(rmpzstr, i)), poo2, poo3, poo4);
        mpz_set(poo2, poo1);
    }
    //do the final pair
    mpz_elegant_unpair_temps(*(mpzstr_get_i_left(rmpzstr, 0)), *(mpzstr_get_i_left(rmpzstr, 1)), poo2, poo3, poo4);
}
void mpz_elegant_unpair_mpzstr(mpz_t* rmpzstr, mpz_t z, int n)
{
    mpz_t* temps = mpzstr_init_malloc(4);

    mpz_elegant_unpair_mpzstr_temps(rmpzstr, z, n, *(temps+0),*(temps+1),*(temps+2),*(temps+3));

    mpzstr_clear_free(temps);
}


// x is the prefix index
// bitlength = floor(log(x+1)/log(2))
void mpz_prefix_index_get_bit_length_temps(mpz_t x, mpz_t bitlength, mpz_t tempMpz)
{
    // mpz_t tempMpz; mpz_init(tempMpz);

    mpz_set(tempMpz, x);
    mpz_add_ui(tempMpz, tempMpz, 1);

    unsigned long neededprecisionBits = mpz_sizeinbase(tempMpz,2);
    mpfr_t tempMpfr; mpfr_init2(tempMpfr, neededprecisionBits);

    mpfr_set_z(tempMpfr, tempMpz, MPFR_RNDZ);
    mpfr_log2(tempMpfr, tempMpfr, MPFR_RNDZ);

    mpfr_get_z(bitlength, tempMpfr, MPFR_RNDZ);

    // mpz_clear(tempMpz);
    mpfr_clear(tempMpfr);
}

void mpz_prefix_index_get_bit_length(mpz_t x, mpz_t bitlength)
{
    mpz_t tempMpz; mpz_init(tempMpz);

    mpz_prefix_index_get_bit_length_temps(x, bitlength, tempMpz);

    mpz_clear(tempMpz);
}

// bitinteger = x - 2^bitlength - 1
// bitinteger = x - 2^floor(log(x+1)/log(2)) - 1
// x is the prefix index
void mpz_prefix_index_get_bit_integer_temps(mpz_t x, mpz_t bitinteger, mpz_t tempMpz, mpz_t tempMpz2)
{
    // mpz_t tempMpz; mpz_init(tempMpz);

    mpz_prefix_index_get_bit_length_temps(x, tempMpz, tempMpz2);
    mpz_set_ui(bitinteger, 1);
    mpz_mul_2exp(bitinteger, bitinteger, mpz_get_ui(tempMpz));
    mpz_sub_ui(bitinteger, bitinteger, 1);

    mpz_sub(bitinteger, x, bitinteger);

    // mpz_clear(tempMpz);
}
void mpz_prefix_index_get_bit_integer(mpz_t x, mpz_t bitinteger)
{
    mpz_t poo1,poo2; mpz_inits(poo1,poo2,NULL);

    mpz_prefix_index_get_bit_integer_temps(x, bitinteger, poo1,poo2);

    mpz_clears(poo1,poo2,NULL);
}

// x = 2^bitlength + bitinteger - 1
inline void mpz_get_prefix_index_from_int_and_length_temps(mpz_t x, mpz_t bitinteger, mpz_t bitlength, mpz_t temp)
{
    // mpz_t temp; mpz_init(temp);

    mpz_set_ui(temp, 1);
    mpz_mul_2exp(temp, temp, mpz_get_ui(bitlength));
    mpz_add(temp, temp, bitinteger);
    mpz_sub_ui(x, temp, 1);

    // mpz_clear(temp);
}

inline void mpz_get_prefix_index_from_int_and_length(mpz_t x, mpz_t bitinteger, mpz_t bitlength)
{
    mpz_t temp; mpz_init(temp);

    mpz_get_prefix_index_from_int_and_length_temps(x, bitinteger, bitlength, temp);

    mpz_clear(temp);
}

// bar_encode(x) = 1^ℓ(x) 0 x
// x is treated as a binary string of length lengthX
void mpz_bar_encode_temps(mpz_t bar_encoded, mpz_t x, int lengthX, mpz_t temp,mpz_t bits)
{
    // mpz_t temp; mpz_init(temp);
    // mpz_t bits; mpz_init(bits);
    mp_bitcnt_t bitsi = 0;
    
    // zero bar_encoded to start off.
    mpz_set_ui(bar_encoded, 0);

    // encode x
    mpz_set(bits, x);
    mpz_ior_bits_lshift(bar_encoded, temp, bits, bitsi);
    // bitsi += mpz_sizeinbase(bits, 2);
    bitsi += lengthX;
    
    // add the 0 before the x
    mpz_set_ui(bits, 0);
    mpz_ior_bits_lshift(bar_encoded, temp, bits, bitsi);
    bitsi += mpz_sizeinbase(bits, 2);
    
    // encode the 1^ℓ(x)
    // int lengthX = mpz_sizeinbase(x, 2); // save for later
    mpz_set_ui(temp, lengthX);
    mpz_set_ui(bits, 1);
    mpz_mul_2exp(bits, bits, lengthX);
    mpz_sub_ui(bits, bits, 1); // bits = 2^ℓ(x) - 1 -> bit string 1^ℓ(x)
    mpz_ior_bits_lshift(bar_encoded, temp, bits, bitsi);
    bitsi += mpz_sizeinbase(bits, 2);
    
    // finished.
    // mpz_clears(temp, bits, NULL);
}

void mpz_bar_encode(mpz_t bar_encoded, mpz_t x, int lengthX)
{
    mpz_t temp; mpz_init(temp);
    mpz_t bits; mpz_init(bits);

    mpz_bar_encode_temps(bar_encoded, x, lengthX, temp, bits);

    mpz_clears(temp, bits, NULL);
}

void mpz_bar_decode_left(mpz_t x, int* lengthx, mpz_t number_with_bar_encoded_left)
{
    if(mpz_cmp_ui(number_with_bar_encoded_left,0) == 0){
        *lengthx = 0;
        mpz_set_ui(x, 0);
        return;
    }
    // gmp_printf("got %Zd at start\n", number_with_bar_encoded_left);
    int lengthOfBarEncodedFull = mpz_sizeinbase(number_with_bar_encoded_left, 2);

    *lengthx = mpz_count_leading_ones(number_with_bar_encoded_left);

    mpz_set_ui(x, 0);

    mpz_t temp; mpz_init(temp);

    const int shifted = lengthOfBarEncodedFull - *lengthx - *lengthx - 1;

    if(shifted < 0){
        mpz_clear(temp);
        return;
    }

    mpz_load_number_of_n_ones(temp, *lengthx);
    mpz_lshift(temp, temp, shifted);
    // gmp_printf("shifted %d left is %Zd\n", shifted, temp);
    mpz_and(temp, temp, number_with_bar_encoded_left);
    // gmp_printf("after and temp is %Zd\n", temp);
    mpz_rshift(x, temp, shifted);
    // gmp_printf("x at end %Zd\n", x);

    mpz_clear(temp);
}

void mpz_bar_decode_left_pop(mpz_t x, int* lengthx, mpz_t number_with_bar_encoded_left)
{
    int lengthOfBarEncodedFull = mpz_sizeinbase(number_with_bar_encoded_left, 2);
    mpz_bar_decode_left(x, lengthx, number_with_bar_encoded_left);

    if(mpz_cmp_ui(number_with_bar_encoded_left,0) == 0){
        *lengthx = 0;
        mpz_set_ui(x, 0);
        return;
    }

    mpz_t temp; mpz_init(temp);

    mpz_load_number_of_n_ones(temp, lengthOfBarEncodedFull - *lengthx - *lengthx - 1);

    mpz_and(number_with_bar_encoded_left, temp, number_with_bar_encoded_left);

    mpz_clear(temp);
}

void mpz_apos_encode_temps(mpz_t apos_encoded, mpz_t x, int lengthX,
    mpz_t temp, mpz_t temp2, mpz_t bits, mpz_t poo1, mpz_t poo2
)
{
    // mpz_t temp; mpz_init(temp);
    // mpz_t temp2; mpz_init(temp2);
    // mpz_t bits; mpz_init(bits);

    // mpz_t poo1,poo2; mpz_inits(poo1,poo2,NULL);

    mp_bitcnt_t bitsi = 0;
    
    // zero apos_encoded to start off.
    mpz_set_ui(apos_encoded, 0);

    // encode x
    mpz_set(bits, x);
    mpz_ior_bits_lshift(apos_encoded, temp, bits, bitsi);
    bitsi += lengthX;

    // encode the bar_encoded(lengthX)
    mpz_set_ui(temp, lengthX);
    mpz_prefix_index_get_bit_length_temps(temp, temp, poo1);

    mpz_set_ui(temp2, lengthX);
    mpz_prefix_index_get_bit_integer_temps(temp2, bits, poo1, poo2);
    
    mpz_bar_encode_temps(temp2, bits, mpz_get_ui(temp), poo1, poo2);
    mpz_set(bits, temp2);
    mpz_ior_bits_lshift(apos_encoded, temp, bits, bitsi);
    bitsi += mpz_sizeinbase(bits, 2);
    
    // finished.
    // mpz_clears(temp, temp2, bits, NULL);
    // mpz_clears(poo1,poo2,NULL);
}

void mpz_apos_encode_temps_str(mpz_t apos_encoded, char* str, mpz_t temp, mpz_t temp2, mpz_t bits, mpz_t poo1, mpz_t poo2, mpz_t x)
{
    int lengthX = strlen(str);
    mpz_set_str(x, str, 2);

    mpz_apos_encode_temps(apos_encoded, x, lengthX, temp, temp2, bits, poo1, poo2);
}

void mpz_apos_encode(mpz_t apos_encoded, mpz_t x, int lengthX)
{
    mpz_t temp; mpz_init(temp);
    mpz_t temp2; mpz_init(temp2);
    mpz_t bits; mpz_init(bits);

    mpz_t poo1,poo2; mpz_inits(poo1,poo2,NULL);

    mpz_apos_encode_temps(apos_encoded, x, lengthX,
        temp, temp2, bits, poo1, poo2);

    mpz_clears(temp, temp2, bits, NULL);
    mpz_clears(poo1,poo2,NULL);
}

void mpz_apos_encode_str(mpz_t apos_encoded, char* str)
{
    mpz_t temp; mpz_init(temp);
    mpz_t x; mpz_init(x);
    mpz_t temp2; mpz_init(temp2);
    mpz_t bits; mpz_init(bits);

    mpz_t poo1,poo2; mpz_inits(poo1,poo2,NULL);

    mpz_apos_encode_temps_str(apos_encoded, str, temp, temp2, bits, poo1, poo2, x);

    mpz_clears(temp, temp2, bits, x, poo1,poo2, NULL);
}

void mpz_apos_encode_prefix_index_temps(mpz_t apos_encoded, mpz_t prefixIndex,
    mpz_t temp, mpz_t poo1, mpz_t poo2, mpz_t poo3, mpz_t poo4, mpz_t poo5
)
{
    // mpz_t temp; mpz_init(temp);
    // mpz_t poo1,poo2,poo3,poo4,poo5; mpz_inits(poo1,poo2,poo3,poo4,poo5,NULL);
    
    mpz_prefix_index_get_bit_length_temps(prefixIndex, temp, poo1);

    int lengthX = mpz_get_ui(temp);

    mpz_prefix_index_get_bit_integer_temps(prefixIndex, temp, poo1, poo2);

    mpz_apos_encode_temps(apos_encoded, temp, lengthX, poo1,poo2,poo3,poo4,poo5);

    // mpz_clear(temp);
    // mpz_clears(poo1,poo2,poo3,poo4,poo5,NULL);
}

void mpz_apos_encode_prefix_index(mpz_t apos_encoded, mpz_t prefixIndex)
{
    mpz_t temp; mpz_init(temp);
    mpz_t poo1,poo2,poo3,poo4,poo5; mpz_inits(poo1,poo2,poo3,poo4,poo5,NULL);

    mpz_apos_encode_prefix_index_temps(apos_encoded, prefixIndex,
        temp, poo1, poo2, poo3, poo4, poo5);

    mpz_clear(temp);
    mpz_clears(poo1,poo2,poo3,poo4,poo5,NULL);
}


int mpz_apos_decode_left_temps(mpz_t x, int* lengthx, mpz_t number_with_apos_encoded_left, mpz_t temp, mpz_t temp2, mpz_t temp3)
{
    if(mpz_cmp_ui(number_with_apos_encoded_left,0) == 0){
        if(lengthx != NULL)*lengthx = 0;
        if(x != NULL)mpz_set_ui(x, 0);
        return 0;
    }

    const int lengthOfFullNumber = mpz_sizeinbase(number_with_apos_encoded_left,2);

    int lengthOfLength;

    mpz_bar_decode_left(temp, &lengthOfLength, number_with_apos_encoded_left);

    mpz_set_ui(temp2, lengthOfLength);
    mpz_get_prefix_index_from_int_and_length_temps(temp, temp, temp2, temp3);
    if(lengthx != NULL)*lengthx = mpz_get_ui(temp);
    int theLengthx = mpz_get_ui(temp);

    int shifted = lengthOfFullNumber - lengthOfLength - lengthOfLength - 1 - theLengthx;
    if(shifted < 0){
        if(lengthx != NULL)*lengthx = 0;
        if(x != NULL)mpz_set_ui(x, 0);

        return 0;
    }

    if(x != NULL){
        // gmp_printf("shifted %d\n", shifted);
        mpz_load_number_of_n_ones(temp, theLengthx);
        mpz_lshift(temp, temp, shifted);
        mpz_and(temp, temp, number_with_apos_encoded_left);
        mpz_rshift(x, temp, shifted);
    }

    return 2*lengthOfLength + 1 + theLengthx;
}

// int mpz_apos_decode_left(mpz_t x, int* lengthx, mpz_t number_with_apos_encoded_left, mpz_t temp, mpz_t temp2, mpz_t temp3);

int mpz_apos_decode_left(mpz_t x, int* lengthx, mpz_t number_with_apos_encoded_left)
{
    mpz_t temp; mpz_init(temp);
    mpz_t temp2; mpz_init(temp2);
    mpz_t temp3; mpz_init(temp3);

    int result = mpz_apos_decode_left_temps(x, lengthx, number_with_apos_encoded_left, temp, temp2, temp3);

    mpz_clears(temp, temp2, temp3, NULL);

    return result;
}


int mpz_apos_decode_prefix_index_left_temps(mpz_t prefixIndex, mpz_t number_with_apos_encoded_left,
    mpz_t xint, mpz_t temp, mpz_t temp2, mpz_t temp3, mpz_t temp4 
)
{
    // mpz_t xint; mpz_init(xint);
    // mpz_t temp; mpz_init(temp);
    // mpz_t temp2; mpz_init(temp2);
    // mpz_t temp3; mpz_init(temp3);
    // mpz_t temp4; mpz_init(temp4);
    int lengthx;

    int aposLength = mpz_apos_decode_left_temps(xint, &lengthx, number_with_apos_encoded_left, temp2, temp3, temp4);

    mpz_set_ui(temp, lengthx);

    mpz_get_prefix_index_from_int_and_length_temps(prefixIndex, xint, temp, temp2);

    // mpz_clears(xint, temp, temp2, temp3, temp4, NULL);

    return aposLength;
}
int mpz_apos_decode_prefix_index_left(mpz_t prefixIndex, mpz_t number_with_apos_encoded_left)
{
    mpz_t xint; mpz_init(xint);
    mpz_t temp; mpz_init(temp);
    mpz_t temp2; mpz_init(temp2);
    mpz_t temp3; mpz_init(temp3);
    mpz_t temp4; mpz_init(temp4);

    int result = mpz_apos_decode_prefix_index_left_temps(prefixIndex, number_with_apos_encoded_left,
    xint, temp, temp2, temp3, temp4 );

    mpz_clears(xint, temp, temp2, temp3, temp4, NULL);

    return result;
}