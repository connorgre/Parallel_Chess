#include <stdio.h>
#include <string.h>

#include "board.h"
#include "user_input.h"
#include "perft_trans_table.h"
#include "trans_table.h"
#include "move_tables.h"
#include "perft.h"
#include "engine.h"
int main()
{
    char in_string[512];
    Board_Data_t* board_data;
    board_data = Init_Board();
    Init_Zob_Array(board_data);
    Init_Board_Move_Table(board_data);
    char* init_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
    Set_From_Fen(init_fen, board_data); 
    Reset_Zob_Key(board_data);
    perft_trans_table_t* perft_tt = Init_Perft_Trans_Table(10009);
    trans_table_t* engine_tt = Init_Trans_Table(10000019);
    while(1)
    {
        printf(">> ");
        fgets(in_string,512,stdin);
        if(in_string[0] == 'q')
        {
            Delete_Board(board_data, 1);
            Delete_Perft_Trans_Table(perft_tt);
            Delete_Trans_Table(engine_tt);
            return 0;
        }
        Handle_Input(in_string, board_data, engine_tt, perft_tt);
    }
    return 0;
}