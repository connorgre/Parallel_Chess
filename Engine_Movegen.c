
#include "move.h"
#include "board.h"
#include "movegen.h"
#include "perft.h"
#include "Engine_Movegen.h"
#include "trans_table.h"
//indexed by [attacker_type % 6][attacked_type % 6] (attacker is the row, attacked is the column)
//the lower the score the better the move (ie the best move is taking the king with a pawn, worst is taking a pawn with the king)
/*                                 //k   q    r    b    n    p 
const int MVVLVA_arr[6][6] = {  { -6,  -5,  -4,  -3,  -2,  -1},  //k
                                {-16, -15, -14, -13, -12, -11},  //q
                                {-26, -25, -24, -23, -22, -21},  //r
                                {-36, -35, -34, -33, -32, -31},  //b
                                {-46, -45, -44, -43, -42, -41},  //n
                                {-56, -55, -54, -53, -52, -51}}; //p
*/
                                 //k   q    r    b    n    p 
const int MVVLVA_arr[6][6] = {  {-51, -41, -31, -21, -11, -1},  //k
                                {-52, -42, -32, -22, -12, -2},  //q
                                {-53, -43, -33, -23, -13, -3},  //r
                                {-54, -44, -34, -24, -14, -4},  //b
                                {-55, -45, -35, -25, -15, -5},  //n
                                {-56, -46, -36, -26, -16, -6}}; //p
//this will load movelist as 
//      movelist[0] == attacks
//      movelist[1] == normal moves
//This will make sorting moves faster, as I don't need to bother sorting moves
void Get_All_Moves_Separate(Board_Data_t* board_data, move_t*** movelist, const int toMove){
    int num_moves_norm = 0;
    int num_moves_attack = 0;
    int team_offset = 6*toMove;
    int enemy_offset = 6*(~toMove & 1);
    U64 enemy_tiles = board_data->team_tiles[(~toMove) & 1];

    for(int i = 0 + team_offset; i < 6+team_offset; i++){
        U64 pieces = board_data->pieces[i];
        while(pieces){
            int from_type = i;
            U64 piece = Get_LSB(pieces);
            pieces ^= piece;
            U64 p_moves = Get_Piece_Moves(board_data, toMove, piece, i%6);
            U64 attack_moves = p_moves & enemy_tiles;
            p_moves ^= attack_moves;
            while(p_moves){
                U64 move = Get_LSB(p_moves);
                p_moves ^= move;
                U64 from = piece;
                U64 to = move;
                //if we didnt find a piece, moving to blank square
                int to_type = NUM_PIECES;
                //if we are promoting
                if((i == team_offset + W_PAWN) && ((to & (top_wall | bottom_wall)) != 0)){
                    //iterate through the allowed promotions (queen->rook->bishop->knight)
                    for(int j = W_QUEEN + team_offset; j < W_PAWN + team_offset; j++){
                        //non-attacking promotion
                        movelist[ATTACK_MOVES][num_moves_attack]->from = from;
                        movelist[ATTACK_MOVES][num_moves_attack]->to = to;
                        movelist[ATTACK_MOVES][num_moves_attack]->from_type = from_type;
                        movelist[ATTACK_MOVES][num_moves_attack]->to_type = to_type;
                        movelist[ATTACK_MOVES][num_moves_attack]->flags = (byte) j;
                        movelist[ATTACK_MOVES][num_moves_attack]->score = -100 + j;
                        num_moves_attack++;
                    }
                }else{
                    //non_attacking move move
                    movelist[NORMAL_MOVES][num_moves_norm]->from = from;
                    movelist[NORMAL_MOVES][num_moves_norm]->to = to;
                    movelist[NORMAL_MOVES][num_moves_norm]->from_type = from_type;
                    movelist[NORMAL_MOVES][num_moves_norm]->to_type = to_type;
                    movelist[NORMAL_MOVES][num_moves_norm]->flags = Set_Flags(board_data, toMove, piece, move);
                    movelist[NORMAL_MOVES][num_moves_norm]->score = 0;
                    num_moves_norm++;
                }
            }
            while(attack_moves){
                U64 move = Get_LSB(attack_moves);
                attack_moves ^= move;
                U64 from = piece;
                U64 to = move;
                int to_type = 0;
                //doing this rather than having 6 if statements that check each value is probably faster
                //could also have 6 cmoves
                for(int j = 0; j < 6; j++){
                    to_type += (j + enemy_offset) * ((board_data->pieces[j+enemy_offset] & move) != ZERO);
                }
                //if we didnt find a piece, moving to blank square
                if(to_type == 0) to_type = NUM_PIECES;
                //if we are promoting
                if((i == team_offset + W_PAWN) && ((to & (top_wall | bottom_wall)) != 0)){
                    //iterate through the allowed promotions (queen->rook->bishop->knight)
                    for(int j = W_QUEEN + team_offset; j < W_PAWN + team_offset; j++){
                        //attacking promotion
                        movelist[ATTACK_MOVES][num_moves_attack]->from = from;
                        movelist[ATTACK_MOVES][num_moves_attack]->to = to;
                        movelist[ATTACK_MOVES][num_moves_attack]->from_type = from_type;
                        movelist[ATTACK_MOVES][num_moves_attack]->to_type = to_type;
                        movelist[ATTACK_MOVES][num_moves_attack]->flags = (byte) j;
                        movelist[ATTACK_MOVES][num_moves_attack]->score = -110 + j;
                        num_moves_attack++;
                    }
                }else{
                    //attacking move
                    movelist[ATTACK_MOVES][num_moves_attack]->from = from;
                    movelist[ATTACK_MOVES][num_moves_attack]->to = to;
                    movelist[ATTACK_MOVES][num_moves_attack]->from_type = from_type;
                    movelist[ATTACK_MOVES][num_moves_attack]->to_type = to_type;
                    movelist[ATTACK_MOVES][num_moves_attack]->flags = Set_Flags(board_data, toMove, piece, move);
                    movelist[ATTACK_MOVES][num_moves_attack]->score = MVVLVA_arr[from_type%6][to_type%6];
                    num_moves_attack++;
                }
            }
        }
    }
    
    movelist[NORMAL_MOVES][num_moves_norm]->from = FULL;
    movelist[NORMAL_MOVES][num_moves_norm]->to = FULL;
    movelist[NORMAL_MOVES][num_moves_norm]->from_type = 0xFF;
    movelist[NORMAL_MOVES][num_moves_norm]->to_type = 0xFF;
    movelist[NORMAL_MOVES][num_moves_norm]->flags = 0xFF;

    movelist[ATTACK_MOVES][num_moves_attack]->from = FULL;
    movelist[ATTACK_MOVES][num_moves_attack]->to = FULL;
    movelist[ATTACK_MOVES][num_moves_attack]->from_type = 0xFF;
    movelist[ATTACK_MOVES][num_moves_attack]->to_type = 0xFF;
    movelist[ATTACK_MOVES][num_moves_attack]->flags = 0xFF;
    return;
}

//just simple selection sort based on mvvlva array
//additionally, it puts the 'lowest' scoring move last,
//but that works bc the mvvlva array puts the best
//moves as the move negative
void Order_Moves_Simple(move_t** movelist){
    if(movelist[0]->from == FULL){
        //if there are no moves to order
        return;
    }
    int i = 0;
    int j;
    move_t* temp_move;
    while(movelist[i+1]->from != FULL){
        int min_idx = i;
        j = i + 1;
        while(movelist[j]->from != FULL){
            if(movelist[j]->score < movelist[min_idx]->score){
                min_idx = j;
            }
            j++;
        }
        //swap the moves
        temp_move = movelist[i];
        movelist[i] = movelist[min_idx];
        movelist[min_idx]= temp_move;
        i++;
    }
}

void Order_Normal_Moves(Board_Data_t* board_data, move_t** movelist, uint** history_arr, uint** butterfly_arr){
    int i = 0;
    int j;
    move_t* temp_move;
    while(movelist[i+1]->from != FULL){
        int max_idx = i;
        j = i + 1;
        float best_score = (float)history_arr[movelist[i]->from_type][movelist[i]->to_type] / (float)butterfly_arr[movelist[i]->from_type][movelist[i]->to_type];
        while(movelist[j]->from != FULL){
            float move_score = (float)history_arr[movelist[j]->from_type][Get_Idx(movelist[j]->to)] / (float)butterfly_arr[movelist[j]->from_type][Get_Idx(movelist[j]->to)];
            if(move_score > best_score){
                max_idx = j;
                best_score = move_score;
            }
            j++;
        }
        //swap the moves
        temp_move = movelist[i];
        movelist[i] = movelist[max_idx];
        movelist[max_idx]= temp_move;
        i++;
    }
}

void Insert_Killer(move_t** movelist, move_t* move){
    if(move->from != movelist[0]->from && move->to != movelist[0]->to){
        memcpy(movelist[1], movelist[0], sizeof(move_t));
        memcpy(movelist[0], move, sizeof(move_t));
    }
}

void Update_History_Table(move_t* move, int depth, uint** history_arr){
    int p_idx = move->from_type;
    int pos_idx = Get_Idx(move->to);
    history_arr[p_idx][pos_idx] += 1 << depth;
}
void Update_Butterfly_Table(move_t* move, int depth, uint** butterfly_arr){
    int p_idx = move->from_type;
    int pos_idx = Get_Idx(move->to);
    butterfly_arr[p_idx][pos_idx] += depth * depth;
}

void Reset_History_Table(uint** history_arr){
    for(int i = 0; i < NUM_PIECES; i++){
        for(int j = 0; j < NUM_PIECES; j++){
            history_arr[i][j] = 0;
        }
    }
}
void Halve_History_Table(uint** history_arr){
    for(int i = 0; i < NUM_PIECES; i++){
        for(int j = 0; j < NUM_PIECES; j++){
            history_arr[i][j] /= 2;
        }
    }
}