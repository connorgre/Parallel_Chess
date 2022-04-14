
#ifndef PARALLEL_H
#define PARALLEL_H

#include "engine.h"
#include "board.h"
#include "move.h"
#include <pthread.h>

#define MAX_THREADS 8

typedef struct Thread_Info{
    trans_table_t* tt;
    Search_Mem_t* search_mem;
    search_data_t* search_data;
    int depth;
    Board_Data_t* board_data;
    int thread_num;
    pthread_mutex_t* search_data_mut;
    pthread_barrier_t* perft_bar;
    move_t* move;
    int tid;
    int* free_threads;
} thread_info_t;

search_data_t Parallel_Perft(Board_Data_t* board_data, int depth, int isMaximizing, trans_table_t* tt);
search_data_t Parallel_Perft_Limited(Board_Data_t* board_data, int depth, int isMaximizing, trans_table_t* tt);
void* Perft_Thread(void *);
void* Perft_Thread_Limited(void * thread_info);
int Check_All_Open(int* free_threads);
int Get_Availible_Thread(int* free_threads);

#endif