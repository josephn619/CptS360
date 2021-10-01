/***** LAB3 base code *****/ 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h> // __environ

char gpath[128];    // hold token strings 
char *arg[64];      // token string pointers
int  n;             // number of token strings

char dpath[128];    // hold dir strings in PATH
char *dir[64];      // dir string pointers
int  ndir;          // number of dirs   

char *head,*tail;

int pd[2], lpd[2], fd[2];

int tokenize(char *pathname) // YOU have done this in LAB2
{                            // YOU better know how to apply it from now on
  strcpy(gpath, pathname);   // copy into global gpath[]
  char *s = strtok(pathname, " ");    
  n = 0;
  while(s != NULL)
  {
    arg[n++] = s;           // token string pointers   
    s = strtok(NULL, " ");
  }
  // arg[n] = NULL;                // arg[n] = NULL pointer 
}

int hasPipe(char *line)
{
  for(int i = strlen(line)-1; i > 0 ; i--)
  {
    if(line[i] == '|' )
    {
      line[i] = 0;
      tail = line + i + 1;
      head = line;
      return 1;
    }
  }
  head = line;
  tail = NULL;
  return 0;
}

void ioRedirect()
{
  for (int i = 0; i < n; i++)
  {
    if (!strcmp(arg[i], ">"))
    {
      arg[i] = NULL;
      close(1);
      int fd = open(arg[i + 1], O_WRONLY | O_CREAT);
      dup2(fd, 1);
    }
    else if (!strcmp(arg[i], ">>"))
    {
      arg[i] = NULL;
      close(1);
      int fd = open(arg[i + 1], O_WRONLY | O_CREAT | O_APPEND);
      dup2(fd, 1);
    }
    else if (!strcmp(arg[i], "<"))
    {
      arg[i] = NULL;
      close(0);
      int fd = open(arg[i + 1], O_RDONLY);
      dup2(fd, 0);
    }
  }
}

int processCommand(char *line)
{
  tokenize(line);
  ioRedirect();
  int r;
  char cmd[128];
  if(arg[0][0] == '.' && arg[0][1] =='/')
  {
    char temp[128];
    getcwd(temp, 128);
    strcat(temp, "/");
    strcat(temp, arg[0]);
    r = execve(temp,arg, __environ);
  }
  for (int i = 0; i < ndir; i++)
  {
    strcpy(cmd, dir[i]);
    strcat(cmd, "/");
    strcat(cmd, arg[0]);
    r = execve(cmd, arg, __environ);
  }
  printf("execve failed r = %d\n", r);
  exit(1);
}

int processLine(char *line, int *pd)
{
  if (pd)
  {
    close(pd[0]);
    dup2(pd[1], 1);
    close(pd[1]);
  }
  int success = hasPipe(line);
  if(success)
  {
    pipe(lpd);
    int pid = fork();
    if(pid){
      close(lpd[1]);
      dup2(lpd[0], 0);
      close(lpd[0]);
      processCommand(tail);
    }
    else
      processLine(head, lpd);
  }
  else
    processCommand(line);
}

int main(int argc, char *argv[ ], char *env[ ])
{
  int pid, status;
  char line[28];

  fd[0] = dup(stdout);
  fd[1] = dup(stdin);

  // The base code assume only ONE dir[0] -> "/bin"
  // YOU do the general case of many dirs from PATH !!!!

  for (int i = 0; env[i]; i++)
  {
    if (!strncmp(env[i], "HOME", 4))
      printf("1. Home Directory: \n%s\n",env[i]);

    if (!strncmp(env[i], "PATH",4))
    {
      printf("2. show PATH: \n%s\n", env[i]);

      printf("3. decompose PATH into dir strings:\n");
      strcpy(dpath, &env[i][5]);

      printf("%s", dpath);
      char *s = strtok(dpath, ":");

      for (ndir = 0; s; ndir++)
      {
        dir[ndir] = s;
        s = strtok(NULL, ":");
      }

      for (int i = 0; i < ndir; i++)
        printf("dir[%d]. %s\n", i, dir[i]);

      break;
    }
  }
  
  printf("*********** kcsh processing loop **********\n");

  while(1)
  {
    printf("sh %d running\n", getpid());
    printf("enter a command line : ");
    fgets(line, 128, stdin);
    line[strlen(line) - 1] = NULL; 
    if (line[0]==0)
      continue;

    printf("line = %s\n", line);
    tokenize(line);    


    for (int i=0; i<n; i++)  
        printf("arg[%d] = %s\n", i, arg[i]);
    // getchar();
    
    char *cmd = arg[0];         // line = arg0 arg1 arg2 ... 

    if (!strcmp(cmd, "cd"))
    {
      chdir(arg[1]);
      continue;
    }
    if (!strcmp(cmd, "exit"))
      exit(0);
    
    pid = fork();
     
    if (pid)
    {
      // printf("sh %d forked a child sh %d\n", getpid(), pid);
      printf("sh %d wait for child sh %d to terminate\n", getpid(), pid);
      pid = wait(&status);
      printf("ZOMBIE child=%d exitStatus=%x\n", pid, status); 
      //printf("main sh %d repeat loop\n", getpid());
    }
    else
    {
      printf("child sh %d running\n", getpid());
      
     //  make a cmd line = dir[0]/cmd for execve()
     //  strcpy(line, dir[0]); 
     //  strcat(line, "/"); 
     //  strcat(line, cmd);
     //  printf("line = %s\n", line);
      
     processLine(gpath,0);       
    }
  }
}

/********************* YOU DO ***********************
1. I/O redirections:

Example: line = arg0 arg1 ... > argn-1

  check each arg[i]:
  if arg[i] == ">" {
     arg[i] = 0; // null terminated arg[ ] array 
     // do output redirection to arg[i+1] as in Page 131 of BOOK
  }
  Then execve() to change image


2. Pipes:

Single pipe   : cmd1 | cmd2 :  Chapter 3.10.3, 3.11.2

Multiple pipes: Chapter 3.11.2
****************************************************/

    
