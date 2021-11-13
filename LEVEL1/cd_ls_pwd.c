  #include <pwd.h>
  #include <grp.h>

  /************* cd_ls_pwd.c file **************/
  int cd()
  {

    // READ Chapter 11.7.3 HOW TO chdir

    int ino = getino(pathname);
    if (ino == 0)
    {
      printf("Error: ino doesnt exist\n");
      return -1;
    }
    MINODE *mip = iget(dev, ino);
    if (!S_ISDIR(mip->INODE.i_mode))
    {
      printf("Error: %s not a directory!\n", pathname);
      iput(mip);
      return -1;
    }
    iput(running->cwd);
    running->cwd = mip;
  }

  int ls_file(MINODE *mip, char *fname)
  {
    INODE *ip = &mip->INODE;
    
    // READ Chapter 11.7.3 HOW TO ls
    char *t1 = "xwrxwrxwr-------";
    char *t2 = "----------------";

    int r, i;
    char *ftime;

    if ((ip->i_mode & 0xF000) == 0x8000)
      printf("-");
    if ((ip->i_mode & 0xF000) == 0x4000)
      printf("d");
    if ((ip->i_mode & 0xF000) == 0xA000)
      printf("l");
    for (i = 8; i >= 0; i--)
    {
      if (ip->i_mode & (1 << i))
        printf("%c", t1[i]);
      else
        printf("%c", t2[i]);
    }
    printf("%4d ", ip->i_links_count);
    struct passwd *pwd;
    struct group *grp;
    if ((pwd = getpwuid(ip->i_uid)))
      printf(" %8.8s", pwd->pw_name);
    else
      printf(" %4d", ip->i_gid);

    if ((grp = getgrgid(ip->i_gid)))
      printf(" %8.8s", grp->gr_name);
    else
      printf(" %4d", ip->i_gid);

    printf("%8d ", ip->i_size);
    
    ftime = ctime((time_t*)&ip->i_mtime); // Gets time
    ftime[strlen(ftime)-1] = 0; // kill \n at end
    printf("%s ", ftime);  // print time

    printf("%s", basename(fname));
    printf("\t\t[%d %d]", mip->dev, mip->ino);

    if ((ip->i_mode) == 0120000)
    {
      printf("=>%s", (char*)ip->i_block);
    }
    
    printf("\n");
  }

  int ls_dir(MINODE *mip)
  {

    char buf[BLKSIZE], temp[256];
    DIR *dp;
    char *cp;

    get_block(dev, mip->INODE.i_block[0], buf);
    dp = (DIR *)buf;
    cp = buf;
    
    while (cp < buf + BLKSIZE){
      int ino = dp->inode;
      MINODE *mip = iget(dev, ino);

      strncpy(temp, dp->name, dp->name_len);
      temp[dp->name_len] = 0;

      ls_file(mip, temp);
      iput(mip);

      cp += dp->rec_len; // traverse dirs
      dp = (DIR *)cp;
    }
    printf("\n");
  }

  int ls()
  {
    if (strcmp(pathname, "") == 0)
    {
      ls_dir(running->cwd);
      return 1;
    }
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);
    if (!S_ISDIR(mip->INODE.i_mode))
    {
      printf("Error: %s not a directory!\n", pathname);
      return -1;
    }
    ls_dir(mip);
    iput(mip);
  }

  void pwd(MINODE *wd)
  {
    if (wd == root){
      printf("/\n");
      return;
    }
    else
    {
      rpwd(wd);
      puts("");
    }
  }

  void rpwd(MINODE *wd)
  {
    if (wd == root)
    {
      return;
    }
    
    int my_ino;
    int parent_ino = findino(wd, &my_ino);  // get ino of current and parent
    
    MINODE *pip = iget(dev, parent_ino);  // get parent minode

    char my_name[128] = "";
    findmyname(pip, my_ino, my_name); // get name of where we are

    rpwd(pip);
    printf("/%s", my_name);
    return;
  }