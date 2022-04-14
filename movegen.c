

#include "movegen.h"

//#define ERROR_CHECK

//returns a pointer to an array of the possible moves   !! NOT the legal moves -- still need to filter those
//TODO: add in en passant, promotions, castling
void Get_All_Moves(Board_Data_t* board_data, move_t* movelist, int toMove){
    //max possible number of moves I could find is 228, 
    //but don't want to malloc for 228 when most of the time it will be like 20-30
    //so save malloc for end after we know how many moves there are


#ifdef ERROR_CHECK
    int error = 0;
    if(toMove == WHITE){
        if((~toMove & 1) != BLACK) error = 1;
    }else{
        if((~toMove & 1) != WHITE) error = 1;
    }
    if(error) printf("Error, incorrect w/b switching\n");
#endif
    //move_t inter_movelist[250];
    /*
    for(int i = 0; i < 250; i++){
        inter_movelist[i].from = FULL;
        inter_movelist[i].to = FULL;
        inter_movelist[i].from_type = 0xFF;
        inter_movelist[i].to_type = 0xFF;
        inter_movelist[i].flags = 0xFF;
    }
    */
    int num_moves = 0;
    int team_offset = 6*toMove;
    int enemy_offset = 6*(~toMove & 1);
    for(int i = 0 + team_offset; i < 6+team_offset; i++){
        U64 pieces = board_data->pieces[i];
        while(pieces){
            int from_type = i;
            U64 piece = Get_LSB(pieces);
            pieces ^= piece;
            U64 p_moves = Get_Piece_Moves(board_data, toMove, piece, i%6);
            while(p_moves){
                U64 move = Get_LSB(p_moves);
                p_moves ^= move;
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
                        movelist[num_moves].from = from;
                        movelist[num_moves].to = to;
                        movelist[num_moves].from_type = from_type;
                        movelist[num_moves].to_type = to_type;
                        movelist[num_moves].flags = (byte) j;
                        num_moves++;
                    }
                }else{
                    movelist[num_moves].from = from;
                    movelist[num_moves].to = to;
                    movelist[num_moves].from_type = from_type;
                    movelist[num_moves].to_type = to_type;
                    movelist[num_moves].flags = Set_Flags(board_data, toMove, piece, move);
                    num_moves++;
                }
            }
        }
    }
    
    movelist[num_moves].from = FULL;
    movelist[num_moves].to = FULL;
    movelist[num_moves].from_type = 0xFF;
    movelist[num_moves].to_type = 0xFF;
    movelist[num_moves].flags = 0xFF;
    num_moves++;
    
    return;
}

byte Set_Flags(Board_Data_t* board_data, int toMove, U64 from, U64 to){
    U64 team_pawns = board_data->pieces[W_PAWN + 6*toMove];
    byte ep_mask = 0x00;
    //this will only be set if we are moving to the en passant square, and it is from one of the pawns on a diagonal to it
    U64 is_ep = ((board_data->ep_tile & to) & (UPLEFT(from & team_pawns) 
                                                | UPRIGHT(from & team_pawns) 
                                                | DOWNLEFT(from & team_pawns) 
                                                | DOWNRIGHT(from & team_pawns)));
    if(is_ep) ep_mask = 0xFF;   //should compile to conditional move

    U64 wk_pos = board_data->pieces[W_KING];
    U64 bk_pos = board_data->pieces[B_KING];

    U64 wk_init_pos = ONE << 4;
    U64 bk_init_pos = ONE << (4+56);

    //w_kc is set IF: 
    //    wk_pos == wk_init == from (ie we're moveing the white king from the start position)
    //and to == landing spot for the white king kingside castling.
    //if these are all true, then w_kc == 0x10.  All the others follow same logic
    byte w_kc = (byte)(((wk_init_pos & wk_pos & from) & ((to & WKC_LAND) >> 2)) << 0);  //gives 0x10 if wqc
    byte w_qc = (byte)(((wk_init_pos & wk_pos & from) & ((to & WQC_LAND) << 2)) << 1);  //gives 0x20 if wkc

    byte b_kc = (byte)((((bk_init_pos & bk_pos & from) & ((to & BKC_LAND) >> 2)) >> 56) << 2); //gives 0x30 if bkc
    byte b_qc = (byte)((((bk_init_pos & bk_pos & from) & ((to & BQC_LAND) << 2)) >> 56) << 3); //gives 0x40 if bqc
    byte cas_mask = w_kc | w_qc | b_kc | b_qc;
#ifdef ERROR_CHECK
    int err = 0;
    if(w_kc != 0 && w_kc != 0x10) err = 1;
    if(w_qc != 0 && w_qc != 0x20) err = 1;
    if(b_kc != 0 && b_kc != 0x40) err = 1;
    if(b_qc != 0 && b_qc != 0x80) err = 1;
    if(cas_mask & ep_mask) err = 1;
    if(err){
        printf("ERROR: Castle and En Passant at same time?\n");
    }
#endif
    return cas_mask | ep_mask;
}

U64 Get_Piece_Moves(Board_Data_t* board_data, int toMove, U64 pos, int type){
    switch(type){
        case(W_KING):
            return Get_King_Moves(board_data, toMove, pos);
        case(W_QUEEN):
            return Get_Queen_Moves(board_data, toMove, pos);
        case(W_ROOK):
            return Get_Rook_Moves(board_data, toMove, pos);
        case(W_BISHOP):
            return Get_Bishop_Moves(board_data, toMove, pos);
        case(W_KNIGHT):
            return Get_Knight_Moves(board_data, toMove, pos);
        case(W_PAWN):
            return Get_Pawn_Moves(board_data, toMove, pos);
        default:
            printf("/////////////////////////////\n\tERROR IN GET PIECE MOVES\n/////////////////////////////\n");
            return ZERO;
    }
}

U64 Get_King_Moves(Board_Data_t* board_data, int toMove, U64 k_pos){
    U64 moves = ZERO;
    moves |= UP(k_pos);
    moves |= UPLEFT(k_pos);
    moves |= UPRIGHT(k_pos);
    moves |= LEFT(k_pos);
    moves |= RIGHT(k_pos);
    moves |= DOWN(k_pos);
    moves |= DOWNLEFT(k_pos);
    moves |= DOWNRIGHT(k_pos);


    //this is going to be slow but whatever
    //purpose of first if is so that we don't need to check(and fail) black/white after a bit
    if(board_data->cas_mask && !In_Check(board_data, toMove)){
        U64 to_tile;
        if(toMove == WHITE){
            if(board_data->cas_mask & 0x01){
                to_tile = (ONE << 5) & ~board_data->occ;
                U64 attackers = Get_Tile_Attackers(board_data, to_tile, toMove);
                if(attackers != ZERO) to_tile = ZERO;
                to_tile = RIGHT(to_tile) & ~board_data->occ;
                attackers = Get_Tile_Attackers(board_data, to_tile, toMove);
                if(attackers != ZERO) to_tile = ZERO;
                moves |= to_tile;
            }
            if(board_data->cas_mask & 0x08){
                to_tile = (ONE << 3) & ~board_data->occ;
                U64 attackers = Get_Tile_Attackers(board_data, to_tile, toMove);
                if(attackers != ZERO) to_tile = ZERO;
                to_tile = LEFT(to_tile) & ~board_data->occ;
                attackers = Get_Tile_Attackers(board_data, to_tile, toMove);
                if(attackers != ZERO) to_tile = ZERO;
                to_tile &= ~(board_data->occ << 1);
                moves |= to_tile;
            }
        }else{
            if(board_data->cas_mask & 0x10){
                to_tile = (ONE << (5+56)) & ~board_data->occ;
                U64 attackers = Get_Tile_Attackers(board_data, to_tile, toMove);
                if(attackers != ZERO) to_tile = ZERO;
                to_tile = RIGHT(to_tile) & ~board_data->occ;
                attackers = Get_Tile_Attackers(board_data, to_tile, toMove);
                if(attackers != ZERO) to_tile = ZERO;
                moves |= to_tile;
            }
            if(board_data->cas_mask & 0x80){
                to_tile = (ONE << (3+56))& ~board_data->occ;
                U64 attackers = Get_Tile_Attackers(board_data, to_tile, toMove);
                if(attackers != ZERO) to_tile = ZERO;
                to_tile = LEFT(to_tile) & ~board_data->occ;
                attackers = Get_Tile_Attackers(board_data, to_tile, toMove);
                if(attackers != ZERO) to_tile = ZERO;
                to_tile &= ~(board_data-> occ << 1);
                moves |= to_tile;
            }
        }
    }
    return moves & ~board_data->team_tiles[toMove];
}

int In_Check(Board_Data_t* board_data, int toMove){
    U64 k_pos = board_data->pieces[W_KING + 6*toMove];
    U64 attackers = Get_Tile_Attackers(board_data, k_pos, toMove);
    return (attackers != 0);
}

//I need this so I don't go into a recursive loop when checking the kings legal moves
//for castling
U64 Get_King_Moves_No_Castles(Board_Data_t* board_data, int toMove, U64 k_pos){
    U64 moves = ZERO;
    moves |= UP(k_pos);
    moves |= UPLEFT(k_pos);
    moves |= UPRIGHT(k_pos);
    moves |= LEFT(k_pos);
    moves |= RIGHT(k_pos);
    moves |= DOWN(k_pos);
    moves |= DOWNLEFT(k_pos);
    moves |= DOWNRIGHT(k_pos);
    return moves & ~board_data->team_tiles[toMove];
}

U64 Get_Tile_Attackers(Board_Data_t* board_data, U64 pos, int toMove){
    U64 pawn_attackers = ((((pos & ~left_wall) << 7) | ((pos & ~right_wall) << 9)) & board_data->pieces[B_PAWN]);
    if(toMove == BLACK) {
        pawn_attackers = (((pos & ~right_wall) >> 7) | ((pos & ~left_wall) >> 9)) & board_data->pieces[W_PAWN];
    }
    U64 tile_attackers = ZERO;
    tile_attackers |= Get_Rook_Moves(board_data, toMove, pos) & (board_data->pieces[B_ROOK - 6*toMove] | board_data->pieces[B_QUEEN - 6*toMove]);
    tile_attackers |= Get_Bishop_Moves(board_data, toMove, pos) & (board_data->pieces[B_BISHOP - 6*toMove] | board_data->pieces[B_QUEEN - 6*toMove]);
    tile_attackers |= Get_Knight_Moves(board_data, toMove, pos) & (board_data->pieces[B_KNIGHT - 6*toMove]);
    tile_attackers |= Get_King_Moves_No_Castles(board_data, toMove, pos) & (board_data->pieces[B_KING - 6*toMove]);

    tile_attackers |= pawn_attackers;

    return tile_attackers;
}

U64 Get_Knight_Moves(Board_Data_t* board_data, int toMove, U64 n_pos){
    U64 moves = ZERO;

    moves |= UPLEFT(LEFT(n_pos));
    moves |= DOWNLEFT(LEFT(n_pos));
    moves |= UPLEFT(UP(n_pos));
    moves |= UPRIGHT(UP(n_pos));
    moves |= UPRIGHT(RIGHT(n_pos));
    moves |= DOWNRIGHT(RIGHT(n_pos));
    moves |= DOWNLEFT(DOWN(n_pos));
    moves |= DOWNRIGHT(DOWN(n_pos));

    return moves & ~board_data->team_tiles[toMove];
}

U64 Get_Pawn_Moves(Board_Data_t* board_data, int toMove, U64 p_pos){
    U64 moves = ZERO;
    U64 free_tiles = ~(board_data->team_tiles[WHITE] | board_data->team_tiles[BLACK]);
    if(toMove == WHITE){
        moves |= UP(p_pos) & free_tiles;    //single push
        moves |= ((UP(moves) & UP(UP((p_pos & UP(bottom_wall))))) & free_tiles);  //only allow double push if starting from second rank
        moves |= ((UPRIGHT(p_pos) | UPLEFT(p_pos)) & (board_data->team_tiles[BLACK] | board_data->ep_tile));
    }else{
        moves |= DOWN(p_pos) & free_tiles;    //single push
        moves |= ((DOWN(moves) & DOWN(DOWN((p_pos & DOWN(top_wall))))) & free_tiles);  //only allow double push if starting from second rank
        moves |= ((DOWNRIGHT(p_pos) | DOWNLEFT(p_pos)) & (board_data->team_tiles[WHITE] | board_data->ep_tile)); 
    }
    return moves;
}

U64 Get_Bishop_Moves(Board_Data_t* board_data, int toMove, U64 b_pos){
    U64 moves = ZERO;

    U64 pos = b_pos;
    while(UPLEFT(pos) & ~board_data->team_tiles[toMove]){
        pos = UPLEFT(pos) & ~board_data->team_tiles[toMove];
        moves |= pos;
        pos &= ~board_data->team_tiles[~toMove & 0x01];
    }
    pos = b_pos;
    while(UPRIGHT(pos) & ~board_data->team_tiles[toMove]){
        pos = UPRIGHT(pos) & ~board_data->team_tiles[toMove];
        moves |= pos;
        pos &= ~board_data->team_tiles[~toMove & 0x01];
    }
    pos = b_pos;
    while(DOWNLEFT(pos) & ~board_data->team_tiles[toMove]){
        pos = DOWNLEFT(pos) & ~board_data->team_tiles[toMove];
        moves |= pos;
        pos &= ~board_data->team_tiles[~toMove & 0x01];
    }
    pos = b_pos;
    while(DOWNRIGHT(pos) & ~board_data->team_tiles[toMove]){
        pos = DOWNRIGHT(pos) & ~board_data->team_tiles[toMove];
        moves |= pos;
        pos &= ~board_data->team_tiles[~toMove & 0x01];   
    }

#ifdef ERROR_CHECK
    int error1 = 0;
    int error2 = 0;
    if(PopCount(moves & board_data->team_tiles[~toMove & 0x01]) > 4) error1 = 1;
    if((moves & board_data->team_tiles[toMove]) != 0) error2 = 1;
    if(error1) printf("Movegen Error: 1: bishop\n");
    if(error2) printf("Movegen Error: 2: Bishop\n");
#endif

    return moves & ~board_data->team_tiles[toMove];
}

U64 Get_Rook_Moves(Board_Data_t* board_data, int toMove, U64 r_pos){
    U64 moves = ZERO;
    U64 pos = r_pos;
    while(LEFT(pos) & ~board_data->team_tiles[toMove]){
        pos = LEFT(pos) & ~board_data->team_tiles[toMove];
        moves |= pos;
        pos &= ~board_data->team_tiles[~toMove & 0x01];
    }
    pos = r_pos;
    while(RIGHT(pos) & ~board_data->team_tiles[toMove]){
        pos = RIGHT(pos) & ~board_data->team_tiles[toMove];
        moves |= pos;
        pos &= ~board_data->team_tiles[~toMove & 0x01];
    }
    pos = r_pos;
    while(UP(pos) & ~board_data->team_tiles[toMove]){
        pos = UP(pos) & ~board_data->team_tiles[toMove];
        moves |= pos;
        pos &= ~board_data->team_tiles[~toMove & 0x01];
    }
    pos = r_pos;
    while(DOWN(pos) & ~board_data->team_tiles[toMove]){
        pos = DOWN(pos) & ~board_data->team_tiles[toMove];
        moves |= pos;
        pos &= ~board_data->team_tiles[~toMove & 0x01];
    }
#ifdef ERROR_CHECK
    int error = 0;
    if(PopCount(moves & board_data->team_tiles[~toMove & 0x01]) > 4) error = 1;
    if((moves & board_data->team_tiles[toMove]) != 0) error = 1;
    if(error) printf("Movegen Error: rook\n");
#endif
    return moves & ~board_data->team_tiles[toMove];
}

U64 Get_Queen_Moves(Board_Data_t* board_data, int toMove, U64 q_pos){
    return Get_Rook_Moves(board_data, toMove, q_pos) | Get_Bishop_Moves(board_data, toMove, q_pos);
}

