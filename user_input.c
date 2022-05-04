 
#include "user_input.h"
#include "parallel_perft.h"
#include "engine.h"
#include "Engine_Movegen.h"
#include "trans_table.h"
#include "evaluation.h"
#include "Parallel_Engine.h"
#include <time.h>
#include <stdio.h>
#include <sys/time.h>

void Handle_Input(char* input, Board_Data_t* board_data, trans_table_t* engine_tt, perft_trans_table_t* perft_tt){

    char** parsed_input = Parse_Input(input);
    char* p_in1 = parsed_input[0];
    if(strcmp(p_in1, "reset") == 0 || strcmp(p_in1, "Reset") == 0){
        if(strcmp(parsed_input[1], "zob") == 0){
            Reset_Zob_Key(board_data);
        }else if(strcmp(parsed_input[1],"tt") == 0){
            Reset_Trans_Table(perft_tt, engine_tt);
        }else{
            char* fen = Input_Reset_String(parsed_input[1]);
            Set_From_Fen(fen, board_data); 
            Reset_Zob_Key(board_data);
        }
    }
    if(strcmp(p_in1, "print") == 0 || strcmp(p_in1, "Print") == 0){
        Input_Print_Board(parsed_input, board_data);
    }
    if(strcmp(p_in1, "move") == 0 || strcmp(p_in1, "Move") == 0){
        Input_Move(parsed_input, board_data);
    }
    if(strcmp(p_in1, "perft") == 0 || strcmp(p_in1, "Perft") == 0){
        if(strcmp(parsed_input[1], "expanded") == 0 || strcmp(parsed_input[1], "Expanded") == 0){
            Input_Expanded_Perft(parsed_input, board_data, perft_tt);
        }else if(strcmp(parsed_input[1], "parallel") == 0 || strcmp(parsed_input[1],"Parallel") == 0){
            Input_Parallel_Perft(parsed_input, board_data, perft_tt);
        }else{
            Input_Perft(parsed_input, board_data, perft_tt);
        }
    }
    if(strcmp(p_in1, "search") == 0 || strcmp(p_in1, "Search") == 0){
        if(strcmp(parsed_input[1], "parallel") == 0 || strcmp(parsed_input[1], "Parallel") == 0){
            Input_Parallel_Search(parsed_input, board_data, engine_tt);
        }else{
            Input_Search(parsed_input, board_data, engine_tt);
        } 
        Reset_Trans_Table(perft_tt, engine_tt);
    }
    Free_Parsed_Input(parsed_input);
}
void Input_Parallel_Search(char** parsed_input, Board_Data_t* board_data, trans_table_t* tt){
    int depth = parsed_input[2][0] - '0';
    if(depth > 10) depth = 1;
    if(parsed_input[2][1] != '\0'){
        depth *= 10;
        depth += parsed_input[2][1] - '0';
    }

    move_t best_move;
    eng_search_mem_t** search_mem = (eng_search_mem_t**)malloc(sizeof(eng_search_mem_t*) * (MAX_ENGINE_THREADS + 1));
    for(int i = 0; i < MAX_ENGINE_THREADS+1; i++){
        search_mem[i] = Init_Eng_Search_Mem();
    }
    eng_thread_info_t* eng_t_info = (eng_thread_info_t*)malloc(sizeof(eng_thread_info_t) * MAX_ENGINE_THREADS);
    
    struct timeval start, end;
    gettimeofday(&start,NULL);

    best_move = Parallel_Iterative_Deepening(board_data, search_mem, tt, depth, eng_t_info);
    
    gettimeofday(&end,NULL);
    long sec_diff = (long)(end.tv_sec - start.tv_sec);
    long usec_diff = (long)(end.tv_usec - start.tv_usec);
    long msec = (sec_diff * 1000) + (usec_diff / 1000);
    
    char* move_str = String_From_Move(best_move);
    printf("Score: %.2f, Best Move: %s\n", (float)best_move.score/PAWN_VAL, move_str);
    printf("\tMilliseconds: %ld\n", msec);
    
    for(int i = 0; i < MAX_ENGINE_THREADS+1; i++){
        Delete_Eng_Search_Mem(search_mem[i]);
    }
    free(search_mem);
    free(eng_t_info);
}

void Input_Search(char** parsed_input, Board_Data_t* board_data, trans_table_t* tt){
    int to_move = board_data->to_move;
    int depth = parsed_input[1][0] - '0';
    if(depth > 10) depth = 1;
    if(parsed_input[1][1] != '\0'){
        depth *= 10;
        depth += parsed_input[1][1] - '0';
    }

    move_t best_move;
    eng_search_mem_t* search_mem = Init_Eng_Search_Mem();

    struct timeval start, end;
    gettimeofday(&start,NULL);

    best_move = Iterative_Deepening(board_data, search_mem, tt, depth, to_move, MIN, MAX);
    
    gettimeofday(&end,NULL);
    long sec_diff = (long)(end.tv_sec - start.tv_sec);
    long usec_diff = (long)(end.tv_usec - start.tv_usec);
    long msec = (sec_diff * 1000) + (usec_diff / 1000);

    char* move_str = String_From_Move(best_move);
    printf("Score: %.2f, Best Move: %s\n", (float)best_move.score/PAWN_VAL, move_str);
    printf("\tMilliseconds: %ld\n", msec);
    Delete_Eng_Search_Mem(search_mem);
}

void Input_Parallel_Perft(char** parsed_input, Board_Data_t* board_data, perft_trans_table_t* tt){
    int to_move;
    if(strcmp(parsed_input[2], "white") == 0 || strcmp(parsed_input[1], "White") == 0){
        to_move = WHITE;
    }
    else if(strcmp(parsed_input[2], "black") == 0 || strcmp(parsed_input[1], "Black") == 0){
        to_move = BLACK;
    }
    else{
        printf("Error: input string\n");
        return;
    }
    //technically cannot search past a depth of 9 for now.  Ok bc perft takes hours to
    //get to 9 anyway.
    int depth = parsed_input[3][0] - '0';
    
    //clock_t start = clock(), diff;
    search_data_t search_data;
    struct timeval start, end;
    gettimeofday(&start,NULL);
    if(strcmp(parsed_input[4],"limited") == 0 || strcmp(parsed_input[4],"Limited") == 0){
        search_data = Parallel_Perft_Limited(board_data, depth, to_move, tt);
    }else{
        search_data = Parallel_Perft(board_data, depth, to_move, tt);
    }
    gettimeofday(&end,NULL);

    long sec_diff = (long)(end.tv_sec - start.tv_sec);
    long usec_diff = (long)(end.tv_usec - start.tv_usec);
    long msec = (sec_diff * 1000) + (usec_diff / 1000);
    //diff = clock() - start;
    //long msec = diff * 1000 / CLOCKS_PER_SEC;

    printf("Milliseconds: %ld\n", msec);
    if(msec > 1000){
        printf("\tKnps: %ld\n", search_data.pos_searched/msec);
    }
    printf("Searched: %ld\n", search_data.pos_searched);
    printf("Captures: %d\n", search_data.captures);
    printf("En Passants: %d\n", search_data.en_passants);
    printf("Castles: %d\n", search_data.castles);
    printf("Promotions: %d\n", search_data.promotions);
    printf("Checks: %d\n", search_data.checks);
    printf("CheckMates: %d\n", search_data.checkmates);
}

void Reset_Trans_Table(perft_trans_table_t* perft_tt, trans_table_t* eng_tt){
    int perft_tt_size = perft_tt->size;
    search_data_t neg_data = {-1,-1,-1,-1,-1,-1,-1};
    perft_tt_entry_t neg_entry;
    neg_entry.search_data = neg_data;
    neg_entry.depth = -1;
    neg_entry.zob_key = FULL;
    neg_entry.num_using = -1;
    for(int i = 0; i < perft_tt_size; i++){
        perft_tt->table_head[i] = neg_entry;
    }

    int eng_tt_size = eng_tt->size;
    tt_entry_t default_entry;
    default_entry.zob_key = FULL;
    default_entry.depth = -100;
    default_entry.move = Default_Small();
    default_entry.flag = NO_FLAG;
    eng_tt->default_entry = default_entry;
    for(int i = 0; i < eng_tt_size; i++){
        eng_tt->table_head[i] = default_entry;
        eng_tt->q_head[i] = default_entry;
    }
}
//makes it so that I can easily load in different fen strings for testing.
char* Input_Reset_String(char* input){
    if(strcmp(input, "kiwipete") == 0 || strcmp(input, "Kiwipete") == 0){
        return "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - ";
    } else if (strcmp(input, "3") == 0)
    {
        return "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w -";
    }else if(strcmp(input, "4") == 0){
        return "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq -";
    }else if(strcmp(input, "4m") == 0){
        return "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ -";
    }else if(strcmp(input, "5") == 0){
        return "3qkp/3pp1/8/6pQ/8/8/7P/KPPP4 b - -";
    }else if(strcmp(input, "6") == 0){
        return "8/k7/3p4/p2P1p2/P2P1P2/8/8/K7 w - - ";
    }else if(strcmp(input, "7") == 0){
        return "r4rk1/pp1n1p1p/1nqP2p1/2b1P1B1/4NQ2/1B3P2/PP2K2P/2R5 w - -";
    }else {
        return "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq -";
    }
}

void Input_Expanded_Perft(char** parsed_input, Board_Data_t* board_data, perft_trans_table_t* tt){
    int to_move = WHITE;
    if(strcmp(parsed_input[2], "black") == 0 || strcmp(parsed_input[2], "Black") == 0){
        to_move = BLACK;
    }
    int depth = parsed_input[3][0] - '0';
    int ply = 0;

    Search_Mem_t* search_mem = Init_Search_Memory();
    Perft_Expanded(board_data, search_mem, depth, ply, to_move, tt);    
    Delete_Search_Memory(search_mem);
}

void Input_Perft(char** parsed_input, Board_Data_t* board_data, perft_trans_table_t* tt){
    int to_move;
    if(strcmp(parsed_input[1], "white") == 0 || strcmp(parsed_input[1], "White") == 0){
        to_move = WHITE;
    }
    else if(strcmp(parsed_input[1], "black") == 0 || strcmp(parsed_input[1], "Black") == 0){
        to_move = BLACK;
    }
    else{
        printf("Error: input string\n");
        return;
    }
    int depth = parsed_input[2][0] - '0';
    int ply = 0;

    clock_t start = clock(), diff;
    Search_Mem_t* search_mem = Init_Search_Memory();
    search_data_t search_data = Perft(board_data, search_mem, depth, ply, to_move, tt);

    Delete_Search_Memory(search_mem);
    diff = clock() - start;
    long msec = diff * 1000 / CLOCKS_PER_SEC;

    printf("Milliseconds: %ld\n", msec);
    if(msec > 1000){
        printf("\t Knps: %ld\n", search_data.pos_searched/msec);
    }
    printf("Searched: %ld\n", search_data.pos_searched);
    printf("Captures: %d\n", search_data.captures);
    printf("En Passants: %d\n", search_data.en_passants);
    printf("Castles: %d\n", search_data.castles);
    printf("Promotions: %d\n", search_data.promotions);
    printf("Checks: %d\n", search_data.checks);
    printf("CheckMates: %d\n", search_data.checkmates);
}

void Input_Print_Board(char** parsed_input, Board_Data_t* board_data){
    if(strcmp(parsed_input[1], "white") == 0 || strcmp(parsed_input[1], "White") == 0){
        Print_Board(board_data->team_tiles[WHITE], board_data);
    }
    else if(strcmp(parsed_input[1], "black") == 0 || strcmp(parsed_input[1], "Black") == 0){
        Print_Board(board_data->team_tiles[BLACK], board_data);
    }
    else if(strcmp(parsed_input[1], "castle") == 0 || strcmp(parsed_input[1], "Castle") == 0){
        Print_Castling_Rights(board_data->cas_mask);
    }
    else if(strcmp(parsed_input[1], "moves") == 0 || strcmp(parsed_input[1],"Moves") == 0){
        Print_Moves(parsed_input, board_data);
    }
    else {
        Print_Board(board_data->occ, board_data);
    }
    printf("To move: %d\n",board_data->to_move);
}

void Print_Moves(char** parsed_input, Board_Data_t* board_data){
    U64 pos;
    int toMove;
    int type;
    move_t move = Move_From_Input(parsed_input[2], board_data);
    pos = move.from;
    toMove = move.from_type/6;
    type = move.from_type%6;    //Get piece moves only takes the piece type
    U64 moves = Get_Piece_Moves(board_data, toMove, pos, type);
    Print_Board(moves | pos, board_data);
}

void Input_Move(char** parsed_input, Board_Data_t* board_data){
    char* move_str = parsed_input[1];
    char* move_flags = parsed_input [2];

    move_t move = Move_From_Input(move_str, board_data);
    Update_Flags_From_Input(move_flags, &move);
    Do_Move(board_data, &move); 
}
//takes input of the form e2e4 = {from_file, from_rank, to_file, to_rank}
move_t Move_From_Input(char* move_str, Board_Data_t* board_data){
    move_t move;
    move.flags = 0;
    if(strcmp(move_str, "O-O") == 0 || strcmp(move_str, "OO") == 0){
        move.from = ONE << 3;
        move.to = ONE << 1;
        move.from_type = W_KING;
        move.to_type = W_KING;
        move.flags = 0x10;
        return move;
    }
    if(strcmp(move_str, "O-O-O") == 0 || strcmp(move_str, "OOO") == 0){
        move.from = ONE << 3;
        move.to = ONE << 5;
        move.from_type = W_KING;
        move.to_type = W_KING;
        move.flags = 0x20;
        return move;
    }
    if(strcmp(move_str, "o-o") == 0 || strcmp(move_str, "oo") == 0){
        move.from = ONE << (3+56);
        move.to = ONE << (1+56);
        move.from_type = B_KING;
        move.to_type = B_KING;
        move.flags = 0x40;
        return move;
    }
    if(strcmp(move_str, "o-o-o") == 0 || strcmp(move_str, "ooo") == 0){
        move.from = ONE << (3+56);
        move.to = ONE << (5+56);
        move.from_type = B_KING;
        move.to_type = B_KING;
        move.flags = 0x80;
        return move;
    }
    int from_x = (int)(move_str[0] - 'a');
    int from_y = (int)(move_str[1] - '1');
    int from_idx = 8*from_y+from_x;
    U64 from_pos = ONE << from_idx;

    int to_x = (int)(move_str[2] - 'a');
    int to_y = (int)(move_str[3] - '1');
    int to_idx = 8*to_y+to_x;
    U64 to_pos = ONE << to_idx;

    int from_type = NUM_PIECES;
    int to_type = NUM_PIECES;

    for(int i = 0; i < NUM_PIECES; i++){
        if((board_data->pieces[i] & from_pos) != ZERO){
            from_type = i;
        }
        if((board_data->pieces[i] & to_pos) != ZERO){
            to_type = i;
        }
    }
    
    if(from_type == NO_PIECE) from_pos = ZERO;
    move.from = from_pos;
    move.to = to_pos;
    move.from_type = from_type;
    move.to_type = to_type;
    move.flags = 0;

    return move;
}

void Update_Flags_From_Input(char* flag, move_t* move){
    //if we are promoting
    
    switch(flag[0]){
        case('K'):
            move->flags = W_KING & 0x0F;
            break;
        case('Q'):
            move->flags = W_QUEEN & 0x0F;
            break;
        case('R'):
            move->flags = W_ROOK & 0x0F;
            break;
        case('B'):
            move->flags = W_BISHOP & 0x0F;
            break;
        case('N'):
            move->flags = W_KNIGHT & 0x0F;
            break;
        case('P'):
            move->flags = W_PAWN & 0x0F;
            break;

        case('k'):
            move->flags = B_KING & 0x0F;
            break;
        case('q'):
            move->flags = B_QUEEN & 0x0F;
            break;
        case('r'):
            move->flags = B_ROOK & 0x0F;
            break;
        case('b'):
            move->flags = B_BISHOP & 0x0F;
            break;
        case('n'):
            move->flags = B_KNIGHT & 0x0F;
            break;
        case('p'):
            move->flags = B_PAWN & 0x0F;
            break;

        default:
            break;
    }
    if(strcmp(flag, "ep") == 0 || strcmp(flag, "Ep") == 0){
        move->flags = 0xFF;
    }
}

//just breaks the input up into words and returns it
char** Parse_Input(char* input){
    
    char** output = (char**) malloc(sizeof(char*) * MAX_INPUT_WORDS);
    for(int i = 0; i < MAX_INPUT_WORDS; i++){
        output[i] = (char*) malloc(sizeof(char)*MAX_WORD_LEN);
        //initialize the memory so valgrind doens't get mad
        for(int j = 0; j < MAX_WORD_LEN; j++){
            output[i][j] = '\0';
        }
    }

    int in_idx = 0; 
    int word_num = 0;
    int word_idx = 0;
    while(input[in_idx] != '\0' && input[in_idx] != '\n'){
        if(input[in_idx] == ' ' || input[in_idx] == '\n'){
            output[word_num][word_idx] = '\0';
            in_idx++;
            word_num++;
            word_idx = 0;
            if(input[in_idx] == '\0'){
                break;
            }
        }
        output[word_num][word_idx] = input[in_idx];
        word_idx++;
        in_idx++;
    }

    return output;
}

//potential error if p_input[0] frees p_input
void Free_Parsed_Input(char** p_input){
    for(int i = 0; i < MAX_INPUT_WORDS; i++){
        free(p_input[i]);
    }
    free(p_input);
}

