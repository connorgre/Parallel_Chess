override CFLAGS := -Wall -Werror -std=gnu99 -pedantic -pthread -g $(CFLAGS)
override CC := gcc



all: trans_table.c engine.c bit_helper.c board.c move.c user_input.c movegen.c main.c parallel_engine.c
	$(CC) $(CFLAGS) -O0 parallel_engine.c trans_table.c engine.c bit_helper.c board.c move.c user_input.c movegen.c main.c -o main

opt1:  parallel_engine.c trans_table.c engine.c bit_helper.c board.c move.c user_input.c movegen.c main.c
	$(CC) $(CFLAGS) -O1 parallel_engine.c trans_table.c engine.c bit_helper.c board.c move.c user_input.c movegen.c main.c -o main

opt2:  parallel_engine.c trans_table.c engine.c bit_helper.c board.c move.c user_input.c movegen.c main.c
	$(CC) $(CFLAGS) -O2  parallel_engine.c trans_table.c engine.c bit_helper.c board.c move.c user_input.c movegen.c main.c -o main

opt3:  parallel_engine.c trans_table.c engine.c bit_helper.c board.c move.c user_input.c movegen.c main.c
	$(CC) $(CFLAGS) -O3  parallel_engine.c trans_table.c engine.c bit_helper.c board.c move.c user_input.c movegen.c main.c -o main
