
#ifndef PARALLEL_H
#define PARALLEL_H

#include "engine.h"
#include "board.h"
#include "move.h"
#include <pthread.h>

#define NUM_THREADS 8

typedef struct Thread_Info{
    trans_table_t* tt;
    Search_Mem_t* search_mem;
    search_data_t* search_data;
    int depth;
    Board_Data_t* board_data;
    int thread_num;
    pthread_mutex_t* search_data_mut;
} thread_info_t;

search_data_t Parallel_Perft(Board_Data_t* board_data, int depth, int isMaximizing, trans_table_t* tt);
void* Perft_Thread(void *);

#endif