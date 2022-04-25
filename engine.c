#include "engine.h"
#include "evaluation.h"
#include "move.h"
#include "movegen.h"
#include "movegen_fast.h"

//does an alpha-beta minimax search on the board.  Haven't gotten to move ordering yet though.
//CURRENTLY IMPLEMENTS SIMPLE NEGAMAX WITHOUT A TRANSPOSITION TABLE OR MOVE ORDERING
int negmax(Board_Data_t* board_data, Search_Mem_t* search_mem,  trans_table_t* tt, int depth, int ply, int isMaximizing, int alpha, int beta, move_t* best_move){

    int score;
    int best = MIN;
    int best_move_idx = 0;

    if(depth == 0 || ply == MAX_PLY){
        int mult = (isMaximizing == WHITE) ? 1 : -1;
        score = mult * score_board(board_data);
        return score;
    }

    Get_All_Moves_fast(board_data, search_mem->move_arr[ply], isMaximizing);
    move_t* moves = search_mem->move_arr[ply];
    int move_idx = 0;
    int moves_done = 0;
    Board_Data_t* copy = search_mem->copy_arr[ply];
    Copy_Board(copy, board_data);
    while(moves[move_idx].from != FULL){
        //copy the board, then do the move on the copied board so we don't
        //need to worry about undoing the move
        Do_Move(copy, moves + move_idx);
        score = MIN-1;
        if(In_Check_fast(copy, isMaximizing) == 0){
            moves_done++;
            score = -1 * negmax(copy, search_mem, tt, depth -1, ply + 1, (~isMaximizing & 1), beta * -1, alpha * -1, NULL);
        }
        Undo_Move(copy, board_data, moves + move_idx); 
        if(best < score){
            best = score;
            best_move_idx = move_idx;
        }
        if(score > alpha){
            alpha = score;
        }
        if(alpha >= beta){
            break;
        }
        move_idx++; 
    }
    if(moves_done == 0){
        return MIN + ply;
    }
    if(ply == 0){
        *best_move = moves[best_move_idx];
    }
    return best;
}