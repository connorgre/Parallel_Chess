

#ifndef BOARD_H
#define BOARD_H
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


typedef long unsigned int U64;
//defining constants
#define NUM_PIECES 12
#define ONE 1ULL
#define ZERO 0ULL
#define ZOB_ROWS 66



#define PIECE_ARR_LEN sizeof(U64)*(NUM_PIECES+1)
#define COLOR_ARR_LEN sizeof(U64)*(2+1)

#define WK_CAS_ZOB 1
#define WQ_CAS_ZOB 2
#define BK_CAS_ZOB 3
#define BQ_CAS_ZOB 4

typedef unsigned char byte;
enum Piece
{
    W_KING = 0,
    W_QUEEN = 1,
    W_ROOK = 2,
    W_BISHOP = 3,
    W_KNIGHT = 4,
    W_PAWN = 5,

    B_KING = 6,
    B_QUEEN = 7,
    B_ROOK = 8,
    B_BISHOP = 9,
    B_KNIGHT = 10,
    B_PAWN = 11, 

    NO_PIECE = 12
};
enum Color{
    WHITE = 0,
    BLACK = 1,
    NO_TEAM = 2
};
typedef struct Move_Tables{
    U64** line_tables;
    U64* rook_mask;
    U64* bishop_mask;
    U64** rook_table;
    U64** bishop_table;
} move_table_t;
typedef struct Board_Data{
    U64* pieces;    //array of 12 U64 to hold piece locations
    U64* team_tiles;

    move_table_t* move_tables;
    U64 ep_tile;
    U64 occ;
    byte cas_mask;
    byte to_move;

    U64 zob_key;
    U64 **zob_array;
} Board_Data_t;

Board_Data_t* Init_Board();
void Delete_Board(Board_Data_t* board, int free_zob);
void Copy_Board(Board_Data_t* copy, Board_Data_t* source_board);
void Copy_Board_Data(Board_Data_t* copy, Board_Data_t* source);
//sets the board up from given FEN string.
//TODO: ADD IN EP, CASTLE, MOVE/HALFMOVE
void Set_From_Fen(char* FEN, Board_Data_t* board);
void Print_Board(U64 piece, Board_Data_t* board);
void Print_Castling_Rights(byte cas_mask);

void Init_Board_Move_Table(Board_Data_t* board_data);
void Init_Zob_Array(Board_Data_t* board_data);
void Reset_Zob_Key(Board_Data_t* board_data);

int Verify_Board(Board_Data_t* board_data, int toMove);
#endif