int symlink()
{
    // 1. check old_file must exist and new_file not yet exist
    int oino;
    MINODE* omip;

    // get old_file path and new_file path
    char old_file[128];
    char new_file[128];

    int i;
    char *s;
    printf("symlink: tokenize %s\n", line);

    s = strtok(line, " ");  // get rid of the cmd
    s = strtok(NULL, " ");
    strcpy(old_file, s);
    s = strtok(NULL, " ");
    strcpy(new_file, s);
    
    printf("symlink: old file: %s\tnew file: %s\n", old_file, new_file);

    // 1. verify old_file exists and is not a DIR
    oino = getino(old_file);
    if (oino == 0)
    {
        printf("symlink: %s does not exist\n", old_file);
        return -1;
    }
    omip = iget(dev, oino);

    // 2. creat new_file. change new_file to LNK type
    my_creat(new_file);
    INODE *ino = getino(new_file);
    MINODE *mip = iget(dev, ino);
    INODE *ip = &mip->INODE;
    ip->i_mode = 0xA000;       // LNK file type
    
    // 3. assume length of old_file name <= 60 chars

    // store old_file name in newfile's INODE.i_block[ ] area
    char *parent = dirname(old_file);

    int pino = getino(parent);
    MINODE *pmip = iget(dev, pino);

    char name[128];
    findmyname(pmip, oino, name);
    strcpy(ip->i_block, name);

    // set file size to length of old_file name
    ip->i_size = strlen(name);

    mip->dirty = 1;
    iput(mip);

    // 4. 
    pmip->dirty = 1;        // for write back by iput(pmip)
    iput(pmip);

}

int readlink()
{
    int ino = getino(pathname);  // get the ino of target file
    MINODE *mip = iget(dev, ino);   // gets the midode pointer of pathname
    INODE  *ip = &mip->INODE;    // sets the inode of MINODE

    char * result = (char *)(ip->i_block); // get the i_block
    result[strlen(result)]='/0';           // sets the last character to result to null
    printf("-> %s \n", result);               // prints the result

    // get file's INODE in memory; verify it's a link file
    // copy target filename from INODE.i_block[] into buffer
    // return file size
}