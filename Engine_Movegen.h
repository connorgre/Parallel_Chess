
#ifndef ENGINE_MOVEGEN_H
#define ENGINE_MOVEGEN_H


#define NUM_MOVE_TYPES 4
enum Move_Types{
    BEST_MOVE = 0,
    ATTACK_MOVES = 1,
    KILLER_MOVE = 2,
    NORMAL_MOVES = 3, 
};

void Get_All_Moves_Separate(Board_Data_t* board_data, move_t*** movelist, const int toMove);
void Order_Moves_Simple(move_t** movelist);
void Insert_Killer(move_t** movelist, move_t* move);
void Order_Normal_Moves(Board_Data_t* board_data, move_t** movelist, uint** history_arr, uint** butterfly_arr);
void Update_History_Table(move_t* move, int depth, uint** history_arr);
void Update_Butterfly_Table(move_t* move, int depth, uint** butterfly_arr);
void Reset_History_Table(uint** history_arr);
void Halve_History_Table(uint** history_arr);
#endif