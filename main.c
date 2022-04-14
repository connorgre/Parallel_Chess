#include <stdio.h>
#include <string.h>

#include "board.h"
#include "user_input.h"
#include "trans_table.h"
int main()
{
    char in_string[512];
    Board_Data_t* board_data;
    board_data = Init_Board();
    Init_Zob_Array(board_data);
    char* init_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
    Set_From_Fen(init_fen, board_data); 
    Reset_Zob_Key(board_data);
    trans_table_t* tt = Init_Trans_Table(10000019);
    while(1)
    {
        printf(">> ");
        fgets(in_string,512,stdin);
        if(in_string[0] == 'q')
        {
            Delete_Board(board_data, 1);
            Delete_Trans_Table(tt);
            return 0;
        }
        Handle_Input(in_string, board_data, tt);
    }
    return 0;
}