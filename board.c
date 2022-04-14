#include "board.h"
#include "bit_helper.h"
#include <stdlib.h>

#define ERROR_CHECK

//just allocates memory to hold the board
Board_Data_t* Init_Board()
{
    Board_Data_t* board = (Board_Data_t*)malloc(sizeof(Board_Data_t));
    U64* pieces = (U64*)malloc(PIECE_ARR_LEN); // the +1 is so we have a slot to hold NO_PIECE (less if statements)
    U64* colors = (U64*)malloc(COLOR_ARR_LEN);  // again, +1 to hold noteam value
    board->pieces = pieces;
    board->team_tiles = colors;
    board->ep_tile = ZERO;
    board->cas_mask = 0;
    board->occ = ZERO;
    board->to_move = 0;
    board->zob_key = ZERO;
    board->zob_array = NULL;
    return board;
}

void Delete_Board(Board_Data_t* board, int free_zob)
{
    free(board->pieces);
    free(board->team_tiles);
    if(free_zob){
        for(int i = 0; i < ZOB_ROWS; i++){
            free(board->zob_array[i]);
        }
        free(board->zob_array);
    }
    free(board);
}

void Init_Zob_Array(Board_Data_t* board_data){
    //allocate ZOB_ROWS(66) rows of pointers to arrays of U64s
    board_data->zob_array = (U64**)malloc(sizeof(U64*)*ZOB_ROWS);
        //now allocate memory for each type of piece at each spot. the +1 is for the null piece attacking
    for(int i = 0; i < ZOB_ROWS; i++){
        board_data->zob_array[i] = (U64*)malloc(sizeof(U64)*(NUM_PIECES+1));
    }

    //just use same initial random seed each time for consistency.
    srand(12345);
    for(int i = 0; i < ZOB_ROWS; i++){
        for(int j = 0; j < NUM_PIECES; j++){
            int rand1 = rand();
            int rand2 = rand();
            U64 random_64 = (U64) rand1;
            random_64 <<= 32;
            random_64 |= (U64) rand2;
            board_data->zob_array[i][j] = random_64;
        }
        //the 13th column of each row is ZERO, this is so when attacking no square,
        //we don't change the zobrist key
        board_data->zob_array[i][NUM_PIECES] = ZERO;
    }
}

//There potentially could be an error with Get_Idx being off by 1.  I might need to 
//subtract 1.
void Reset_Zob_Key(Board_Data_t* board_data){
    U64 zobrist_key = 0;
    //set the piece values into the zobrist array
    for(int i = 0; i < NUM_PIECES; i++)
    {
        U64 pieces = board_data->pieces[i];
        while(pieces != ZERO){
            U64 piece = Get_LSB(pieces);
            pieces &= ~piece;
            int p_idx = Get_Idx(piece);

#ifdef ERROR_CHECK
    if(p_idx < 0 || p_idx > 63) printf("Error with p_idx");
#endif
            zobrist_key ^= board_data->zob_array[p_idx][i];
        }
    }
    if(board_data->to_move){
        zobrist_key ^= board_data->zob_array[64][0];
    }
    if(board_data->cas_mask & 0x01){
        zobrist_key ^= board_data->zob_array[64][WK_CAS_ZOB];
    }
    if(board_data->cas_mask & 0x08){
        zobrist_key ^= board_data->zob_array[64][WQ_CAS_ZOB];
    }
    if(board_data->cas_mask & 0x10){
        zobrist_key ^= board_data->zob_array[64][BK_CAS_ZOB];
    }
    if(board_data->cas_mask & 0x80){
        zobrist_key ^= board_data->zob_array[64][BQ_CAS_ZOB];
    }
    //put the ep tile in the first row, then get the index of it. 
    //I think I could fit the ep indexes into row 64, bc 1 slot for move, 4 for castle, 
    //then 8 for the ep_idx, for a total of 13, don't wanna risk fucking it up tho, this works
    //for now
    U64 ep_tile = board_data->ep_tile;
    if(ep_tile){
        int ep_idx = Get_Idx((ep_tile >> (2*8)) | (ep_tile >> (5*8)));
#ifdef ERROR_CHECK
    if(ep_idx < 0 || ep_idx > 7) printf("Reset zob ep error: %d\n", ep_idx);
#endif
        zobrist_key ^= board_data->zob_array[65][ep_idx];
    }
    board_data->zob_key = zobrist_key;
    return;
}

//makes a deep copy of the board being passed
void Copy_Board(Board_Data_t* copy, Board_Data_t* source){
    //Board_Data_t* copy = Init_Board();

    copy->occ = source->occ;
    copy->ep_tile = source->ep_tile;
    copy->cas_mask = source->cas_mask;
    copy->to_move = source->to_move;
    copy->zob_key = source->zob_key;
    copy->zob_array = source->zob_array;

    memcpy(copy->pieces, source->pieces, PIECE_ARR_LEN);
    memcpy(copy->team_tiles, source->team_tiles, COLOR_ARR_LEN);    
}

//sets the board up from given FEN string.
//TODO: ADD IN EP, CASTLE, MOVE/HALFMOVE
void Set_From_Fen(char* FEN, Board_Data_t* board){
    U64* pieces = board->pieces;
    
    int idx = 0;
    int b_idx = 63 - 7;
    for(int i = 0; i < NUM_PIECES+1; i++){
        board->pieces[i] = ZERO;
    }
    int extra = 0;
    while(b_idx >=0 && FEN[idx] != ' '){
        switch(FEN[idx]){
            case 'K':
                pieces[W_KING] |= ONE << b_idx;
                break;
            case 'Q':
                pieces[W_QUEEN] |= ONE << b_idx;
                break;
            case 'R':
                pieces[W_ROOK] |= ONE << b_idx;
                break;
            case 'B':
                pieces[W_BISHOP] |= ONE << b_idx;
                break;
            case 'N':
                pieces[W_KNIGHT] |= ONE << b_idx;
                break;
            case 'P':
                pieces[W_PAWN] |= ONE << b_idx;
                break;

            case 'k':
                pieces[B_KING] |= ONE << b_idx;
                break;
            case 'q':
                pieces[B_QUEEN] |= ONE << b_idx;
                break;
            case 'r':
                pieces[B_ROOK] |= ONE << b_idx;
                break;
            case 'b':
                pieces[B_BISHOP] |= ONE << b_idx;
                break;
            case 'n':
                pieces[B_KNIGHT] |= ONE << b_idx;
                break; 
            case 'p':
                pieces[B_PAWN] |= ONE << b_idx;
                break;
            case '/':
                extra = 8 * (b_idx%8 == 0);
                b_idx = 8*(b_idx/8);    //move to beginning of line
                b_idx -= (8+extra);
                idx++;
                continue;
            default:
                b_idx += (int)(FEN[idx] - '0');
                //I also increment idx right here bc if we reached the end of the 
                //row with this, then the next char will be a / 
                if((b_idx)%8 == 0) {
                    b_idx -= 16;
                    idx++;
                }
                idx++;
                continue;
        }
        idx++;
        b_idx++;
    }
    if(FEN[idx] == ' ') idx++;
    if(FEN[idx] == 'w')   board->to_move = WHITE;
    else                    board->to_move = BLACK;
    idx++;
    idx++;
    board->cas_mask = 0;
    while(FEN[idx] != ' '){
        if(FEN[idx] == 'K') board->cas_mask |= 0x01;
        if(FEN[idx] == 'Q') board->cas_mask |= 0x08;
        if(FEN[idx] == 'k') board->cas_mask |= 0x10;
        if(FEN[idx] == 'q') board->cas_mask |= 0x80;
        if(FEN[idx] == '-') break;
        idx++;
    }
    if(FEN[idx] == ' ') idx++;
    if(FEN[idx] == ' ') idx++;
    board->ep_tile = ZERO;
    if(FEN[idx] != '-'){
        int ep_x = FEN[idx++] - 'a';
        int ep_y = FEN[idx++] - '1';
        int ep_pos = 8*ep_y + ep_x;
        board->ep_tile = ONE << ep_pos;
    }
    board->team_tiles[WHITE] = ZERO;
    board->team_tiles[BLACK] = ZERO;
    board->team_tiles[NO_TEAM] = ZERO;  //just so it has an initialized value
    for(int i = 0; i < 6; i++){
        board->team_tiles[WHITE] |= pieces[i];
        board->team_tiles[BLACK] |= pieces[i+6];
    }
    board->occ = board->team_tiles[WHITE] | board->team_tiles[BLACK];


    return;
}



void Print_Castling_Rights(byte cas_mask){
    char w_k = ((cas_mask & 0x01) != 0) ? 'y' : 'n';
    char w_q = ((cas_mask & 0x08) != 0) ? 'y' : 'n';
    char b_k = ((cas_mask & 0x10) != 0) ? 'y' : 'n';
    char b_q = ((cas_mask & 0x80) != 0) ? 'y' : 'n';
    printf("\t\tCastling Rights\nWhite Kingside: %c, White Queenside: %c, Black Kingside: %c, Black Queenside: %c\n", w_k, w_q, b_k, b_q);
}

//quite long just so it is formatted nicely
void Print_Board(U64 piece, Board_Data_t* board){
    U64* all_pieces = board->pieces;
    char to_print[8][8];
    for(int i = 0; i < 8; i++){
        for(int j = 0; j < 8; j++){
            to_print[i][j] = '.';
        }
    }

    int num_set = PopCount(piece) + 1;
    U64* set_list = (U64*) malloc(sizeof(U64) * num_set);
    Get_Individual(piece, set_list);

    //sets the appropriate values in the right position on the print arrray
    int idx = 0;
    while(set_list[idx] != 0ULL){
        U64 p = set_list[idx];
        //x and y are flipped because of how the print buffer is.
        int y = GetX(p);
        int x = GetY(p);
        to_print[x][y] = 'x';
        //really only reason to have this p here is to make this a collapseable section
        if(p){
            if ((p & all_pieces[W_KING]) != 0)
            {
                to_print[x][y] = 'K';
            }
            else if ((p & all_pieces[W_QUEEN]) != 0)
            {
                to_print[x][y] = 'Q';
            }
            else if ((p & all_pieces[W_KNIGHT]) != 0)
            {
                to_print[x][y] = 'N';
            }
            else if ((p & all_pieces[W_BISHOP]) != 0)
            {
                to_print[x][y] = 'B';
            }
            else if ((p & all_pieces[W_ROOK]) != 0)
            {
                to_print[x][y] = 'R';
            }
            else if ((p & all_pieces[W_PAWN]) != 0)
            {
                to_print[x][y] = 'P';
            }

            else if ((p & all_pieces[B_KING]) != 0)
            {
                to_print[x][y] = 'k';
            }
            else if ((p & all_pieces[B_QUEEN]) != 0)
            {
                to_print[x][y] = 'q';
            }
            else if ((p & all_pieces[B_KNIGHT]) != 0)
            {
                to_print[x][y] = 'n';
            }
            else if ((p & all_pieces[B_BISHOP]) != 0)
            {
                to_print[x][y] = 'b';
            }
            else if ((p & all_pieces[B_ROOK]) != 0)
            {
                to_print[x][y] = 'r';
            }
            else if ((p & all_pieces[B_PAWN]) != 0)
            {
                to_print[x][y] = 'p';
            }
        }
        idx++;
    }
    
    U64 ep_tile = board->ep_tile;
    if(ep_tile != ZERO){
        int y = GetX(ep_tile);
        int x = GetY(ep_tile);

        //if the en passant square is also a target square
        if(to_print[x][y] != '.'){
            to_print[x][y] = 'o';
        }else{
            to_print[x][y] = '#';
        }
    }
    
    free(set_list);
    char print_buf[1024];
    idx = 0;
    //put a lil boundry around the board
    print_buf[idx++] = '-';
    print_buf[idx++] = '-';
    print_buf[idx++] = '-';
    for(int i = 0; i < 8; i++){
        print_buf[idx++] = (char)('a' + i);
        print_buf[idx++] = ' ';
    }
    print_buf[idx++] = '-';
    print_buf[idx++] = '-';
    print_buf[idx++] = '-';
    print_buf[idx++] = '\n';

    print_buf[idx++] = '-';
    print_buf[idx++] = '-';
    print_buf[idx++] = '=';
    for(int i = 0; i < 8; i++){
        print_buf[idx++] = '=';
        print_buf[idx++] = '=';
    }
    print_buf[idx++] = '=';
    print_buf[idx++] = '-';
    print_buf[idx++] = '-';
    print_buf[idx++] = '\n';

    for(int i = 0; i < 8; i++){
        print_buf[idx++] = '8' - i;
        print_buf[idx++] = '-';
        print_buf[idx++] = '|';
        for(int j = 0; j < 8; j++){
            print_buf[idx++] = to_print[i][j];
            print_buf[idx++] = ' ';
        }
        print_buf[idx++] = '|';
        print_buf[idx++] = '-';
        print_buf[idx++] = '8' - i;
        print_buf[idx++] = '\n';
    }

    print_buf[idx++] = '-';
    print_buf[idx++] = '-';
    print_buf[idx++] = '=';
    for(int i = 0; i < 8; i++){
        print_buf[idx++] = '=';
        print_buf[idx++] = '=';
    }
    print_buf[idx++] = '=';
    print_buf[idx++] = '-';
    print_buf[idx++] = '-';
    print_buf[idx++] = '\n';

    print_buf[idx++] = '-';
    print_buf[idx++] = '-';
    print_buf[idx++] = '-';
    for(int i = 0; i < 8; i++){
        print_buf[idx++] = (char)('a' + i);
        print_buf[idx++] = ' ';
    }
    print_buf[idx++] = '-';
    print_buf[idx++] = '-';
    print_buf[idx++] = '-';
    print_buf[idx++] = '\n';
    print_buf[idx] = '\0';
    printf("%s\n", print_buf);
}

