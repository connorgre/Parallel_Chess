# Parallel_Chess
EC527 final project. 
Command line Chess engine

Commands: 

"perft [parallel] <color> <depth> [limited]"
  to time the exhaustive search (no move pruning)
  Does ~10M positions / second (more on some positions)
  Wouldn't go past depth 5 most of the time

"search <depth>"
  to use the actual engine, can hit depth 15 in 3-8 seconds in most positions
  Parallel version does not work yet
 
"reset <option>"
  can reset the board to different positions (specify number)
  If option="", the board is reset to the starting board
  if option="tt", then the transposition (hash) table is cleared
  
"q"
  Will exit the program and free up memory
