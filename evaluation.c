
#include "evaluation.h"
#include "movegen.h"
#include "movegen_fast.h"
#include <immintrin.h>

//including this to have some more to optimize
//white will have positive score, black will have negative
int score_board(Board_Data_t* board_data){
    int score = 0;
    U64* pieces = board_data->pieces;
    score += king_val * (PopCount(pieces[W_KING]) - PopCount(pieces[B_KING]));
    score += queen_val * (PopCount(pieces[W_QUEEN]) - PopCount(pieces[B_QUEEN]));
    score += rook_val * (PopCount(pieces[W_ROOK]) - PopCount(pieces[B_ROOK]));
    score += bishop_val * (PopCount(pieces[W_BISHOP]) - PopCount(pieces[B_BISHOP]));
    score += knight_val * (PopCount(pieces[W_KNIGHT]) - PopCount(pieces[B_KNIGHT]));
    score += pawn_val * (PopCount(pieces[W_PAWN]) - PopCount(pieces[B_PAWN]));
    
    //should be the squares in the center
    const U64 mid_4x4 = 0x0000001818000000ULL;
    const U64 mid_6x6 = 0x00003C3C3C3C0000ULL;

    //bonus for having a bishop pair
    if(PopCount(pieces[W_BISHOP]) >= 2) score += bishop_pair_val;
    if(PopCount(pieces[B_BISHOP]) >= 2) score -= bishop_pair_val;
    
    //points for having pawns in the middle of the board
    score += mid_6x6_val * (PopCount(pieces[W_PAWN] & mid_6x6) - PopCount(pieces[B_PAWN] & mid_6x6));
    score += mid_4x4_val * (PopCount(pieces[W_PAWN] & mid_4x4) - PopCount(pieces[B_PAWN] & mid_4x4));

    //pawns that are defending pawns get double points
    U64 def_pawn_white = pieces[W_PAWN] & (UPLEFT(pieces[W_PAWN]) | UPRIGHT(pieces[W_PAWN]));
    U64 def_pawn_black = pieces[B_PAWN] & (DOWNLEFT(pieces[B_PAWN]) | DOWNRIGHT(pieces[B_PAWN]));
    score += defended_pawn_val * (PopCount(def_pawn_white) - PopCount(def_pawn_black));   

    //extra points for having a rook behind a passed pawn
    U64 open_files = Get_Open_Files(pieces);
    score += open_files_val * (PopCount(pieces[W_ROOK] & open_files) - PopCount(pieces[B_ROOK] & open_files));

    //extra points for moving pieces towards the middle of the board
    score += not_border_val * (PopCount(board_data->team_tiles[WHITE] & ~border) - PopCount(board_data->team_tiles[BLACK] & ~border));

    //extra points for having the king castled
    score += castle_val * (PopCount(pieces[W_KING] & (0xC7)) - PopCount(pieces[B_KING] & ((U64)0xC7 << (8*7))));

    U64 white_passed = Get_Passed_Pawns(pieces,WHITE);
    U64 black_passed = Get_Passed_Pawns(pieces,BLACK);

    //50 points for a passed pawn, because it has strong queening potential
    score += passed_pawn_val * (PopCount(white_passed) - PopCount(black_passed));

    //lose points for doubled pawns
    score += double_pawn_val * (PopCount(pieces[W_PAWN] & UP(pieces[W_PAWN])) - PopCount(pieces[B_PAWN] & DOWN(pieces[B_PAWN])));

    //bonus points for bishops next to each other
    score += connect_bishop_val * (PopCount(pieces[W_BISHOP] & (UP(pieces[W_BISHOP]) | DOWN(pieces[W_BISHOP]) | LEFT(pieces[W_BISHOP]) | RIGHT(pieces[W_BISHOP]))));
    score -= connect_bishop_val * (PopCount(pieces[B_BISHOP] & (UP(pieces[B_BISHOP]) | DOWN(pieces[B_BISHOP]) | LEFT(pieces[B_BISHOP]) | RIGHT(pieces[B_BISHOP]))));

    //this causes a pretty severe slowdown
    score += moves_val * (PopCount(Get_Team_Move_Mask(board_data, WHITE)) - PopCount(Get_Team_Move_Mask(board_data, BLACK)));


    U64 w_king_defender = King_Moves(board_data, WHITE) & board_data->team_tiles[WHITE];
    U64 b_king_defender = King_Moves(board_data, BLACK) & board_data->team_tiles[BLACK];

    U64 w_king_pawn_defender = w_king_defender & board_data->pieces[W_PAWN];
    U64 b_king_pawn_defender = b_king_defender & board_data->pieces[B_PAWN];

    score += king_defenders_val * (PopCount_fast(w_king_defender) - PopCount_fast(b_king_defender));
    score += king_pawn_defenders_val * (PopCount_fast(w_king_pawn_defender) - PopCount_fast(b_king_pawn_defender));
    
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
    __m256i score_kqrb = _mm256_set_epi64x(king_val, queen_val, rook_val, bishop_val);
    __m256i kqrb = _mm256_sub_epi64(white_kqrb, black_kqrb);
    vec_score = _mm256_mul_epi32(kqrb, score_kqrb);

    const U64 mid_4x4 = 0x0000001818000000ULL;
    const U64 mid_6x6 = 0x00003C3C3C3C0000ULL;
    //knight, pawn, pawn6x6, pawn 4x4 -- 300, 100, 20, 50
    __m256i white_np64 = AVX_PopCount(_mm256_set_epi64x(pieces[W_KNIGHT], pieces[W_PAWN],
                                        pieces[W_PAWN] & mid_6x6, pieces[W_PAWN] & mid_4x4));
    __m256i black_np64 = AVX_PopCount(_mm256_set_epi64x(pieces[B_KNIGHT], pieces[B_PAWN],
                                        pieces[B_PAWN] & mid_6x6, pieces[B_PAWN] & mid_4x4)); 
    __m256i score_np64 = _mm256_set_epi64x(knight_val, pawn_val, mid_6x6_val, mid_4x4_val);
    __m256i np64 = _mm256_sub_epi64(white_np64, black_np64);    
    np64 = _mm256_mul_epi32(np64, score_np64);
    vec_score = _mm256_add_epi64(np64, vec_score);

    //bonus for having a bishop pair
    if(PopCount_fast(pieces[W_BISHOP]) >= 2) score += bishop_pair_val;
    if(PopCount_fast(pieces[B_BISHOP]) >= 2) score -= bishop_pair_val;

    //pawns that are defending pawns get double points
    U64 def_pawn_white = pieces[W_PAWN] & (UPLEFT(pieces[W_PAWN]) | UPRIGHT(pieces[W_PAWN]));
    U64 def_pawn_black = pieces[B_PAWN] & (DOWNLEFT(pieces[B_PAWN]) | DOWNRIGHT(pieces[B_PAWN]));
    U64 open_files = Get_Open_Files(pieces);
    //defended pawns, rooks on open file, pieces off the border, kings castled
    //      -- 50, 150, 5, 300
    __m256i white_dp_or_m_kc = AVX_PopCount(_mm256_set_epi64x(def_pawn_white, pieces[W_ROOK] & open_files,
                                                board_data->team_tiles[WHITE] & ~border, pieces[W_KING] & (0xE7)));
    __m256i black_dp_or_m_kc = AVX_PopCount(_mm256_set_epi64x(def_pawn_black, pieces[B_ROOK] & open_files,
                                                board_data->team_tiles[BLACK] & ~border, pieces[B_KING] & ((U64)0xE7 << (8*7))));
    __m256i score_dp_or_m_kc = _mm256_set_epi64x(defended_pawn_val, open_files_val, not_border_val, castle_val);
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
    __m256i score_pas_mov_dbl_bnex = _mm256_set_epi64x(passed_pawn_val, moves_val, double_pawn_val, connect_bishop_val);
    __m256i pas_mov_dbl_bnex = _mm256_sub_epi64(white_pas_mov_dbl_bnex, black_pas_mov_dbl_bnex);    
    pas_mov_dbl_bnex = _mm256_mul_epi32(pas_mov_dbl_bnex, score_pas_mov_dbl_bnex);
    vec_score = _mm256_add_epi64(pas_mov_dbl_bnex, vec_score);

    ////    ONCE I GET A MULTIPLE OF 4, I MAKE IT AVX INSTRUCTION   ////

    U64 w_king_defender = King_Moves(board_data, WHITE) & board_data->team_tiles[WHITE];
    U64 b_king_defender = King_Moves(board_data, BLACK) & board_data->team_tiles[BLACK];

    U64 w_king_pawn_defender = w_king_defender & board_data->pieces[W_PAWN];
    U64 b_king_pawn_defender = b_king_defender & board_data->pieces[B_PAWN];

    score += king_defenders_val * (PopCount_fast(w_king_defender) - PopCount_fast(b_king_defender));
    score += king_pawn_defenders_val * (PopCount_fast(w_king_pawn_defender) - PopCount_fast(b_king_pawn_defender));

    int score0 = _mm256_extract_epi64(vec_score,0);
    int score1 = _mm256_extract_epi64(vec_score,1);
    int score2 = _mm256_extract_epi64(vec_score,2);
    int score3 = _mm256_extract_epi64(vec_score,3);
    return (score + score0 + score1 + score2 + score3);
}

int score_board_simple(Board_Data_t* board_data){
    int score = 0;
    U64* pieces = board_data->pieces;
    score += king_val * (_mm_popcnt_u64(pieces[W_KING]) - _mm_popcnt_u64(pieces[B_KING]));
    score += queen_val * (_mm_popcnt_u64(pieces[W_QUEEN]) - _mm_popcnt_u64(pieces[B_QUEEN]));
    score += rook_val * (_mm_popcnt_u64(pieces[W_ROOK]) - _mm_popcnt_u64(pieces[B_ROOK]));
    score += bishop_val * (_mm_popcnt_u64(pieces[W_BISHOP]) - _mm_popcnt_u64(pieces[B_BISHOP]));
    score += knight_val * (_mm_popcnt_u64(pieces[W_KNIGHT]) - _mm_popcnt_u64(pieces[B_KNIGHT]));
    score += pawn_val * (_mm_popcnt_u64(pieces[W_PAWN]) - _mm_popcnt_u64(pieces[B_PAWN]));
    return score;
}

//returns all the pieces around the king,
//as long as the king isn't in the middle of the board
U64 King_Moves(Board_Data_t* board_data, int toMove){
    U64 middle_rows = LEFT(LEFT(LEFT(right_wall)));
    middle_rows |= LEFT(middle_rows);
    U64 king = board_data->pieces[W_KING + 6*toMove] & ~middle_rows;
    U64 defenders = ZERO;
    defenders |= UP(king);
    defenders |= DOWN(king);
    defenders |= LEFT(king);
    defenders |= RIGHT(king);
    defenders |= UPLEFT(king);
    defenders |= DOWNLEFT(king);
    defenders |= UPRIGHT(king);
    defenders |= DOWNRIGHT(king);
    return defenders;
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