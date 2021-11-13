int my_mkdir()
  {
      // Divide pathname into dirname and basename, then dirname=/a/b; basename = c;
      char *bpath = basename(pathname);
      char *dpath = dirname(pathname);
      printf("dirname = %s\n", dpath);
      printf("basename = %s\n", bpath);

      //Checking if dirname exists and is a DIR
      int pino = getino(dpath);

      if (pino == 0) // return error if ino == 0;
      {
          printf("%s does not exists\n", dpath); // Checks if the ino exists in the dirname
          return -1;
      }

      MINODE *pmip = iget(dev, pino); // Gets the parent INODE

      if (S_ISDIR(pmip->INODE.i_mode)) // Check if mip->INODE is a dir
      {
          if (search(pmip, bpath) == 0) // Checks to make sure that basename does not already exist
          {
              kmkdir(pmip, bpath); // Calls kmkdir to creat a new directory
          }
          else
          {
              printf("%s already exists!\n", bpath);
              return -1;
          }
      }
      else
      {
          printf("%s is not a directory!\n", dpath);
          return -1;
      }

      return 0;
  }

  int kmkdir(MINODE *pmip, char *bpath)
  {
      // Allocate an INODE and a disk block
      int ino = ialloc(dev);
      int blk = balloc(dev);

      printf("ino=%d\nblk=%d\n", ino, blk);

      MINODE *mip = iget(dev, ino); // load INODE into a minode

      INODE *ip = &mip->INODE;
      ip->i_mode = 0x41ED;      // 040755: DIR type and permissions
      ip->i_uid = running->uid; // owner uid
      ip->i_gid = running->gid; // group Id
      ip->i_size = BLKSIZE;     // size in bytes
      ip->i_links_count = 2;    // links count=2 because of . and ..
      ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
      ip->i_blocks = 2;     // LINUX: Blocks count in 512-byte chunks
      ip->i_block[0] = blk; // new DIR has one data block

      for (int i = 1; i < 15; i++)
      {
          ip->i_block[i] = 0; // ip->i_block[1] to ip->i_block[14] = 0;
      }
      mip->dirty = 1; // mark minode dirty
      iput(mip);      // write INODE to disk

      // make data block 0 of INODE to contain . and .. entries

      char buf[BLKSIZE];
      bzero(buf, BLKSIZE); // optional: clear buf[ ] to 0
      DIR *dp = (DIR *)buf;

      // make . entry
      dp->inode = ino;
      dp->rec_len = 12;
      dp->name_len = 1;
      dp->name[0] = '.';
      

      // make .. entry: pino=parent DIR ino, blk=allocated block
      dp = (char *)dp + 12;
      dp->inode = pmip->ino;
      dp->rec_len = BLKSIZE - 12; // rec_len spans block
      dp->name_len = 2;
      dp->name[0] = '.';
      dp->name[1] = '.';
      put_block(dev, blk, buf); // write to blk on disk

      enter_name(pmip, ino, bpath); // enter_child(pmip, ino, basename) which enters (ino, basename) as a dir_entry to the parent INODE

      //increment parent INODE's links_count by 1 and mark pmip dirty;
      pmip->INODE.i_links_count++;
      pmip->INODE.i_atime = time(0L);
      pmip->dirty = 1;

      iput(pmip);

      return 0;
  }

  // tst_bit, set_bit functions
  int tst_bit(char *buf, int bit)
  {
      return buf[bit / 8] & (1 << (bit % 8));
  }

  int set_bit(char *buf, int bit)
  {
      buf[bit / 8] |= (1 << (bit % 8));
  }

  int decFreeInodes(int dev)
  {
      char buf[BLKSIZE];
      // dec free inodes count in SUPER and GD
      get_block(dev, 1, buf);
      sp = (SUPER *)buf;
      sp->s_free_inodes_count--; // Decrement free_inodes_count in super
      put_block(dev, 1, buf);
      get_block(dev, 2, buf);
      gp = (GD *)buf;
      gp->bg_free_inodes_count--; // Decrement free_inodes_count in group descriptor
      put_block(dev, 2, buf);
  }

  int decFreeBlocks(int dev)
  {
      char buf[BLKSIZE];
      // dec free blocks count in SUPER and GD
      get_block(dev, 1, buf);
      sp = (SUPER *)buf;
      sp->s_free_blocks_count--; // Decrement free_block_count in super
      put_block(dev, 1, buf);
      get_block(dev, 2, buf);
      gp = (GD *)buf;
      gp->bg_free_blocks_count--; // Decrement free_block_count in group descriptor
      put_block(dev, 2, buf);
  }

  int ialloc(int dev) // allocate an inode number from inode_bitmap
  {
      int i;
      char buf[BLKSIZE];

      // read inode_bitmap block
      get_block(dev, imap, buf);

      for (i = 0; i < ninodes; i++)
      {
          if (tst_bit(buf, i) == 0)
          {
              set_bit(buf, i);
              decFreeInodes(dev);
              put_block(dev, imap, buf);
              printf("allocated ino = %d\n", i + 1); // bits count from 0; ino from 1
              return i + 1;
          }
      }
      printf("ERROR: OUT OF FREE INDOES\n");
      return 0; // out of FREE INODES
  }

  int balloc(int dev) // allocates a free disk block (number) from a device and returns a FREE disk block number
  {
      int i;
      char buf[BLKSIZE];

      // read block from block bitmap
      get_block(dev, bmap, buf);

      for (i = 0; i < nblocks; i++)
      {
          if (tst_bit(buf, i) == 0)
          {
              set_bit(buf, i);
              decFreeBlocks(dev); // Decrements the free blocks count in superblock and group descriptor
              put_block(dev, bmap, buf);
              return i;
          }
      }
      printf("ERROR: OUT OF FREE BLOCKS\n");
      return 0; // out of FREE blocks
  }

  int enter_name(MINODE *pip, int ino, char *name)
  {
      // for each data block of parent DIR do // assume: only 12 direct blocks

      INODE *parent_ip = &pip->INODE;
      char buf[BLKSIZE];
      int bno = 0;
      int ideal_length = 0;
      int need_length = 0;
      int remain = 0;
      int p_block = 0;

      DIR *dp;
      char *cp;

      for (int i = 0; i < 13; i++)
      {
          if (parent_ip->i_block[i] == 0)
          {
              p_block = i;
              break; // if i_block[i] == 0 BREAK; // to step (5) below
          }

          need_length = 4 * ((8 + strlen(name)+3) / 4);  // Calculate the needed length based on the name of the new DIR
          printf("need line is %d\n", need_length); // In order to enter a new entry of name with n_len


          // Step to the last entry in the data block:
          bno = parent_ip->i_block[i]; // get the current parent node block number

          get_block(dev, bno, buf); // get parent's data block into a buf[];

          dp = (DIR *)buf; // cast the parent data block into a directory pointer
          cp = buf; // Set cp also to point to the same memory block as dp

          while (cp + dp->rec_len < buf + BLKSIZE)
          {
              cp += dp->rec_len;
              dp = (DIR *)cp;
          }

          // dp NOW points at last entry in block
          printf("last entry is %s\n", dp->name);
          printf("new entry is %s\n", name);
          cp = (char *)dp;

          ideal_length = 4 * ((8 + dp->name_len+3) / 4); // ideal length uses name len of last dir entry to calculate how to squeeze new entry into remaining space

          printf("ideal length %d\n", ideal_length);
          printf("need length %d\n", need_length);

          remain = dp->rec_len - ideal_length; // remain = LAST entry's rec_len - its ideal_length;
          printf("remain is %d\n", remain);

          if (remain >= need_length)
          {
              dp->rec_len = ideal_length; // set rec_len to ideal, set that last entries length to ideal length

              // set the current pointer equal to the end of ideal, now we are setting dp to the end of the last record
              cp += dp->rec_len;
              dp = (DIR *)cp;

              dp->name_len = strlen(name); // sets the string length of name to the given pathname
              dp->inode = ino; // Sets dp to the given ino
              dp->file_type = EXT2_FT_DIR; // Sets file type to DIR
              dp->rec_len = BLKSIZE - (cp - buf); // trim the previous rec_len to its ideal length;
              strcpy(dp->name, name); // Set dp name to given name

              put_block(dev, bno, buf); // new dir is ready to be placed in memory and we put in block
              return 1;
          }
      }

      bno = balloc(dev);           // no space in existing data blocks
      parent_ip->i_block[p_block] = bno; // set parents next i_block to the newly allocated block number

      parent_ip->i_size += BLKSIZE;
      pip->dirty = 1;

      get_block(dev, bno, buf); //get the new block into buf

      dp = (DIR *)buf; // Cast buf as a dir entry
      cp = buf;

      dp->inode = ino;             //set inode number of the new block to the child ino
      dp->rec_len = BLKSIZE;       // set the default record length
      dp->name_len = strlen(name); //set the name length to the length of the file
      dp->file_type = EXT2_FT_DIR; // Set to directory type
      strcpy(dp->name, name);      // Copy the name of the dir
      put_block(dev, bno, buf);    // Write dat block to disk

      return 1;
  }

  int my_creat(char *pathname)
  {
      // Divide pathname into dirname and basename, then dirname=/a/b; basename = c;
      char *bpath = basename(pathname);
      char *dpath = dirname(pathname);
      printf("dirname = %s\n", dpath);
      printf("basename = %s\n", bpath);

      //Checking if dirname exists and is a DIR
      int pino = getino(dpath);

      if (pino == 0) // return error if ino == 0;
      {
          printf("%s does not exists\n", dpath); // Checks if the ino exists in the dirname
          return -1;
      }

      MINODE *pmip = iget(dev, pino); // Gets the parent INODE

      if (S_ISDIR(pmip->INODE.i_mode)) // Check if mip->INODE is a dir
      {
          if (search(pmip, bpath) == 0) // Checks to make sure that basename does not already exist
          {
              kcreat(pmip, bpath); // Calls kcreat to creat a new directory
          }
          else
          {
              printf("%s already exists!\n", bpath);
              return -1;
          }
      }
      else
      {
          printf("%s is not a directory!\n", dpath);
          return -1;
      }

      return 0;
  }

  int kcreat(MINODE *pmip, char *bpath)
  {
      // Allocate an INODE and a disk block
      int ino = ialloc(dev);
      int bno = balloc(dev);

      printf("ino=%d\n", ino);

      // mip = iget(dev, ino) // load INODE into a minode
      MINODE *mip = iget(dev, ino); // load INODE into a minode

      INODE *ip = &mip->INODE;
      ip->i_mode = 0x81A4;      // FILE type and permissions
      ip->i_uid = running->uid; // owner uid
      ip->i_gid = running->gid; // group Id
      ip->i_size = 0;     // File size of 0 (EMPTY FILE)
      ip->i_links_count = 1;    // links count=1 links to parent directory
      ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
      ip->i_blocks = 0;     // LINUX: Blocks count in 512-byte chunks (set to 0 due to no . and ..)

      for (int i = 0; i < 15; i++)
      {
          ip->i_block[i] = 0; // ip->i_block[0] to ip->i_block[14] = 0; NO DATA BLOCK IS ALLOCATED
      }
      mip->dirty = 1; // mark minode dirty
      iput(mip);      // write INODE to disk

      enter_name(pmip, ino, bpath); // enter_child(pmip, ino, basename) which enters (ino, basename) as a dir_entry to the parent INODE

      pmip->INODE.i_atime = time(0L);
      pmip->dirty = 1;

      iput(pmip);

      return 0;
  }