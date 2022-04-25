
#ifndef ENGINE_H
#define ENGINE_H

#include "board.h"
#include "trans_table.h"
#include "move.h"

#define MIN -999999
#define MAX  999999

typedef struct Move_Score{
    move_t best_move;
    int score;
} score_t;

int negmax(Board_Data_t* board_data, Search_Mem_t* search_mem,  trans_table_t* tt, int depth, int ply, int isMaximizing, int alpha, int beta, move_t* best_move);


#endif