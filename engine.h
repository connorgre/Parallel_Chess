
#ifndef ENGINE_H
#define ENGINE_H

#include "board.h"
#include "move.h"
#include <pthread.h>

#define NUM_THREADS 8

#define MAX_PLY 16  //would be 64 or 128 for a real engine bc of quiescence search
#define MAX_MOVES 128    //going to assume that there won't be more than 128 moves in a position

typedef struct search_data{
    int pos_searched;
    int captures;
    int checks;
    int castles;
    int checkmates;
    int promotions;
    int en_passants;
} search_data_t;
//this is bigger than it could be because it is holding the search data.  only NEED to store the 
//zob key and depth, but the search data helps verify it working correctly.  This shouldn't be too hard to change 
//later if I need to. Would also need to store the best move and score if I was implementing this in an engine, but 
//It should be fine for now.
typedef struct TT_Entry{
    U64 zob_key;
    int depth;
    search_data_t search_data;
}   tt_entry_t;

typedef struct Trans_Table{
    tt_entry_t* table_head;
    pthread_mutex_t* mutex_arr;
    int size;
} trans_table_t;

typedef struct Search_Memory{
    Board_Data_t** copy_arr;
    move_t** move_arr;
} Search_Mem_t;


void Perft_Expanded(Board_Data_t* board_data, Search_Mem_t* search_mem, int depth, int ply, int isMaximizing, trans_table_t* tt);
search_data_t Perft(Board_Data_t* board_data, Search_Mem_t* search_mem, int depth, int ply, int isMaximizing, trans_table_t* tt);
void Init_Search_Data(search_data_t* data);
void Add_Search_Data(search_data_t* data1, search_data_t* data2);
void Copy_Search_Data(search_data_t* data1, search_data_t* data2);

Search_Mem_t* Init_Search_Memory();
void Delete_Search_Memory(Search_Mem_t* search_mem);

#endif