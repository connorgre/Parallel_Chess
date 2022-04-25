override CFLAGS := -Wall -Werror -std=gnu99 -pedantic -pthread -mavx2 $(CFLAGS)
override CC := gcc



all: evaluation.c engine.c movegen_fast.c trans_table.c perft.c bit_helper.c board.c move.c user_input.c movegen.c main.c parallel_perft.c
	$(CC) $(CFLAGS) -O0 -g evaluation.c engine.c movegen_fast.c parallel_perft.c trans_table.c perft.c bit_helper.c board.c move.c user_input.c movegen.c main.c -o main

opt1:  evaluation.c engine.c movegen_fast.c parallel_perft.c trans_table.c perft.c bit_helper.c board.c move.c user_input.c movegen.c main.c
	$(CC) $(CFLAGS) -O1 evaluation.c engine.c movegen_fast.c parallel_perft.c trans_table.c perft.c bit_helper.c board.c move.c user_input.c movegen.c main.c -o main

opt2:  evaluation.c engine.c movegen_fast.c parallel_perft.c trans_table.c perft.c bit_helper.c board.c move.c user_input.c movegen.c main.c
	$(CC) $(CFLAGS) -O2  evaluation.c engine.c movegen_fast.c parallel_perft.c trans_table.c perft.c bit_helper.c board.c move.c user_input.c movegen.c main.c -o main

opt3:  evaluation.c engine.c movegen_fast.c parallel_perft.c trans_table.c perft.c bit_helper.c board.c move.c user_input.c movegen.c main.c
	$(CC) $(CFLAGS) -O3  evaluation.c engine.c movegen_fast.c parallel_perft.c trans_table.c perft.c bit_helper.c board.c move.c user_input.c movegen.c main.c -o main
