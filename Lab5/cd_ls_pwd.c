#include <pwd.h>
#include <grp.h>

/************* cd_ls_pwd.c file **************/
int cd()
{
  int ino = getino(pathname); // Gets inode from pathname

  if (ino == 0) // return error if ino == 0;
  {
    printf("Error\n");
    return -1;
  }

  MINODE *mip = iget(dev, ino); // Gets mip

  if (S_ISDIR(mip->INODE.i_mode)) // Check if mip->INODe is a dir
  {
    iput(running->cwd); // Release old cwd
    running->cwd = mip; // Change cwd to mip
  }
  else
  {
    printf("Not a valid directory!\n");
  }
}

int ls_file(MINODE *mip, char *name)
{
  char *t1 = "xwrxwrxwr-------";
  char *t2 = "----------------";
  char *ftime;
  struct passwd *pwd;
  struct group *grp;
  int i;

  INODE *ip = &mip->INODE; // use mip->INODE and name to list the file info

  if ((ip->i_mode & 0xF000) == 0x8000) // if (S_ISREG())
    printf("-");
  if ((ip->i_mode & 0xF000) == 0x4000) // if (S_ISDIR())
    printf("d");
  if ((ip->i_mode & 0xF000) == 0xA000) // if (S_ISLNK())
    printf("l");

  for (i = 8; i >= 0; i--)
  {
    if (ip->i_mode & (1 << i)) // print r|w|x
      printf("%c", t1[i]);
    else
      printf("%c", t2[i]); // or print -
  }

  printf("%4d ", ip->i_links_count); //link count

  if ((pwd = getpwuid(ip->i_uid))) // Get UID
  {
    printf("%-8.8s", pwd->pw_name);
  }
  else
  {
    printf("%4d", ip->i_gid);
  }

  if ((grp = getgrgid(ip->i_gid))) // Get GID
  {
    printf("%-8.8s", grp->gr_name);
  }
  else
  {
    printf("%4d", ip->i_gid);
  }

  printf("%8d ", ip->i_size); // file size

  ftime = ctime((time_t *)&ip->i_mtime); // Gets time
  ftime[strlen(ftime) - 1] = 0;          // kill \n at end
  printf("%s  ", ftime);                 // print time
  printf("%s  ", name);                  // print name
  printf("\n");
}

int ls_dir(MINODE *mip)
{
  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;

  get_block(dev, mip->INODE.i_block[0], buf); // We assume not many directories/Files (Reads only 1 block)
  dp = (DIR *)buf;
  cp = buf;

  while (cp < buf + BLKSIZE)
  {                               // Step through the dir_entries in the data block 0
    int ino = dp->inode;              // Each dir_entry contains the inode #, ino, and name of a file
    MINODE *mip = iget(dev, ino); // For each dir_entry, use iget() to get its minode, as in...
    ls_file(mip, dp->name);       // Then call ls_file(mip, name)

    cp += dp->rec_len;
    dp = (DIR *)cp;
  }
  printf("\n");
}

int ls()
{
  // printf("ls: list CWD only! YOU FINISH IT for ls pathname\n");
  if (strcmp(pathname, "") == 0)
  {
    ls_dir(running->cwd);
  }
  else
  {
    int ino = getino(pathname); // Gets inode from pathname
    if (ino == 0)               // return error if ino == 0;
    {
      printf("Error\n");
      return -1;
    }
    MINODE *mip = iget(dev, ino); // Gets mip
    ls_dir(mip);
  }
}

void rpwd(MINODE *wd)
{
  int my_ino, parent_ino;
  char my_name[128] = "";

  if (wd == root)
  {
    return;
  }

  parent_ino = findino(wd, &my_ino); //From wd->INODE.i_block[0] get my_ino and parent_ino

  MINODE *pip = iget(dev, parent_ino);

  findmyname(pip, my_ino, my_name); // Get my_name string by my_ino as LOCAL

  rpwd(pip); // Recursive call rpwd(pip) with parent minode
  printf("/%s", my_name);
}

void pwd(MINODE *wd)
{
  printf("PWD: ");
  if (wd == root)
  {
    printf("/\n");
  }
  else
  {
    rpwd(wd);
    printf("\n");
  }
}
