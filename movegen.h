
#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "board.h"
#include "move.h"
#include "bit_helper.h"

#define WKC_LAND 0x0000000000000040ULL
#define WQC_LAND 0x0000000000000004ULL
#define BKC_LAND 0x4000000000000000ULL
#define BQC_LAND 0x0400000000000000ULL

#define WK_START 0x0000000000000010ULL
#define BK_START 0x1000000000000000ULL

void Get_All_Moves(Board_Data_t* board_data, move_t* movelist, int toMove);

byte Set_Flags(Board_Data_t* board_data, int toMove, U64 from, U64 to);

U64 Get_Team_Move_Mask(Board_Data_t* board_data, int toMove);
U64 Get_Piece_Moves(Board_Data_t* board_data, int toMove, U64 pos, int type);
U64 Get_King_Moves(Board_Data_t* board_data, int toMove, U64 k_pos);
U64 Get_King_Moves_No_Castles(Board_Data_t* board_data, int toMove, U64 k_pos);
U64 Get_Queen_Moves(Board_Data_t* board_data, int toMove, U64 q_pos);
U64 Get_Rook_Moves(Board_Data_t* board_data, int toMove, U64 r_pos);
U64 Get_Bishop_Moves(Board_Data_t* board_data, int toMove, U64 b_pos);
U64 Get_Knight_Moves(Board_Data_t* board_data, int toMove, U64 n_pos);
U64 Get_Pawn_Moves(Board_Data_t* board_data, int toMove, U64 p_pos);
U64 Get_Tile_Attackers(Board_Data_t* board_data, U64 pos, int toMove);

int In_Check(Board_Data_t* board_data, int toMove);

#endif