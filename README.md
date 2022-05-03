# Parallel_Chess
EC527 final project. 
Command line Chess engine

Commands: 

"perft [parallel] <color> <depth> [limited]"
  to time the exhaustive search (no move pruning)
  Does ~10M positions / second (more on some positions)
  Wouldn't go past depth 5 most of the time

"search <depth>"
  to use the actual engine, can hit depth 15 in 1-8 seconds in most positions
  Parallel version does not work yet
 
"reset <option>"
  can reset the board to different positions (specify number)
  If option="", the board is reset to the starting board
  if option="tt", then the transposition (hash) table is cleared
  
"print <option>"
  Will print the board
  Options for "black" and "white"
  
"move <move>"
  Will do the move specified (ie a4b5 moves the piece on a4 to b5)
  Castling is OO, OOO, oo, ooo
  
"q"
  Will exit the program and free up memory
