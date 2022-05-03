
#ifndef PERFT_TT_H
#define PERFT_TT_H

#include "move.h"
#include "board.h"
#include "perft.h"

//I use an array of mutexes to have less likelyhood of collisions in the transposition tables
//I don't think this will cause too much of a slowdown.
#define NUM_MUTEX 511

//THE TRANS_TABLE TYPES ARE DEFINED IN PERFT.H BECAUSE THE PERFT.H FILE NEEDS ACCESS
//TO THOSE DATA TYPES, AND THIS NEED ACCESS TO SEARCH DATA

perft_trans_table_t* Init_Perft_Trans_Table(int size);
void Delete_Perft_Trans_Table(perft_trans_table_t* tt);
search_data_t Probe_Perft_Trans_Table(U64 zob_key, int depth, perft_trans_table_t* tt);
void Insert_Perft_Trans_Table(U64 zob_key, int depth, search_data_t* search_data, perft_trans_table_t* tt);

#endif