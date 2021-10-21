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

#define MAX 256
#define PORT 1234
#define END "END"

int n;

char *cmds[] = {"get","put","ls","cd","pwd","mkdir","rmdir","rm"};

char ans[MAX];
char line[MAX];

int sfd, cfd; 

int sget(char *filename)
{
    int r;
    char temp[MAX];

    struct stat fstat, *sp;
    sp = &fstat;
    r = lstat(filename, &fstat);
    int file_size = sp->st_size;
    sprintf(temp, "%d", file_size);

    write(cfd, temp, MAX);
    int fp = open(filename, O_RDONLY);
    if (fp > 0) 
    {
        char buf[MAX];
        bzero(buf,MAX);
        int n = read(fp, buf, MAX); // read 256 bytes from the file
        while(n > 0)
        {
            write(cfd, buf, n);
            n = read(fp, buf, MAX);
        }
    }
    close(fp);
    return 0;
}

void sput(char *filename)
{
    char buf[MAX];
    int fd;
    int b = read(cfd, buf, MAX);
    int file_size = atoi(buf);
    bzero(buf,MAX);
    fd = open(filename, O_WRONLY|O_CREAT, 0644);
    if (fd > 0) 
    {
        int bytes_read = 0;
        int packet_size = 0;
        while (file_size > 0) 
        {
            read(cfd, buf, MAX);
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

int sls_dir(char *pathname)
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
            sls_file(fullPath);
        }

    } while (dp != NULL);

    closedir(mydir);
    return 0;
}

int sls_file(char *filename)
{
    int n;
    char buf[MAX];
    char fmt[MAX];
    char linkname[MAX];
    char *t1 = "xwrxwrxwr-------";
    char *t2 = "----------------";

    struct stat fstat, *sp;
    int r, i;
    char ftime[64];
    sp = &fstat;
    if ((r = lstat(filename, &fstat)) < 0)
    {
        sprintf(fmt,"can’t stat %s\n", filename);
        strcpy(buf, fmt);
        n = write(cfd, buf, MAX);
        printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, buf);
        exit(1);
    }
    if ((sp->st_mode & 0xF000) == 0x8000){ // if (S_ISREG())
        sprintf(fmt, "%c", '-');
        strcat(buf, fmt);
    }
    if ((sp->st_mode & 0xF000) == 0x4000){ // if (S_ISDIR())
        sprintf(fmt, "%c", 'd');
        strcat(buf, fmt);
    }   
    if ((sp->st_mode & 0xF000) == 0xA000){ // if (S_ISLNK())
        sprintf(fmt, "%c", 'l');
        strcat(buf, fmt);
    }
    for (i = 8; i >= 0; i--)
    {
        if (sp->st_mode & (1 << i)){
            sprintf(fmt, "%c", t1[i]); // print r|w|x printf("%c", t1[i]);
            strcat(buf, fmt);
        }
        else{
            sprintf(fmt, "%c", t2[i]); // or print -
            strcat(buf, fmt);
        }
    }
    sprintf(fmt, "%4d ", sp->st_nlink); // link count
    strcat(buf, fmt);
    sprintf(fmt, "%4d ", sp->st_gid);   // gid
    strcat(buf, fmt);
    sprintf(fmt, "%4d ", sp->st_uid);   // uid
    strcat(buf, fmt);
    sprintf(fmt, "%8ld ", sp->st_size);  // file size
    strcat(buf, fmt);

    strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form ftime[strlen(ftime)-1] = 0; // kill \n at end
    ftime[strlen(ftime) - 1] = 0;        // removes the \n
    sprintf(fmt, "%s ", ftime);                // prints the time
    strcat(buf, fmt);

    sprintf(fmt, "%s", basename(filename)); // print file basename // print -> linkname if symbolic file
    strcat(buf, fmt);
    if ((sp->st_mode & 0xF000) == 0xA000)
    {
        readlink(filename, linkname, MAX);
        sprintf(fmt," -> %s", linkname); // print linked name }
        strcat(buf, fmt);
    }

    // strcat(buf, "\n");
    n = write(cfd, buf, MAX);

    printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, buf);
    bzero(buf, MAX);
}

int sls(char *pathname)
{
    char buf[MAX];
    getcwd(buf, MAX-1);
    if (!strcmp(pathname, ""))
    {
        sls_dir(buf);
        return 1;
    }
    sls_dir(pathname);
    return 0;
}

int scd(char *pathname)
{
    strcpy(line, "OK.");
    n = write(cfd, line, MAX);
    printf("server: wrote n=%d bytes;[%s]\n", n, line);
    return chdir(pathname);
}

int spwd()
{
    getcwd(line, MAX);
    n = write(cfd, line, MAX);
    printf("server: wrote n=%d bytes;[%s]\n", n, line);
    return printf("%s\n", line);
}

int smkdir(char *pathname)
{
    strcpy(line, "OK.");
    n = write(cfd, line, MAX);
    printf("server: wrote n=%d bytes;[%s]\n", n, line);
    return mkdir(pathname, 0755);
}

int srmdir(char *pathname)
{
    strcpy(line, "OK.");
    n = write(cfd, line, MAX);
    printf("server: wrote n=%d bytes;[%s]\n", n, line);
    return rmdir(pathname);
}

int srm(char *pathname)
{
    strcpy(line, "OK.");
    n = write(cfd, line, MAX);
    printf("server: wrote n=%d bytes;[%s]\n", n, line);
    return unlink(pathname);
}

int main() 
{ 
    int len; 
    struct sockaddr_in saddr, caddr; 
    int i, length;

    chdir("/");
    chroot("/");
    
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
    while(1)
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
        
        // Processing loop
        while(1)
        {
            printf("server ready for next request ....\n");
            n = read(cfd, line, MAX);
            if (n==0)
            {
                printf("server: client died, server loops\n");
                close(cfd);
                break;
            }
             
            // show the line string

            char cmd[MAX], arg[MAX];

            printf("server: read  n=%d bytes; line=[%s]\n", n, line);

            /* Check if any arguments */

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
                strcpy(arg,"");
            }

            /* Find command */
            int j = 0;
            
            for (; cmds[j]; j++)
                if (!strcmp(cmd,cmds[j]))
                    break;

            bzero(line,MAX);
                 
            switch(j)
            {
                case 0: sget(arg); break;
                case 1: sput(arg); break;
                case 2: sls(arg); break;
                case 3: scd(arg); break;
                case 4: spwd(); break;
                case 5: smkdir(arg); break;
                case 6: srmdir(arg); break;
                case 7: srm(arg); break;
            }

            n = write(cfd, END, MAX);

            printf("server: wrote n=%d bytes;[%s]\n", n, END);
            printf("server: ready for next request\n");
       }
    }
}

