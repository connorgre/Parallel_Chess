

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

