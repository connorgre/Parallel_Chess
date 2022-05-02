

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

enum Scores{
    king_val = 5000,
    queen_val = 900,
    rook_val = 500,
    bishop_val = 330,
    knight_val = 300,
    pawn_val = 100,
    mid_4x4_val = 50,
    mid_6x6_val = 25,
    defended_pawn_val = 50,
    bishop_pair_val = 50,
    open_files_val = 150,
    not_border_val = 5,
    castle_val = 300,
    passed_pawn_val = 400,
    double_pawn_val = -50,
    connect_bishop_val = 100,
    moves_val = 15,
    king_defenders_val = 25,
    king_pawn_defenders_val = 100
};

int score_board(Board_Data_t* board_data);
int score_board_fast(Board_Data_t* board_data);
U64 Get_Open_Files(U64* board);
U64 Get_Passed_Pawns(U64* board, int toMove);
int score_board_simple(Board_Data_t* board_data);
U64 King_Moves(Board_Data_t* board_data, int toMove);

#endif