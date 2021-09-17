#include <stdio.h>             // for I/O
#include <stdlib.h>            // for I/O
#include <string.h>

typedef struct node {
    char  name[64];       // node's name string
    char  type;           // 'D' for DIR; 'F' for file
    struct node* child, * sibling, * parent;
}NODE;

typedef enum bool{
  false = 0, true = 1
} Bool;

NODE* root, * cwd, * start;
char line[128];
char command[16], pathname[64];

//                0       1      2    3     4      5     6     7       8       9      10
char* cmd[] = { "mkdir","rmdir","ls","cd","pwd","creat","rm","save","reload","menu","quit",0 };

int findCmd(char* command)
{
    for (int i = 0; cmd[i]; i++)
        if (strcmp(command, cmd[i]) == 0)
            return i;
    return -1;
}

void insert_child(NODE* location, NODE* newNode)
{
    NODE* cur = location->child;
    printf("insert NODE %s into END of parent child list\n", newNode->name);
    if (cur == NULL)
    {
        location->child = newNode;
    }
    else
    {
        while (cur->sibling)
            cur = cur->sibling;
        cur->sibling = newNode;
    }
    newNode->parent = location;
    newNode->child = NULL;
    newNode->sibling = NULL;
}

void remove_child(NODE* rmnode)
{
    NODE* cur = rmnode->parent->child;

    if (cur == rmnode)
        if (!rmnode->sibling)
        {
            free(rmnode);
            cur = NULL;
        }
        else
        {
            cur = rmnode->sibling;
            free(rmnode);
        }
    else
    {
        while (cur->sibling != rmnode)
            cur = cur->sibling;
        cur->sibling = rmnode->sibling;
        free(rmnode);
    }

}

NODE* findNode(NODE* cur, char* name)
{
    if (!cur)
    {
        printf("%s cannot be found\n", name);
        return cur;
    }
    else if (!strcmp(cur->name, name))
        return cur;
    else
        return findNode(cur->sibling, name);
}

NODE* parse(char* pathname)
{
    NODE* location, * cur;

    if (pathname[0] == "/")
        if (!strcmp(pathname, "/"))
            location = root;
        else
        {
            char* s1 = strtok(pathname, "/");
            cur = root;
            if (strcmp(s1, "."))
            {
                while (s1)
                {
                    cur = findNode(cur->child, s1);
                    if (!cur)
                        break;
                    s1 = strtok(NULL, "/");
                }
            }
            location = cur;
        }
    else
    {
        if (!strcmp(pathname, "."))
            location = cwd;
        else if (!strcmp(pathname, ".."))
            location = cwd->parent;
        else
        {
            char* s2 = strtok(pathname, "/");
            cur = cwd;
            if (strcmp(s2, "."))
            {
                while (s2)
                {
                    cur = findNode(cur->child, s2);
                    if (!cur)
                        break;
                    s2 = strtok(NULL, "/");
                }
            }
            location = cur;
        }
    }
    return location;
}

/***************************************************************
 This mkdir(char *name) makes a DIR in the current directory
 You MUST improve it to mkdir(char *pathname) for ANY pathname
****************************************************************/

int mkdir(char* name)
{
    printf("mkdir: name=%s\n", name);

    if (!strcmp(name, "/") || !strcmp(name, ".") || !strcmp(name, ".."))
    {
        printf("can't mkdir with %s\n", name);
        return -1;
    }

    printf("check whether %s already exists\n", name);

    start = parse(name);
    if (start)
    {
        printf("name %s already exists, mkdir FAILED\n", name);
        return -1;
    }
    else
        start = cwd;

    printf("--------------------------------------\n");
    printf("ready to mkdir %s\n", name);

    NODE* newNode = (NODE*)malloc(sizeof(NODE));
    newNode->type = 'D';
    strcpy(newNode->name, name);
    insert_child(start, newNode);

    printf("mkdir %s OK\n", name);
    printf("--------------------------------------\n");
    return 0;
}

int rmdir(char* name)
{
    printf("rmdir: name=%s\n", name);

    if (!strcmp(name, "/") || !strcmp(name, ".") || !strcmp(name, ".."))
    {
        printf("can't rmdir with %s\n", name);
        return -1;
    }

    start = parse(name);
    if (!start)
        start = cwd;

    printf("--------------------------------------\n");
    printf("ready to rmdir %s\n", name);

    remove_child(start);

    printf("rmdir %s done\n", name);
    printf("--------------------------------------\n");
    return 0;
}

// This ls() list CWD. You MUST improve it to ls(char *pathname)
void ls(char *pathname)
{
    NODE* cur;
    if (!strcmp(pathname, ""))
        cur = cwd;
    else
    {
        cur = parse(pathname);
    }

    printf("cwd contents = ");

    if (cur)
        cur = cur->child;
    while (cur)
    {
        printf("[%c %s] ", cur->type, cur->name);
        cur = cur->sibling;
    }
    printf("\n");
}

void cd(char *pathname)
{
    NODE* location;
    if (pathname)
    {
        location = parse(pathname);
        if (!strcmp(pathname, ".."))
            if (location->parent)
                location = location->parent;
        else if (!strcmp(pathname, "/"))
            cwd = root;
        else
            location = findNode(location->child, pathname);
    }
    else
        return;
    if (!location)
    {
        printf("Directory not found.\n");
        return;
    }
    cwd = location;
}

void pwdhelp(NODE* cur)
{
    if (cur->name == root->name)
    {
        printf("/");
        return;
    }
    pwdhelp(cur->parent);
    if (cur->parent == root)
        printf("%s", cur->name);
    else
        printf("/%s", cur->name);
}

void pwd()
{
    pwdhelp(cwd);
    putchar('\n');
}

int creat(char *name)
{
    printf("Filename: name=%s\n", name);

    if (!strcmp(name, "/") || !strcmp(name, ".") || !strcmp(name, ".."))
    {
        printf("can't create file with %s\n", name);
        return -1;
    }

    start = parse(name);
    if (start)
    {
        printf("name %s already exists, mkdir FAILED\n", name);
        return -1;
    }
    else
        start = cwd;

    printf("check whether %s already exists\n", name);    

    printf("--------------------------------------\n");
    printf("ready to mkdir %s\n", name);

    NODE* newNode = (NODE*)malloc(sizeof(NODE));
    newNode->type = 'F';
    strcpy(newNode->name, name);
    insert_child(start, newNode);

    printf("File %s OK\n", name);
    printf("--------------------------------------\n");
    return 0;
}

int rm(char *name)
{
    printf("rm file: name=%s\n", name);

    if (!strcmp(name, "/") || !strcmp(name, ".") || !strcmp(name, ".."))
    {
        printf("can't rm with %s\n", name);
        return -1;
    }

    start = parse(name);
    if (!start)
        start = cwd;

    printf("--------------------------------------\n");
    printf("ready to rm %s\n", name);

    remove_child(start);

    printf("rm %s done\n", name);
    printf("--------------------------------------\n");
    return 0;
}


void filepwd(FILE* outfile, NODE* cur)
{
    if (cur->name == root->name)
    {
        fprintf(outfile,"/");
        return;
    }
    filepwd(outfile, cur->parent);
    if (cur->parent == root)
        fprintf(outfile,"%s", cur->name);
    else
        fprintf(outfile,"/%s", cur->name);
}

void printFile(FILE* outfile, NODE* location)
{
    if (!location)
        return;
    fprintf(outfile,"%c\t", location->type);
    filepwd(outfile, location);
    fprintf(outfile, "\n");
    printFile(outfile, location->child);
    printFile(outfile, location->sibling);
}

void save(char* filename)
{
    FILE* outfile = fopen(filename, "w+");
    printFile(outfile, root);
    fclose(outfile);
}

void reload(char* filename)
{
    FILE* outfile = fopen(filename, "r");

    char type,line[128],path[128];

    fgets(line, 128, outfile);

    while (fgets(line, 128, outfile))
    {
        line[strlen(line) - 1] = NULL;
        sscanf(line, "%c %s", &type, &path);

        switch (type)
        {
        case 'D':
            mkdir(path);
            break;
        case 'F':
            creat(path);
            break;
        }
    }
}

void quit()
{
    printf("Program exit\n");
    save("savefile.txt");
    exit(NULL);
    // improve quit() to SAVE the current tree as a Linux file
    // for reload the file to reconstruct the original tree
}

void initialize()
{
    root = (NODE*)malloc(sizeof(NODE));
    strcpy(root->name, "/");
    root->parent = root;
    root->sibling = 0;
    root->child = 0;
    root->type = 'D';
    cwd = root;
    printf("Root initialized OK\n");
}

void menu()
{
    printf("Commands = [mkdir|rmdir|ls|cd|pwd|creat|rm|save|reload|quit]\n");
}

int main(void)
{
    int index;

    initialize();

    menu();

    while (true) {
        printf("Enter command line : ");
        fgets(line, 128, stdin);
        line[strlen(line) - 1] = 0;

        command[0] = pathname[0] = 0;
        sscanf(line, "%s %s", command, pathname);
        printf("command=%s pathname=%s\n", command, pathname);

        if (command[0] == 0)
            continue;

        index = findCmd(command);

        switch (index)
        {
        case 0: mkdir(pathname);        break;
        case 1: rmdir(pathname);	    break;
        case 2: ls(pathname);           break;
        case 3: cd(pathname);		    break;
        case 4: pwd();	                break;
        case 5: creat(pathname);        break;
        case 6: rm(pathname);	        break;
        case 7: save("savefile.txt");   break;
        case 8: reload("savefile.txt"); break;
        case 9: menu();                 break;
        case 10: quit();		        break;
        }
    }
}

