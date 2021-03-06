


#include "perft.h"
#include "movegen.h"
#include "movegen_fast.h"
#include "bit_helper.h"
#include "perft_trans_table.h"
#include "evaluation.h"
#include <semaphore.h>
#include <pthread.h>
#include <stdio.h>

#include "Engine_Movegen.h"

//#define ERROR_CHECK
#define USE_TT 1
#define USE_AVX 1
#define SCORE_BOARD 0


search_data_t Perft(Board_Data_t* board_data, Search_Mem_t* search_mem, int depth, int ply, int isMaximizing, perft_trans_table_t* tt){
   search_data_t search_data = {0,0,0,0,0,0,0,0};
    #ifdef ERROR_CHECK
        if(isMaximizing != board_data->to_move){
            printf("Error: maximizing != to_move\n");
        }
    #endif
    #if USE_TT
        search_data_t tt_data = Probe_Perft_Trans_Table(board_data->zob_key, depth, tt);
        if(tt_data.pos_searched != -1){
            #ifdef ERROR_CHECK
                if(tt_data.pos_searched == 0){
                    printf("error, tt table has 0 positions searched\n");
                }
            #endif
            return tt_data;
        }
    #endif

    #ifdef ERROR_CHECK
        if(In_Check_fast(board_data, isMaximizing) != In_Check(board_data, isMaximizing)){
            int incheck = In_Check_fast(board_data, isMaximizing);
            printf("Error, fast check incorrect: %d\n",incheck);
        }
    #endif


    if(depth <= 0 || ply == MAX_PLY){
        search_data.pos_searched++;
        int in_check;
        int score;
        #if USE_AVX == 1
            in_check = In_Check_fast(board_data, isMaximizing);
            #if SCORE_BOARD == 1
                score = score_board_fast(board_data);
            #endif
        #else
            in_check  = In_Check(board_data, isMaximizing);
            #if SCORE_BOARD == 1
                score = score_board(board_data);
            #endif
        #endif
        #if SCORE_BOARD == 0
            score = 0;
        #endif
        if(in_check){
            search_data.checks++;
        } else{
            search_data.score = score;
            #if USE_TT
                Insert_Perft_Trans_Table(board_data->zob_key, depth, &search_data, tt);
            #endif
            return search_data;
        }
    }
            
    #if USE_AVX == 1
        Get_All_Moves_fast(board_data, search_mem->move_arr[ply], isMaximizing);
    #else
        Get_All_Moves(board_data, search_mem->move_arr[ply], isMaximizing);
    #endif
    move_t* moves = search_mem->move_arr[ply];
    int move_idx = 0;
    int moves_done = 0;
    Board_Data_t* copy = search_mem->copy_arr[ply];
    Copy_Board(copy, board_data);
    while(moves[move_idx].from != FULL){
        //copy the board, then do the move on the copied board so we don't
        //need to worry about undoing the move
        Do_Move(copy, moves + move_idx);
    #ifdef ERROR_CHECK
        U64 zob = copy->zob_key;
        Reset_Zob_Key(copy);
        if(zob != copy->zob_key){
            printf("Error: zob key wrong after move. flag: %x\n", (moves+move_idx)->flags);
        }
    #endif
    #if USE_AVX == 1
        if(In_Check_fast(copy, isMaximizing) == 0){
    #else
        if(In_Check(copy, isMaximizing) == 0){
    #endif
            moves_done++;
            if(depth == 1){
                if(moves[move_idx].to_type != NUM_PIECES) search_data.captures++;
                if(((moves[move_idx].flags & 0x0F) != 0) && ((moves[move_idx].flags & 0xF0) == 0)) search_data.promotions++;
                if(((moves[move_idx].flags & 0xF0) != 0) && ((moves[move_idx].flags & 0x0F) == 0)) search_data.castles++;
                if(moves[move_idx].flags == 0xFF) {search_data.en_passants++; search_data.captures++;}
            }
            if(depth > 0){
                search_data_t result_data = Perft(copy, search_mem, depth -1, ply + 1, (~isMaximizing & 1), tt);
                Add_Search_Data(&search_data, &result_data);
            }
        }
        Undo_Move(copy, board_data, moves + move_idx); 
        move_idx++;
    }
    if(depth == 0){
        if(moves_done == 0){
            search_data.checkmates++;
        }
    }
    #if USE_TT
        Insert_Perft_Trans_Table(board_data->zob_key, depth, &search_data, tt);
    #endif
    return search_data;
}


void Perft_Expanded(Board_Data_t* board_data, Search_Mem_t* search_mem, int depth, int ply, int isMaximizing, perft_trans_table_t* tt){

    move_t* moves = search_mem->move_arr[0];
    Get_All_Moves(board_data, moves, isMaximizing);
    int move_idx = 0;
    search_data_t search_data;
    printf("Expanded Perft results: \n");
    int printed = 0;
    U64 prev_pos = moves[0].from;
    Board_Data_t* copy = Init_Board();
    while(moves[move_idx].from != FULL){
        Init_Search_Data(&search_data);
        char* move_str = String_From_Move(moves[move_idx]);
        Copy_Board(copy, board_data);
        Do_Move(copy, moves + move_idx);
        if(In_Check(copy, isMaximizing) == 0){
            search_data = Perft(copy, search_mem, depth-1, 1, (~isMaximizing & 1), tt);
        }
        move_idx++;
        if(search_data.pos_searched > 0){
            printf("\t%s: %ld\n", move_str, search_data.pos_searched);
            printed++;
            if(prev_pos != moves[move_idx].from){
                printf("\n");
                prev_pos = moves[move_idx].from;
            }
        }
        free(move_str);
    }
    Delete_Board(copy, 0);
    printf("\n");
}




void Init_Search_Data(search_data_t* data){
    data->pos_searched = 0;
    data->score = 0;
    data->captures = 0;
    data->checks = 0;
    data->castles = 0;
    data->checkmates = 0;
    data->promotions = 0;
    data->en_passants = 0;
}

void Add_Search_Data(search_data_t* data1, search_data_t* data2){
    data1->pos_searched += data2->pos_searched;
    data1->score = (data1->score > data2->score) ? data1->score : data2->score;
    data1->captures += data2->captures;
    data1->checks += data2->checks;
    data1->castles += data2->castles;
    data1->checkmates += data2->checkmates;
    data1->promotions += data2->promotions;
    data1->en_passants += data2->en_passants;
}

void Copy_Search_Data(search_data_t* data1, search_data_t* data2){
    data1->pos_searched = data2->pos_searched;
    data1->score = data2->score;
    data1->captures = data2->captures;
    data1->checks = data2->checks;
    data1->castles = data2->castles;
    data1->checkmates = data2->checkmates;
    data1->promotions = data2->promotions;
    data1->en_passants = data2->en_passants;
}

Search_Mem_t* Init_Search_Memory(){
    Search_Mem_t* search_mem = (Search_Mem_t*)malloc(sizeof(Search_Mem_t));
    Board_Data_t** copy_arr = (Board_Data_t**)malloc(sizeof(Board_Data_t*)*MAX_PLY);
    move_t** move_stack = (move_t**)malloc(sizeof(move_t*)*MAX_PLY);
    for(int i = 0; i < MAX_PLY; i++){
        copy_arr[i] = Init_Board();
        move_stack[i] = (move_t*)malloc(sizeof(move_t)*MAX_MOVES);
    }
    search_mem->copy_arr = copy_arr;
    search_mem->move_arr = move_stack;
    return search_mem;
}

void Delete_Search_Memory(Search_Mem_t* search_mem){
    for(int i = 0; i < MAX_PLY; i++){
        Delete_Board(search_mem->copy_arr[i],0);
        free(search_mem->move_arr[i]);
    }
    free(search_mem->copy_arr);
    free(search_mem->move_arr);
    free(search_mem);
}