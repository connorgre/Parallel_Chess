#include "engine.h"
#include "evaluation.h"
#include "move.h"
#include "movegen.h"
#include "movegen_fast.h"
#include "Engine_Movegen.h"
#include "trans_table.h"
#include <math.h>


#define ALPHA_BETA_PRUNE 1
#define ORDER_MOVES 1
#define Q_SEARCH 1
#define USE_TT 1
#define NULL_PRUNE 1
#define PRINT_SEARCHED 1
#define USE_KILLER_MOVES 1
#define LATE_MOVE_REDUCTION 1
#define HISTORY_HUERISTIC 1
#define NEGSCOUT 1
#define MULTI_CUT 1
#define CHECK_EXTENSION 1
#define FUTILITY_PRUNE 1
#define EXTENDED_FUTILITY_PRUNE 1
#define ERROR_CHECK 0

long int depth_moves = 0;
long int quiscence_moves = 0;
long int trans_pos_found = 0;
long int null_moves_pruned = 0;
long int null_moves_searched = 0;
long int max_ply_reached = 0;
long int cut_tested = 0;
long int cut_pruned = 0;
long int futility_pruned = 0;
long int extended_futility_pruned = 0;

move_t Iterative_Deepening(Board_Data_t* board_data, eng_search_mem_t* search_mem,  trans_table_t* tt, int depth, int isMaximizing, int alpha, int beta){
    depth_moves = 0;
    quiscence_moves = 0;
    trans_pos_found = 0;
    null_moves_pruned = 0;
    null_moves_searched = 0;
    max_ply_reached = 0;
    cut_tested = 0;
    cut_pruned = 0;
    futility_pruned = 0;
    extended_futility_pruned = 0;
    
    int initial_d = (depth > 3) ? 3 : depth;
    move_t best_move = negmax(board_data, search_mem, tt, initial_d, 0, isMaximizing, alpha, beta, 1, 1, 0, 1, NULL, NULL);
    for(int i = 4; i <= depth; i++){
        //Reset_History_Table(search_mem->history_array);
        Halve_History_Table(search_mem->history_array);
        best_move = negmax(board_data, search_mem, tt, i, 0, isMaximizing, alpha, beta, 1, 1, 0, 1, NULL, NULL);
    }
    
    #if PRINT_SEARCHED == 1
        printf("Total Moves: %ld, \tDepth Moves: %ld, \nQuiescense Moves: %ld, Trans Found: %ld\nNull Moves Searched: %ld, Null Moves Pruned: %ld\n", 
                quiscence_moves, depth_moves, (quiscence_moves - depth_moves), trans_pos_found, null_moves_searched, null_moves_pruned); 
        printf("Cut tested: %ld, Cut Pruned: %ld\n", cut_tested, cut_pruned);
        printf("Max ply reached: %ld\n",max_ply_reached);
        printf("Futility Pruned: %ld, Extended Futility Pruned: %ld\n", futility_pruned, extended_futility_pruned);
    #endif
    return best_move;
}


move_t negmax(Board_Data_t* board_data, eng_search_mem_t* search_mem,  trans_table_t* tt, int depth, int ply, int isMaximizing, int alpha, int beta, int allow_null, int onPV, int cut, int probe_tt, int* parallel_alpha, int* parallel_beta){
    //if(depth_moves % 5000 == 1) {printf("Search moves: %ld\n",depth_moves); }
    
    #if ERROR_CHECK
        int errors = Verify_Board(board_data, isMaximizing);
        if(errors != 0){
            printf("Error: incorrect board: %d\n", errors);
            Verify_Board(board_data, isMaximizing);
        }
    #endif
    
    //all children of PV nodes are cut nodes, children of cut nodes are all nodes, children of all nodes are cut nodes, and PV nodes are not cut nodes
    if(onPV == 1) cut = 0;
    int next_cut = (cut == 1) ? 0 : 1;

    move_t score_move = Default_Move();
    move_t best_move = Default_Move();
    best_move.score = MIN + ply;
    
    #if USE_TT == 1
        int tt_flag = ALPHA_FLAG;
        move_t tt_move = Probe_Trans_Table(board_data->zob_key, depth, alpha, beta, tt);

        if(tt_move.score != INVALID_SCORE && probe_tt == 1){
            trans_pos_found++;
            if(tt_move.score == BEING_SEARCHED){
                //return max if the position is being searched, so it appears super bad in the next level up,
                //so it won't be considered
                if(ply == 0){
                    #if ERROR_CHECK == 1
                        printf("Error, shouldn't return being searched from ply 0\n");
                    #endif                 
                }
                //tt_move.score = INVALID_SCORE;
                tt_move.score = MAX - ply;
                return tt_move;
            }else{
                if(Verify_Legal_Move(board_data, &tt_move)){
                    return tt_move;
                }
            }
        }
        memcpy(search_mem->move_arr[ply][BEST_MOVE][0], &tt_move, sizeof(move_t));
    #endif
    if(depth <= 0 || ply >= MAX_PLY){
        #if Q_SEARCH == 1
            //if another thread lowered beta
            if(parallel_alpha != NULL){
                if((*parallel_alpha) * -1 < beta){
                    beta = (*parallel_alpha) * -1;
                }
            }
            //if another thread raised alpha
            if(parallel_beta != NULL){
                if((*parallel_beta) * -1 > alpha){
                    alpha = (*parallel_beta) * -1;
                }
            }
            best_move = Quiscence_Search(board_data, search_mem, ply, isMaximizing, alpha, beta, tt);
        #else
            int mult = (isMaximizing == WHITE) ? 1 : -1;
            best_move.score = mult * score_board_fast(board_data);
        #endif
        depth_moves++;
        #if USE_TT == 1
            Insert_Trans_Table(board_data->zob_key, depth, tt_flag, best_move, tt);
        #endif
        return best_move;
    }
    Board_Data_t* copy = search_mem->copy_arr[ply];
    Copy_Board(copy, board_data);

    #if FUTILITY_PRUNE == 1 || CHECK_EXTENSION == 1 || NULL_PRUNE == 1
        int in_check = In_Check_fast(board_data, isMaximizing);
    #endif
    //if we cannot raise our score above alpha - threshold, 
    //dont bother searching the next depth
    #if FUTILITY_PRUNE == 1
        if(depth == 1 && in_check == 0 && onPV == 0){
            int mult = (isMaximizing == WHITE) ? 1 : -1;
            int score = mult * score_board_simple(board_data);
            
            if(score < alpha - QUEEN_VAL){  //if were down more than a queen just to the q search 
                score = mult * score_board_fast(board_data);
                best_move.score = score;
                Insert_Trans_Table(board_data->zob_key, depth, tt_flag, best_move, tt);
                futility_pruned++;
                return best_move;
            }else if(score < alpha){    //if static eval doesn't improve position, then do qsearch if it doesn't improve by at least a knight
                score = mult * score_board_fast(board_data);
                if(score < alpha - KNIGHT_VAL){
                    best_move = Quiscence_Search(board_data, search_mem, ply, isMaximizing, alpha, beta, tt);
                    Insert_Trans_Table(board_data->zob_key, depth, tt_flag, best_move, tt);
                    futility_pruned++;
                    return best_move;
                }
            }
        }
        #if EXTENDED_FUTILITY_PRUNE == 1
            if(depth == 2 && in_check == 0 && onPV == 0){
                int mult = (isMaximizing == WHITE) ? 1 : -1;
                int score = mult * score_board_simple(board_data);
                if(score < alpha){
                    int score = mult * score_board_fast(board_data);
                    if(score < alpha - ROOK_VAL){
                        best_move = Quiscence_Search(board_data, search_mem, ply, isMaximizing, alpha, beta, tt);
                        Insert_Trans_Table(board_data->zob_key, depth, tt_flag, best_move, tt);
                        extended_futility_pruned++;
                        return best_move;
                    }
                }
            }
        #endif
    #endif


    #if CHECK_EXTENSION == 1
        if(in_check && ply > 1) depth++;   //increase search depth if in check
    #endif
    #if NULL_PRUNE == 1
        int null_r = (int) (2 * log((double) depth));
        if(null_r < 3) null_r = 3;

        if(depth > 3 && allow_null == 1 && onPV == 0 && in_check == 0){
            //Null move pruning, if the opponent cannot improve their position when given
            //a free move, then we assume we have a beta cutoff
            Make_Null_Move(copy);
            null_moves_searched++;
            //alpha=beta*-1 -- if another thread raised alpha
            if(parallel_beta != NULL){
                if((*parallel_beta)*-1 > alpha){
                    alpha = (*parallel_beta) * -1;
                }
            }
            score_move = negmax(copy, search_mem, tt, depth - 1 - null_r, ply + 1, (~isMaximizing & 1), (alpha + 1) * -1, alpha * -1, 0, 0, 1, 1, NULL, NULL);
            score_move.score *= -1;
            Undo_Null_Move(copy, board_data); 
            //beta=alpha*-1 -- another thread lowered beta
            if(parallel_alpha != NULL){
                if(*parallel_alpha * -1 < beta){
                    beta = *parallel_alpha * -1;
                }
            }
            if(score_move.score >= beta){
                tt_flag = BETA_FLAG;
                best_move.score = score_move.score;
                #if USE_TT == 1                        
                    Insert_Trans_Table(board_data->zob_key, depth, tt_flag, best_move, tt);
                #endif
                null_moves_pruned++;
                return best_move;
            }
        }
    #endif

    Get_All_Moves_Separate(board_data, search_mem->move_arr[ply], isMaximizing);
    #if ORDER_MOVES == 1
        Order_Moves_Simple(search_mem->move_arr[ply][ATTACK_MOVES]);
        #if USE_TT == 1
            search_mem->move_arr[ply][BEST_MOVE][0][0] = tt_move;
        #endif
    #endif

    int moves_done = 0;
    #if MULTI_CUT == 1
        int multi_r = (int) (2 * log((double) depth));
        if(multi_r < 3) multi_r = 3;
        int num_cut = 0;
        int cut_threshold = multi_r - 1;
        int max_moves = 4 * multi_r;
        if(depth > multi_r && cut == 1 && in_check == 0 && onPV == 0){
            #if ERROR_CHECK == 1
                if(onPV != 0){
                    printf("Error, doing multi-cut on PV\n");
                }
            #endif
            
            cut_tested++;
            for(int movetype = 0; movetype < NUM_MOVE_TYPES; movetype++){
                int move_idx = 0;
                move_t** moves = search_mem->move_arr[ply][movetype];
                #if HISTORY_HUERISTIC == 1
                    if(movetype == NORMAL_MOVES){
                        Order_Normal_Moves(board_data, search_mem->move_arr[ply][NORMAL_MOVES], search_mem->history_array, search_mem->butterfly_array);
                    }
                #endif
                while(moves[move_idx]->from != FULL){
                    if(moves_done > max_moves){ break;}
                    if(movetype != ATTACK_MOVES && movetype != NORMAL_MOVES){
                        if(Verify_Legal_Move(board_data, moves[move_idx]) == 0){
                            move_idx++;
                            continue;
                        }
                    }
                    Do_Move(copy, moves[move_idx]);
                    #if ERROR_CHECK
                        errors = Verify_Board(copy, (~isMaximizing & 1));
                        if(errors == 1){
                            printf("Error: incorrect board (copy): %d\n", errors);
                            Verify_Board(copy, (~isMaximizing & 1));
                        }
                    #endif
                    //alpha=beta*-1 -- if a different thread raised alpha
                    if(parallel_beta != NULL){
                        if((*parallel_beta)*-1 > alpha){
                            alpha = (*parallel_beta) * -1;
                        }
                    }
                    score_move.score = MIN-1;
                    if(In_Check_fast(copy, isMaximizing) == 0){
                        score_move = negmax(copy, search_mem, tt, depth - 1 - multi_r, ply + 1, (~isMaximizing & 1), (alpha + 1) * -1, alpha * -1, 1, 0, 0, 1,NULL, NULL);
                        score_move.score *= -1;
                        moves_done++;
                    }
                    if(best_move.score < score_move.score){
                        best_move = score_move;
                    }
                    Undo_Move(copy, board_data, moves[move_idx]); 
                    //beta=alpha*-1 -- if a different thread lowered beta
                    if(parallel_alpha != NULL){
                        if((*parallel_alpha) * -1 < beta){
                            beta = (*parallel_alpha) * -1;
                        }
                    }
                    if(best_move.score >= beta) { 
                        num_cut++;
                        if(num_cut == cut_threshold){
                            cut_pruned++;
                            best_move.score = beta;
                            #if USE_TT == 1
                                Insert_Trans_Table(board_data->zob_key, depth, BETA_FLAG, best_move, tt);

                            #endif
                            return best_move;
                        }
                    }
                    move_idx++; 
                }
                if(moves_done > max_moves){ break;}
            }
        }
        moves_done = 0;
        score_move = Default_Move();
        best_move = Default_Move();
        best_move.score = MIN + ply;
    #endif

    #if LATE_MOVE_REDUCTION == 1
        //for late move reduction
        int failed_high = 0;
    #endif
    int lmr_val = 0;

//////////////////////////////////////////////////////////////////////////////////////
///////////////////////// -- MAIN SEARCH LOOP --       ///////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

    //best move is 0, killer moves are 1, attacks are 2, normal moves are 3
    int rev_beta = beta;
    for(int movetype = 0; movetype < NUM_MOVE_TYPES; movetype++){
        int move_idx = 0;
        move_t** moves = search_mem->move_arr[ply][movetype];
        #if HISTORY_HUERISTIC == 1
            if(movetype == NORMAL_MOVES){
                Order_Normal_Moves(board_data, search_mem->move_arr[ply][NORMAL_MOVES], search_mem->history_array, search_mem->butterfly_array);
            }
        #endif
        while(moves[move_idx]->from != FULL){
            #if LATE_MOVE_REDUCTION == 1
                if(moves_done > 2 && onPV == 0 && depth > 2){
                    if(failed_high == 0){
                        lmr_val = (int)(log((double)depth) * log((double)moves_done));
                        //if we are going to reduce by more than the depth, just stop the search
                        if(lmr_val > depth) break;
                    }
                }
            #endif
            //if a parallel thread has managed to raise alpha, raise it for this thread
            if(movetype != ATTACK_MOVES && movetype != NORMAL_MOVES){
                if(Verify_Legal_Move(board_data, moves[move_idx]) == 0){
                    move_idx++;
                    continue;
                }
            }
            //copy the board, then do the move on the copied board so we don't
            //need to worry about undoing the move
            #if ERROR_CHECK
                if(moves[move_idx]->to_type == W_KING || moves[move_idx]->to_type == B_KING){
                    printf("Attempting to take king\n");
                    Print_Board(board_data->occ, board_data);
                }
            #endif
            Do_Move(copy, moves[move_idx]);
            #if ERROR_CHECK
                errors = Verify_Board(copy, (~isMaximizing & 1));
                if(errors == 1){
                    printf("Error: incorrect board (copy): %d\n", errors);
                    Verify_Board(copy, (~isMaximizing & 1));
                }
            #endif
            //alpha=beta*-1 -- if a different thread raised alpha
            if(parallel_beta != NULL){
                if((*parallel_beta)*-1 > alpha){
                    if(rev_beta == alpha + 1){
                        rev_beta = (*parallel_beta) * -1 + 1;
                    }
                    alpha = (*parallel_beta) * -1;
                }
            }
            score_move.score = MIN-1;
            if(In_Check_fast(copy, isMaximizing) == 0){
                score_move = negmax(copy, search_mem, tt, depth - 1 - lmr_val, ply + 1, (~isMaximizing & 1), rev_beta * -1, alpha * -1, 1, onPV, next_cut, 1, NULL, NULL);
                score_move.score *= -1;
                #if NEGSCOUT == 1
                    //alpha=beta*-1 -- if a different thread raised alpha
                    if(parallel_beta != NULL){
                        if((*parallel_beta)*-1 > alpha){
                            alpha = (*parallel_beta) * -1;
                        }
                    }
                    //beta=alpha*-1 -- if a different thread lowered beta
                    if(parallel_alpha != NULL){
                        if((*parallel_alpha)*-1 < beta){
                            beta = (*parallel_alpha) * -1;
                        }
                    }
                    if(score_move.score > alpha && score_move.score < beta && moves_done > 0){
                        //dont probe the transposition table at root node tho
                        score_move = negmax(copy, search_mem, tt, depth - 1 - lmr_val, ply + 1, (~isMaximizing & 1), beta * -1, alpha * -1, 1, onPV, next_cut, 0, NULL, NULL);
                        score_move.score *= -1;
                    }
                #endif
                //after weve done a move we guaranteed arent on the PV anymore
                onPV = 0;
                moves_done++;
            }
            #if ALPHA_BETA_PRUNE == 0
                Undo_Move(copy, board_data, moves[move_idx]); 
            #endif
            if(best_move.score < score_move.score){
                best_move = *moves[move_idx];
                best_move.score = score_move.score;
            }
            //alpha=beta*-1 -- if a different thread raised alpha
            if(parallel_beta != NULL){
                if((*parallel_beta)*-1 > alpha){
                    alpha = (*parallel_beta) * -1;
                }
            }
            //beta=alpha*-1 -- if a different thread lowered beta
            if(parallel_alpha != NULL){
                if((*parallel_alpha)*-1 < beta){
                    beta = (*parallel_alpha) * -1;
                }
            }
            #if ALPHA_BETA_PRUNE == 1
                #if LATE_MOVE_REDUCTION == 1
                    //if we were doing a late move reduction failed high, research at full depth
                    if(lmr_val != 0 && score_move.score > alpha){
                        score_move = negmax(copy, search_mem, tt, depth -1, ply + 1, (~isMaximizing & 1), beta * -1, alpha * -1, 1, onPV, next_cut, 1, NULL, NULL);
                        score_move.score *= -1;
                    }
                #endif
                Undo_Move(copy, board_data, moves[move_idx]);
                //alpha=beta*-1 -- if a different thread raised alpha
                if(parallel_beta != NULL){
                    if((*parallel_beta)*-1 > alpha){
                        alpha = (*parallel_beta) * -1;
                    }
                }
                //beta=alpha*-1 -- if a different thread lowered beta
                if(parallel_alpha != NULL){
                    if((*parallel_alpha)*-1 < beta){
                        beta = (*parallel_alpha) * -1;
                    }
                }
                if(score_move.score > alpha) { 
                    #if LATE_MOVE_REDUCTION == 1
                        failed_high = 1;
                    #endif
                    #if USE_TT == 1
                        tt_flag = EXACT_FLAG; 
                    #endif
                    alpha = score_move.score;
                }
                if(alpha >= beta) { 
                    
                    #if USE_KILLER_MOVES == 1
                        //if this is a normal move that causes a beta cutoff, try sooner
                        if(movetype == NORMAL_MOVES){
                            Insert_Killer(search_mem->move_arr[ply][KILLER_MOVE], moves[move_idx]);
                            #if HISTORY_HUERISTIC == 1
                                for(int i = 0; i < move_idx; i++){
                                    Update_Butterfly_Table(moves[move_idx], depth - 1 - lmr_val, search_mem->butterfly_array);
                                }
                            #endif

                        }
                    #endif
                    
                    #if HISTORY_HUERISTIC == 1
                        //if non-attacking move, update history heuristic
                        if(moves[move_idx]->to_type == NUM_PIECES){
                            Update_History_Table(moves[move_idx], depth -1 - lmr_val, search_mem->history_array);
                        }
                    #endif
                    #if USE_TT == 1
                        tt_flag = BETA_FLAG;
                    #endif
                    break;
                }
                #if HISTORY_HUERISTIC == 1
                else{
                    if(moves[move_idx]->to_type == NUM_PIECES){
                        Update_Butterfly_Table(moves[move_idx], depth - 1 - lmr_val, search_mem->butterfly_array);
                    }
                }
                #endif
            #endif
            #if NEGSCOUT == 1
                rev_beta = alpha + 1;
            #endif
            move_idx++; 
        }
        if(alpha >= beta) { break; }
    }
    if(moves_done == 0) { best_move.score = MIN + ply; }
    #if USE_TT == 1
        Insert_Trans_Table(board_data->zob_key, depth, tt_flag, best_move, tt);
    #endif
    return best_move;
}



/////////////////////////////////////////////////////////////////////////////////////
//////////////////////// -- QUISCENCE SEARCH --      ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////

move_t Quiscence_Search(Board_Data_t* board_data, eng_search_mem_t* search_mem, int ply, int isMaximizing, int alpha, int beta, trans_table_t* tt){
    max_ply_reached = (ply > max_ply_reached) ? ply : max_ply_reached;
    #if USE_TT == 1
        int tt_flag = ALPHA_FLAG;
        move_t tt_move = Probe_Q_Trans_Table(board_data->zob_key, alpha, beta, tt);
        #if ERROR_CHECK == 1
                if(PopCount(tt_move.from) != 1 && tt_move.from != FULL){
                    printf("Error, invalid moves -- full -- main tt\n");
                }
                if(PopCount(tt_move.to) != 1 && tt_move.to != FULL){
                    printf("Error, invalid moves -- full -- main tt\n");
                }
        #endif
        if(tt_move.score != INVALID_SCORE){
            trans_pos_found++;
            quiscence_moves++;
            if(tt_move.score == BEING_SEARCHED){
                //return max if the position is being searched, so it appears super bad in the next level up,
                //then it just wont be searched.
                //tt_move.score = INVALID_SCORE;
                tt_move.score = MAX-1;
                return tt_move;
            }else{
                if(Verify_Legal_Move(board_data, &tt_move)){
                    return tt_move;
                }
            }
        }
    #endif
    
    
    move_t score_move = Default_Move();
    score_move.score = MIN/2;
    move_t best_move = Default_Move();
    best_move.score = MIN + ply;
    int mult = (isMaximizing == WHITE) ? 1 : -1;
    int in_check = In_Check_fast(board_data, isMaximizing);
    //if(quiscence_moves % 5000 == 1) {printf("Quiscence moves: %ld\n", quiscence_moves); }
    if(ply >= MAX_PLY){
        if(in_check == 0){
            score_move.score = mult * score_board_fast(board_data);
            #if USE_TT == 1
                tt_flag = EXACT_FLAG;
            #endif
        }else{
            score_move.score = MIN + ply;
        }
        quiscence_moves++;
        #if USE_TT == 1
            Insert_Q_Trans_Table(board_data->zob_key, tt_flag, score_move, tt);
        #endif
        return score_move;
    }

    if(in_check == 0){
        //stand pat score
        int simple_score = mult * score_board_simple(board_data);
        if(simple_score >= beta - QUEEN_VAL){
            score_move.score = mult * score_board_fast(board_data);
            if(score_move.score >= beta){
                quiscence_moves++;

                #if USE_TT == 1
                    tt_flag = BETA_FLAG;
                    Insert_Q_Trans_Table(board_data->zob_key, tt_flag, score_move, tt);
                #endif
                return score_move;
            }
        }

        //delta pruning, we don't bother searching if it doesn't improve our score by 
        //at least 1.5 pawns
        int delta = PAWN_VAL * 1.5;
        if(simple_score < alpha - delta + QUEEN_VAL){
            if(score_move.score == MIN/2) {score_move.score = mult * score_board_fast(board_data);}
            if(score_move.score < alpha - delta){ 
                quiscence_moves++; 
                score_move.score = alpha;
                #if USE_TT == 1
                    tt_flag = ALPHA_FLAG;
                    Insert_Q_Trans_Table(board_data->zob_key, tt_flag, score_move, tt);
                #endif
                return score_move; 
            }
        }
        if(simple_score > alpha - QUEEN_VAL){
            if(score_move.score == MIN/2) { score_move.score = mult * score_board_fast(board_data);}
            if(score_move.score > alpha)   {alpha = score_move.score; }
        }
    }

    Get_All_Moves_Separate(board_data, search_mem->move_arr[ply], isMaximizing);
    //if there are no moves
    if(search_mem->move_arr[ply][ATTACK_MOVES][0]->from == FULL && in_check == 0){
        int mult = (isMaximizing == WHITE) ? 1 : -1;
        score_move = *(search_mem->move_arr[ply][NORMAL_MOVES][0]);    //just return a normal move if there arent any attacking ones
        score_move.score = mult * score_board_fast(board_data) - ply;
        quiscence_moves++;
        #if USE_TT == 1
            tt_flag = EXACT_FLAG;
            Insert_Q_Trans_Table(board_data->zob_key, tt_flag, score_move, tt);
        #endif
        return score_move;
    }
    #if ORDER_MOVES == 1
        Order_Moves_Simple(search_mem->move_arr[ply][ATTACK_MOVES]);
    #endif
    int moves_done = 0;
    Board_Data_t* copy = search_mem->copy_arr[ply];
    Copy_Board(copy, board_data);

    //attacks are 0, normal moves are 1
    
    for(int i = 0; i < NUM_MOVE_TYPES; i++){
        //only explore non attacking moves if we are in check
        if(i != ATTACK_MOVES && in_check == 0) continue;
        move_t** moves = search_mem->move_arr[ply][ATTACK_MOVES];
        int move_idx = 0;
        while(moves[move_idx]->from != FULL){
            //copy the board, then do the move on the copied board so we don't
            //need to worry about undoing the move
            Do_Move(copy, moves[move_idx]);
            score_move.score = MIN+ply;
            if(In_Check_fast(copy, isMaximizing) == 0){
                moves_done++;
                score_move = Quiscence_Search(copy, search_mem, ply + 1, (~isMaximizing & 1), beta * -1, alpha * -1, tt);
                score_move.score *= -1;
            }
            Undo_Move(copy, board_data, moves[move_idx]); 
            if(best_move.score < score_move.score){ best_move = *moves[move_idx]; best_move.score = score_move.score;}
            #if ALPHA_BETA_PRUNE == 1
                    if(score_move.score > alpha) { 
                        #if USE_TT == 1
                            tt_flag = EXACT_FLAG; 
                        #endif
                        alpha = score_move.score;
                    }
                    if(alpha >= beta) { 
                        #if USE_TT == 1
                            tt_flag = BETA_FLAG;
                        #endif
                        break;
                    }
                #endif
            move_idx++; 
        }
    }
    if(moves_done == 0){
        best_move.score = MIN + ply;
    }
    #if USE_TT == 1
        Insert_Q_Trans_Table(board_data->zob_key, tt_flag, best_move, tt);
    #endif
    return best_move;
}

eng_search_mem_t* Init_Eng_Search_Mem(){
    eng_search_mem_t* search_mem = (eng_search_mem_t*)malloc(sizeof(eng_search_mem_t));
    search_mem->move_arr = (move_t****)malloc(sizeof(move_t***) * MAX_PLY);
    search_mem->copy_arr = (Board_Data_t**)malloc(sizeof(Board_Data_t*)*MAX_PLY);
    search_mem->history_array = (uint**)malloc(sizeof(uint*) * NUM_PIECES);
    search_mem->butterfly_array = (uint**)malloc(sizeof(uint*) * NUM_PIECES);
    for(int i = 0; i < NUM_PIECES; i++){
        search_mem->history_array[i] = malloc(sizeof(int) * 64);
        search_mem->butterfly_array[i] = malloc(sizeof(int) * 64);

        for(int j = 0; j < 64; j++){
            search_mem->history_array[i][j] = 0;
            search_mem->butterfly_array[i][j] = 1;
        }
    }
    for(int i = 0; i < MAX_PLY; i++){
        search_mem->move_arr[i] = (move_t***) malloc(sizeof(move_t**) * NUM_MOVE_TYPES);
        search_mem->copy_arr[i] = Init_Board();
        //allocating memory for the best move
        search_mem->move_arr[i][BEST_MOVE] = (move_t**) malloc(sizeof(move_t*) * 2);
        search_mem->move_arr[i][BEST_MOVE][0] = (move_t*)malloc(sizeof(move_t));
        search_mem->move_arr[i][BEST_MOVE][1] = (move_t*)malloc(sizeof(move_t));

        //space for the two killer moves per ply
        search_mem->move_arr[i][KILLER_MOVE] = (move_t**) malloc(sizeof(move_t*) * 3);
        search_mem->move_arr[i][KILLER_MOVE][0] = (move_t*)malloc(sizeof(move_t));
        search_mem->move_arr[i][KILLER_MOVE][1] = (move_t*)malloc(sizeof(move_t));
        search_mem->move_arr[i][KILLER_MOVE][2] = (move_t*)malloc(sizeof(move_t));
    
        search_mem->move_arr[i][BEST_MOVE][0][0] = Default_Move();
        search_mem->move_arr[i][BEST_MOVE][1][0] = Default_Move();
        search_mem->move_arr[i][KILLER_MOVE][0][0] = Default_Move();
        search_mem->move_arr[i][KILLER_MOVE][1][0] = Default_Move();
        search_mem->move_arr[i][KILLER_MOVE][2][0] = Default_Move();

        search_mem->move_arr[i][ATTACK_MOVES] = (move_t**) malloc(sizeof(move_t*) * MAX_MOVES);
        search_mem->move_arr[i][NORMAL_MOVES] = (move_t**) malloc(sizeof(move_t*) * MAX_MOVES);
        for(int j = 0; j < MAX_MOVES; j++){
            search_mem->move_arr[i][ATTACK_MOVES][j] = (move_t*) malloc(sizeof(move_t));
            search_mem->move_arr[i][NORMAL_MOVES][j] = (move_t*) malloc(sizeof(move_t));
        }
    }
    return search_mem;
}
void Delete_Eng_Search_Mem(eng_search_mem_t* search_mem){
    for(int i = 0; i < MAX_PLY; i++){
        free(search_mem->move_arr[i][BEST_MOVE][0]);
        free(search_mem->move_arr[i][BEST_MOVE][1]);
        free(search_mem->move_arr[i][BEST_MOVE]);

        free(search_mem->move_arr[i][KILLER_MOVE][0]);
        free(search_mem->move_arr[i][KILLER_MOVE][1]);
        free(search_mem->move_arr[i][KILLER_MOVE][2]);
        free(search_mem->move_arr[i][KILLER_MOVE]);

        for(int j = 0; j < MAX_MOVES; j++){
            free(search_mem->move_arr[i][ATTACK_MOVES][j]);
            free(search_mem->move_arr[i][NORMAL_MOVES][j]);
        }
        free(search_mem->move_arr[i][ATTACK_MOVES]);
        free(search_mem->move_arr[i][NORMAL_MOVES]);
        free(search_mem->move_arr[i]);
    }
    free(search_mem->move_arr);
    for(int i = 0; i < NUM_PIECES; i++){
        free(search_mem->history_array[i]);
        free(search_mem->butterfly_array[i]);
    }
    free(search_mem->butterfly_array);
    free(search_mem->history_array);
    free(search_mem);
}



//verifys that for a given board, the move is legal
//could potentially need to be more thourough/ robust
//used for tranposition table errors
int Verify_Legal_Move(Board_Data_t* board_data, move_t* move){
    //if we arent moving from the piece we think we are
    if((board_data->pieces[move->from_type] & move->from) != move->from){ 
        return 0;
    }
    if(move->to_type != NUM_PIECES){
        //if we aren't capturing the correct piece
        if((board_data->pieces[move->to_type] & move->to) != move->to && (move->to != board_data->ep_tile)){ 
            return 0; 
        }
    }else{
        //if think were moving to empty square that is occupied
        if((move->to & board_data->occ) != ZERO){
            return 0;
        }
    }
    //if either of these are the default move condition
    if(move->from == FULL || move->to == FULL){
        return 0;
    }
    //if we are moving from no piece.
    if(move->from_type == NUM_PIECES){
        return 0;
    }
    U64 legal_moves = Get_Piece_Moves(board_data, board_data->to_move, move->from, (move->from_type)%6);
    if((move->to & legal_moves) == ZERO){
        return 0;
    }
    return 1;
}