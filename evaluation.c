
#include "evaluation.h"
#include "movegen.h"
#include "movegen_fast.h"
#include <immintrin.h>

//including this to have some more to optimize
//white will have positive score, black will have negative
int score_board(Board_Data_t* board_data){
    int score = 0;
    U64* pieces = board_data->pieces;
    score += KING_VAL * (PopCount(pieces[W_KING]) - PopCount(pieces[B_KING]));
    score += QUEEN_VAL * (PopCount(pieces[W_QUEEN]) - PopCount(pieces[B_QUEEN]));
    score += ROOK_VAL * (PopCount(pieces[W_ROOK]) - PopCount(pieces[B_ROOK]));
    score += BISHOP_VAL * (PopCount(pieces[W_BISHOP]) - PopCount(pieces[B_BISHOP]));
    score += KNIGHT_VAL * (PopCount(pieces[W_KNIGHT]) - PopCount(pieces[B_KNIGHT]));
    score += PAWN_VAL * (PopCount(pieces[W_PAWN]) - PopCount(pieces[B_PAWN]));
    
    //should be the squares in the center
    const U64 mid_4x4 = 0x0000001818000000ULL;
    const U64 mid_6x6 = 0x00003C3C3C3C0000ULL;

    //bonus for having a bishop pair
    if(PopCount(pieces[W_BISHOP]) >= 2) score += 50;
    if(PopCount(pieces[B_BISHOP]) >= 2) score -= 50;
    
    //points for having pawns in the middle of the board
    score += 20 * (PopCount(pieces[W_PAWN] & mid_6x6) - PopCount(pieces[B_PAWN] & mid_6x6));
    score += 50 * (PopCount(pieces[W_PAWN] & mid_4x4) - PopCount(pieces[B_PAWN] & mid_4x4));

    //pawns that are defending pawns get double points
    U64 def_pawn_white = pieces[W_PAWN] & (UPLEFT(pieces[W_PAWN]) | UPRIGHT(pieces[W_PAWN]));
    U64 def_pawn_black = pieces[B_PAWN] & (DOWNLEFT(pieces[B_PAWN]) | DOWNRIGHT(pieces[B_PAWN]));
    score += 100 * (PopCount(def_pawn_white) - PopCount(def_pawn_black));   

    //extra points for having a rook behind a passed pawn
    U64 open_files = Get_Open_Files(pieces);
    score += 150 * (PopCount(pieces[W_ROOK] & open_files) - PopCount(pieces[B_ROOK] & open_files));

    //extra points for moving pieces towards the middle of the board
    score += 5 * (PopCount(board_data->team_tiles[WHITE] & ~border) - PopCount(board_data->team_tiles[BLACK] & ~border));

    //extra points for having the king castled
    score += 150 * (PopCount(pieces[W_KING] & (0xC7)) - PopCount(pieces[B_KING] & ((U64)0xC7 << (8*7))));

    U64 white_passed = Get_Passed_Pawns(pieces,WHITE);
    U64 black_passed = Get_Passed_Pawns(pieces,BLACK);

    //50 points for a passed pawn, because it has strong queening potential
    score += 400 * (PopCount(white_passed) - PopCount(black_passed));

    //lose points for doubled pawns
    score -= 50 * (PopCount(pieces[W_PAWN] & UP(pieces[W_PAWN])) - PopCount(pieces[B_PAWN] & DOWN(pieces[B_PAWN])));

    //bonus points for bishops next to each other
    score += 100 * (PopCount(pieces[W_BISHOP] & (UP(pieces[W_BISHOP]) | DOWN(pieces[W_BISHOP]) | LEFT(pieces[W_BISHOP]) | RIGHT(pieces[W_BISHOP]))));
    score -= 100 * (PopCount(pieces[B_BISHOP] & (UP(pieces[B_BISHOP]) | DOWN(pieces[B_BISHOP]) | LEFT(pieces[B_BISHOP]) | RIGHT(pieces[B_BISHOP]))));

    //this causes a pretty severe slowdown
    score += 5 * (PopCount(Get_Team_Move_Mask(board_data, WHITE)) - PopCount(Get_Team_Move_Mask(board_data, BLACK)));

    return score;
}

int score_board_fast(Board_Data_t* board_data){
    int score = 0;
    U64* pieces = board_data->pieces;
    __m256i vec_score;

    //king, queen, rook, bishop -- 5000, 900, 500, 330
    __m256i white_kqrb = AVX_PopCount(_mm256_set_epi64x(pieces[W_KING], pieces[W_QUEEN], 
                                                pieces[W_ROOK], pieces[W_BISHOP]));
    __m256i black_kqrb = AVX_PopCount(_mm256_set_epi64x(pieces[B_KING], pieces[B_QUEEN], 
                                                pieces[B_ROOK], pieces[B_BISHOP]));
    __m256i score_kqrb = _mm256_set_epi64x(KING_VAL, QUEEN_VAL, ROOK_VAL, BISHOP_VAL);
    __m256i kqrb = _mm256_sub_epi64(white_kqrb, black_kqrb);
    vec_score = _mm256_mul_epi32(kqrb, score_kqrb);

    const U64 mid_4x4 = 0x0000001818000000ULL;
    const U64 mid_6x6 = 0x00003C3C3C3C0000ULL;
    //knight, pawn, pawn6x6, pawn 4x4 -- 300, 100, 20, 50
    __m256i white_np64 = AVX_PopCount(_mm256_set_epi64x(pieces[W_KNIGHT], pieces[W_PAWN],
                                        pieces[W_PAWN] & mid_6x6, pieces[W_PAWN] & mid_4x4));
    __m256i black_np64 = AVX_PopCount(_mm256_set_epi64x(pieces[B_KNIGHT], pieces[B_PAWN],
                                        pieces[B_PAWN] & mid_6x6, pieces[B_PAWN] & mid_4x4)); 
    __m256i score_np64 = _mm256_set_epi64x(KNIGHT_VAL, PAWN_VAL, 20, 50);
    __m256i np64 = _mm256_sub_epi64(white_np64, black_np64);    
    np64 = _mm256_mul_epi32(np64, score_np64);
    vec_score = _mm256_add_epi64(np64, vec_score);

    //bonus for having a bishop pair
    if(PopCount_fast(pieces[W_BISHOP]) >= 2) score += 50;
    if(PopCount_fast(pieces[B_BISHOP]) >= 2) score -= 50;

    //pawns that are defending pawns get double points
    U64 def_pawn_white = pieces[W_PAWN] & (UPLEFT(pieces[W_PAWN]) | UPRIGHT(pieces[W_PAWN]));
    U64 def_pawn_black = pieces[B_PAWN] & (DOWNLEFT(pieces[B_PAWN]) | DOWNRIGHT(pieces[B_PAWN]));
    U64 open_files = Get_Open_Files(pieces);
    //defended pawns, rooks on open file, pieces off the border, kings castled
    //      -- 100, 150, 5, 150
    __m256i white_dp_or_m_kc = AVX_PopCount(_mm256_set_epi64x(def_pawn_white, pieces[W_ROOK] & open_files,
                                                board_data->team_tiles[WHITE] & ~border, pieces[W_KING] & (0xC7)));
    __m256i black_dp_or_m_kc = AVX_PopCount(_mm256_set_epi64x(def_pawn_black, pieces[B_ROOK] & open_files,
                                                board_data->team_tiles[BLACK] & ~border, pieces[B_KING] & ((U64)0xC7 << (8*7))));
    __m256i score_dp_or_m_kc = _mm256_set_epi64x(100, 150, 5, 150);
    __m256i dp_or_m_kc = _mm256_sub_epi64(white_dp_or_m_kc, black_dp_or_m_kc);    
    dp_or_m_kc = _mm256_mul_epi32(dp_or_m_kc, score_dp_or_m_kc);
    vec_score = _mm256_add_epi64(dp_or_m_kc, vec_score);

    U64 white_passed = Get_Passed_Pawns(pieces,WHITE);
    U64 black_passed = Get_Passed_Pawns(pieces,BLACK);
    U64 white_mask = Get_Team_Move_Mask_fast(board_data, WHITE);
    U64 black_mask = Get_Team_Move_Mask_fast(board_data, BLACK);
    U64 white_double = pieces[W_PAWN] & UP(pieces[W_PAWN]);
    U64 black_double = pieces[B_PAWN] & DOWN(pieces[B_PAWN]);
    U64 white_bnext = pieces[W_BISHOP] & (UP(pieces[W_BISHOP]) | DOWN(pieces[W_BISHOP]) | LEFT(pieces[W_BISHOP]) | RIGHT(pieces[W_BISHOP]));
    U64 black_bnext = pieces[B_BISHOP] & (UP(pieces[B_BISHOP]) | DOWN(pieces[B_BISHOP]) | LEFT(pieces[B_BISHOP]) | RIGHT(pieces[B_BISHOP]));

    //passed pawn, move mask, doubled pawns, adjacent bishops -- 400, 5, -50, 100
    __m256i white_pas_mov_dbl_bnex = AVX_PopCount(_mm256_set_epi64x(white_passed, white_mask, 
                                                                    white_double, white_bnext));
    __m256i black_pas_mov_dbl_bnex = AVX_PopCount(_mm256_set_epi64x(black_passed, black_mask, 
                                                                    black_double, black_bnext));
    __m256i score_pas_mov_dbl_bnex = _mm256_set_epi64x(400, 5, -50, 100);
    __m256i pas_mov_dbl_bnex = _mm256_sub_epi64(white_pas_mov_dbl_bnex, black_pas_mov_dbl_bnex);    
    pas_mov_dbl_bnex = _mm256_mul_epi32(pas_mov_dbl_bnex, score_pas_mov_dbl_bnex);
    vec_score = _mm256_add_epi64(pas_mov_dbl_bnex, vec_score);

    int score0 = _mm256_extract_epi64(vec_score,0);
    int score1 = _mm256_extract_epi64(vec_score,1);
    int score2 = _mm256_extract_epi64(vec_score,2);
    int score3 = _mm256_extract_epi64(vec_score,3);
    return (score + score0 + score1 + score2 + score3);
}



//scan a file across the board and add it to an open files mask if there
//are no pawns on the file
U64 Get_Open_Files(U64* board){
    U64 pawns  = board[W_PAWN] | board[B_PAWN];
    U64 file = right_wall;
    U64 open_files = ZERO;
    for(int i = 0; i < 8; i++){
        if((file & pawns) == ZERO){
            open_files |= file;
        }
        file  = LEFT(file);
    }
    return open_files;
}

//slides the enemy pawns down the board zeroing out team pawns
//I think I could maybe only calculate it once but idk
U64 Get_Passed_Pawns(U64* board, int toMove){
    U64 white_pawns = board[W_PAWN];
    U64 black_pawns = board[B_PAWN];
    if(toMove == WHITE){
        black_pawns |= RIGHT(black_pawns) | LEFT(black_pawns);
        while(black_pawns != ZERO){
            black_pawns = DOWN(black_pawns);
            white_pawns &=  ~(black_pawns);
        }
        return white_pawns;
    }else{
        white_pawns |= RIGHT(white_pawns) | LEFT(white_pawns);
        while(white_pawns != ZERO){
            white_pawns = UP(white_pawns);
            black_pawns &=  ~(white_pawns);
        }
        return black_pawns;
    }
}