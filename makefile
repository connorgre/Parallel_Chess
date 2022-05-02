override CFLAGS := -Wall -Werror -std=gnu99 -pedantic -pthread -mavx2 -lm $(CFLAGS)
override CC := gcc



all: Parallel_Engine.c trans_table.c Engine_Movegen.c move_tables.c evaluation.c engine.c movegen_fast.c perft_trans_table.c perft.c bit_helper.c board.c move.c user_input.c movegen.c main.c parallel_perft.c
	$(CC) $(CFLAGS) -O0 -g Parallel_Engine.c trans_table.c Engine_Movegen.c move_tables.c evaluation.c engine.c movegen_fast.c parallel_perft.c perft_trans_table.c perft.c bit_helper.c board.c move.c user_input.c movegen.c main.c -o main

opt1:  Parallel_Engine.c trans_table.c Engine_Movegen.c move_tables.c evaluation.c engine.c movegen_fast.c parallel_perft.c perft_trans_table.c perft.c bit_helper.c board.c move.c user_input.c movegen.c main.c
	$(CC) $(CFLAGS) -O1 Parallel_Engine.c trans_table.c Engine_Movegen.c move_tables.c evaluation.c engine.c movegen_fast.c parallel_perft.c perft_trans_table.c perft.c bit_helper.c board.c move.c user_input.c movegen.c main.c -o main

opt2:  Parallel_Engine.c trans_table.c Engine_Movegen.c move_tables.c evaluation.c engine.c movegen_fast.c parallel_perft.c perft_trans_table.c perft.c bit_helper.c board.c move.c user_input.c movegen.c main.c
	$(CC) $(CFLAGS) -O2  Parallel_Engine.c trans_table.c Engine_Movegen.c move_tables.c evaluation.c engine.c movegen_fast.c parallel_perft.c perft_trans_table.c perft.c bit_helper.c board.c move.c user_input.c movegen.c main.c -o main

opt3:  Parallel_Engine.c trans_table.c Engine_Movegen.c move_tables.c evaluation.c engine.c movegen_fast.c parallel_perft.c perft_trans_table.c perft.c bit_helper.c board.c move.c user_input.c movegen.c main.c
	$(CC) $(CFLAGS) -O3  Parallel_Engine.c trans_table.c Engine_Movegen.c move_tables.c evaluation.c engine.c movegen_fast.c parallel_perft.c perft_trans_table.c perft.c bit_helper.c board.c move.c user_input.c movegen.c main.c -o main
