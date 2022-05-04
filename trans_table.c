

#include "trans_table.h"
#include "bit_helper.h"
#include <pthread.h>

//#define ERROR_CHECK
#define USE_MUTEX

trans_table_t* Init_Trans_Table(int size){
    trans_table_t* tt = (trans_table_t*)malloc(sizeof(trans_table_t));
    tt->table_head = (tt_entry_t*)malloc(sizeof(tt_entry_t) * size);
    tt->q_head = (tt_entry_t*)malloc(sizeof(tt_entry_t) * size);
    
    tt_entry_t default_entry;
    default_entry.zob_key = FULL;
    default_entry.depth = -100;
    default_entry.move = Default_Small();
    default_entry.flag = NO_FLAG;

    tt->default_entry = default_entry;

    for(int i = 0; i < size; i++){
        tt->table_head[i] = default_entry;
        tt->q_head[i] = default_entry;
    }
    pthread_mutex_t* mutex_arr = (pthread_mutex_t*)malloc(sizeof(*mutex_arr)*ENG_NUM_MUTEX);
    for(int i = 0; i < ENG_NUM_MUTEX; i++){
        if(pthread_mutex_init(&mutex_arr[i], NULL)){
            printf("mutex create error\n");
        }
    }
    tt->size = size;
    tt->mutex_arr = mutex_arr;
    return tt;
}

void Delete_Trans_Table(trans_table_t* tt){
    free(tt->table_head);
    free(tt->q_head);
    for(int i = 0; i < ENG_NUM_MUTEX; i++){
        if(pthread_mutex_destroy(&(tt->mutex_arr[i]))){
            printf("mutex destroy error\n");
        }
    }
    free(tt->mutex_arr);
    free(tt);
    return;
}

//returns default entry when we should continue the search (our position isn't in the table/ depth too high)
//returns a move when either: the position has already been searched or another thread is searching it
    //if another thread is searching it, returned move will have move->from == FULL
    //if this happens, then we treat it as the worst possible move, and return MIN for the score (done in engine)
move_t Probe_Trans_Table(U64 zob_key, int depth, int alpha, int beta, trans_table_t* tt){
    int idx = ((zob_key) % (tt->size));
    int mut_idx = (zob_key) % ENG_NUM_MUTEX;  
    if(pthread_mutex_lock(&(tt->mutex_arr[mut_idx]))){
        printf("Mutex lock error\n");
    }
    tt_entry_t entry = tt->table_head[idx];
    move_t ret_move;
    if(entry.zob_key == zob_key && entry.depth >= depth)
    {//we found the position
        ret_move = Small_to_Move(entry.move);
        if(ret_move.score != BEING_SEARCHED)
        {//if we are actually going to take the score of the position
            #if ERROR_CHECK == 1
                if(ret_move.score == INVALID_SCORE){
                    printf("Error: score shouldn't be invalid in probe\n");
                }
            #endif
            if(entry.flag == ALPHA_FLAG && entry.move.score <= alpha){
                ret_move.score = alpha;
            }else if(entry.flag == BETA_FLAG && entry.move.score >= beta){
                ret_move.score = beta;
            }else if(entry.flag != EXACT_FLAG){
                ret_move.score = INVALID_SCORE;
            }
        }
    }else if(entry.zob_key == zob_key){
        ret_move = Small_to_Move(entry.move);
        ret_move.score = INVALID_SCORE;
    }else{
        ret_move  = Small_to_Move(tt->default_entry.move);
    }
    if(entry.depth < depth)
    {// if we are searching a deeper than current index
        tt->table_head[idx].depth = depth;
        tt->table_head[idx].zob_key = zob_key;
        tt->table_head[idx].move.score = BEING_SEARCHED;
    }
    if(pthread_mutex_unlock(&(tt->mutex_arr[mut_idx]))){
        printf("Mutex lock error\n");
    }
    return ret_move;
}

//simple always replace if at a higher depth
void Insert_Trans_Table(U64 zob_key, int depth, int flag, move_t move, trans_table_t* tt){
    int idx = ((zob_key) % (tt->size));
    int mut_idx = (zob_key) % ENG_NUM_MUTEX;
    if(pthread_mutex_lock(&(tt->mutex_arr[mut_idx]))){
        printf("Mutex lock error\n");
    }
    //if we are at a higher depth than the currently entered position
    if(tt->table_head[idx].depth <= depth){
        tt->table_head[idx].zob_key = zob_key;
        tt->table_head[idx].depth = depth;
        tt->table_head[idx].move = Move_to_Small(move);
        tt->table_head[idx].flag = flag;
    }
    if(pthread_mutex_unlock(&(tt->mutex_arr[mut_idx]))){
        printf("Mutex lock error\n");
    }
}

void Reset_Trans_Table_Entry(U64 zob_key, trans_table_t* tt, int q_table){
    int idx = (zob_key % tt->size);
    int mut_idx = (zob_key) % ENG_NUM_MUTEX;
    if(pthread_mutex_lock(&(tt->mutex_arr[mut_idx]))){
        printf("Mutex lock error\n");
    }
    tt_entry_t* entry;
    if(q_table == 0){
        entry = &(tt->table_head[idx]);
    }else{
        entry = &(tt->q_head[idx]);
    }
    if(entry->zob_key == zob_key){
        *entry = tt->default_entry;
    }
    if(pthread_mutex_unlock(&(tt->mutex_arr[mut_idx]))){
        printf("Mutex lock error\n");
    }
}

move_t Probe_Q_Trans_Table(U64 zob_key, int alpha, int beta, trans_table_t* tt){
    int idx = ((zob_key) % (tt->size));
    int mut_idx = (zob_key) % ENG_NUM_MUTEX;  
    if(pthread_mutex_lock(&(tt->mutex_arr[mut_idx]))){
        printf("Mutex lock error\n");
    }
    tt_entry_t entry = tt->q_head[idx];
    move_t ret_move;
    if(entry.zob_key == zob_key)
    {//we found the position
        ret_move = Small_to_Move(entry.move);
        if(ret_move.score != BEING_SEARCHED)
        {//if we are actually going to take the score of the position
            #if ERROR_CHECK == 1
                if(ret_move.score == INVALID_SCORE){
                    printf("Error: score shouldn't be invalid in probe\n");
                }
            #endif
            if(entry.flag == ALPHA_FLAG && entry.move.score <= alpha){
                ret_move.score = alpha;
            }else if(entry.flag == BETA_FLAG && entry.move.score >= beta){
                ret_move.score = beta;
            }else if(entry.flag != EXACT_FLAG){
                ret_move.score = INVALID_SCORE;
            }
        }
    }
    else
    {// if we are searching a deeper than current index
        ret_move = Small_to_Move(tt->default_entry.move);
        tt->q_head[idx].zob_key = zob_key;
        tt->q_head[idx].move.score = BEING_SEARCHED;
    }
    if(pthread_mutex_unlock(&(tt->mutex_arr[mut_idx]))){
        printf("Mutex lock error\n");
    }
    return ret_move;
}
void Insert_Q_Trans_Table(U64 zob_key, int flag, move_t move, trans_table_t* tt){
    int idx = ((zob_key) % (tt->size));
    int mut_idx = (zob_key) % ENG_NUM_MUTEX;
    if(pthread_mutex_lock(&(tt->mutex_arr[mut_idx]))){
        printf("Mutex lock error\n");
    }
    tt->q_head[idx].zob_key = zob_key;
    tt->q_head[idx].move = Move_to_Small(move);
    tt->q_head[idx].flag = flag;
    if(pthread_mutex_unlock(&(tt->mutex_arr[mut_idx]))){
        printf("Mutex lock error\n");
    }
}

move_t Default_Move(){
    move_t default_move;
    default_move.from = FULL;
    default_move.to = FULL;
    default_move.from_type = NUM_PIECES;
    default_move.to_type = NUM_PIECES;
    default_move.flags = 0xFF;
    default_move.score = INVALID_SCORE;
    return default_move;
}

small_move_t Default_Small(){
    small_move_t default_move;
    default_move.from = 0xFF;
    default_move.to = 0xFF;
    default_move.from_type = NUM_PIECES;
    default_move.to_type = NUM_PIECES;
    default_move.flags = 0xFF;
    default_move.score = INVALID_SCORE;
    return default_move;
}

move_t Small_to_Move(small_move_t small){
    move_t move;
    move.flags = small.flags;
    move.from_type = small.from_type;
    move.to_type = small.to_type;
    move.score = small.score;
    if(small.from == 0xFF)  {move.from = FULL;}
    else                    {move.from = ONE << small.from;}
    if(small.to == 0xFF)    {move.to = FULL;}
    else                    {move.to = ONE << small.to;}
    return move;
}

small_move_t Move_to_Small(move_t move){
    small_move_t small;
    small.flags = move.flags;
    small.from_type = move.from_type;
    small.to_type = move.to_type;
    small.score = move.score;
    if(move.from == FULL)   {small.from = 0xFF;}
    else                    {small.from = Get_Idx(move.from);}
    if(move.to == FULL)     {small.to = 0xFF;}
    else                    {small.to = Get_Idx(move.to);}
    return small;
}
