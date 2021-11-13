
int link()
{

    // get old_file path and new_file path
    char old_file[128];
    char new_file[128];

    int i;
    char *s;
    printf("link: tokenize %s\n", line);

    s = strtok(line, " ");  // get rid of the cmd
    s = strtok(NULL, " ");
    strcpy(old_file, s);
    s = strtok(NULL, " ");
    strcpy(new_file, s);
    
    printf("link: old file: %s\tnew file: %s\n", old_file, new_file);

    // 1. verify old_file exists and is not a DIR
    int oino = getino(old_file);
    if (oino == 0)
    {
        printf("link: %s does not exist\n", old_file);
        return -1;
    }
    MINODE* omip = iget(dev, oino);
    if (S_ISDIR(omip->INODE.i_mode))  // check if old file is a dir
    {
        printf("link: Cannot link DIR %s\n", old_file);
        return -1;
    }

    // 2. new file must not exist yet
    if (getino(new_file) != 0)  // must return 0
    {
        printf("link: %s already exists\n", new_file);
        return -1;
    }

    // 3. creat new_file with the same inode number of old_file
    char *parent = dirname(new_file);
    char *child = basename(new_file);

    int pino = getino(parent);
    MINODE *pmip = iget(dev, pino);
    printf("link: parent INODE %d\n", pino);

    // creat entry in new parent DIR with same inode number of old_file
    enter_name(pmip, oino, child);

    // 4. 
    printf("link: omip link count++\n");
    printf("link: omip %d\n", omip->ino);
	INODE *oip = &omip->INODE;
	oip->i_links_count++;    // inc INODE's links_count by 1
    omip->dirty = 1;                // for write back by iput(omip)
    printf("link: oldfile link count = %d\n", omip->INODE.i_links_count);

    printf("link: old INODE %d\tnew INODE %d\n", oino, getino(new_file));
    iput(omip);
    iput(pmip);
    return 1;
}

int unlink()
{
    // 1. get filename's minode
    int ino = getino(pathname);
    MINODE *mip = iget(dev, ino);
    // check its a REG or symbolic LINK file; can not be a DIR
    if (S_ISDIR(mip->INODE.i_mode))
    {
        printf("link: Cannot unlink DIR %s\n", pathname);
        return -1;
    }

    // 2. remove name entry from parent DIR's data block:
    char *parent = dirname(pathname);
    char *child = basename(pathname);
    int pino = getino(parent);
    MINODE *pmip = iget(dev, pino);

    rm_child(pmip, child);

    pmip->dirty = 1;
    iput(pmip);

    // 3. decrement INODE's link_count by 1
    mip->INODE.i_links_count--;

    // 4.
    if (mip->INODE.i_links_count > 0)
    {
        mip->dirty = 1;     // for write INODE back 
    }
    // 5.   if links_count = 0: remove filename
    else
    {
        // deallocate all data blocks in INODE
        bdalloc(dev, pmip->ino); // Deallocate the data block
        // deallocate INODE
        idalloc(dev, pmip->ino); // Deallocate the data block
    }

    iput(mip);      // release mip

}

