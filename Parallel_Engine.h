
#ifndef PARALLEL_ENGINE_H
#define PARALLEL_ENGINE_H

#include "engine.h"
#include "board.h"
#include "move.h"
#include "perft.h"
#include "trans_table.h"
#include "parallel_perft.h"
#include <pthread.h>

#define MAX_ENGINE_THREADS 4

//how this will work is it will only do a multithreaded search of the
//the principal variation, and will do it at each level starting from
//the bottom of the search.  

typedef struct Engine_Thread_Info{
    trans_table_t* tt;
    eng_search_mem_t* search_mem;
    move_t* best_move;
    move_t* move;
    Board_Data_t* board_data;
    int thread_num;
    pthread_mutex_t* search_data_mut;
    pthread_barrier_t* perft_bar;
    int tid;
    int* free_threads;
    int depth; 
    int ply;
    int* alpha; 
    int* beta;
    int* moves_done;
} eng_thread_info_t;

move_t Parallel_Iterative_Deepening(Board_Data_t* board_data, eng_search_mem_t** search_mem,  trans_table_t* tt, int depth, eng_thread_info_t* t_info_arr);
move_t Parallel_Negmax(Board_Data_t* board_data, eng_search_mem_t** search_mem,  trans_table_t* tt, int depth, int ply, int isMaximizing, int alpha, int beta, eng_thread_info_t* t_info_arr);
void* Engine_Thread(void *);

int Get_Availible_Thread_Engine(int* free_threads);
int Check_All_Open_Engine(int* free_threads);




#endif