
#include "move.h"
#include "movegen.h"
#include "bit_helper.h"

//#define ERROR_CHECK

//with this implementation I will need figure out what piece I am attacking
//in the move generator.  This should be ok, just shifts the location of the check
void Do_Move(Board_Data_t* board_data, move_t* move){
    //cas_mask_mask - parts of the mask we want to affect;
    board_data->to_move = ~board_data->to_move & 1;
    board_data->zob_key ^= board_data->zob_array[64][0];
    if(board_data->ep_tile){
        int ep_idx = Get_Idx((board_data->ep_tile >> (2*8)) | (board_data->ep_tile >> (5*8)));
    #ifdef ERROR_CHECK
        if(ep_idx < 0 || ep_idx > 7) printf("ep_idx error: %d\n", ep_idx);
    #endif
        board_data->zob_key ^= board_data->zob_array[65][ep_idx];
    }
    if(move->flags == 0){

        byte cas_change;
        U64 new_ep;
        //this removes the bits from the castle mask if we are moving a piece involved in castling
        cas_change = 0;
        cas_change |= Get_White_Castle_Change(board_data, move->from, move->to);
        cas_change |= Get_Black_Castle_Change(board_data, move->from, move->to);
        new_ep = Get_Ep_Tile(board_data, move->from, move->to);

        //this shouldn't change on most iterations, so the branch penalty shouldn't
        //be too bad
        if(cas_change & 0x01 & board_data->cas_mask)   board_data->zob_key ^= board_data->zob_array[64][WK_CAS_ZOB];
        if(cas_change & 0x08 & board_data->cas_mask)   board_data->zob_key ^= board_data->zob_array[64][WQ_CAS_ZOB];
        if(cas_change & 0x10 & board_data->cas_mask)   board_data->zob_key ^= board_data->zob_array[64][BK_CAS_ZOB];
        if(cas_change & 0x80 & board_data->cas_mask)   board_data->zob_key ^= board_data->zob_array[64][BQ_CAS_ZOB];
        int ep_idx = 0;
        if(new_ep){
            ep_idx = Get_Idx((new_ep >> (2*8)) | (new_ep >> (5*8)));
            board_data->zob_key ^= board_data->zob_array[65][ep_idx];
        }

        int from_pos = Get_Idx(move->from);
        int to_pos = Get_Idx(move->to);
        #ifdef ERROR_CHECK
            if(ep_idx < 0 || ep_idx > 7) printf("ep_idx error: %d\n",ep_idx);
            if(from_pos < 0 || from_pos > 63) printf("from_pos error: %d\n",from_pos);
            if(to_pos < 0 || to_pos > 63) printf("to_pos error: %d\n", to_pos);
        #endif
        board_data->zob_key ^= board_data->zob_array[from_pos][move->from_type];
        board_data->zob_key ^= board_data->zob_array[to_pos][move->from_type];
        board_data->zob_key ^= board_data->zob_array[to_pos][move->to_type];

        board_data->pieces[(int)move->from_type] ^= (move->from | move->to);
        board_data->pieces[(int)move->to_type] ^= move->to;

        board_data->team_tiles[(int)move->from_type/6] ^= (move->from | move->to);
        board_data->team_tiles[(int)move->to_type/6] ^= move->to;
        board_data->occ = board_data->team_tiles[0] |  board_data->team_tiles[1];
        
        board_data->cas_mask &= ~cas_change;
        board_data->ep_tile = new_ep;
        return;
    } else if(move->flags == 0xFF){ //en passant
        board_data->pieces[(int)move->from_type] ^= (move->from | move->to);
        board_data->team_tiles[(int)move->from_type/6] ^= (move->from | move->to);

        int enemy_type = W_PAWN;
        U64 ep_remove = UP(board_data->ep_tile);
        if(move->from_type == W_PAWN) {
            enemy_type = B_PAWN;
            ep_remove = DOWN(board_data->ep_tile);
        }
        board_data->pieces[enemy_type] &= ~ep_remove;
        board_data->team_tiles[enemy_type/6] &= ~ep_remove;
        board_data->occ = board_data->team_tiles[0] | board_data->team_tiles[1];

        int from_pos = Get_Idx(move->from);
        int to_pos = Get_Idx(move->to);
        int remove_pos = Get_Idx(ep_remove);
        #ifdef ERROR_CHECK
            if(remove_pos != to_pos-8 && remove_pos != to_pos+8) 
                printf("remove_idx error: %d\n", remove_pos);
            if(from_pos < 0 || from_pos > 63) printf("from_pos error: %d\n", from_pos);
            if(to_pos < 0 || to_pos > 63) printf("to_pos error: %d\n",to_pos);
        #endif
        board_data->zob_key ^= board_data->zob_array[from_pos][move->from_type];
        board_data->zob_key ^= board_data->zob_array[to_pos][move->from_type];
        board_data->zob_key ^= board_data->zob_array[remove_pos][enemy_type];
        board_data->ep_tile = ZERO;
        return;
    } else if(move->flags & 0x0F){  //promotion
        byte cas_change = Get_White_Castle_Change(board_data, move->from, move->to);
        cas_change |= Get_Black_Castle_Change(board_data, move->from, move->to);

        if(cas_change & 0x01 & board_data->cas_mask)   board_data->zob_key ^= board_data->zob_array[64][WK_CAS_ZOB];
        if(cas_change & 0x08 & board_data->cas_mask)   board_data->zob_key ^= board_data->zob_array[64][WQ_CAS_ZOB];
        if(cas_change & 0x10 & board_data->cas_mask)   board_data->zob_key ^= board_data->zob_array[64][BK_CAS_ZOB];
        if(cas_change & 0x80 & board_data->cas_mask)   board_data->zob_key ^= board_data->zob_array[64][BQ_CAS_ZOB];
        board_data->cas_mask &= ~cas_change;

        int from_pos = Get_Idx(move->from);
        int to_pos = Get_Idx(move->to);

        #ifdef ERROR_CHECK
            if(from_pos < 0 || from_pos > 63) printf("from_pos error: %d\n", from_pos);
            if(to_pos < 0 || to_pos > 63) printf("to_pos error: %d\n",to_pos);
            if(move->flags/6 != move->from_type/6) printf("promotion flag is off\n");
        #endif
        board_data->zob_key ^= board_data->zob_array[from_pos][move->from_type];
        board_data->zob_key ^= board_data->zob_array[to_pos][move->flags];
        board_data->zob_key ^= board_data->zob_array[to_pos][move->to_type];

        board_data->ep_tile = ZERO;
        board_data->pieces[(int)move->from_type] ^= move->from;
        board_data->pieces[(int)move->flags] ^= move->to;
        board_data->pieces[(int)move->to_type] ^= move->to;

        board_data->team_tiles[(int)move->from_type/6] ^= (move->from | move->to);
        board_data->team_tiles[(int)move->to_type/6] ^= move->to;
        board_data->occ = board_data->team_tiles[0] |  board_data->team_tiles[1];
        return;
    }else{                         //castle
        board_data->ep_tile = ZERO;
        switch(move->flags){
            case (0x10):
                White_KingSide_Castle(board_data);
                board_data->cas_mask &= 0xF0;
                break;
            case(0x20):
                White_QueenSide_Castle(board_data);
                board_data->cas_mask &= 0xF0;
                break;
            case(0x40): 
                Black_KingSide_Castle(board_data);
                board_data->cas_mask &= 0x0F;
                break;
            case(0x80):
                Black_QueenSide_Castle(board_data);
                board_data->cas_mask &= 0x0F;
                break;
            default:
                printf("Error - invalid castle flag\n");
                break;
        }
    }
    return;
}



char* String_From_Move(move_t move){
    char* pos_str = (char*)malloc(sizeof(char)*5);

    pos_str[0] = GetX(move.from) + 'a';
    pos_str[1] = 7 - GetY(move.from) + '1';
    pos_str[2] = GetX(move.to) + 'a';
    pos_str[3] = 7 - GetY(move.to) + '1';
    pos_str[4] = '\0';

    return pos_str;
}

byte Get_White_Castle_Change(Board_Data_t* board_data, U64 from, U64 to){
    byte wcmask_change = ((byte)((board_data->pieces[W_KING] & (from | to) & (ONE << 4)) >> 4));
    wcmask_change |= (wcmask_change << 3);
    wcmask_change |= (byte)(((board_data->pieces[W_ROOK] & (from | to)) & (ONE << 0)) << (3));
    wcmask_change |= (byte)(((board_data->pieces[W_ROOK] & (from | to)) & (ONE << 7)) >> (7));
    return wcmask_change;
}

byte Get_Black_Castle_Change(Board_Data_t* board_data, U64 from, U64 to){
    byte bcmask_change = ((byte)((board_data->pieces[B_KING] & (from | to) & (ONE << (4+56))) >> (4+56-4)));
    bcmask_change |= (bcmask_change << 3);
    bcmask_change |= (byte)(((board_data->pieces[B_ROOK] & (from | to)) & (ONE << (0+56))) >> (56)) << 7;
    bcmask_change |= (byte)(((board_data->pieces[B_ROOK] & (from | to)) & (ONE << (7+56))) >> (56)) >> 3;
    return bcmask_change;
}

U64 Get_Ep_Tile(Board_Data_t* board_data, U64 from, U64 to){
    //if the moving piece is a pawn, and we are pushing it to, then the tile behind the second push gets set
    U64 w_ep = (((board_data->pieces[W_PAWN] & from) << 16) & to) >> 8;
    U64 b_ep = (((board_data->pieces[B_PAWN] & from) >> 16) & to) << 8;
    return w_ep | b_ep;
}

//putting each castle in a separate function to make the move simpler.
//These could probably be inlined.
void White_KingSide_Castle(Board_Data_t* board_data){
    board_data->pieces[W_KING] = WKC_LAND;
    board_data->pieces[W_ROOK] ^= ((LEFT(WKC_LAND)) | (ONE << 7));
    board_data->team_tiles[WHITE] ^= ((WKC_LAND) | (WK_START) | (LEFT(WKC_LAND)) | (ONE << 7));
    board_data->occ ^= ((WKC_LAND) | (WK_START) | (LEFT(WKC_LAND)) | (ONE << 7));

    board_data->zob_key ^= board_data->zob_array[64][WK_CAS_ZOB];
    if(board_data->cas_mask & 0x08) board_data->zob_key ^= board_data->zob_array[64][WQ_CAS_ZOB];
    int rook_from = 7;
    int rook_to = 5;
    int king_from = 4;
    int king_to = 6;
    board_data->zob_key ^= board_data->zob_array[rook_from][W_ROOK];
    board_data->zob_key ^= board_data->zob_array[rook_to][W_ROOK];
    board_data->zob_key ^= board_data->zob_array[king_from][W_KING];
    board_data->zob_key ^= board_data->zob_array[king_to][W_KING];
    
}
void White_QueenSide_Castle(Board_Data_t* board_data){
    board_data->pieces[W_KING] = WQC_LAND;
    board_data->pieces[W_ROOK] ^= ((RIGHT(WQC_LAND)) | (ONE << 0));
    board_data->team_tiles[WHITE] ^= ((WQC_LAND) | (WK_START) | (RIGHT(WQC_LAND)) | (ONE << 0));
    board_data->occ ^= ((WQC_LAND) | (WK_START) | (RIGHT(WQC_LAND)) | (ONE << 0));

    if(board_data->cas_mask & 0x01) board_data->zob_key ^= board_data->zob_array[64][WK_CAS_ZOB];
    board_data->zob_key ^= board_data->zob_array[64][WQ_CAS_ZOB];
    int rook_from = 0;
    int rook_to = 3;
    int king_from = 4;
    int king_to = 2;
    board_data->zob_key ^= board_data->zob_array[rook_from][W_ROOK];
    board_data->zob_key ^= board_data->zob_array[rook_to][W_ROOK];
    board_data->zob_key ^= board_data->zob_array[king_from][W_KING];
    board_data->zob_key ^= board_data->zob_array[king_to][W_KING];
}
void Black_KingSide_Castle(Board_Data_t* board_data){
    board_data->pieces[B_KING] = BKC_LAND;
    board_data->pieces[B_ROOK] ^= ((LEFT(BKC_LAND)) | (ONE << (7+56)));
    board_data->team_tiles[BLACK] ^= ((BKC_LAND) | (BK_START) | (LEFT(BKC_LAND)) | (ONE << (7+56)));
    board_data->occ ^= ((BKC_LAND) | (BK_START) | (LEFT(BKC_LAND)) | (ONE << (7+56)));

    board_data->zob_key ^= board_data->zob_array[64][BK_CAS_ZOB];
    if(board_data->cas_mask & 0x80) board_data->zob_key ^= board_data->zob_array[64][BQ_CAS_ZOB];
    int rook_from = 7+56;
    int rook_to = 5+56;
    int king_from = 4+56;
    int king_to = 6+56;
    board_data->zob_key ^= board_data->zob_array[rook_from][B_ROOK];
    board_data->zob_key ^= board_data->zob_array[rook_to][B_ROOK];
    board_data->zob_key ^= board_data->zob_array[king_from][B_KING];
    board_data->zob_key ^= board_data->zob_array[king_to][B_KING];
}
void Black_QueenSide_Castle(Board_Data_t* board_data){
    board_data->pieces[B_KING] = BQC_LAND;
    board_data->pieces[B_ROOK] ^= ((RIGHT(BQC_LAND)) | (ONE << (56)));
    board_data->team_tiles[BLACK] ^= ((BQC_LAND) | (BK_START) | (RIGHT(BQC_LAND)) | (ONE << (56)));
    board_data->occ ^= ((BQC_LAND) | (BK_START) | (RIGHT(BQC_LAND)) | (ONE << (56)));

    if(board_data->cas_mask & 0x10) board_data->zob_key ^= board_data->zob_array[64][BK_CAS_ZOB];
    board_data->zob_key ^= board_data->zob_array[64][BQ_CAS_ZOB];
    int rook_from = 0+56;
    int rook_to = 3+56;
    int king_from = 4+56;
    int king_to = 2+56;
    board_data->zob_key ^= board_data->zob_array[rook_from][B_ROOK];
    board_data->zob_key ^= board_data->zob_array[rook_to][B_ROOK];
    board_data->zob_key ^= board_data->zob_array[king_from][B_KING];
    board_data->zob_key ^= board_data->zob_array[king_to][B_KING];
}
