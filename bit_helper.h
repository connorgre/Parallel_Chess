

#ifndef BIT_HELPER_H
#define BIT_HELPER_H
#define _GNU_SOURCE
#include "board.h"
#include "immintrin.h"

#define left_wall   0x0101010101010101ULL
#define right_wall  0x8080808080808080ULL
#define top_wall    0xFF00000000000000ULL
#define bottom_wall 0x00000000000000FFULL
#define border      0xFF818181818181FFULL

#define FULL 0xFFFFFFFFFFFFFFFFULL

//list of macros for shifting the bits up or down
#define UP(p) ((p & ~top_wall) << 8)
#define DOWN(p) ((p & ~bottom_wall) >> 8)
#define LEFT(p) ((p & ~left_wall) >> 1)
#define RIGHT(p) ((p & ~right_wall) << 1)
#define UPLEFT(p) ((p & ~(top_wall | left_wall)) << 7)
#define UPRIGHT(p) ((p & ~(top_wall | right_wall)) << 9)
#define DOWNLEFT(p) ((p & ~(bottom_wall | left_wall)) >> 9)
#define DOWNRIGHT(p) ((p & ~(bottom_wall | right_wall)) >> 7)

int GetX(U64 pos);
int GetY(U64 pos);
U64 Get_LSB(U64 pos);
U64 Get_MSB(U64 pos);
void Get_Individual(U64 pos, U64* set_list);
int PopCount(U64 pos);
int PopCount_fast(U64 pos);
int Get_Idx(U64 pos);
__m256i AVX_PopCount(__m256i pos);

#endif