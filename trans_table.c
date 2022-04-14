

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
    neg_entry.num_using = -1;

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
    if(pthread_mutex_lock(&(tt->mutex_arr[mut_idx]))){
        printf("Mutex lock error\n");
    }
    //we found the position
    if(tt->table_head[idx].zob_key == zob_key && tt->table_head[idx].depth == depth){
        if(pthread_mutex_unlock(&(tt->mutex_arr[mut_idx]))){
            printf("Mutex lock error\n");
        }
        return tt->table_head[idx].search_data;
    //first time encountering the position -- set the zob key but leave depth as -1.
    }else if(tt->table_head[idx].depth == -1 && tt->table_head[idx].zob_key == FULL){
        //node never been explored
        tt->table_head[idx].zob_key = zob_key;
        tt->table_head[idx].num_using++;
        search_data_t neg_data = {-1,-1,-1,-1,-1,-1,-1};
        if(pthread_mutex_unlock(&(tt->mutex_arr[mut_idx]))){
            printf("Mutex lock error\n");
        }
        return neg_data;
    //means the position has been found, but another node is already exploring it 
        //(so no point in exploring it twice)
    }else if(tt->table_head[idx].zob_key == zob_key && tt->table_head[idx].depth == -1 && tt->table_head[idx].num_using == -1){
        //node currently being explored by another branch
        search_data_t zero_data = {0,0,0,0,0,0,0};
        tt->table_head[idx].num_using++;
        if(pthread_mutex_unlock(&(tt->mutex_arr[mut_idx]))){
            printf("Mutex lock error\n");
        }
        return zero_data;
    }else{
        search_data_t neg_data = {-1,-1,-1,-1,-1,-1,-1};
        //printf("probably shouldn't get here... idk tho\n");
        if(pthread_mutex_unlock(&(tt->mutex_arr[mut_idx]))){
            printf("Mutex lock error\n");
        }
        return neg_data;
    }
}

void Insert_Trans_Table(U64 zob_key, int depth, search_data_t* search_data, trans_table_t* tt){
    int idx = (zob_key+depth) % tt->size;

    int mut_idx = (zob_key+depth) % NUM_MUTEX;
    if(pthread_mutex_lock(&(tt->mutex_arr[mut_idx]))){
        printf("Mutex lock error\n");
    }
    //if its a shallower depth, or unusued, and no threads are waiting for the result
    if((tt->table_head[idx].depth <= depth || tt->table_head[idx].depth == -1) && tt->table_head[idx].num_using < 1){
        tt->table_head[idx].depth = depth;
        tt->table_head[idx].zob_key = zob_key;
        tt->table_head->num_using = -1;
        Copy_Search_Data(&(tt->table_head[idx].search_data), search_data);
    }else if(tt->table_head[idx].zob_key == zob_key && tt->table_head[idx].depth == -1 && tt->table_head->num_using > 0){
        //if this is the correct index, and it has multiple threads waiting on it
        tt->table_head[idx].depth = depth;
        tt->table_head[idx].zob_key = zob_key;
        Copy_Search_Data(&(tt->table_head[idx].search_data), search_data);
        for(int i = 0; i < tt->table_head[idx].num_using; i++){
            //for every thread that stopped searching while this node was searching, 
            //we add an extra search data onto that.  Could add a multiply, but this
            //should be fine.
            Add_Search_Data(search_data, &(tt->table_head[idx].search_data));
        }
        tt->table_head[idx].num_using = -1;
    }
    if(pthread_mutex_unlock(&(tt->mutex_arr[mut_idx]))){
        printf("Mutex lock error\n");
    }
}

