int my_rmdir(char* pathname)
{
    // 1. Get in-memory INODE of pathname ================================================
    int ino = getino(pathname); 

    if (ino == 0) // return error if ino == 0;
    {
      printf("Directory does not exists\n"); // Checks if the ino exists in the dirname
      return -1;
    }

    MINODE *mip = iget(dev, ino);   // Gets the in-memory INODE of pathname
    INODE *ip = &mip->INODE;

    // Divide pathname into dirname and basename, then dirname=/a/b; basename = c;
    char *bpath = basename(pathname);
    char *dpath = dirname(pathname);
    printf("dirname = %s\n", dpath);
    printf("basename = %s\n", bpath);


    // 2. verify INODE is a DIR ========================================================
    if (!S_ISDIR(mip->INODE.i_mode))
    {
        printf("Error: %s not a directory!\n", pathname);
        iput(mip);
        return -1;
    }

    // check if minode is busy
    printf("%d mip->refCount\n", mip->refCount);
    if (mip->refCount != 1)
    {
        printf("MINODE is busy!\n");
        iput(mip);
        return -1;
    }

    // verify DIR is empty
    printf("%d link count\n", ip->i_links_count);
    if (ip->i_links_count > 2)
    {
        printf("%s is not empty!\n", bpath);
        iput(mip);
        return -1;
    }

    // Check if dir is empty (thorough)
    int i;
    char *cp, c, buf[BLKSIZE], name[256];
    DIR *dp;
    int dirIsEmpty = 1;

    // Using search function to iterate
    get_block(dev, ip->i_block[0], buf);
    dp = (DIR *)buf;
    cp = buf;

    while (cp < buf + BLKSIZE)
    {
      strncpy(name, dp->name, dp->name_len);
      name[dp->name_len] = 0;

      if (strcmp(name, ".") != 0 && strcmp(name, "..") != 0)
      {
        dirIsEmpty = 0;
        break;
      }

      cp += dp->rec_len;
      dp = (DIR *)cp;
    }
    if (dirIsEmpty == 0)
    {
      printf("Directory not empty, unable to delete.\n");
      iput(mip);
      return 1;
    }

    // 3. get parents ino and inode =========================================================
    printf("rmdir: ino = %d\n", ino);
    int pino = findino(mip, &ino);
    printf("rmdir: ino after get parent = %d\n", ino);
    printf("rmdir: pino = %d\n", pino);
    MINODE *pmip = iget(mip->dev, pino); 
    INODE *pip = &pmip->INODE;

    // 4. get name from parent DIR's data block =============================================
    char pname[128];
    findmyname(pmip, ino, pname);

    // 5. remove name from parent directory ================================================
    rm_child(pmip, pname);

    // 6. dec parent links_count by 1; mark parent pmip dirty ============================
    printf("current parent link count: %d\n", pmip->INODE.i_links_count);
    printf("dec parent link count by 1\n");
    pip->i_links_count--; 
    //pmip->INODE.i_atime = pmip->INODE.i_mtime = pmip->INODE.i_ctime = time(0L);
    pmip->dirty = 1; 
    printf("updated parent link count: %d\n", pmip->INODE.i_links_count);

    iput(pmip);

    // 7. deallocate its datablocks and inode =============================================
    bdalloc(mip->dev, pip->i_block[0]);
    idalloc(mip->dev, mip->ino);
    iput(mip);

    printf("Removed %s\n", bpath);
    return 1; 
}

int rm_child(MINODE *pmip, char *name)
{
    char *cp;
    char c; 
    char *last_cp; 
    char buf[BLKSIZE];
    char temp_name[256];
    DIR *prev_dp, *last_dp;
    DIR *dp;
    INODE *pip;
    int i;
    int found = 0;

    pip = &(pmip->INODE);

    // 1. search parent INODE's data blocks for the entry of name =================================

    for (i = 0; i < 12 && found == 0; i++)
    {
        if (pip->i_block[i] == 0)
        {
            return -1;
        }

        get_block(dev, pip->i_block[i], buf);
        printf("rm_child: pip->iblock = %d\n", i);

        dp = (DIR *)buf;
        cp = buf;

        // iterate through DIR entries in block
        while (cp < buf + BLKSIZE)
        {
            strncpy(temp_name, dp->name, dp->name_len);
            temp_name[dp->name_len] = 0;
            printf("rm_child: checking for dir %s at %s\n", name, temp_name);
            if (strcmp(temp_name, name) == 0) // Searching for entry to remove
            {
                printf("rm_child: found dir %s in pip->i_block[%d]\n\n", name, i);
                i--;
                found = 1;
                break;
            }
            
            // Increment to the next entry
            prev_dp = dp;
            cp += dp->rec_len;
            dp = (DIR *)cp;
        }
    }

    // 2. delete name entry from parent directory

    // case 1: first and only entry in a data block (ignoring . and ..)
    if (dp->rec_len == BLKSIZE - 24) 
    {
        printf("First and Only entry in block!\n");

        // 1. deallocate the data block
        bdalloc(dev, pmip->ino);

        // 2. reduce parent's file size by BLKSIZE
        pip->i_size -= BLKSIZE; 

        // 3. compact parent's i_block[ ] array to eliminate the deleted entry if it's between nonzero entries
        printf("previous = [%d %s] LAST entry = [%d %s]\n", prev_dp->rec_len, prev_dp->name, dp->rec_len, dp->name);
        printf("add LAST rec_len=%d to previous rec_len=%d ==> %d\n", dp->rec_len, prev_dp->rec_len, dp->rec_len + prev_dp->rec_len);
        prev_dp->rec_len += dp->rec_len;

        put_block(dev, pip->i_block[i], buf);
    }
    // case 2: last entry in data block
    else if (cp + dp->rec_len == buf + BLKSIZE) // Last entry in block
    {
        printf("Last entry in the block!\n");
        prev_dp->rec_len += dp->rec_len; // Absorb its rec_len to the predecessor entry
        put_block(dev, pip->i_block[i], buf);
    }
    // case 3: first but not the only one, or its in the middle of the data block
    else 
    {
        printf("Entry is first but not the only entry or in the middle of the block!\n");

        // Move all trailing entries LEFT to overlay the deleted entry; ================================================

        // traverse to last entry and keep track of size
        int size = 0;
        last_dp = dp;
        last_cp = cp;

        // ignore current cp/dp as thats the one we want to delete
        last_cp += last_dp->rec_len;
        last_dp = (DIR*)last_cp;

        // iterating over all entries to the right of the one we're trying to delete
        while (last_cp + last_dp->rec_len < buf + BLKSIZE)
        {
            printf("rm_child: stepping over %s size %d\n", last_dp->name, last_dp->rec_len);
            size += last_dp->rec_len;
            last_cp += last_dp->rec_len;
            last_dp = (DIR*)last_cp;
        }
        printf("\nrm_child: reached last entry: %s size %d\n", last_dp->name, last_dp->rec_len);
        size += last_dp->rec_len;   // fencepost bug
        printf("rm_child: size to shift: %d\n", size);

        int deleted_len = dp->rec_len;  // save deleted size to add onto tail
        printf("rm_child: dp before shift: %s size %d\n", dp->name, dp->rec_len);
        printf("rm_child: last_dp before shift: %s size %d\n", last_dp->name, last_dp->rec_len);
        memcpy(cp, cp + dp->rec_len, size);           // shift left

        // Move the rec_len to the previous rec_length
        // add deleted rec_len to the LAST entry
        last_cp -= deleted_len;
        last_dp = (DIR*)last_cp;

        last_dp->rec_len += deleted_len;

        printf("rmdir: dp after shift: %s size %d\n", dp->name, dp->rec_len);
        printf("rmdir: last_dp after shift: %s size %d\n", last_dp->name, last_dp->rec_len);

        put_block(dev, pip->i_block[i], buf);
    }
}

int idalloc(int dev, int ino)
{
  char buf[BLKSIZE];

  if (ino > ninodes){  
    printf("inumber %d out of range\n", ino);
    return -1;
  }
  
  get_block(dev, imap, buf);  // get inode bitmap block into buf[]
  printf("idalloc: deallocate inode #%d\n", ino);

  clr_bit(buf, ino-1);        // clear bit ino-1 to 0

  put_block(dev, imap, buf);  // write buf back

  incFreeInodes(dev);
}

int clr_bit(char *buf, int bit) // clear bit in char buf[BLKSIZE]
{
  buf[bit / 8] &= ~(1 << (bit % 8));
}

int incFreeInodes(int dev)
{
  char buf[BLKSIZE];
  // inc free inodes count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_inodes_count++;
  put_block(dev, 1, buf);
  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_inodes_count++;
  put_block(dev, 2, buf);
}

int bdalloc(int dev, int bno)
{
  int i;  
  char buf[BLKSIZE];

  if (bno > nblocks){  
    printf("bnumber %d out of range\n", bno);
    return -1;
  }
  
  get_block(dev, bmap, buf);  // get block from bmap
  printf("bdalloc: deallocate data block #%d\n", bno);

  clr_bit(buf, bno-1);        // clear bit bno-1 to 0

  put_block(dev, bmap, buf);  // write buf back

  incFreeBlocks(dev);
}

int incFreeBlocks(int dev)
{
  char buf[BLKSIZE];
  // inc free block count in SUPER and GD
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;
  sp->s_free_blocks_count++;
  put_block(dev, 1, buf);
  get_block(dev, 2, buf);
  gp = (GD *)buf;
  gp->bg_free_blocks_count++;
  put_block(dev, 2, buf);
}