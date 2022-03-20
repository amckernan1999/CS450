#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include "constants.h"
#include "parsetools.h"

// 
int split_cmd_words( char** all_words, int size, int all_redirects[MAX_LINE_WORDS + 1], char* list_to_populate[MAX_LINE_WORDS + 1][MAX_LINE_WORDS + 1],char* files[MAX_LINE_WORDS + 1][2])
{
  int length = 0;
  int pos = 0;
  int current_cmd;
  int options;
  int j;
  
  //list_to_populate[1][0] = "ls";
  //printf("%s",all_words);
  for( int i=0; pos < size; i++)
    {
      length++;
      //printf("\ni = %d\n", i);
      current_cmd = 1;
      options = 1;
      j = 0;
      while( current_cmd )
	{
	  int bump = 0;
	  int a = all_words[pos][0] == INPUT_REDIRECT;
	  int b = all_words[pos][0] == OUTPUT_REDIRECT;
	  int c = all_words[pos][0] == PIPE;
	  int d = all_words[pos][0] == '"';
	  if(d)
	    {
	      all_words[pos] = &all_words[pos][1];
	      int k = 1;
	      while(all_words[pos][k]!='"')
		{
		  if (all_words[pos][k]=='\0')
		    {
		      all_words[pos][k]=' ';
		      bump++;
		    }
		  //printf("%c",all_words[pos][k]);
		  k++;
		}
	      all_words[pos][k]='\0';
	      //printf("%s",all_words[pos]);
	    }
	  if( a || b || c)
	    {
	      //possibly store ending sequences
	      //printf("\nredirect char detected");			
	      //i++;
	      
	      if (b && all_words[pos][1] == OUTPUT_REDIRECT)
		b = b+1;
	      all_redirects[i] = all_redirects[i] + a + 2 * b + 8 * c;
	      if (a)
		{
		  files[i][0] = all_words[pos+1];
		}
	      if (b)
		{
		  files[i][1] = all_words[pos+1];
	        }
	      if (c)
		{
		  all_redirects[i+1] = 16;
		  current_cmd = 0;
		}
	      options = 0;
	    }
	  
	  else if (options)
	    {
	      list_to_populate[i][j] = all_words[pos];
	      pos = pos+bump;
	    }
	  //printf("%s",all_words[pos]);
	  
	  if( ++pos >= size)
	    {
	      //printf("\nExiting with i = %d, j = %d, pos = %d, length = %d\n", i, j, pos, length);
	      return length;
	      }	
	  j++;
	}
      
    }
  return length;
}


int main()
{
  // Buffer for reading one line of input
  char line[MAX_LINE_CHARS];
  // holds separated words based on whitespace
  //char* fN;
  //int redirectIn = 0;
  //int redirectOut = 0;
  //int pipe = 0;
  int loc = 0;
  
  while( fgets(line, MAX_LINE_CHARS, stdin) ) 
    {
      int redirect_list[MAX_LINE_WORDS + 1] = {0};
      char* line_words[MAX_LINE_WORDS + 1];
      char* cmd_list[MAX_LINE_WORDS + 1][MAX_LINE_WORDS + 1]={NULL};
      char* files[MAX_LINE_WORDS + 1][2];
      int num_words = split_cmd_line(line, line_words);
      int num_cmds = split_cmd_words(line_words, num_words, redirect_list, cmd_list, files);
      pid_t pidList[MAX_LINE_WORDS + 1];
      int pfds[2][2]; //used to store 2 pairs of fds that setting up a pipe returns
      int p = 1; //
      
      //printf("\nNum_cmds: %d\n", num_cmds);
      int redirectIn = 0;
      
      for(int i =0; i < num_cmds; i++)
	{
	  p = 1-p;
	  pid_t pid;
	  char* ifn;
	  int ifd;
	  char* fn;
	  int fd;
	  if (redirect_list[i] & 1)
            {
              ifn = files[i][0];
	      ifd = open(ifn, O_RDONLY);
	      if (ifd < 0)
		{
		  perror("");
		}
	    }
	  if (redirect_list[i] & 6)
	    {
	      fn = files[i][1];
	      if (redirect_list[i] & 2)
		fd = open(fn, O_WRONLY | O_CREAT|O_TRUNC, 0666);
	      else
		fd = open(fn, O_WRONLY|O_APPEND|O_CREAT, 0666);
	      if (fd < 0)
		{
		  perror("");
		}
	    }
	  if (redirect_list[i] & 8)
	    {
	      pipe(pfds[p]); //populates arrays with fds forming a pipe
	    }
	  pid = fork();
	  if (pid == 0) //if child
	    {
	      if (redirect_list[i] & 8)
		{
		  dup2(pfds[p][1], 1); //change stdout to point to pipe entrance
		  close(pfds[p][1]);
		  close(pfds[p][0]);
		}
	      if (redirect_list[i] & 16)
		{
		  dup2(pfds[1-p][0], 0);
		  close(pfds[1-p][0]);
		}
	      if (redirect_list[i] & 6)
                {
                  dup2(fd, 1); //change stdout to point to file
                  close(fd);
                }
	      if (redirect_list[i] & 1)
		{
		  dup2(ifd, 0); //change stdin to point to file
		  close(ifd);
		}
	      execvp(cmd_list[i][0], cmd_list[i]);
	    }
	  if (redirect_list[i] & 8)
	    {
	      close(pfds[p][1]);
	    }
	  if (redirect_list[i] & 16)
	    {
	      close(pfds[1-p][0]);
	    }
	  if (redirect_list[i] & 6)
	    {
	      close(fd);
	    }
	  if (redirect_list[i] & 1)
	    {
	      close(ifd);
	    }
	  pidList[i] = pid;
	  //waitpid(pid, NULL, 0);
	}
      for (int j = 0; j < num_cmds; j++)
	waitpid(pidList[j], NULL, 0);
    }
}


