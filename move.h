
#ifndef MOVE_H
#define MOVE_H
#include "board.h"

//holds the from position, to position, and flags
    //top 4 bits of flags hold castling rights, bottom 4 hold promotion type,
    //if flag = 0xFF, then it is en passant
typedef struct Move{
    U64 from;
    U64 to;
    byte from_type;
    byte to_type;
    byte flags;
    byte alignment;
} move_t;

void Do_Move(Board_Data_t* board_data, move_t* move);
void White_KingSide_Castle(Board_Data_t* board_data);
void White_QueenSide_Castle(Board_Data_t* board_data);
void Black_KingSide_Castle(Board_Data_t* board_data);
void Black_QueenSide_Castle(Board_Data_t* board_data);
void Undo_Move(Board_Data_t* curr_board, Board_Data_t* orig_board, move_t* move);
char* String_From_Move(move_t move);

byte Get_White_Castle_Change(Board_Data_t* board_data, U64 from, U64 to);
byte Get_Black_Castle_Change(Board_Data_t* board_data, U64 from, U64 to);
U64 Get_Ep_Tile(Board_Data_t* board_data, U64 from, U64 to);

#endif