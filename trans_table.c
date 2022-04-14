

#include "trans_table.h"
#include "bit_helper.h"
#include <pthread.h>


//#define ERROR_CHECK
#define USE_MUTEX
trans_table_t* Init_Trans_Table(int size){
    trans_table_t* tt = (trans_table_t*)malloc(sizeof(trans_table_t));
    tt->table_head = (tt_entry_t*)malloc(sizeof(tt_entry_t) * size);
    
    search_data_t neg_data = {-1,-1,-1,-1,-1,-1,-1};
    tt_entry_t neg_entry;
    neg_entry.search_data = neg_data;
    neg_entry.depth = -1;
    neg_entry.zob_key = FULL;

    for(int i = 0; i < size; i++){
        tt->table_head[i] = neg_entry;
    }
    pthread_mutex_t* mutex_arr = (pthread_mutex_t*)malloc(sizeof(*mutex_arr)*NUM_MUTEX);
    for(int i = 0; i < NUM_MUTEX; i++){
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
    for(int i = 0; i < NUM_MUTEX; i++){
        if(pthread_mutex_destroy(&(tt->mutex_arr[i]))){
            printf("mutex destroy error\n");
        }
    }
    free(tt->mutex_arr);
    free(tt);
    return;
}

search_data_t Probe_Trans_Table(U64 zob_key, int depth, trans_table_t* tt){
    int idx = (zob_key+depth) % tt->size;
    int mut_idx = (zob_key+depth) % NUM_MUTEX;
    #ifdef USE_MUTEX
    if(pthread_mutex_lock(&(tt->mutex_arr[mut_idx]))){
        printf("Mutex lock error\n");
    }
    #endif
    //we found the position
    if(tt->table_head[idx].zob_key == zob_key && tt->table_head[idx].depth == depth){
        #ifdef ERROR_CHECK
            if(tt->table_head[idx].search_data.pos_searched <= 0 || tt->table_head[idx].search_data.pos_searched > 200000){
                printf("Error, position in tt with bad value: %d\n",tt->table_head[idx].search_data.pos_searched);
            }
            if(tt->table_head[idx].depth == 1){
                if(tt->table_head[idx].search_data.pos_searched > 100){
                    printf("Error: depth 1 has more than 100 moves: (%d)\n", tt->table_head[idx].search_data.pos_searched);
                }
            }
        #endif
        #ifdef USE_MUTEX
        if(pthread_mutex_unlock(&(tt->mutex_arr[mut_idx]))){
            printf("Mutex lock error\n");
        }
        #endif
        return tt->table_head[idx].search_data;
    //first time encountering the position
    }else if(tt->table_head[idx].depth == -1 && tt->table_head[idx].zob_key == FULL){
        //node never been explored
        tt->table_head[idx].depth = depth;
        search_data_t neg_data = {-1,-1,-1,-1,-1,-1,-1};
        #ifdef USE_MUTEX
        if(pthread_mutex_unlock(&(tt->mutex_arr[mut_idx]))){
            printf("Mutex lock error\n");
        }
        #endif
        return neg_data;
    //means the position has been found, but another node is already exploring it 
        //(so no point in exploring it twice)
    }else if(tt->table_head[idx].zob_key == zob_key && tt->table_head[idx].depth == -1){
        //node currently being explored by another branch
        search_data_t zero_data = {0,0,0,0,0,0,0};
        #ifdef USE_MUTEX
        if(pthread_mutex_unlock(&(tt->mutex_arr[mut_idx]))){
            printf("Mutex lock error\n");
        }
        #endif
        return zero_data;
    }else{
        search_data_t neg_data = {-1,-1,-1,-1,-1,-1,-1};
        #ifdef USE_MUTEX
        if(pthread_mutex_unlock(&(tt->mutex_arr[mut_idx]))){
            printf("Mutex lock error\n");
        }
        #endif
        return neg_data;
    }
}

void Insert_Trans_Table(U64 zob_key, int depth, search_data_t search_data, trans_table_t* tt){
    int idx = (zob_key+depth) % tt->size;

    #ifdef USE_MUTEX
    int mut_idx = (zob_key+depth) % NUM_MUTEX;
    if(pthread_mutex_lock(&(tt->mutex_arr[mut_idx]))){
        printf("Mutex lock error\n");
    }
    #endif
    if(tt->table_head[idx].depth <= depth || tt->table_head[idx].depth == -1){
        tt->table_head[idx].depth = depth;
        tt->table_head[idx].zob_key = zob_key;
        Copy_Search_Data(&tt->table_head[idx].search_data, &search_data);
    }
    #ifdef USE_MUTEX
    if(pthread_mutex_unlock(&(tt->mutex_arr[mut_idx]))){
        printf("Mutex lock error\n");
    }
    #endif
}

