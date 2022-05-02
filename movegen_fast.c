#include "movegen_fast.h"
#include "movegen.h"
#include <immintrin.h>
#include <emmintrin.h>
#include "move_tables.h"
//#define ERROR_CHECK
#define USE_LOOKUP 1
//doesn't give any speedup... :(
    
//returns a pointer to an array of the possible moves   !! NOT the legal moves -- still need to filter those
//TODO: add in en passant, promotions, castling
void Get_All_Moves_fast(Board_Data_t* board_data, move_t* movelist, int toMove){
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
            U64 p_moves = Get_Piece_Moves_fast(board_data, toMove, piece, i%6);
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

U64 Get_Piece_Moves_fast(Board_Data_t* board_data, int toMove, U64 pos, int type){
    #ifdef ERROR_CHECK
        ;
        U64 lookup;
        U64 normal;
    #endif
    switch(type){
        case(W_KING):
            return Get_King_Moves_fast(board_data, toMove, pos);
        case(W_QUEEN):
            #ifdef ERROR_CHECK 
                lookup = Lookup_Queen(board_data, toMove, pos);
                normal = Get_Queen_Moves_fast(board_data, toMove, pos);
                if(lookup != normal){
                    printf("Error, queen lookup != normal \n");
                }
            #endif
            #if USE_LOOKUP
                return Lookup_Queen(board_data, toMove, pos);
            #else
                return Get_Queen_Moves_fast(board_data, toMove, pos);
            #endif
        case(W_ROOK):
            #ifdef ERROR_CHECK 
                lookup = Lookup_Rook(board_data, toMove, pos);
                normal = Get_Rook_Moves_fast(board_data, toMove, pos);
                if(lookup != normal){
                    printf("Error, rook lookup != normal \n");
                }
            #endif
            #if USE_LOOKUP
                return Lookup_Rook(board_data, toMove, pos);
            #else
                return Get_Rook_Moves_fast(board_data, toMove, pos);
            #endif
        case(W_BISHOP):
            #ifdef ERROR_CHECK 
                lookup = Lookup_Bishop(board_data, toMove, pos);
                normal = Get_Bishop_Moves_fast(board_data, toMove, pos);
                if(lookup != normal){
                    printf("Error, bishop lookup != normal \n");
                }
            #endif
            #if USE_LOOKUP
                return Lookup_Bishop(board_data, toMove, pos);
            #else
                return Get_Bishop_Moves_fast(board_data, toMove, pos);
            #endif
        case(W_KNIGHT):
            return Get_Knight_Moves_fast(board_data, toMove, pos);
        case(W_PAWN):
            return Get_Pawn_Moves_fast(board_data, toMove, pos);
        default:
            printf("/////////////////////////////\n\tERROR IN GET PIECE MOVES\n/////////////////////////////\n");
            return ZERO;
    }
}

U64 Get_King_Moves_fast(Board_Data_t* board_data, int toMove, U64 k_pos){
    U64 moves;
    U64 moves0;
    U64 moves1;
    U64 moves2; 
    moves0 = UP(k_pos);
    moves1 = UPLEFT(k_pos);
    moves2 = UPRIGHT(k_pos);
    moves0 |= LEFT(k_pos);
    moves1 |= RIGHT(k_pos);
    moves2 |= DOWN(k_pos);
    moves0 |= DOWNLEFT(k_pos);
    moves1 |= DOWNRIGHT(k_pos);

    moves = moves0 | moves1 | moves2;

    //this is going to be slow but whatever
    //purpose of first if is so that we don't need to check(and fail) black/white after a bit
    if(board_data->cas_mask){
    if(!In_Check_fast(board_data, toMove)){
        U64 to_tile;
        if(toMove == WHITE){
            if(board_data->cas_mask & 0x01){
                to_tile = (ONE << 5) & ~board_data->occ;
                U64 attackers = Get_Tile_Attackers_fast(board_data, to_tile, toMove);
                if(attackers != ZERO) to_tile = ZERO;
                to_tile = RIGHT(to_tile) & ~board_data->occ;
                attackers = Get_Tile_Attackers_fast(board_data, to_tile, toMove);
                if(attackers != ZERO) to_tile = ZERO;
                moves |= to_tile;
            }
            if(board_data->cas_mask & 0x08){
                to_tile = (ONE << 3) & ~board_data->occ;
                U64 attackers = Get_Tile_Attackers_fast(board_data, to_tile, toMove);
                if(attackers != ZERO) to_tile = ZERO;
                to_tile = LEFT(to_tile) & ~board_data->occ;
                attackers = Get_Tile_Attackers_fast(board_data, to_tile, toMove);
                if(attackers != ZERO) to_tile = ZERO;
                to_tile &= ~(board_data->occ << 1);
                moves |= to_tile;
            }
        }else{
            if(board_data->cas_mask & 0x10){
                to_tile = (ONE << (5+56)) & ~board_data->occ;
                U64 attackers = Get_Tile_Attackers_fast(board_data, to_tile, toMove);
                if(attackers != ZERO) to_tile = ZERO;
                to_tile = RIGHT(to_tile) & ~board_data->occ;
                attackers = Get_Tile_Attackers_fast(board_data, to_tile, toMove);
                if(attackers != ZERO) to_tile = ZERO;
                moves |= to_tile;
            }
            if(board_data->cas_mask & 0x80){
                to_tile = (ONE << (3+56))& ~board_data->occ;
                U64 attackers = Get_Tile_Attackers_fast(board_data, to_tile, toMove);
                if(attackers != ZERO) to_tile = ZERO;
                to_tile = LEFT(to_tile) & ~board_data->occ;
                attackers = Get_Tile_Attackers_fast(board_data, to_tile, toMove);
                if(attackers != ZERO) to_tile = ZERO;
                to_tile &= ~(board_data-> occ << 1);
                moves |= to_tile;
            }
        }
    }
    }
    return moves & ~board_data->team_tiles[toMove];
}


U64 Get_Team_Move_Mask_fast(Board_Data_t* board_data, int toMove){
    U64 mask = ZERO;
    int team_offset = 6*toMove;
    for(int i = 0 + team_offset; i < 6+team_offset; i++){
        U64 pieces = board_data->pieces[i];
        while(pieces){
            U64 piece = Get_LSB(pieces);
            pieces ^= piece;
            mask |= Get_Piece_Moves_fast(board_data, toMove, piece, i%6);
        }
    }
    return mask;
}

int In_Check_fast(Board_Data_t* board_data, int toMove){
    U64 k_pos = board_data->pieces[W_KING + 6*toMove];
    #if ERROR_CHECK
        if(k_pos == ZERO){
            printf("Error, kpos == ZERO\n");
            return 1;
        }
    #endif
    U64 attackers;
    #if USE_LOOKUP
        attackers = Lookup_Tile_Attackers(board_data, k_pos, toMove);
    #else
        attackers = Get_Tile_Attackers_fast(board_data, k_pos, toMove);
    #endif
    return (attackers != ZERO);
}

//I need this so I don't go into a recursive loop when checking the kings legal moves
//for castling
U64 Get_King_Moves_No_Castles_fast(Board_Data_t* board_data, int toMove, U64 k_pos){
    U64 moves0;
    U64 moves1;
    U64 moves2; 
    moves0 = UP(k_pos);
    moves1 = UPLEFT(k_pos);
    moves2 = UPRIGHT(k_pos);
    moves0 |= LEFT(k_pos);
    moves1 |= RIGHT(k_pos);
    moves2 |= DOWN(k_pos);
    moves0 |= DOWNLEFT(k_pos);
    moves1 |= DOWNRIGHT(k_pos);
    return (moves0 | moves1 | moves2) & ~board_data->team_tiles[toMove];
}

U64 Lookup_Tile_Attackers(Board_Data_t* board_data, U64 pos, int toMove){
    U64 pawn_attackers = ((((pos & ~left_wall) << 7) | ((pos & ~right_wall) << 9)) & board_data->pieces[B_PAWN]);
    if(toMove == BLACK) {
        pawn_attackers = (((pos & ~right_wall) >> 7) | ((pos & ~left_wall) >> 9)) & board_data->pieces[W_PAWN];
    }
    U64 tile_attackers = ZERO;
    tile_attackers |= Lookup_Rook(board_data, toMove, pos) & (board_data->pieces[B_ROOK - 6*toMove] | board_data->pieces[B_QUEEN - 6*toMove]);
    tile_attackers |= Lookup_Bishop(board_data, toMove, pos) & (board_data->pieces[B_BISHOP - 6*toMove] | board_data->pieces[B_QUEEN - 6*toMove]);
    tile_attackers |= Get_Knight_Moves_fast(board_data, toMove, pos) & (board_data->pieces[B_KNIGHT - 6*toMove]);
    tile_attackers |= Get_King_Moves_No_Castles_fast(board_data, toMove, pos) & (board_data->pieces[B_KING - 6*toMove]);

    tile_attackers |= pawn_attackers;

    return tile_attackers;
}
U64 Get_Tile_Attackers_fast(Board_Data_t* board_data, U64 pos, int toMove){
    U64 pawn_attackers = ((((pos & ~left_wall) << 7) | ((pos & ~right_wall) << 9)) & board_data->pieces[B_PAWN]);
    if(toMove == BLACK) {
        pawn_attackers = (((pos & ~right_wall) >> 7) | ((pos & ~left_wall) >> 9)) & board_data->pieces[W_PAWN];
    }    

    U64 team_tiles = board_data->team_tiles[toMove];
    //will use rotate lefts to do this.  The rotate left moves bits from the high order to carry flag, then carry flag 
    //to low order bit. I do this to allow calculating each sliding direction at the same time. A normal lookup table
    //would definitely be faster, but for the purpose of EC527 final, this has more to do with optimizing hpc code.
    __m256i move_vec1 = _mm256_set1_epi64x(ZERO);
    __m256i move_vec2 = _mm256_set1_epi64x(ZERO);
    __m256i pos_vec1 = _mm256_set1_epi64x(pos);  //(ne, nw, n, e)
    __m256i pos_vec2 = _mm256_set1_epi64x(pos);  //(sw, se, s, w)              

    __m256i shifts_vec = _mm256_set_epi64x(9,7,8,1);   //(ne, nw, n, e) or (sw, se, s, w)
    //these are the squares we are allowed to move from for each direction
    //we take the wall, and our teams tiles.  We cannot move from walls were we will wrap around after shifting,
    //and we cannot move if the direction we are about to move to 
    __m256i blocked1 = _mm256_set_epi64x(~((right_wall | top_wall) | DOWNLEFT(team_tiles)),            //ne
                                            ~((left_wall | top_wall) | DOWNRIGHT(team_tiles)),      //nw
                                            ~((top_wall) | DOWN(team_tiles)),                       //n
                                            ~((right_wall) | LEFT(team_tiles)));                    //e
    __m256i blocked2 = _mm256_set_epi64x(~((left_wall | bottom_wall) | UPRIGHT(team_tiles)),           //sw
                                            ~((right_wall | bottom_wall) | UPLEFT(team_tiles)),     //se
                                            ~((bottom_wall) | UP(team_tiles)),                      //s
                                            ~((left_wall) | RIGHT(team_tiles)));                    //w
    __m256i enemy_tile = _mm256_set1_epi64x(~board_data->team_tiles[~toMove & 0x01]);
    
    //while any value in pos_vec != 0
    while(_mm256_testz_si256(pos_vec1, pos_vec1) == 0 || _mm256_testz_si256(pos_vec2, pos_vec2) == 0){
        //0 if we are about to wrap around, or if we are about to run into a team piece
        pos_vec1 = _mm256_and_si256(pos_vec1, blocked1);
        pos_vec2 = _mm256_and_si256(pos_vec2, blocked2);
        
        //shift over in the direction we want to go.
        pos_vec1 = _mm256_sllv_epi64(pos_vec1, shifts_vec);   
        pos_vec2 = _mm256_srlv_epi64(pos_vec2, shifts_vec);

        move_vec1 = _mm256_or_si256(pos_vec1, move_vec1);
        move_vec2 = _mm256_or_si256(pos_vec2, move_vec2);

        pos_vec1 = _mm256_and_si256(pos_vec1, enemy_tile);
        pos_vec2 = _mm256_and_si256(pos_vec2, enemy_tile);
    }

    move_vec1 = _mm256_or_si256(move_vec1, move_vec2);
    U64 rook0 = _mm256_extract_epi64(move_vec1,0);
    U64 rook1 = _mm256_extract_epi64(move_vec1,1);
    U64 bishop0 = _mm256_extract_epi64(move_vec1,2);
    U64 bishop1 = _mm256_extract_epi64(move_vec1,3);

    U64 rook_attackers = ((rook0 | rook1) & (board_data->pieces[B_ROOK - 6*toMove] | board_data->pieces[B_QUEEN - 6*toMove]));
    U64 bishop_attackers = ((bishop0 | bishop1) & (board_data->pieces[B_BISHOP - 6*toMove] | board_data->pieces[B_QUEEN - 6*toMove]));
    rook_attackers |= (Get_Knight_Moves_fast(board_data, toMove, pos) & (board_data->pieces[B_KNIGHT - 6*toMove]));
    bishop_attackers |= (Get_King_Moves_No_Castles_fast(board_data, toMove, pos) & (board_data->pieces[B_KING - 6*toMove]));

    U64 attackers = rook_attackers | pawn_attackers | bishop_attackers;

    return attackers;
}

//fastest would just be lookup table but for class doing mulitple accumulators.
U64 Get_Knight_Moves_fast(Board_Data_t* board_data, int toMove, U64 n_pos){
    U64 moves = ZERO;
    U64 moves2 = ZERO;
    U64 moves3 = ZERO;

    moves |= UPLEFT(LEFT(n_pos));
    moves2 |= DOWNLEFT(LEFT(n_pos));
    moves3 |= UPLEFT(UP(n_pos));
    moves |= UPRIGHT(UP(n_pos));
    moves2 |= UPRIGHT(RIGHT(n_pos));
    moves3 |= DOWNRIGHT(RIGHT(n_pos));
    moves |= DOWNLEFT(DOWN(n_pos));
    moves2 |= DOWNRIGHT(DOWN(n_pos));
    moves |= moves3 | moves2;

    return moves & ~board_data->team_tiles[toMove];
}

U64 Get_Pawn_Moves_fast(Board_Data_t* board_data, int toMove, U64 p_pos){
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

//going to vectorize the loop, since each while loop is doing the same thing, just in a different direction
U64 Get_Bishop_Moves_fast(Board_Data_t* board_data, int toMove, U64 b_pos){
    U64 team_tiles = board_data->team_tiles[toMove];

    //will use rotate lefts to do this.  The rotate left moves bits from the high order to carry flag, then carry flag 
    //to low order bit. I do this to allow calculating each sliding direction at the same time. A normal lookup table
    //would definitely be faster, but for the purpose of EC527 final, this has more to do with optimizing hpc code.
    __m128i move_vec1 = _mm_set1_epi64x(ZERO);
    __m128i move_vec2 = _mm_set1_epi64x(ZERO);
    __m128i pos_vec1 = _mm_set1_epi64x(b_pos);  //(ne, nw)
    __m128i pos_vec2 = _mm_set1_epi64x(b_pos);  //(se, sw)              

    __m128i shifts_vec = _mm_set_epi64x(9,7);   //(ne, nw) or (sw, se)
    //these are the squares we are allowed to move from for each direction
    //we take the wall, and our teams tiles.  We cannot move from walls were we will wrap around after shifting,
    //and we cannot move if the direction we are about to move to 
    __m128i blocked1 = _mm_set_epi64x(~((right_wall | top_wall) | DOWNLEFT(team_tiles)),            //ne
                                            ~((left_wall | top_wall) | DOWNRIGHT(team_tiles)));          //nw
    __m128i blocked2 = _mm_set_epi64x(~((left_wall | bottom_wall) | UPRIGHT(team_tiles)),       //sw
                                            ~((right_wall | bottom_wall) | UPLEFT(team_tiles)));    //se
    __m128i enemy_tile = _mm_set1_epi64x(~board_data->team_tiles[~toMove & 0x01]);
    
    //while any value in pos_vec != 0
    //should be ok to do these in the same loop even if one finishes early bc these instructions
    //all have a latency of 1, but a cpi of .5, it should be the same speed regardless
    while(_mm_testz_si128(pos_vec1, pos_vec1) == 0 || _mm_testz_si128(pos_vec2, pos_vec2) == 0){
        //0 if we are about to wrap around, or if we are about to run into a team piece
        pos_vec1 = _mm_and_si128(pos_vec1, blocked1);
        pos_vec2 = _mm_and_si128(pos_vec2, blocked2);
        
        //shift over in the direction we want to go.
        pos_vec1 = _mm_sllv_epi64(pos_vec1, shifts_vec);   
        pos_vec2 = _mm_srlv_epi64(pos_vec2, shifts_vec);

        move_vec1 = _mm_or_si128(pos_vec1, move_vec1);
        move_vec2 = _mm_or_si128(pos_vec2, move_vec2);

        pos_vec1 = _mm_and_si128(pos_vec1, enemy_tile);
        pos_vec2 = _mm_and_si128(pos_vec2, enemy_tile);
    }
    U64 move0 = _mm_extract_epi64(move_vec1,0);
    U64 move1 = _mm_extract_epi64(move_vec1,1);
    U64 move2 = _mm_extract_epi64(move_vec2,0);
    U64 move3 = _mm_extract_epi64(move_vec2,1);
    return (move0 | move1) | (move2 | move3);
}

U64 Get_Rook_Moves_fast(Board_Data_t* board_data, int toMove, U64 r_pos){
    U64 team_tiles = board_data->team_tiles[toMove];

    //will use rotate lefts to do this.  The rotate left moves bits from the high order to carry flag, then carry flag 
    //to low order bit. I do this to allow calculating each sliding direction at the same time. A normal lookup table
    //would definitely be faster, but for the purpose of EC527 final, this has more to do with optimizing hpc code.
    __m128i move_vec1 = _mm_set1_epi64x(ZERO);
    __m128i move_vec2 = _mm_set1_epi64x(ZERO);
    __m128i pos_vec1 = _mm_set1_epi64x(r_pos);  //(ne, nw)
    __m128i pos_vec2 = _mm_set1_epi64x(r_pos);  //(se, sw)              

    __m128i shifts_vec = _mm_set_epi64x(1,8);   //(e, n) or (w, s)
    //these are the squares we are allowed to move from for each direction
    //we take the wall, and our teams tiles.  We cannot move from walls were we will wrap around after shifting,
    //and we cannot move if the direction we are about to move to 
    __m128i blocked1 = _mm_set_epi64x(~((right_wall) | LEFT(team_tiles)),            //ne
                                            ~((top_wall) | DOWN(team_tiles)));          //nw
    __m128i blocked2 = _mm_set_epi64x(~((left_wall) | RIGHT(team_tiles)),       //sw
                                            ~((bottom_wall) | UP(team_tiles)));    //se
    __m128i enemy_tile = _mm_set1_epi64x(~board_data->team_tiles[~toMove & 0x01]);
    
    //while any value in pos_vec != 0
    //should be ok to do these in the same loop even if one finishes early bc these instructions
    //all have a latency of 1, but a cpi of .5, it should be the same speed regardless
    while(_mm_testz_si128(pos_vec1, pos_vec1) == 0 || _mm_testz_si128(pos_vec2, pos_vec2) == 0){
        //0 if we are about to wrap around, or if we are about to run into a team piece
        pos_vec1 = _mm_and_si128(pos_vec1, blocked1);
        pos_vec2 = _mm_and_si128(pos_vec2, blocked2);
        
        //shift over in the direction we want to go.
        pos_vec1 = _mm_sllv_epi64(pos_vec1, shifts_vec);   
        pos_vec2 = _mm_srlv_epi64(pos_vec2, shifts_vec);

        move_vec1 = _mm_or_si128(pos_vec1, move_vec1);
        move_vec2 = _mm_or_si128(pos_vec2, move_vec2);

        pos_vec1 = _mm_and_si128(pos_vec1, enemy_tile);
        pos_vec2 = _mm_and_si128(pos_vec2, enemy_tile);
    }
    U64 move0 = _mm_extract_epi64(move_vec1,0);
    U64 move1 = _mm_extract_epi64(move_vec1,1);
    U64 move2 = _mm_extract_epi64(move_vec2,0);
    U64 move3 = _mm_extract_epi64(move_vec2,1);
    return (move0 | move1) | (move2 | move3);
}

//probably would be faster to use 256 bit registers for this.
U64 Get_Queen_Moves_fast(Board_Data_t* board_data, int toMove, U64 q_pos){
    U64 team_tiles = board_data->team_tiles[toMove];
    //will use rotate lefts to do this.  The rotate left moves bits from the high order to carry flag, then carry flag 
    //to low order bit. I do this to allow calculating each sliding direction at the same time. A normal lookup table
    //would definitely be faster, but for the purpose of EC527 final, this has more to do with optimizing hpc code.
    __m256i move_vec1 = _mm256_set1_epi64x(ZERO);
    __m256i move_vec2 = _mm256_set1_epi64x(ZERO);
    __m256i pos_vec1 = _mm256_set1_epi64x(q_pos);  //(ne, nw, n, e)
    __m256i pos_vec2 = _mm256_set1_epi64x(q_pos);  //(sw, se, s, w)              

    __m256i shifts_vec = _mm256_set_epi64x(9,7,8,1);   //(ne, nw, n, e) or (sw, se, s, w)
    //these are the squares we are allowed to move from for each direction
    //we take the wall, and our teams tiles.  We cannot move from walls were we will wrap around after shifting,
    //and we cannot move if the direction we are about to move to 
    __m256i blocked1 = _mm256_set_epi64x(~((right_wall | top_wall) | DOWNLEFT(team_tiles)),            //ne
                                            ~((left_wall | top_wall) | DOWNRIGHT(team_tiles)),      //nw
                                            ~((top_wall) | DOWN(team_tiles)),                       //n
                                            ~((right_wall) | LEFT(team_tiles)));                    //e
    __m256i blocked2 = _mm256_set_epi64x(~((left_wall | bottom_wall) | UPRIGHT(team_tiles)),           //sw
                                            ~((right_wall | bottom_wall) | UPLEFT(team_tiles)),     //se
                                            ~((bottom_wall) | UP(team_tiles)),                      //s
                                            ~((left_wall) | RIGHT(team_tiles)));                    //w
    __m256i enemy_tile = _mm256_set1_epi64x(~board_data->team_tiles[~toMove & 0x01]);
    
    //while any value in pos_vec != 0
    while(_mm256_testz_si256(pos_vec1, pos_vec1) == 0 || _mm256_testz_si256(pos_vec2, pos_vec2) == 0){
        //0 if we are about to wrap around, or if we are about to run into a team piece
        pos_vec1 = _mm256_and_si256(pos_vec1, blocked1);
        pos_vec2 = _mm256_and_si256(pos_vec2, blocked2);
        
        //shift over in the direction we want to go.
        pos_vec1 = _mm256_sllv_epi64(pos_vec1, shifts_vec);   
        pos_vec2 = _mm256_srlv_epi64(pos_vec2, shifts_vec);

        move_vec1 = _mm256_or_si256(pos_vec1, move_vec1);
        move_vec2 = _mm256_or_si256(pos_vec2, move_vec2);

        pos_vec1 = _mm256_and_si256(pos_vec1, enemy_tile);
        pos_vec2 = _mm256_and_si256(pos_vec2, enemy_tile);
    }
    
    move_vec1 = _mm256_or_si256(move_vec1, move_vec2);
    U64 rook0 = _mm256_extract_epi64(move_vec1,0);
    U64 rook1 = _mm256_extract_epi64(move_vec1,1);
    U64 bishop0 = _mm256_extract_epi64(move_vec1,2);
    U64 bishop1 = _mm256_extract_epi64(move_vec1,3);

    return rook0 | rook1 | bishop0 | bishop1;
}