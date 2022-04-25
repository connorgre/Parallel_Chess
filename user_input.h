
#ifndef USER_INPUT_H
#define USER_INPUT_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "board.h"
#include "move.h"
#include "movegen.h"
#include "perft.h"
#include "trans_table.h"

#define MAX_WORD_LEN 64
#define MAX_INPUT_WORDS 8

void Handle_Input(char* input, Board_Data_t* board, trans_table_t* tt);
char** Parse_Input(char* input);
void Free_Parsed_Input(char** parsed_input);
void Input_Move(char** parsed_input, Board_Data_t* board);
move_t Move_From_Input(char* move_str, Board_Data_t* board);
void Input_Print_Board(char** paresed_input, Board_Data_t* board);
void Print_Moves(char** parsed_input, Board_Data_t* board_data);
void Update_Flags_From_Input(char* flag, move_t* move);
char* Input_Reset_String(char* input);
void Input_Perft(char** parsed_input, Board_Data_t* board_data, trans_table_t* tt);
void Input_Expanded_Perft(char** parsed_input, Board_Data_t* board_data, trans_table_t* tt);
void Input_Parallel_Perft(char** parsed_input, Board_Data_t* board_data, trans_table_t* tt);
void Reset_Trans_Table(trans_table_t* tt);
void Input_Search(char** parsed_input, Board_Data_t* board_data, trans_table_t* tt);

#endif