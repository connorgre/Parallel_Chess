
#include "Parallel_Engine.h"
#include "Engine_Movegen.h"
#include "movegen_fast.h"

#define PRINT_DEBUG 0

move_t Parallel_Iterative_Deepening(Board_Data_t* board_data, eng_search_mem_t** search_mem,  trans_table_t* tt, int depth, eng_thread_info_t* t_info_arr){

    move_t best_move = Parallel_Negmax(board_data, search_mem, tt, 3, 0, board_data->to_move, MIN, MAX, t_info_arr);
    for(int i = 4; i <= depth; i++){
        best_move = Parallel_Negmax(board_data, search_mem, tt, i, 0, board_data->to_move, MIN, MAX, t_info_arr);
    }
    return best_move;
}

//this will go down the principal variation, spawning new threads at each level as it moves back up
move_t Parallel_Negmax(Board_Data_t* board_data, eng_search_mem_t** search_mem,  trans_table_t* tt, int depth, int ply, int isMaximizing, int alpha, int beta, eng_thread_info_t* t_info_arr){
    #if PRINT_DEBUG == 1
        printf("Called Parallel_Negmax, D=%d\n", depth);
    #endif
    move_t score_move = Default_Move();
    move_t best_move = Default_Move();
    best_move.score = MIN + ply;

    int tt_flag = ALPHA_FLAG;
    move_t tt_move = Probe_Trans_Table(board_data->zob_key, depth, alpha, beta, tt);
    if(tt_move.score != INVALID_SCORE){
        if(tt_move.score == BEING_SEARCHED){
            //return max if the position is being searched, so it appears super bad in the next level up,
            //so it won't be considered
            if(ply == 0){
                #if ERROR_CHECK == 1
                    printf("Error, shouldn't return being searched from ply 0\n");
                #endif                 
            }
            tt_move.score = MAX-1;
            return tt_move;
        }else{
            if(Verify_Legal_Move(board_data, &tt_move)){
                return tt_move;
            }
        }
    }
    memcpy(search_mem[MAX_ENGINE_THREADS]->move_arr[ply][BEST_MOVE][0], &tt_move, sizeof(move_t));


    if(depth <= 0 || ply >= MAX_PLY){
        best_move = Quiscence_Search(board_data, search_mem[MAX_ENGINE_THREADS], ply, isMaximizing, alpha, beta, tt);
        Insert_Trans_Table(board_data->zob_key, depth, tt_flag, best_move, tt);
        return best_move;
    }
    Get_All_Moves_Separate(board_data, search_mem[MAX_ENGINE_THREADS]->move_arr[ply], isMaximizing);
    Order_Moves_Simple(search_mem[MAX_ENGINE_THREADS]->move_arr[ply][ATTACK_MOVES]);
    Order_Normal_Moves(board_data, search_mem[MAX_ENGINE_THREADS]->move_arr[ply][NORMAL_MOVES], search_mem[MAX_ENGINE_THREADS]->history_array, search_mem[MAX_ENGINE_THREADS]->butterfly_array);
    search_mem[MAX_ENGINE_THREADS]->move_arr[ply][BEST_MOVE][0][0] = tt_move;


//////////////////////////////////////////////////////////////////////////////////////////////////////////
    Board_Data_t* copy = search_mem[MAX_ENGINE_THREADS]->copy_arr[ply];
    Copy_Board(copy, board_data);
    int moves_done = 0;
    for(int movetype = 0; movetype < NUM_MOVE_TYPES; movetype++){
        int move_idx = 0;
        move_t** moves = search_mem[MAX_ENGINE_THREADS]->move_arr[ply][movetype];
        while(moves[move_idx]->from != FULL && moves_done == 0){
            if(movetype != ATTACK_MOVES && movetype != NORMAL_MOVES){
                if(Verify_Legal_Move(board_data, moves[move_idx]) == 0){
                    move_idx++;
                    continue;
                }
            }
         /////////////////////////////////////////////////////////////////////
            //continue PV search.
            Do_Move(copy, moves[move_idx]);
            if(In_Check_fast(board_data, isMaximizing) == 0){
                moves_done++;
                score_move = Parallel_Negmax(copy, search_mem, tt, depth -1, ply + 1, (~isMaximizing) & 1, beta * -1, alpha * -1, t_info_arr);
                score_move.score *= -1;
                if(best_move.score < score_move.score){
                    best_move = *moves[move_idx];
                    best_move.score = score_move.score;
                }
            }
            Undo_Move(copy, board_data, moves[move_idx]);
            if(best_move.score > alpha) { 
                tt_flag = EXACT_FLAG; 
                alpha = score_move.score;
            }
            if(alpha >= beta) { 
                if(movetype == NORMAL_MOVES){
                    Insert_Killer(search_mem[MAX_ENGINE_THREADS]->move_arr[ply][KILLER_MOVE], &best_move);
                }
                //if non-attacking move, update history heuristic
                if(moves[move_idx]->to_type == NUM_PIECES){
                    Update_History_Table(&best_move, depth, search_mem[MAX_ENGINE_THREADS]->history_array);
                }
                tt_flag = BETA_FLAG;
                break;
            }
            move_idx++;
        }
    }
    pthread_mutex_t search_data_mut;
    pthread_barrier_t perft_bar;
    pthread_mutex_init(&search_data_mut, NULL);

    if(pthread_barrier_init(&perft_bar, NULL, 2)){
        printf("barrier init error\n");
    }
    int free_threads[MAX_ENGINE_THREADS];
    for(int i = 0; i < MAX_ENGINE_THREADS; i++){
        t_info_arr[i].tt = tt;
        (t_info_arr[i].search_mem) = search_mem[i];
        t_info_arr[i].depth = depth;
        t_info_arr[i].ply = ply;
        t_info_arr[i].alpha = alpha;
        t_info_arr[i].beta = beta;
        t_info_arr[i].board_data = board_data;
        t_info_arr[i].thread_num = i;
        t_info_arr[i].best_move = &best_move;
        t_info_arr[i].perft_bar = &perft_bar;       
        t_info_arr[i].free_threads = free_threads; 
        t_info_arr[i].moves_done = &moves_done;
        t_info_arr[i].search_data_mut = &search_data_mut;
        t_info_arr[i].perft_bar = &perft_bar;  
    }
    pthread_t pid[MAX_ENGINE_THREADS];
    int tid;
    for(int i = 0; i < MAX_ENGINE_THREADS; i++){
        free_threads[i] = 1;
    }
////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    for(int movetype = 1; movetype < NUM_MOVE_TYPES; movetype++){
        int move_idx = 0;
        move_t** moves = search_mem[MAX_ENGINE_THREADS]->move_arr[ply][movetype];
        while(moves[move_idx]->from != FULL){
            if(movetype != ATTACK_MOVES && movetype != NORMAL_MOVES){
                if(Verify_Legal_Move(board_data, moves[move_idx]) == 0){
                    move_idx++;
                    continue;
                }
            }
         /////////////////////////////////////////////////////////////////////
            if(Get_Availible_Thread_Engine(free_threads) == -1){
                int bar_ret = pthread_barrier_wait(&perft_bar);
                if(bar_ret != 0 && bar_ret != PTHREAD_BARRIER_SERIAL_THREAD){
                    printf("barrier wait error\n");
                }


                //only one thread at a time is allowed to update the best_move, 
                //so this should be safe to do like this.
                if(best_move.score > alpha) { 
                    tt_flag = EXACT_FLAG; 
                    alpha = score_move.score;
                }
                if(alpha >= beta) { 
                    if(best_move.to_type == NUM_PIECES){
                        Insert_Killer(search_mem[MAX_ENGINE_THREADS]->move_arr[ply][KILLER_MOVE], &best_move);
                    }
                    //if non-attacking move, update history heuristic
                    if(best_move.to_type == NUM_PIECES){
                        Update_History_Table(&best_move, depth, search_mem[MAX_ENGINE_THREADS]->history_array);
                    }
                    tt_flag = BETA_FLAG;
                    break;
                }
                bar_ret = pthread_barrier_wait(&perft_bar);
                if(bar_ret != 0 && bar_ret != PTHREAD_BARRIER_SERIAL_THREAD){
                    printf("barrier wait error\n");
                }
            }
            tid = Get_Availible_Thread_Engine(free_threads);   //get open memory
            if(tid == -1){
                printf("Error, tid = -1\n");
            }
            free_threads[tid] = -1;                     //set memory being used
            t_info_arr[tid].move = (moves[move_idx]);  //set the move as the right one
            t_info_arr[tid].tid = tid;                  //so that the thread can set its memory as availible from inside
            t_info_arr[tid].alpha = alpha;
            t_info_arr[tid].beta = beta;
            pthread_create(&(pid[tid]), NULL, Engine_Thread, (void*) &(t_info_arr[tid]));
            move_idx++;
         /////////////////////////////////////////////////////////////////////
        }
    }
    #if PRINT_DEBUG == 1
        printf("All threads created: D=%d\n", depth);
    #endif
    int open = Check_All_Open_Engine(free_threads);
    while(open != 1){
        pthread_barrier_wait(&perft_bar);
        pthread_barrier_wait(&perft_bar);
        open = Check_All_Open_Engine(free_threads);
        #if PRINT_DEBUG == 1
            printf("Cleared extra thread: D=%d\n",depth);
        #endif
    }
    #if PRINT_DEBUG == 1
        printf("Through the barriers: D=%d\n",depth);
    #endif
    for(int i = 0; i < MAX_ENGINE_THREADS; i++){
        pthread_join(pid[i], NULL);
    }
    #if PRINT_DEBUG == 1
        printf("All threads Joined: D=%d\n", depth);
    #endif
    if(best_move.score > alpha) { 
        tt_flag = EXACT_FLAG; 
    }
    if(alpha >= beta) { 
        if(best_move.to_type == NUM_PIECES){
            Insert_Killer(search_mem[MAX_ENGINE_THREADS]->move_arr[ply][KILLER_MOVE], &best_move);
        }
        //if non-attacking move, update history heuristic
        if(best_move.to_type == NUM_PIECES){
            Update_History_Table(&best_move, depth, search_mem[MAX_ENGINE_THREADS]->history_array);
        }
        tt_flag = BETA_FLAG;
    }

    //keep the history tables coherent
    for(int i = 0; i < NUM_PIECES; i++){
        for(int j = 0; j < 64; j++){
            for(int k = 0; k < MAX_ENGINE_THREADS; k++){
                search_mem[k]->history_array[i][j] += search_mem[MAX_ENGINE_THREADS]->history_array[i][j]/2;
            }
        }
    }
    if(moves_done == 0){
        best_move.score = MIN + ply;
    }
    Insert_Q_Trans_Table(board_data->zob_key, tt_flag, best_move, tt);
    return best_move;
}


void* Engine_Thread(void * thread_info){
    eng_thread_info_t* t_info = (eng_thread_info_t*)thread_info;
    int depth = t_info->depth;
    int ply = t_info->ply;
    trans_table_t* tt = t_info->tt;
    eng_search_mem_t* search_mem = t_info->search_mem;
    Board_Data_t* board_data = t_info->board_data;
    pthread_mutex_t* sd_mut = t_info->search_data_mut;
    pthread_barrier_t* pt_bar = t_info->perft_bar;
    move_t* move = t_info->move;
    move_t* best_move = t_info->best_move;
    int alpha = t_info->alpha;
    int beta = t_info->beta;
    int* moves_done = t_info->moves_done;
    int tid = t_info->tid;
    int* free_threads = t_info->free_threads;

    #if PRINT_DEBUG == 1
        printf("\tCalled Engine_Thread: %d, D=%d\n", tid, depth);
    #endif
    //make the threads move
    Board_Data_t* copy = search_mem->copy_arr[ply];
    Copy_Board(copy, board_data);
    move_t score_move = Default_Move();
    score_move.score = MIN + ply;
    int did_move = 0;
    if(Verify_Legal_Move(board_data, move)){
        Do_Move(copy, move);
        if(In_Check_fast(copy, board_data->to_move) == 0){
            did_move = 1;
            score_move = negmax(copy, search_mem, tt, depth -1, ply + 1, board_data->to_move, beta * -1, alpha * -1, 1, 0, 1);
            score_move.score *= -1;
        }else{
            score_move.score = MIN + ply;
        }
        Undo_Move(copy, board_data, move);
    }else{
        score_move.score = MIN + ply;
    }

    pthread_mutex_lock(sd_mut);
        #if PRINT_DEBUG == 1
            printf("\tIn mutex lock: %d, D=%d\n", tid, depth);
        #endif
        if(best_move->score < score_move.score){
            *best_move = *move;
            best_move->score = score_move.score;
        }
        if(did_move == 1){
            *moves_done += 1;
        }
        int bar_ret = pthread_barrier_wait(pt_bar);
        if(bar_ret != 0 && bar_ret != PTHREAD_BARRIER_SERIAL_THREAD){
            printf("barrier wait error\n");
        }
        free_threads[tid] = 1;
        bar_ret = pthread_barrier_wait(pt_bar);
        if(bar_ret != 0 && bar_ret != PTHREAD_BARRIER_SERIAL_THREAD){
            printf("barrier wait error\n");
        }
    pthread_mutex_unlock(sd_mut);
    #if PRINT_DEBUG == 1
        printf("\tFinished Engine Thread: %d, D=%d\n", tid, depth);
    #endif
    return NULL;
}


//returns the index of the free thread
int Get_Availible_Thread_Engine(int* free_threads){
    for(int i = 0; i < MAX_ENGINE_THREADS; i++){
        if(free_threads[i] == 1){
            return i;
        }
    }
    return -1;
}

//returns 1 only when all the threads are availible.
int Check_All_Open_Engine(int* free_threads){
    for(int i = 0; i < MAX_ENGINE_THREADS; i++){
        if(free_threads[i] != 1){
            return -1;
        }
    }
    return 1;
}