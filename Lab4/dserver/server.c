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
#include <libgen.h>
#include <time.h>
#include <math.h>

#define MAX 256
#define PORT 1234

int n;

char ans[MAX];
char line[MAX];
char reply[MAX];
char *arg[56];

char parsedLine[256];

int tokenize(char *inputLine);
int ls_file(char *fname);
int ls_dir(char *dname, int cfd);

int main()
{
    char cwd[128];
    char clientPath[128];
    char *command;
    char *pathname;
    int sfd, cfd, len;
    struct sockaddr_in saddr, caddr;
    int i, length;

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
    //saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    saddr.sin_port = htons(PORT);

    printf("3. bind socket to server\n");
    if ((bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr))) != 0)
    {
        printf("socket bind failed\n");
        exit(0);
    }

    // Now server is ready to listen and verification
    if ((listen(sfd, 5)) != 0)
    {
        printf("Listen failed\n");
        exit(0);
    }
    while (1)
    {
        // Try to accept a client connection as descriptor newsock
        length = sizeof(caddr);
        cfd = accept(sfd, (struct sockaddr *)&caddr, &length);
        if (cfd < 0)
        {
            printf("server: accept error\n");
            exit(1);
        }

        printf("server: accepted a client connection from\n");
        printf("-----------------------------------------------\n");
        printf("    IP=%s  port=%d\n", "127.0.0.1", ntohs(caddr.sin_port));
        printf("-----------------------------------------------\n");

        // strcpy(cwd, "/"); // Sets the CWD of the server to root
        getcwd(cwd, 128);
        chroot(cwd);

        // Processing loop
        while (1)
        {
            printf("server ready for next request ....\n");
            n = read(cfd, line, MAX);
            if (n == 0)
            {
                printf("server: client died, server loops\n");
                close(cfd);
                break;
            }

            // show the line string
            printf("server: read  n=%d bytes; line=[%s]\n", n, line);
            strcpy(reply, line); // copy line to reply
            tokenize(reply);

            printf("Reply: %s\n", reply);
            printf("cwd = %s\n", cwd);

            command = arg[0]; // Gets the cmd of the line

            if (strcmp(command, "cd") == 0)
            {
                int r = chdir(arg[1]);
            }
            else if (strcmp(command, "exit") == 0)
            {
                exit(0);
            }
            else if (strcmp(command, "mkdir") == 0)
            {
                printf("Running mkdir command\n");
                mkdir(arg[1], 0766);
                sprintf(line, "mkdir %s ok", clientPath);
            }
            else if (strcmp(command, "rmdir") == 0)
            {
                printf("Running rmdir command\n");
                rmdir(arg[1]);
                sprintf(line, "rmdir %s ok", clientPath);
            }
            else if (strcmp(command, "rm") == 0)
            {
                printf("Running rm command\n");
                remove(arg[1]);
                sprintf(line, "rm %s ok", clientPath);
            }
            else if (strcmp(command, "pwd") == 0)
            {
                getcwd(cwd, 128);
                printf("cwd = %s\n", cwd);
                sprintf(line, "pwd %s", cwd);
            }
            else if (strcmp(command, "ls") == 0)
            {
                struct stat mystat, *sp = &mystat;
                int r;
                char *filename, path[1024], cwd[256];
                filename = "./"; // default to CWD

                if (arg[1] == NULL)
                {
                    arg[1] = "";
                }

                if (strcmp(arg[1], "") != 0)
                {
                    filename = arg[1]; // if specified a filename
                }

                if (r = lstat(filename, sp) < 0)
                {
                    printf("no such file %s\n", filename);
                    exit(1);
                }
                strcpy(path, filename);
                if (path[0] != '/')
                    ;
                { // filename is relative : get CWD path
                    getcwd(cwd, 256);
                    strcpy(path, cwd);
                    strcat(path, "/");
                    strcat(path, filename);
                }

                if (S_ISDIR(sp->st_mode))
                    ls_dir(path, cfd);
                else
                    ls_file(path);

                write(cfd, "endofls", MAX);
            }
            else if (strcmp(command, "get") == 0)
            {
                char buf[MAX];
                int fd = 0;
                int n = 0;
                fd = open(arg[1], O_RDONLY);

                if (fd == -1) // Check if file opened
                {
                    printf("File does not exist\n");
                    return 0;
                }
                else // If file is found
                {
                    struct stat fstat, *sp;
                    sp = &fstat;

                    int r = lstat(arg[1], &fstat); // Creates stat for file
                    int sz = sp->st_size;

                    sprintf(line, "%8d", sz);  // Adds file size to line
                    n = write(cfd, line, MAX); // Sends file size to client

                    while ((n = read(fd, buf, MAX)) != 0) // Reads a fd and adds it to buf??
                    {
                        n = write(cfd, buf, n); // Sending line (containing file) to client
                    }
                }
                close(fd); // Closing file
            }
            else if (strcmp(command, "put") == 0)
            {
                char buf[MAX];
                int gd = 0;

                n = read(cfd, ans, MAX);                        // Reads the file size from server
                int size = (int)strtol(ans, (char **)NULL, 10); // Converts file size to an integer

                gd = open(arg[1], O_WRONLY | O_CREAT, 0644); // Opens a copy of a file in client

                if (gd == -1)
                    printf("<P>Permission problems<P>");

                printf("Expecting %8d bytes from server\n", size);

                n = 0; // Set n to 0 bytes

                while (n < size)
                {
                    if (n + MAX > size)
                    {
                        int rembytes = size - n;
                        n += read(cfd, ans, rembytes); // Reads a line from file || increments n bytes
                        write(gd, ans, rembytes);      // Writing file to line
                    }
                    else
                    {
                        n += read(cfd, ans, MAX); // Reads a line from file || increments n bytes
                        write(gd, ans, MAX);      // Writing file to line
                    }
                }
                close(gd);
            }

            // send the echo line to client
            n = write(cfd, line, MAX);

            // printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, line);
            printf("server: ready for next request\n");
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
    char *cp = line;
    sprintf(line, "");
    if ((r = lstat(fname, &fstat)) < 0)
    {
        cp += sprintf(cp, "can’t stat %s\n", fname);
        printf("can’t stat %s\n", fname);
        exit(1);
    }
    if ((sp->st_mode & 0xF000) == 0x8000) // if (S_ISREG())
    {
        cp += sprintf(cp, "-");
        printf("-");
    }
    if ((sp->st_mode & 0xF000) == 0x4000) // if (S_ISDIR())
    {
        cp += sprintf(cp, "d");
        printf("d");
    }
    if ((sp->st_mode & 0xF000) == 0xA000) // if (S_ISLNK())
    {
        cp += sprintf(cp, "l");
        printf("l");
    }
    for (i = 8; i >= 0; i--)
    {
        if (sp->st_mode & (1 << i)) // print r|w|x
        {
            cp += sprintf(cp, "%c", t1[i]);
            printf("%c", t1[i]);
        }
        else // or print -
        {
            cp += sprintf(cp, "%c", t2[i]);
            printf("%c", t2[i]);
        }
    }

    cp += sprintf(cp, "%4ld ", sp->st_nlink); // link count
    cp += sprintf(cp, "%4d ", sp->st_gid);    // gid
    cp += sprintf(cp, "%4d ", sp->st_uid);    // uid
    cp += sprintf(cp, "%8ld ", sp->st_size);  // file size

    printf("%4ld ", sp->st_nlink); // link count
    printf("%4d ", sp->st_gid);    // gid
    printf("%4d ", sp->st_uid);    // uid
    printf("%8ld ", sp->st_size);  // file size
    //// print time
    strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form
    ftime[strlen(ftime) - 1] = 0;        // kill \n at end
    cp += sprintf(cp, "%s ", ftime);
    cp += sprintf(cp, "%s", basename(fname)); // print file basename
    // cp += sprintf(cp, "\n");

    printf("%s ", ftime);
    printf("%s", basename(fname)); // print file basename
    printf("\n");
}

int ls_dir(char *dname, int cfd)
{
    // use opendir(), readdir(); then call ls_file(name)
    struct dirent *ep;
    DIR *dp;

    dp = opendir(dname);
    while (ep = readdir(dp))
    {
        ls_file(ep->d_name);
        n = write(cfd, line, MAX);
    }
}