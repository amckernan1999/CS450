/*
* CS 450: Sudoku Checker
* This project takes a 9x9 grid of numbers and verifies whether it is
* a valid sudoku puzzle. This is done by utilizing threads or
* forks in order to create a proccess to check each row, column or block.
*
* Created by: Michael Krent, Nicholas Uribe, and Andrew McKernan
* 10/2/2020
*/

#include <stdio.h>  
#include <unistd.h> 
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

/* These are the only two global variables allowed in your program */
static int verbose = 0;
static int use_fork = 0;


// This is a simple function to parse the --fork argument.
// It also supports --verbose, -v
void parse_args(int argc, char *argv[])
{
    int c;
    while (1)
    {
        static struct option long_options[] =
        {
            {"verbose", no_argument,       0, 'v'},
            {"fork",    no_argument,       0, 'f'},
            {0, 0, 0, 0}
        };
        int option_index = 0;
        c = getopt_long (argc, argv, "vf", long_options, &option_index);
        if (c == -1) break;

        switch (c)
        {
            case 'f':
                use_fork = 1;
                break;
            case 'v':
                verbose = 1;
                break;
            default:
                exit(1);
        }
    }
}

struct section {
  pthread_t thread_id;
  int (*sudoku)[9][9];
  int s;
};

/*
* Function: checkRow
* calculates whether a given row is valid by checking for  valid entries and no duplicates
* -----------------------------------------------
* returns a 0 if the row is valid, or a non-zero if the row is invalid
* sudoku[9][9]: a 2-D array containing the entire sudoku grid
* r: the row number that is checked
*/
int checkRow(int sudoku[9][9], int r) {
  int mask = 0x1ff;
  int gap = 1;
  int val = 0x1ff;
  for (int i = 0; i < 9; i = i + 1) {
    if (sudoku[r][i] > 0 && sudoku[r][i] < 10) {
      val = (mask ^ (gap << (sudoku[r][i]-1))) & val;
    }
    else {return -1;}
  }
  return val;
}

/*
* Function: checkCol
* calculates whether a given column is valid by checking for  valid entries and no duplicates
* -----------------------------------------------
* returns a 0 if the column is valid, or a non-zero if the column is invalid
* sudoku[9][9]: a 2-D array containing the entire sudoku grid
* c: the column number that is checked
*/
int checkCol(int sudoku[9][9], int c) {
  int mask = 0x1ff;
  int gap = 1;
  int val = 0x1ff;
  for (int i = 0; i < 9; i = i + 1) {
    if (sudoku[i][c] > 0 && sudoku[i][c] < 10) {
      val = (mask ^ (gap << (sudoku[i][c]-1))) & val;
    }
    else {return -1;}
  }
  return val;
}

/*
* Function: checkBlock
* calculates whether a given block is valid by checking for valid entries and no duplicates
* -----------------------------------------------
* returns a 0 if the block is valid, or a non-zero if the row is invalid
* sudoku[9][9]: a 2-D array containing the entire sudoku grid
* r: the row number the block begins at
* c: the column number the block begins at
*/
int checkBlock(int sudoku[9][9], int r, int c) {
  int mask = 0x1ff;
  int gap = 1;
  int val = 0x1ff;
  for (int i = 0; i < 3; i = i + 1) {
    for (int j = 0; j < 3; j = j + 1) {
      if (sudoku[i+r][j+c] > 0 && sudoku[i+r][j+c] < 10) {
	val = (mask ^ (gap << (sudoku[i+r][j+c]-1))) & val;
      }
      else {return -1;}
    }
  }
  return val;
}

/*
* Function: threadRow
* passed to pthread_create in order to create a process for each row operation. Invokes checkRow on each row.
* -----------------------------------------------
* returns a void star pointing to an int holding a 0 if the row is valid, and a non-zero if the row is invalid
* info: a struct holding the sudoku puzzle, the thread id, and row number.
*/
void * threadRow(void * info) {
  int val = 1;
  struct section *tinfo = info;
  if (checkRow(*tinfo->sudoku,tinfo->s)){
    printf("Row %d doesn't have the required values.\n", (tinfo->s)+1);
    val = 0;
    return (void *)val;
  }
  return (void *)val;
}

/*
* Function: threadCol
* passed to pthread_create in order to create a process for each column operation. Invokes checkCol on each column.
* -----------------------------------------------
* returns a void star pointing to an int holding a 0 if the column is valid, and a non-zero if the column is invalid
* info: a struct holding the sudoku puzzle, the thread id, and column number.
*/
void * threadCol(void * info) {
  int val = 1;
  struct section *tinfo = info;
  if (checkCol(*tinfo->sudoku,tinfo->s)){
    printf("Column %d doesn't have the required values.\n", (tinfo->s)+1);
    val = 0;
    return (void *) val;
  }
  return (void *) val;
}
/*
* Function: threadBlock
* passed to pthread_create in order to create a process for each block operation. Invokes checkBlock on each Block.
* -----------------------------------------------
* returns a void star pointing to an int holding a 0 if the block is valid, and a non-zero if the block is invalid
* info: a struct holding the sudoku puzzle, the thread id, and an int which can be manipulated to give row and column.
*/
void * threadBlock(void * info) {
  int val = 1;
  int * valptr = &val;
  struct section *tinfo = info;
  int r = (tinfo->s % 3) * 3;
  int c = (tinfo->s / 3) * 3;
  if (checkBlock(*tinfo->sudoku,r,c)){
    char row[7];
    char col[7];
    switch (r)
      {
      case 0:
	strcpy(row,"top");
	break;
      case 3:
	strcpy(row,"middle");
	break;
      case 6:
	strcpy(row,"bottom");
	break;
      }
    switch (c)
      {
      case 0:
	strcpy(col,"left");
	break;
      case 3:
	strcpy(col,"center");
	break;
      case 6:
	strcpy(col,"right");
	break;
      }
    printf("The %s %s subgrid doesn't have the required values.\n", row, col);
    val = 0;
    return (void *) val;
  }
  return (void *) val;
}
  
int main(int argc, char *argv[])
{
    parse_args(argc, argv);

    int input;
    int sudoku[9][9];
    // Reading input
    for (int i = 0; i<9; i = i+1) {
      for (int j = 0; j<9; j = j+1) {
	scanf("%d", &input);
	sudoku[i][j] = input;
      }
    }

    if (verbose && use_fork) {
        printf("We are forking child processes as workers.\n");
	for (int i = 0; i<9; i = i+1) {
	  for (int j = 0; j<9; j = j+1) {
	    if (j%3==0) {printf(" ");}
	    printf("%d ", sudoku[i][j]);
	  }
	  printf("\n");
	  if (i%3==2) {printf("\n");}
	}
    }
    else if (verbose) {
        printf("We are using worker threads.\n");
	// For testing purposes
	for (int i = 0; i<9; i = i+1) {
	  for (int j = 0; j<9; j = j+1) {
	    if (j%3==0) {printf(" ");}
	    printf("%d ", sudoku[i][j]);
	  }
	  printf("\n");
	  if (i%3==2) {printf("\n");}
	}
    }

    else if (use_fork) { //with forks
      int invalid = 0;
      int value;
      pid_t first = fork();
      pid_t second = fork(); 
    
	if (!first && !second) 
	{
	  pid_t a = fork();
	  pid_t b, c, d;
	  if (a){    //this if chunk is used for creating 9 forks which check each row
	    b = fork();
	    c = fork();
	    d = fork();
	  }
	  if (!a)
	    {
	      if (checkRow(sudoku, 0))
		{
		  invalid = 1;
		}
	    }
	  else if (b && c && d)
	    {
	      if (checkRow(sudoku, 1))
		{
		  invalid = 2;
		}
	    }
	  else if (b && c && !d)
	    {
	      if (checkRow(sudoku, 2))
		{
		  invalid = 3;
		}
	    }
	  else if (b && !c && d)
	    {
	      if (checkRow(sudoku, 3))
		{
		  invalid = 4;
		}
	    }
	  else if (b && !c && !d)
	    {
	      if (checkRow(sudoku, 4))
		{
		  invalid = 5;
		}
	    }
	  else if (!b && c && d)
	    {
	      if (checkRow(sudoku, 5))
		{
		  invalid = 6;
		}
	    }
	  else if (!b && c && !d)
	    {
	      if (checkRow(sudoku, 6))
		{
		  invalid = 7;
		}
	    }
	  else if (!b && !c && d)
	    {
	      if (checkRow(sudoku, 7))
		{
		  invalid = 8;
		}
	    }
	  else if (!b && !c && !d)
	    {
	      if (checkRow(sudoku, 8))
		{
		  invalid = 9;
		}
	    }
	  if (invalid)
	    {
	      printf("Row %d doesn't have the required values.\n", invalid);
	    }
	  while(waitpid(-1, &value, 0) != -1) {
	    if (WIFEXITED(value))
	      {
			value = WEXITSTATUS(value);
	      }
	    invalid = invalid + value;
	    
	  } //reap all children
	  exit(invalid);
	}
      else if (first && !second) 
	{
	  pid_t a = fork();
	  pid_t b, c, d;
	  
	  if (a){    //this if chunk is used for creating 9 additonal forks which check each column
	    b = fork();
	    c = fork();
	    d = fork();
	  }
	  if (!a)
	    {
	      if (checkCol(sudoku, 0))
		{
		  invalid = 1;
		}
	    }
	  else if (b && c && d)
	    {
	      if (checkCol(sudoku, 1))
		{
		  invalid = 2;
		}
	    }
	  else if (b && c && !d)
	    {
	      if (checkCol(sudoku, 2))
		{
		  invalid = 3;
		}
	    }
	  else if (b && !c && d)
	    {
	      if (checkCol(sudoku, 3))
		{
		  invalid = 4;
		}
	    }
	  else if (b && !c && !d)
	    {
	      if (checkCol(sudoku, 4))
		{
		  invalid = 5;
		}
	    }
	  else if (!b && c && d)
	    {
	      if (checkCol(sudoku, 5))
		{
		  invalid = 6;
		}
	    }
	  else if (!b && c && !d)
	    {
	      if (checkCol(sudoku, 6))
		{
		  invalid = 7;
		}
	    }
	  else if (!b && !c && d)
	    {
	      if (checkCol(sudoku, 7))
		{
		  invalid = 8;
		}
	    }
	  else if (!b && !c && !d)
	    {
	      if (checkCol(sudoku, 8))
		{
		  invalid = 9;
		}
	    }
	  if (invalid)
	    {
	      printf("Col %d doesn't have the required values.\n", invalid);
	    }
	  while(waitpid(-1, &value, 0) != -1) {
	    if (WIFEXITED(value))
	      {
			value = WEXITSTATUS(value);
	      }
	    invalid = invalid + value;
	  } //reap all children
	  exit(invalid);
	}
      else if (!first && second) 
	{
	  pid_t a = fork();
	  pid_t b, c, d;
	  
	  int ro;
	  int co;
	      
	  if (a){    //this if chunk is used for creating 9 additonal forks which check each block
	    b = fork();
	    c = fork();
	    d = fork();
	  }
	  if (!a)
	    {
	      if (checkBlock(sudoku, 0,0))
		{
		  invalid = 1;
		  ro = 0;
		  co = 0;
		}
	    }
	  else if (b && c && d)
	    {
	      if (checkBlock(sudoku, 3,0))
		{
		  invalid = 2;
		  ro = 3;
		  co = 0;
		}
	    }
	  else if (b && c && !d)
	    {
	      if (checkBlock(sudoku, 6,0))
		{
		  invalid = 3;
		  ro = 6;
		  co = 0;
		}
	    }
	  else if (b && !c && d)
	    {
	      if (checkBlock(sudoku, 0,3))
		{
		  invalid = 4;
		  ro = 0;
		  co = 3;
		}
	    }
	  else if (b && !c && !d)
	    {
	      if (checkBlock(sudoku, 3,3))
		{
		  invalid = 5;
		  ro = 3;
		  co = 3;
		}
	    }
	  else if (!b && c && d)
	    {
	      if (checkBlock(sudoku, 6,3))
		{
		  invalid = 6;
		  ro = 6;
		  co = 3;
		}
	    }
	  else if (!b && c && !d)
	    {
	      if (checkBlock(sudoku, 0,6))
		{
		  invalid = 7;
		  ro = 0;
		  co = 6;
		}
	    }
	  else if (!b && !c && d)
	    {
	      if (checkBlock(sudoku, 3,6))
		{
		  invalid = 8;
		  ro = 3;
		  co = 6;
		}
	    }
	  else if (!b && !c && !d)
	    {
	      if (checkBlock(sudoku, 6,6))
		{
		  invalid = 9;
		  ro = 6;
		  co = 6;
		}
	    }
	  if (invalid)
	    {
	      char row[7];
	      char col[7];
	      switch (ro)
		{
		case 0:
		  strcpy(row,"top");
		  break;
		case 3:
		  strcpy(row,"middle");
		  break;
		case 6:
		  strcpy(row,"bottom");
		  break;
		}
	      switch (co)
		{
		case 0:
		  strcpy(col,"left");
		  break;
		case 3:
		  strcpy(col,"center");
		  break;
		case 6:
		  strcpy(col,"right");
		  break;
		}
	      printf("The %s %s subgrid doesn't have the required values.\n", row, col);
	    }
	  while(waitpid(-1, &value, 0) != -1) {
	    if (WIFEXITED(value))
	      {
		value = WEXITSTATUS(value);
	      }
	    invalid = invalid + value;
	  } //reap all children
	  exit(invalid);
	}
	while(waitpid(-1, &value, 0) != -1) {
	  if (WIFEXITED(value))
	    {
	      value = WEXITSTATUS(value);
	    }
	  invalid = invalid + value;
	} //reap all children
	
	if (invalid){printf("The input is not a valid sudoku.\n");}
	else {printf("The input is a valid sudoku.\n");}
    }
    
    else { //with threads
      struct section tinfo[27];
      for (int i = 0; i < 9; i = i + 1) {
	for (int j = 0; j < 3; j = j + 1) {
	  tinfo[3*i+j].sudoku = &sudoku;
	  tinfo[3*i+j].s = i;
	}
      }
      for (int k = 0; k < 9; k = k + 1) {
	pthread_create(&tinfo[3*k].thread_id, NULL, &threadRow, &tinfo[3*k]);
	pthread_create(&tinfo[3*k+1].thread_id, NULL, &threadCol, &tinfo[3*k+1]);
	pthread_create(&tinfo[3*k+2].thread_id, NULL, &threadBlock, &tinfo[3*k+2]);
      }

      int valid = 1;
      int section;
      for (int t = 0; t < 27; t = t + 1) {
	pthread_join(tinfo[t].thread_id, (void **) &section);
	valid = valid & section;
      }
      
      if (valid){printf("The input is a valid sudoku.\n");}
      else {printf("The input is not a valid sudoku.\n");}
    }
    return 0;
}

