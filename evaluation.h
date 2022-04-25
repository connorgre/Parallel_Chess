

#ifndef EVALUATE_H
#define EVALUATE_H

#include "bit_helper.h"
#include "perft.h"
#include "board.h"

#define KING_VAL    5000
#define QUEEN_VAL   (90 * BASE_VAL)
#define ROOK_VAL    (50 * BASE_VAL)
#define BISHOP_VAL  (33 * BASE_VAL)
#define KNIGHT_VAL  (30 * BASE_VAL)
#define PAWN_VAL    (10 * BASE_VAL)
#define BASE_VAL    10

int score_board(Board_Data_t* board_data);
int score_board_fast(Board_Data_t* board_data);
U64 Get_Open_Files(U64* board);
U64 Get_Passed_Pawns(U64* board, int toMove);
#endif