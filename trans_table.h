
#ifndef TT_H
#define TT_H

#include "move.h"
#include "board.h"
#include "engine.h"

//I use an array of mutexes to have less likelyhood of collisions in the transposition tables
//I don't think this will cause too much of a slowdown.
#define ENG_NUM_MUTEX 10009
    //scores that I don't think can be reached by a position, 
#define INVALID_SCORE -12345
#define BEING_SEARCHED 12345

enum TT_Flags{
    EXACT_FLAG = 0,
    ALPHA_FLAG = -1,
    BETA_FLAG = 1,
    PRUNED = 2,
    NO_FLAG = -2
};
//THE TRANS_TABLE TYPES ARE DEFINED IN ENGINE.H BECAUSE THE ENGINE.H FILE NEEDS ACCESS
//TO THOSE DATA TYPES, AND THIS NEED ACCESS TO SEARCH DATA
move_t Default_Move();
small_move_t Default_Small();
trans_table_t* Init_Trans_Table(int size);
void Delete_Trans_Table(trans_table_t* tt);
move_t Probe_Trans_Table(U64 zob_key, int depth, int alpha, int beta, trans_table_t* tt);
void Insert_Trans_Table(U64 zob_key, int depth, int flag, move_t move, trans_table_t* tt);

move_t Probe_Q_Trans_Table(U64 zob_key, int alpha, int beta, trans_table_t* tt);
void Insert_Q_Trans_Table(U64 zob_key, int flag, move_t move, trans_table_t* tt);
void Reset_Trans_Table_Entry(U64 zob_key, trans_table_t* tt, int q_table);

move_t Small_to_Move(small_move_t small);
small_move_t Move_to_Small(move_t move);


#endif