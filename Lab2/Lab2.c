#include <stdio.h>             // for I/O
#include <stdlib.h>            // for I/O
#include <libgen.h>            // for dirname()/basename()
#include <string.h>
#include <ctype.h>

typedef struct node{
         char  name[64];       // node's name string
         char  type;           // 'D' for DIR; 'F' for file
   struct node *child, *sibling, *parent;
}NODE;


NODE *root, *cwd, *start;
char line[128];
char command[16], pathname[64];

NODE *search_child(NODE *parent, char *name);
void printPreorder(FILE *outfile, NODE *current);

//               0       1      2    
char *cmd[] = {"mkdir", "rmdir", "cd", "ls", "pwd", "creat", "rm", "save", "reload", "?", "quit", 0};

int tokenize(char *pathname)
{
  char *s;
  s = strtok(pathname, "/"); // first call to strtok()
  while(s)
  {
    printf("%s ", s);
    s = strtok(0, "/"); // call strtok() until it returns NULL
  }
}

int str_to_upperCase(char *str, char *upper)
{
  int length = strlen(str);

  for (int i = 0; i < length; i++)
  {
    upper[i] = toupper(str[i]);
  }

  return 0;
}

int str_to_lowerCase(char *str, char *lower)
{
  int length = strlen(str);

  for (int i = 0; i < length; i++)
  {
    lower[i] = tolower(str[i]);
  }

  return 0;
}

int followPathname(char *pathname)
{
  NODE *p;

  // printf("INSIDE followPathname: pathname=%s\n", pathname);

  if (pathname[0]=='/')
    start = root;
  else
    start = cwd;

  char *s;
  s = strtok(pathname, "/"); // first call to strtok()
  while(s)
  {
    p = search_child(start, s);

    if (!strcmp(s, "..") && pathname[0] != '/')
    {
      start = start->parent; // Set start pointer to its parent node
    }
    else if(p && p->type == 'D')
    {
        start = p;
    }
    else
    {
      // printf("There is no directory %s\n", s);
      start = cwd;
      return 0;
    }
    s = strtok(0, "/"); // call strtok() until it returns NULL
  }

  return 0;
}

int cd(char *pathname) //change CWD to pathname, or to / if no pathname
{
  NODE *p;
  char savePathname[128];

  strcpy(savePathname, pathname);

  if (pathname[0] == '\0')
  {
    cwd = root; // If pathname is empty, set cwd to root
    return 1;
  }

  followPathname(dirname(pathname));

  p = search_child(start, basename(savePathname));

  // printf("INSIDE CD: pathname=%s, savePathname=%s\n", pathname, savePathname);

  if (!strcmp(basename(savePathname), ".."))
  {
    cwd = start->parent;
  }
  else if (p && p->type == 'D')
  {
    cwd = p;
  }
  else
  {
    printf("There is no directory %s\n", basename(savePathname));
    start = cwd;
    return 0;
  }

  return 0;
}

int findCmd(char *command)
{
   int i = 0;
   while(cmd[i]){
     if (strcmp(command, cmd[i])==0)
         return i;
     i++;
   }
   return -1;
}

NODE *search_child(NODE *parent, char *name)
{
  NODE *p;
  printf("search for %s in parent DIR\n", name);
  p = parent->child;
  if (p==0)
    return 0;
  while(p){
    if (strcmp(p->name, name)==0)
      return p;
    p = p->sibling;
  }
  return 0;
}

NODE *search_prev_child(NODE *parent, char *name)
{
  NODE *p, *q;
  p = parent->child;
  q = NULL;
  if (p==0)
    return 0;
  while(p){
    if (strcmp(p->name, name)==0)
      return q;
    q = p;
    p = p->sibling;
  }
  return 0;
}

// Precondition: Needs to check if the NODE is of the correct type, has no children, and has the correct name beforehand
int *remove_node(NODE *prev, NODE *deletedNode)
{
  NODE *p, *q;
  printf("remove NODE %s\n", deletedNode->name);

  //If its the only node in the directory
  if (prev == NULL && deletedNode->sibling == NULL)
  {
    deletedNode->parent->child = 0; // The parent Node now points to nothing
  }
  else if (prev == NULL && deletedNode->sibling) //If its at the beginning and some NODE follows
  {
    p = deletedNode->sibling; // Set p to next node
    deletedNode->parent->child = p; //Beginning node parent now points to next node
  }
  else if(prev && deletedNode->sibling) //If it is in the middle
  {
    p = deletedNode->sibling; // p = nextNode
    prev->sibling = p; // nextNode points to prevNode
  }
  else if(prev && deletedNode->sibling == NULL) //If it is in the end
  {
    prev->sibling = 0; //Set prev to point to NULL
  }

  free(deletedNode); // Delete Node
  return 0;
}

int insert_child(NODE *parent, NODE *q)
{
  NODE *p;
  printf("insert NODE %s into END of parent child list\n", q->name);
  p = parent->child;
  if (p==0)
    parent->child = q;
  else{
    while(p->sibling)
      p = p->sibling;
    p->sibling = q;
  }
  q->parent = parent;
  q->child = 0;
  q->sibling = 0;
}

/***************************************************************
 This mkdir(char *name) makes a DIR in the current directory
 You MUST improve it to mkdir(char *pathname) for ANY pathname
****************************************************************/
int mkdir(char *pathname)
{
  char savePathname[128];
  strcpy(savePathname, pathname);

  followPathname(dirname(pathname));
  char *name = basename(savePathname);

  printf("name: %s start: %s\n", name, start->name);

  NODE *p, *q;
  printf("mkdir: name=%s\n", name);

  if (!strcmp(name, "/") || !strcmp(name, ".") || !strcmp(name, "..")){
    printf("can't mkdir with %s\n", name);
    return -1;
  }

  // printf("Dirname: %s Basename: %s\n", dirname(name), basename(name));

  printf("check whether %s already exists\n", name);
  p = search_child(start, name);
  if (p){
    printf("name %s already exists, mkdir FAILED\n", name);
    return -1;
  }
  printf("--------------------------------------\n");
  printf("ready to mkdir %s\n", name);
  q = (NODE *)malloc(sizeof(NODE));
  q->type = 'D';
  strcpy(q->name, name);
  insert_child(start, q);
  printf("mkdir %s OK\n", name);
  printf("--------------------------------------\n");
    
  return 0;
}

int rmdir(char *pathname) //removes the directory, if it is empty
{
  char savePathname[128];
  strcpy(savePathname, pathname);

  followPathname(dirname(pathname));
  char *name = basename(savePathname);

  printf("name: %s start: %s\n", name, start->name);

  NODE *p, *q;
  printf("mkdir: name=%s\n", name);

  if (!strcmp(name, "/") || !strcmp(name, ".") || !strcmp(name, "..")){
    printf("can't rmdir with %s\n", name);
    return -1;
  }

  printf("Dirname: %s Basename: %s\n", dirname(name), basename(name));

  printf("check whether %s exists\n", name);
  p = search_child(start, name); // Node that needs to be deleted
  q = search_prev_child(start, name); // Prev node
  if (p && p->type == 'D' && p->child == NULL)
  {
    remove_node(q, p);
    printf("%s removed from directory\n", name);
    return 1;
  }
  else if (!p)
  {
    printf("%s could not be found in the directory", name);
    return -1;
  }
  else if (p->type != 'D')
  {
    printf("%s is not of the correct type!\n", name);
    return -1;
  }
  else if (p->child)
  {
    printf("%s is not empty!\n", name);
    return -1;
  }
    
  return 0;
}

// This ls() list CWD. You MUST improve it to ls(char *pathname)
int ls(char *pathname) //list the directory contents of pathname or CWD
{
  if (pathname[0] == '\0') // If ls is empty than print cwd and return
  {
    NODE *p = cwd->child;
    printf("cwd contents = ");
    while(p){
      printf("[%c %s] ", p->type, p->name);
      p = p->sibling;
    }
    printf("\n");
    return 0;
  }
  
  NODE *q;
  char savePathname[128];
  strcpy(savePathname, pathname);

  followPathname(dirname(pathname));

  q = search_child(start, basename(savePathname));

  if (!strcmp(basename(savePathname), ".."))
  {
    start = start->parent;
  }
  else if (q && q->type == 'D')
  {
    start = q;
  }
  else
  {
    printf("There is no directory %s\n", basename(savePathname));
    start = cwd;
    return 0;
  }

  NODE *p = start->child;
  printf("%s contents = ", savePathname);
  while(p){
    printf("[%c\t%s] ", p->type, p->name);
    p = p->sibling;
  }
  printf("\n");
}

int pwd_recurse(NODE *nodePtr)
{
  if (nodePtr == root)
  {
    return 0;
  }
  pwd_recurse(nodePtr->parent);
  printf("/%s", nodePtr->name);
}

int pwd() //print the absolute pathname of CWD
{
  printf("cwd = ");
  if (start == root)
  {
    printf("/\n");
    return 0;
  }
  pwd_recurse(start);
  printf("\n");
  return 0;
}

int pwd_recurse_file(FILE *outfile, NODE *nodePtr)
{
  // char upper[128] = {'\0'};
  // char lower[128] = {'\0'};

  if (nodePtr == root)
  {
    return 0;
  }
  pwd_recurse_file(outfile, nodePtr->parent);
  if (nodePtr->type == 'D')
  {
    // str_to_upperCase(nodePtr->name, upper);
    fprintf(outfile, "/%s", nodePtr->name);
  }
  else
  {
    // str_to_lowerCase(nodePtr->name, lower);
    fprintf(outfile, "/%s", nodePtr->name);
  }
}

int pwd_file(FILE *outfile, NODE *current) //print the absolute pathname of CWD
{
  start = current;
  if (start == root)
  {
    fprintf(outfile,"/\n");
    return 0;
  }
  pwd_recurse_file(outfile, start);
  fprintf(outfile, "\n");
  return 0;
}

int creat(char *pathname) //create a FILE node
{
  char savePathname[128];
  strcpy(savePathname, pathname);

  followPathname(dirname(pathname));
  char *name = basename(savePathname);

  printf("name: %s start: %s\n", name, start->name);

  NODE *p, *q;
  printf("mkdir: name=%s\n", name);

  if (!strcmp(name, "/") || !strcmp(name, ".") || !strcmp(name, "..")){
    printf("can't file with %s\n", name);
    return -1;
  }

  printf("Dirname: %s Basename: %s\n", dirname(name), basename(name));

  printf("check whether %s already exists\n", name);
  p = search_child(start, name);
  if (p){
    printf("name %s already exists, creat FAILED\n", name);
    return -1;
  }
  printf("--------------------------------------\n");
  printf("ready to file %s\n", name);
  q = (NODE *)malloc(sizeof(NODE));
  q->type = 'F';
  strcpy(q->name, name);
  insert_child(start, q);
  printf("file %s OK\n", name);
  printf("--------------------------------------\n");
    
  return 0;
}

int rm(char *pathname) //remove the FILE node
{
  char savePathname[128];
  strcpy(savePathname, pathname);

  followPathname(dirname(pathname));
  char *name = basename(savePathname);

  printf("name: %s start: %s\n", name, start->name);

  NODE *p, *q;
  printf("mkdir: name=%s\n", name);

  if (!strcmp(name, "/") || !strcmp(name, ".") || !strcmp(name, "..")){
    printf("can't rmdir with %s\n", name);
    return -1;
  }

  printf("Dirname: %s Basename: %s\n", dirname(name), basename(name));

  printf("check whether %s exists\n", name);
  p = search_child(start, name); // Node that needs to be deleted
  q = search_prev_child(start, name); // Prev node
  if (p && p->type == 'F' && p->child == NULL)
  {
    remove_node(q, p);
    printf("%s removed from directory\n", name);
    return 1;
  }
  else if (!p)
  {
    printf("%s could not be found in the directory", name);
    return -1;
  }
  else if (p->type != 'F')
  {
    printf("%s is not of the correct type!\n", name);
    return -1;
  }
    
  return 0;
}

int save()
{
  FILE *outfile = fopen("saveFile", "w"); // Open saveFile (write only)

  if (outfile == NULL)
  {
    printf("Could not open outfile");
    return 0;
  }

  NODE *current = root;

  printPreorder(outfile, current); // PreOrder Traversal to outfile

  fclose(outfile); // Close file to update
  return 0;
}

void printPreorder(FILE *outfile, NODE *current)
{
  if(current == NULL)
  {
    return;
  }

  fprintf(outfile, "%c\t", current->type);
  pwd_file(outfile, current);

  printPreorder(outfile, current->child); // Recurs all the children
  printPreorder(outfile, current->sibling); // Recurs all the siblings
}

int reload()
{
  char type;
  char garbage;
  char *path;
  char buf[50];

  FILE *infile = fopen("saveFile", "r"); // Open saveFile (read only)

  if (infile == NULL)
  {
    printf("Could not open infile");
    return 0;
  }

  fgets(buf, 50, infile);

  while(fscanf(infile, "%c\t%s%c", &type, path, &garbage) == 3)
  {
    // printf("type: %c\n", type);
    // printf("pathname: %s\n", path);

    if (type == 'D')
    {
      mkdir(path);
    }
    else if (type == 'F')
    {
      creat(path);
    }
  }

  fclose(infile);
  return 0;
}

int menu()
{
  printf("====================== MENU =======================\n");
  printf("mkdir rmdir ls  cd  pwd  creat  rm  save reload  quit\n");
  printf("=====================================================\n");
}

int quit()
{
  printf("Program exit\n");
  exit(0);
  // improve quit() to SAVE the current tree as a Linux file
  // for reload the file to reconstruct the original tree
}

int initialize()
{
    root = (NODE *)malloc(sizeof(NODE));
    strcpy(root->name, "/");
    root->parent = root;
    root->sibling = 0;
    root->child = 0;
    root->type = 'D';
    cwd = root;
    printf("Root initialized OK\n");
}

int main()
{
  int index;

  initialize();

  printf("NOTE: commands = [mkdir|rmdir|cd|ls|pwd|creat|rm|save|reload|?|quit]\n");
  printf("Enter ? for help menu\n");

  while(1){
      printf("Enter command line : ");
      fgets(line, 128, stdin); // Get at most 128 chars from stdin
      line[strlen(line)-1] = 0; // Kill \n at end of line

      char missing[128];

      command[0] = pathname[0] = 0;
      sscanf(line, "%s %s", command, pathname); // Extracts by format (chars, strings, integers)
      printf("command=%s pathname=%s\n", command, pathname);
      
      if (command[0]==0) 
         continue;

      index = findCmd(command);

      if (index < 0 || index > 10)
      {
        printf("invalid command\n");
      }

      switch (index){
        case 0: mkdir(pathname); break;
        case 1: rmdir(pathname); break;
        case 2: cd(pathname); break;
        case 3: ls(pathname); break;
        case 4: 
          start = cwd; 
          pwd(); 
          break;
        case 5: creat(pathname); break;
        case 6: rm(pathname); break;
        case 7: save(); break;
        case 8: reload(); break;
        case 9: menu(); break;
        case 10:
          save();
          quit(); 
          break;
      }
      printf("cwd: %s\n", cwd->name);
  }
}
