
#include "board.h"
#include "immintrin.h"
#include "bit_helper.h"
#include "move_tables.h"
#include "magic_lookup_tables.h"



U64 Lookup_Rook(Board_Data_t* board_data, int toMove, U64 pos){
    U64 blockers = board_data->occ;
    U64 potential_moves;
    U64 team_pieces;

    team_pieces = board_data->team_tiles[toMove];
    int idx = Get_Idx(pos);
    blockers &= board_data->move_tables->rook_mask[idx];
    U64 key = (blockers * rook_magics[idx]) >> (64ull - rook_idx_bits[idx]);
    potential_moves = board_data->move_tables->rook_table[idx][key];
    return (potential_moves & ~team_pieces);
}
U64 Lookup_Bishop(Board_Data_t* board_data, int toMove, U64 pos){
    U64 blockers = board_data->occ;
    U64 potential_moves;
    U64 team_pieces;

    team_pieces = board_data->team_tiles[toMove];
    int idx = Get_Idx(pos);
    blockers &= board_data->move_tables->bishop_mask[idx];
    U64 key = (blockers * bishop_magics[idx]) >> (64ull - bishop_idx_bits[idx]);
    potential_moves = board_data->move_tables->bishop_table[idx][key];
    return (potential_moves & ~team_pieces);
}
U64 Lookup_Queen(Board_Data_t* board_data, int toMove, U64 pos){
    return Lookup_Bishop(board_data, toMove, pos) | Lookup_Rook(board_data, toMove, pos);
}

move_table_t* Init_Move_Table(){
    move_table_t* move_table = (move_table_t*) malloc(sizeof(move_table_t));
    Init_Line_Table(move_table);
    Init_Rook_Line_Table(move_table);
    Init_Bishop_Line_Table(move_table);
    InitRookMagicTable(move_table);
    InitBishopMagicTable(move_table);
    return move_table;
}

void Delete_Move_Table(move_table_t* move_table){
    for(int i = 0; i < 8; i++){
        free(move_table->line_tables[i]);
    }
    free(move_table->line_tables);
    for(int i = 0; i < 64; i++){
        free(move_table->rook_table[i]);
        free(move_table->bishop_table[i]);
    }
    free(move_table->bishop_table);
    free(move_table->rook_table);
    free(move_table->bishop_mask);
    free(move_table->rook_mask);
    free(move_table);
}

void Init_Line_Table(move_table_t* move_table){

    U64** line_table = (U64 **) malloc(sizeof(U64*) * 8);
    for(int i = 0; i < 8; i++){
        line_table[i] = (U64*) malloc(sizeof(U64) * 64);
    }

    for(int i = 0; i < 64; i++){
        int row = i / 8;
        int col = i % 8;
        //Precomputes and stores all sliding attacks for each position into a table.
        line_table[north][i] = 0x0101010101010100ULL << i;
        line_table[south][i] = 0x0080808080808080ULL >> (63 - i);
        line_table[east][i] = (U64)2 * (((U64)1 << (i | 7)) - ((U64)1 << i));
        line_table[west][i] = ((U64)1 << i) - ((U64)1 << (i & 56));

        line_table[northwest][i] = West_f(0x102040810204000ULL, 7 - col) << (row * 8);
        line_table[northeast][i] = East_f(0x8040201008040200ULL, col) << (row * 8);
        line_table[southwest][i] = West_f(0x40201008040201ULL, 7 - col) >> ((7 - row) * 8);
        line_table[southeast][i] = East_f(0x2040810204080ULL, col) >> ((7 - row) * 8);
    }
    move_table->line_tables = line_table;
}

U64 East_f(U64 pos, int n){
    for (int i = 0; i < n; i++)
    {
        pos = ((pos << 1) & (~left_wall));
    }
    return pos;
}
U64 West_f(U64 pos, int n){
    for (int i = 0; i < n; i++)
    {
        pos = ((pos >> 1) & (~right_wall));
    }
    return pos;
}
void Init_Rook_Line_Table(move_table_t* move_table)
{
    U64* rook_mask = (U64*) malloc(sizeof(U64) * 64);
    U64** line_table = move_table->line_tables;
    for (int i = 0; i < 64; i++)
    {
        rook_mask[i] = (line_table[east][i] & ~right_wall)|
                            (line_table[west][i] & ~left_wall)|
                            (line_table[south][i] & ~bottom_wall)|
                            (line_table[north][i] & ~top_wall);
    }
    move_table->rook_mask = rook_mask;
}
void Init_Bishop_Line_Table(move_table_t* move_table)
{
    U64* bishop_mask = (U64*) malloc(sizeof(U64) * 64);
    U64** line_table = move_table->line_tables;
    for(int i = 0; i < 64; i++)
    {
        bishop_mask[i] = ((line_table[northeast][i] |
                            line_table[northwest][i] |
                            line_table[southeast][i] |
                            line_table[southwest][i]) & ~border);
    }
    move_table->bishop_mask = bishop_mask;
}
U64 GetBlockersFromIndex(int index, U64 _mask){
    U64 blockers = ZERO;
    int bits = PopCount(_mask);
    U64 mask = _mask;
    //PrintBoard(mask);
    for (int i = 0; i < bits; i++)
    {
        U64 lsb = Get_LSB(mask);
        mask &= ~lsb;
        if ((index & (1 << i)) != 0)
        {
            blockers |= lsb;
        }
    }
    //PrintBoard(blockers);
    return blockers;
}

//Potentially, I will update this to use the PEXT instruction, would be much faster,
//but my laptop doesn't have bmi2
//also, using the bswap64 intrinsic, I could only store half the board.
    //ie only store the bottom half of the board moves, and or it with the
    //vertically mirrored board
void InitRookMagicTable(move_table_t* move_table)
{
    U64** rook_move_table = (U64**)malloc(sizeof(U64*) * 64);
    U64* rook_mask = move_table->rook_mask;
    for(int i = 0; i < 64; i++){
        rook_move_table[i] = malloc(sizeof(U64) * 4096);
    }
    // For all squares
    for (int square = 0; square < 64; square++)
    {
        // For all possible blockers for this square
        for (int blockerIndex = 0; blockerIndex < (ONE << rook_idx_bits[square]); blockerIndex++)
        {
            U64 blockers = GetBlockersFromIndex(blockerIndex, rook_mask[square]);
            rook_move_table[square][(blockers * rook_magics[square]) >> (64ull - rook_idx_bits[square])] =
                GetRookAttacksSlow(square, blockers, move_table);
        }
    }
    move_table->rook_table = rook_move_table;
}
void InitBishopMagicTable(move_table_t* move_table){
    U64** bishop_move_table = (U64**)malloc(sizeof(U64*)*64);
    U64* bishop_mask = move_table->bishop_mask;
    for(int i = 0; i < 64; i++){
        bishop_move_table[i] = malloc(sizeof(U64) * 1024);
    }
    // For all squares
    for (int square = 0; square < 64; square++)
    {
        // For all possible blockers for this square
        for (int blockerIndex = 0; blockerIndex < (ONE << bishop_idx_bits[square]); blockerIndex++)
        {
            U64 blockers = GetBlockersFromIndex(blockerIndex, bishop_mask[square]);
            bishop_move_table[square][(blockers * bishop_magics[square]) >> (64ull - bishop_idx_bits[square])] =
                GetBishopAttacksSlow(square, blockers, move_table);
        }
    }
    move_table->bishop_table = bishop_move_table;
}

U64 GetRookAttacksSlow(int pos, U64 blockers, move_table_t* move_table){
    U64 attacks = ZERO;
    U64** line_table = move_table->line_tables;
    // North
    attacks |= line_table[north][pos];
    if ((line_table[north][pos] & blockers) != 0)
    {
        //takes away the rest of the line after getting to the first piece
        int idx = Get_Idx(Get_LSB(line_table[north][pos] & blockers));
        attacks &= ~(line_table[north][idx]);
    }
    
    //South
    attacks |= line_table[south][pos];
    if ((line_table[south][pos] & blockers) != 0)
    {
        //takes away the rest of the line after getting to the first piece
        int idx = Get_Idx(Get_MSB(line_table[south][pos] & blockers));
        attacks &= ~(line_table[south][idx]);
    }

    // East
    attacks |= line_table[east][pos];
    if ((line_table[east][pos] & blockers) != 0)
    {
        //takes away the rest of the line after getting to the first piece
        int idx = Get_Idx(Get_LSB(line_table[east][pos] & blockers));
        attacks &= ~(line_table[east][idx]);
    }

    // West
    attacks |= line_table[west][pos];
    if ((line_table[west][pos] & blockers) != 0)
    {
        //takes away the rest of the line after getting to the first piece
        int idx = Get_Idx(Get_MSB(line_table[west][pos] & blockers));
        attacks &= ~(line_table[west][idx]);
    }

    return attacks;
}

U64 GetBishopAttacksSlow(int pos, U64 blockers, move_table_t* move_table){
    U64 attacks = ZERO;
    U64** line_table = move_table->line_tables;

    // North
    attacks |= line_table[northeast][pos];
    if ((line_table[northeast][pos] & blockers) != 0)
    {
        //takes away the rest of the line after getting to the first piece
        attacks &= ~(line_table[northeast][Get_Idx(Get_LSB(line_table[northeast][pos] & blockers))]);
    }

    //South
    attacks |= line_table[southeast][pos];
    if ((line_table[southeast][pos] & blockers) != 0)
    {
        //takes away the rest of the line after getting to the first piece
        attacks &= ~(line_table[southeast][Get_Idx(Get_MSB(line_table[southeast][pos] & blockers))]);
    }

    // East
    attacks |= line_table[northwest][pos];
    if ((line_table[northwest][pos] & blockers) != 0)
    {
        //takes away the rest of the line after getting to the first piece
        attacks &= ~(line_table[northwest][Get_Idx(Get_LSB(line_table[northwest][pos] & blockers))]);
    }

    // West
    attacks |= line_table[southwest][pos];
    if ((line_table[southwest][pos] & blockers) != 0)
    {
        //takes away the rest of the line after getting to the first piece
        attacks &= ~(line_table[southwest][Get_Idx(Get_MSB(line_table[southwest][pos] & blockers))]);
    }

    return attacks;
}