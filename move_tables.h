
#ifndef MOVE_TABLE_H
#define MOVE_TABLE_H
#include "board.h"

//hold the lines for each direction at each position
enum Directions{
    north = 0,
    east = 1,
    south = 2,
    west = 3,
    northeast = 4,
    southeast = 5,
    northwest = 6,
    southwest = 7
};

move_table_t* Init_Move_Table();
void Delete_Move_Table(move_table_t* move_table);
void Init_Line_Table(move_table_t* move_table);
U64 East_f(U64 pos, int n);
U64 West_f(U64 pos, int n);
void Init_Rook_Line_Table(move_table_t* move_table);
void Init_Bishop_Line_Table(move_table_t* move_table);
U64 GetBlockersFromIndex(int index, U64 _mask);
void InitRookMagicTable(move_table_t* move_table);
void InitBishopMagicTable(move_table_t* move_table);
U64 GetRookAttacksSlow(int pos, U64 blockers, move_table_t* move_table);
U64 GetBishopAttacksSlow(int pos, U64 blockers, move_table_t* move_table);

U64 Lookup_Rook(Board_Data_t* board_data, int toMove, U64 pos);
U64 Lookup_Bishop(Board_Data_t* board_data, int toMove, U64 pos);
U64 Lookup_Queen(Board_Data_t* board_data, int toMove, U64 pos);
#endif
