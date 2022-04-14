#include "parallel_engine.h"
#include "move.h"
#include "engine.h"
#include "movegen.h"
#include <pthread.h>
/*This could potentially be more confusing than it needs to be, but the way I am planning on doing this is
    to have one function called to handle running the parallel search.  This function will generate all the moves for
    the top level, and spawn a thread for each of those moves.  Since each thread will be using a fair amount of memory,
    I am limiting the max number of running threads to 8.  To make this work, the program will run through and make the
    first 8 threads like normal, then for each one after that, I will wait on a barrier for 1 thread to join.  Before that 
    thread joins the barrier it will set a mutex, and within that mutex it will update its index in an array of thread infos
    to being ready to run. Then it will join the barrier, and, return.  In main, once the child thread joins the barrier,
    it will search for the position in the availible threads array, and get the index of the open thread. Then it will create
    a new thread with the memory search information the old thread was using.  After creating this thread, it will mark that 
    data as being used, then unlock the mutex, then go back to waiting on the barrier for another thread. 

    The mutex barrier combo is used to make sure only one child thread is able to take control of the barrier at a time,
    so the parent thread is able to make sure it isn't giving out memory 

    for now, I am just going to spawn as many threads as there are moves at ply 0 and see what happens, but I suspsect
    the above implementation will be faster due to transposition table entries being used within the same thread more often,
    and cache having to be cleared less.
*/



search_data_t Parallel_Perft(Board_Data_t* board_data, int depth, int isMaximizing, trans_table_t* tt){
    /*Search_Mem_t search_mem_arr[NUM_THREADS];
    for(int i = 0; i < NUM_THREADS; i++){
        Init_Search_Memory(&(search_mem_arr[i]));
    }
    */
    move_t* moves = (move_t*)malloc(sizeof(move_t)*MAX_MOVES);
    search_data_t search_data;
    Init_Search_Data(&search_data);

    Get_All_Moves(board_data, moves, isMaximizing);
    int num_moves = 0;
    while(moves[num_moves].from != FULL){
        num_moves++;
    }
    num_moves--;
    thread_info_t* t_info_arr = (thread_info_t*)malloc(sizeof(thread_info_t)*num_moves);

    pthread_mutex_t search_data_mut;
    pthread_mutex_init(&search_data_mut, NULL);

    for(int i = 0; i < num_moves; i++){
        t_info_arr[i].tt = tt;
        Init_Search_Memory((t_info_arr[i].search_mem));
        t_info_arr[i].search_data = &search_data;
        t_info_arr[i].depth = depth;
        t_info_arr[i].board_data = board_data;
        t_info_arr[i].thread_num = i;
        t_info_arr[i].search_data_mut = &search_data_mut;
    }
    pthread_t pid[num_moves];
    for(int i = 0; i < num_moves; i++){
        pthread_create(&(pid[i]), NULL, Perft_Thread, (void*) &(t_info_arr[i]));
    }
    for(int i = 0; i < num_moves; i++){
        pthread_join(pid[i], NULL);
        Delete_Search_Memory((t_info_arr[i].search_mem));
    }
    free(t_info_arr);
    free(moves);
    pthread_mutex_destroy(&search_data_mut);
    return search_data;
}

void* Perft_Thread(void * thread_info){
    thread_info_t* t_info = (thread_info_t*)thread_info;
    int depth = t_info->depth;
    trans_table_t* tt = t_info->tt;
    Search_Mem_t* search_mem = t_info->search_mem;
    Board_Data_t* board_data = t_info->board_data;
    pthread_mutex_t* sd_mut = t_info->search_data_mut;
    search_data_t* search_data = t_info->search_data;
    search_data_t thread_search_data;   

    thread_search_data = Perft(board_data, search_mem, depth, 0, board_data->to_move, tt);
    
    pthread_mutex_lock(sd_mut);
        Add_Search_Data(search_data,&thread_search_data);
    pthread_mutex_unlock(sd_mut);
    return NULL;
}


//returns the index of the free thread
int Get_Availible_Thread(int* free_threads){
    for(int i = 0; i < NUM_THREADS; i++){
        if(free_threads[i] == 1){
            return i;
        }
    }
    return -1;
}