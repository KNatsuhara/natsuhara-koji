#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h> // for dirname()/basename()
#include <time.h>

#define MAX 256
#define PORT 1234

char line[MAX], ans[MAX];
int n;
char saveLine[MAX];

struct sockaddr_in saddr;
int sfd;

char *arg[56];
char parsedLine[256];

int tokenize(char *inputLine);
int ls_file(char *fname);
int ls_dir(char *dname);
void print_Command_Menu();

int main(int argc, char *argv[], char *env[])
{
    char cwd[128];
    char *command;
    char *pathname;
    int n;
    char how[64];
    int i;

    printf("1. create a socket\n");
    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0)
    {
        printf("socket creation failed\n");
        exit(0);
    }

    printf("2. fill in server IP and port number\n");
    bzero(&saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    saddr.sin_port = htons(PORT);

    printf("3. connect to server\n");
    if (connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) != 0)
    {
        printf("connection with the server failed...\n");
        exit(0);
    }

    printf("********  processing loop  *********\n");
    while (1)
    {
        print_Command_Menu();
        printf("input a line : ");
        bzero(line, MAX);        // zero out line[ ]
        fgets(line, MAX, stdin); // get a line (end with \n) from stdin

        line[strlen(line) - 1] = 0; // kill \n at end
        if (line[0] == 0)           // exit if NULL line
            exit(0);

        getcwd(cwd, 128); // Gets the CWD of the client

        strcpy(saveLine, line);
        tokenize(saveLine);

        command = arg[0];  // Gets the cmd of the line
        pathname = arg[1]; // Gets the pathname of the line

        printf("command=%s pathname=%s\n", command, pathname);

        if (strcmp(command, "lcd") == 0)
        {
            chdir(pathname);
        }
        else if (strcmp(command, "lmkdir") == 0)
        {
            mkdir(pathname, 0766);
        }
        else if (strcmp(command, "lrmdir") == 0)
        {
            rmdir(pathname);
        }
        else if (strcmp(command, "lrm") == 0)
        {
            remove(pathname);
        }
        else if (strcmp(command, "lpwd") == 0)
        {
            printf("PWD: %s\n", cwd);
        }
        else if (strcmp(command, "lls") == 0)
        {
            struct stat mystat, *sp = &mystat;
            int r;
            char *filename, path[1024], lcwd[256];
            filename = "./"; // default to lcwd

            if (arg[1] == NULL)
            {
                arg[1] = "";
            }

            if (strcmp(arg[1], "") != 0)
            {
                printf("arg[1]=%s\n", arg[1]);
                filename = arg[1]; // if specified a filename
            }

            if (r = lstat(filename, sp) < 0)
            {
                printf("no such file %s\n", filename);
                exit(1);
            }
            strcpy(path, filename);

            if (path[0] != '/') // filename is relative : get lcwd path
            {
                getcwd(lcwd, 256);
                strcpy(path, lcwd);
                strcat(path, "/");
                strcat(path, filename);
            }

            if (S_ISDIR(sp->st_mode))
                ls_dir(path);
            else
                ls_file(path);
        }
        else if (strcmp(command, "lcat") == 0)
        {
            FILE *fptr;

            char c;

            fptr = fopen(pathname, "r");

            if (fptr == NULL)
            {
                printf("file does not exist\n");
                return -1;
            }

            c = fgetc(fptr);
            while (c != EOF)
            {
                printf("%c", c); // Prints out each character in the file
                c = fgetc(fptr);
            }

            fclose(fptr);
        }
        else if (strcmp(command, "lexit") == 0)
        {
            exit(0);
        }
        else if (strcmp(command, "ls") == 0)
        {
            n = write(sfd, line, MAX);
            n = read(sfd, ans, MAX);
            while (strcmp(ans, "endofls") != 0)
            {
                printf("%s\n", ans);
                n = read(sfd, ans, MAX);
            }
            n = read(sfd, ans, MAX);
        }
        else if (strcmp(command, "get") == 0)
        {
            char buf[MAX];
            n = write(sfd, line, MAX); // Tell server to run the get command
            int gd = 0;

            n = read(sfd, ans, MAX); // Reads the file size from server
            int size = (int) strtol(ans, (char **)NULL, 10); // Converts file size to an integer

            gd = open(arg[1], O_WRONLY | O_CREAT, 0644); // Opens a copy of a file in client

            if (gd == -1)
                printf("<P>Permission problems<P>");


            printf("Expecting %8d bytes from server\n", size);

            n = 0;

            while (n < size)
            {
                if (n + MAX > size)
                {
                    int rembytes = size - n;
                    n += read(sfd, ans, rembytes); // Reads a line from file
                    write(gd, ans, rembytes); // Writing file to line
                }
                else
                {
                    n += read(sfd, ans, MAX); // Reads a line from file
                    write(gd, ans, MAX); // Writing file to line
                }
            }
            close(gd);
            n = read(sfd, ans, MAX);
        }
        else if (strcmp(command, "put") == 0) // Puts a file from client to server
        {
            n = write(sfd, line, MAX); // Tell server to run put command
            char buf[MAX];
            int fd = 0;
            int n = 0;
            fd = open(arg[1], O_RDONLY);

            if (fd == -1) // Check if file opened
            {
                printf("File does not exist\n");
                return 0;
            }
            else // If file exists
            {
                struct stat fstat, *sp;
                sp = &fstat;

                int r = lstat(arg[1], &fstat); // Creates stat for file
                int sz = sp->st_size;

                sprintf(line, "%8d", sz);  // Adds file size to line
                n = write(sfd, line, MAX); // Sends file size to server

                while ((n = read(fd, buf, MAX)) != 0) // Reads a fd and adds it to buf
                {
                    n = write(sfd, buf, n); // Sending line (containing file) to server with n bytes
                }
            }
            close(fd); // Closing file
        }
        else // Send command to the server
        {
            // Send ENTIRE line to server
            n = write(sfd, line, MAX);
            // printf("client: wrote n=%d bytes; line=(%s)\n", n, line);

            // Read a line from sock and show it
            n = read(sfd, ans, MAX);
            // printf("client: read  n=%d bytes; echo=(%s)\n", n, ans);
            printf("%s\n", ans);
        }
    }
}

int tokenize(char *inputLine) // YOU have done this in LAB2
{                             // YOU better know how to apply it from now on
    char *s;
    strcpy(parsedLine, inputLine); // copy into global parsedLine[]
    s = strtok(parsedLine, " ");
    n = 0;
    while (s)
    {
        arg[n++] = s; // token string pointers
        s = strtok(0, " ");
    }
    arg[n] = 0; // arg[n] = NULL pointer
}

int ls_file(char *fname)
{
    char *t1 = "xwrxwrxwr-------";
    char *t2 = "----------------";
    struct stat fstat, *sp;
    int r, i;
    char ftime[64];
    sp = &fstat;
    if ((r = lstat(fname, &fstat)) < 0)
    {
        printf("can’t stat %s\n", fname);
        exit(1);
    }
    if ((sp->st_mode & 0xF000) == 0x8000) // if (S_ISREG())
        printf("-");
    if ((sp->st_mode & 0xF000) == 0x4000) // if (S_ISDIR())
        printf("d");
    if ((sp->st_mode & 0xF000) == 0xA000) // if (S_ISLNK())
        printf("l");
    for (i = 8; i >= 0; i--)
    {
        if (sp->st_mode & (1 << i)) // print r|w|x
            printf("%c", t1[i]);
        else
            printf("%c", t2[i]); // or print -
    }

    printf("%4ld ", sp->st_nlink); // link count
    printf("%4d ", sp->st_gid);    // gid
    printf("%4d ", sp->st_uid);    // uid
    printf("%8ld ", sp->st_size);  // file size
    //// print time
    strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form
    ftime[strlen(ftime) - 1] = 0;        // kill \n at end
    printf("%s ", ftime);
    ////// print name
    printf("%s", basename(fname)); // print file basename
    printf("\n");
}

int ls_dir(char *dname)
{
    // use opendir(), readdir(); then call ls_file(name)
    struct dirent *ep;
    DIR *dp;

    dp = opendir(dname);
    while (ep = readdir(dp))
    {
        ls_file(ep->d_name);
    }
}

void print_Command_Menu()
{
    printf("********************** Menu ***********************\n");
    printf("* get  put  ls   cd   pwd   mkdir   rmdir   rm  *\n");
    printf("* lcat     lls  lcd  lpwd  lmkdir  lrmdir  lrm  *\n");
    printf("***************************************************\n");
}
