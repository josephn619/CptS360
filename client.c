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
#include <libgen.h>     // for dirname()/basename()
#include <time.h> 

#define MAX 256
#define PORT 1234
#define END "END"

char line[MAX], ans[MAX];
int n;

char *cmds[] = {"lcat","lls","lcd","lpwd","lmkdir","lrmdir","lrm"};

struct sockaddr_in saddr; 
int sfd;

void lcat(char *filename)
{
    char buf[512];
    FILE *fd = fopen(filename, "r");
    if (fd != NULL)
    {
        while (fgets(buf, 512, fd) != NULL)
        {
            buf[strlen(buf) - 1] = '\0';
            puts(buf);
        }
    }
    else
    {
        printf("fopen failed, aborting\n");
        return 1;
    }
    return 0;
}

int lls_dir(char *pathname)
{
    struct dirent *dp;
    DIR *mydir;
    char fullPath[MAX * 2];
    bzero(fullPath, MAX);

    if ((mydir = opendir(pathname)) == NULL)
    {
        perror("couldn't open pathname");
        return 1;
    }

    do
    {
        if ((dp = readdir(mydir)) != NULL)
        {
            bzero(fullPath, MAX);
            strcpy(fullPath, pathname);
            strcat(fullPath, "/");
            strcat(fullPath, dp->d_name);
            lls_file(fullPath);
        }

    } while (dp != NULL);

    closedir(mydir);
    return 0;
}

int lls_file(char *filename)
{
    char linkname[MAX];
    char *t1 = "xwrxwrxwr-------";
    char *t2 = "----------------";

    struct stat fstat, *sp;
    int r, i;
    char ftime[64];
    sp = &fstat;
    if ((r = lstat(filename, &fstat)) < 0)
    {
        printf("can’t stat %s\n", filename);
        exit(1);
    }
    if ((sp->st_mode & 0xF000) == 0x8000) // if (S_ISREG())
        printf("%c", '-');
    if ((sp->st_mode & 0xF000) == 0x4000) // if (S_ISDIR())
        printf("%c", 'd');
    if ((sp->st_mode & 0xF000) == 0xA000) // if (S_ISLNK())
        printf("%c", 'l');
    for (i = 8; i >= 0; i--)
    {
        if (sp->st_mode & (1 << i))
            printf("%c", t1[i]); // print r|w|x printf("%c", t1[i]);
        else
            printf("%c", t2[i]); // or print -
    }
    printf("%4d ", sp->st_nlink); // link count
    printf("%4d ", sp->st_gid);   // gid
    printf("%4d ", sp->st_uid);   // uid
    printf("%8ld ", sp->st_size);  // file size

    strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form ftime[strlen(ftime)-1] = 0; // kill \n at end
    ftime[strlen(ftime) - 1] = 0;        // removes the \n
    printf("%s ", ftime);                // prints the time

    printf("%s", basename(filename)); // print file basename // print -> linkname if symbolic file
    if ((sp->st_mode & 0xF000) == 0xA000)
    {
        readlink(filename, linkname, MAX);
        printf(" -> %s", linkname); // print linked name }
    }

    printf("\n");
    return 0;
}

int lls(char *pathname)
{
    if (!strcmp(pathname, ""))
    {
        lls_dir("./");
        return 1;
    }
    lls_dir(pathname);
    return 0;
}

int lcd(char *pathname)
{
    return chdir(pathname);
}

int lpwd()
{
    char buf[MAX];
    getcwd(buf, MAX);
    printf("%s\n", buf);
}

int lmkdir(char *pathname)
{
    return mkdir(pathname, 0775);
}

int lrmdir(char *pathname)
{
    return rmdir(pathname);
}

int lrm(char *pathname)
{
    return unlink(pathname);
}

int main(int argc, char *argv[], char *env[]) 
{ 
    int n, i;

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
        printf("********************** menu ***********************\n");
        printf("*  get  put  ls   cd   pwd   mkdir   rmdir   rm   *\n");
        printf("*  lcat     lls  lcd  lpwd  lmkdir  lrmdir  lrm   *\n");
        printf("***************************************************\n");

        printf("input a line : ");
        bzero(line, MAX);                // zero out line[ ]
        fgets(line, MAX, stdin);         // get a line (end with \n) from stdin

        line[strlen(line)-1] = '\0';

        if (line[0]=='\0')                  // exit if NULL line
            exit(0);

        char cmd[MAX], arg[MAX];
        

        /*Check if any arguments */

        int empty = 0;

        for (int i = 0; i < strlen(line); i++)
                if (line[i] == ' ')
                    if (line[i+1] != ' ' || line[i+1] != '\0')
                    {
                        sscanf(line,"%s %s",cmd, arg);
                        empty = 1;
                    }
        
        if (!empty) // If no arguments found
        {
            strcpy(cmd,line);
            strcpy(arg, "");
        }
                    
        /* Find command */
        
        int j = 0, found = 0;
            
        for (; cmds[j]; j++)
        {            
            if (!strcmp(cmd,cmds[j]))
            {
                found = 1;
                break;
            }
        }

        if (found) // client side operations
        {
            switch(j)
            {
                case 0: lcat(arg); break;
                case 1: lls(arg); break;
                case 2: lcd(arg); break;
                case 3: lpwd(); break;
                case 4: lmkdir(arg); break;
                case 5: lrmdir(arg); break;
                case 6: lrm(arg); break;
            }
        }
        else // server side operations
        {
            // Send ENTIRE line to server
            n = write(sfd, line, MAX);
            printf("client: wrote n=%d bytes; line=(%s)\n", n, line);

            if (!strcmp(cmd, "get"))
            {
                char buf[MAX];
                int fd;
                int b = read(sfd, buf, MAX);
                int file_size = atoi(buf);
                bzero(buf, MAX);
                fd = open(arg, O_WRONLY|O_CREAT, 0644);
                if (fd > 0) 
                {
                    int bytes_read = 0; 
                    int packet_size = 0;
                    while (file_size > 0) 
                    {
                        read(sfd, buf, MAX);
                        bytes_read += MAX;
                        if(file_size < MAX)
                        {
                            write(fd, buf, file_size);
                            file_size -= file_size;
                        }
                        else
                        {
                            write(fd, buf, MAX);
                            file_size -= MAX;
                        }
                    }
                    close(fd);
                }
            }
            else if (!strcmp(cmd, "put"))
            {
                int r;
                char temp[MAX];

                struct stat fstat, *sp;
                sp = &fstat;
                r = lstat(arg, &fstat);
                int file_size = sp->st_size;
                sprintf(temp, "%d", file_size);

                write(sfd, temp, MAX);
                int fp = open(arg, O_RDONLY);
                if (fp > 0) 
                {
                    char buf[MAX];
                    bzero(buf,MAX);
                    int n = read(fp, buf, MAX);
                    while(n > 0)
                    {
                        write(sfd, buf, n);
                        n = read(fp, buf, MAX);
                    }
                }
                close(fp);
                bzero(ans,MAX);
                while (strcmp(ans, END))
                {
                    bzero(ans,MAX);
                    n = read(sfd, ans, MAX);
                    if (strcmp(ans,END))
                        printf("client: read  n=%d bytes; response=(%s)\n",n, ans);
                }
            }
            else
            {
                bzero(ans,MAX);
                while (strcmp(ans,END))
                {
                    bzero(ans,MAX);
                    n = read(sfd, ans, MAX);
                    if (strcmp(ans,END)) // Just formatting so it doesn't print END
                        printf("client: read  n=%d bytes; response=(%s)\n",n, ans);
                }
            }
        } 
    }
}