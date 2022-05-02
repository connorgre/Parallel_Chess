
#ifndef ENGINE_H
#define ENGINE_H

#include "board.h"
#include "perft_trans_table.h"
#include "move.h"

#define MIN (-0x7fff)
#define MAX ( 0x7fff)


//takes the size of move from 21 bytes to 7 bytes
//size of tt entry from 31 to 17 bytes
//the from and to are the offset (so just need to bitshift)
typedef struct Small_Move{
    byte from;
    byte to;
    byte from_type;
    byte to_type;
    byte flags;
    int16_t score;
} small_move_t; 

//I am going to hold the score of the position in the 
//score field of the move_t struct
typedef struct TT_Entry{
    U64 zob_key;
    small_move_t move;
    int8_t depth;
    int8_t flag;
}   tt_entry_t;

typedef struct Trans_Table{
    tt_entry_t* table_head;
    tt_entry_t* q_head;
    tt_entry_t default_entry;
    pthread_mutex_t* mutex_arr;
    int size;
} trans_table_t;
//4 pointers deep bc it is 
    //a pointer to an array             --each ply gets its own               
        //of an array                   --to differentiate normal and attack moves
            //of an array               --the actual array of move pointers
                //of pointers to moves  --pointers to moves so that sorting the moves is less memory movement
typedef struct Eng_Search_Mem{
    move_t**** move_arr;
    Board_Data_t** copy_arr;
    uint** history_array;
    uint** butterfly_array;
} eng_search_mem_t;

move_t Iterative_Deepening(Board_Data_t* board_data, eng_search_mem_t* search_mem,  trans_table_t* tt, int depth, int isMaximizing, int alpha, int beta);
move_t negmax(Board_Data_t* board_data, eng_search_mem_t* search_mem,  trans_table_t* tt, int depth, int ply, int isMaximizing, int alpha, int beta, int allow_null, int onPV, int cut);
move_t Quiscence_Search(Board_Data_t* board_data, eng_search_mem_t* search_mem, int ply, int isMaximizing, int alpha, int beta, trans_table_t* tt);
eng_search_mem_t* Init_Eng_Search_Mem();
void Delete_Eng_Search_Mem(eng_search_mem_t* search_mem);
int Verify_Legal_Move(Board_Data_t* board_data, move_t* move);
#endif