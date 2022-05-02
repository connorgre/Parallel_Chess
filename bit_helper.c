

#include "bit_helper.h"
//gets the x and y positions, bottom left: x = 0, y=0
int GetX(U64 p){
    return (ffsl(p)-1)%8;
}
int GetY(U64 p){
    return 7-(ffsl(p)-1)/8;
}

//COULD BE AN ISSUE
int Get_Idx(U64 p){
    return ffsl(p)-1;
}

//returns LSB
U64 Get_LSB(U64 p)
{
    return p & (~(p-1));
}
U64 Get_MSB(U64 p){
    U64 bit = p;
    while(PopCount(bit) > 1)
    {
        bit &= (bit - 1);
    }
    return bit;
}

//fills loc_list with U64s of individual bits of piece
//end of list is 0
void Get_Individual(U64 piece, U64* loc_list){
    U64 p = piece;
    U64 pos = Get_LSB(p);
    int idx = 0;
    while (pos != 0)
    {
        loc_list[idx++] = pos;
        p &= (~pos);
        pos = Get_LSB(p);
    }
    loc_list[idx] = 0ULL;
}

//returns number of set bits
int PopCount(U64 p){
    int count;
    for (count = 0; p != 0; count++) p &= p - 1;
    return count;
}

//computes the popcount of 4 64 bit ints in parallel
__m256i AVX_PopCount(__m256i pos){
    __m256i lookup =_mm256_setr_epi8(   0, 1, 1, 2, 1, 2, 2, 3, 
                                        1, 2, 2, 3, 2, 3, 3, 4, 
                                        0, 1, 1, 2, 1, 2, 2, 3,
                                        1, 2, 2, 3, 2, 3, 3, 4 );
    __m256i low_mask = _mm256_set1_epi8(0x0f);
    __m256i lo = _mm256_and_si256(pos, low_mask);
    __m256i hi = _mm256_and_si256(_mm256_srli_epi32(pos, 4), low_mask);
    __m256i popcnt1 = _mm256_shuffle_epi8(lookup ,lo);
    __m256i popcnt2 = _mm256_shuffle_epi8(lookup ,hi);
    __m256i total = _mm256_add_epi8(popcnt1, popcnt2);
    return _mm256_sad_epu8 (total ,_mm256_setzero_si256());
}

//uses builtin cpu popcount
int PopCount_fast(U64 pos){
    return _mm_popcnt_u64(pos);
}