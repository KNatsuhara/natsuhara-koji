/***** LAB3 base code *****/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <elf.h>

void pipeProcess(char *head, char *tail);
void runSimpleIORedirections(char *line, int env, char *cmd);

char gpath[128]; // hold token strings
char *arg[64];   // token string pointers
int n;           // number of token strings

char dpath[128]; // hold dir strings in PATH
char *dir[64];   // dir string pointers
int ndir;        // number of dirs

int tokenize(char *pathname) // YOU have done this in LAB2
{                            // YOU better know how to apply it from now on
  char *s;
  strcpy(gpath, pathname); // copy into global gpath[]
  s = strtok(gpath, " ");
  n = 0;
  while (s)
  {
    arg[n++] = s; // token string pointers
    s = strtok(0, " ");
  }
  arg[n] = 0; // arg[n] = NULL pointer
}

int tokenizeColon(char *pathname) // YOU have done this in LAB2
{                                 // YOU better know how to apply it from now on
  char *s;
  strcpy(dpath, pathname); // copy into global dpath[]
  s = strtok(dpath, ":");
  ndir = 0;
  while (s)
  {
    dir[ndir++] = s; // token string pointers
    s = strtok(0, ":");
  }
}

int main(int argc, char *argv[], char *env[])
{
  int i;
  int pid, status;
  char *cmd;
  char line[28];

  // The base code assume only ONE dir[0] -> "/bin"
  // YOU do the general case of many dirs from PATH !!!!
  //dir[0] = "/bin";
  char *home = getenv("HOME");
  printf("1: show HOME directory: HOME = %s\n", home);
  char *path = getenv("PATH");
  printf("2: show PATH: Path=%s\n", path);
  tokenizeColon(path);

  for (int i = 0; i < ndir; i++)
  {
    printf("dir[%d] = %s\n", i, dir[i]);
  }

  while (1)
  {
    printf("sh %d running\n", getpid());
    printf("enter a command line : ");
    fgets(line, 128, stdin);
    line[strlen(line) - 1] = 0;
    if (line[0] == 0)
      continue;

    tokenize(line);

    for (i = 0; i < n; i++)
    { // show token strings
      printf("arg[%d] = %s\n", i, arg[i]);
    }
    // getchar();

    cmd = arg[0]; // line = arg0 arg1 arg2 ...

    if (strcmp(cmd, "cd") == 0)
    {
      chdir(arg[1]);
      continue;
    }
    if (strcmp(cmd, "exit") == 0)
      exit(0);

    pid = fork(); //fork a child (to share the PIPE)

    // char head[128];
    // char tail[128];

    // strcat(head, arg[0]);
    // strcat(head, " ");
    // strcat(head, arg[1]);
    // printf("Head: %s\n", head);

    // arg[2] = 0;

    // strcat(tail, arg[3]);
    // strcat(tail, " ");
    // strcat(tail, arg[4]);
    // printf("Tail: %s\n", tail);

    if (pid)
    {
      printf("sh %d forked a child sh %d\n", getpid(), pid);
      printf("sh %d wait for child sh %d to terminate\n", getpid(), pid);
      pid = wait(&status);
      printf("ZOMBIE child=%d exitStatus=%x\n", pid, status);
      printf("main sh %d repeat loop\n", getpid());
    }
    else
    {
      printf("child sh %d running\n", getpid());

      Elf64_Ehdr header;
      int r = -1;
      for (int i = 0; i < ndir; i++)
      {
        if (line[0] == '/')
        {
          r = execve(line, arg, env);
        }
          strcpy(line, dir[i]);
          strcat(line, "/");
          strcat(line, cmd);
          FILE *file = fopen(line, "rb");
          if (file)
          {
            fread(&header, sizeof(header), 1, file);
          }
          if (memcmp(header.e_ident, ELFMAG, SELFMAG) == 0)
          {
            for (int i = 0; i < n; i++)
            {
              if (strcmp(arg[i], ">") == 0) // Check I/O direction
              {
                arg[i] = 0;
                close(1);
                int fd = open(arg[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644); //open filename for WRITE
                dup2(fd, 1);
              }
              else if (strcmp(arg[i], ">>") == 0)
              {
                arg[i] = 0;
                close(1);
                int fd = open(arg[i + 1], O_WRONLY | O_APPEND | O_CREAT, 0644); //open filename for WRITE
                dup2(fd, 1);
              }
              else if (strcmp(arg[i], "<") == 0)
              {
                arg[i] = 0;
                close(0);
                int fd = open(arg[i + 1], O_RDONLY); //open filename for READ
                dup2(fd, 0);
              }
            }
            r = execve(line, arg, env);
          }
          else
          {
            printf("i = %d\tcmd = %s\n", i, line);
          }
        }
        printf("execve failed r = %d\n", r);
        exit(1);
      }
    }
    return 0;
}

  /********************* YOU DO ***********************
1. I/O redirections:

Example: line = arg0 arg1 ... > argn-1

  check each arg[i]:
  if arg[i] = ">" {
     arg[i] = 0; // null terminated arg[ ] array 
     // do output redirection to arg[i+1] as in Page 131 of BOOK
  }
  Then execve() to change image

2. Pipes:

Single pipe   : cmd1 | cmd2 :  Chapter 3.10.3, 3.11.2

Multiple pipes: Chapter 3.11.2
****************************************************/
