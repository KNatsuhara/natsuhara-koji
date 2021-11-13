/*************** type.h file for LEVEL-1 ****************/
typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef struct ext2_super_block SUPER;
typedef struct ext2_group_desc  GD;
typedef struct ext2_inode       INODE;
typedef struct ext2_dir_entry_2 DIR;

SUPER *sp;
GD    *gp;
INODE *ip;
DIR   *dp;   

#define FREE        0
#define READY       1

#define BLKSIZE  1024
#define NMINODE   128
#define NPROC       2

typedef struct minode{
  INODE INODE;           // INODE structure on disk
  int dev, ino;          // (dev, ino) of INODE
  int refCount;          // in use count
  int dirty;             // 0 for clean, 1 for modified

  int mounted;           // for level-3
  struct mntable *mptr;  // for level-3
}MINODE;

typedef struct proc{
  struct proc *next;
  int          pid;      // process ID  
  int          uid;      // user ID
  int          gid;
  int          status;
  int          ppid;
  MINODE      *cwd;      // CWD directory pointer  
}PROC;

// util
// int findino(MINODE *mip, u32 *myino);
int lseek(int dev, long blk, int buf);
int read(int dev, char *buf, int size);
int write(int dev, char *buf, int size);
int get_block(int dev, int blk, char *buf);
int tokenize(char *pathname);
int put_block(int dev, int blk, char *buf);
MINODE *iget(int dev, int ino);
void iput(MINODE *mip);
int search(MINODE *mip, char *name);
int getino(char *pathname);
int findmyname(MINODE *parent, u32 myino, char *myname);

// cd_ls_pwd
int cd();
int ls_file(MINODE *mip, char *name);
int ls_dir(MINODE *mip);
int ls();
void rpwd(MINODE *wd);
void pwd(MINODE *wd);

// main
int quit();


// mkdir_creat
int mkdir_creat();
int kmkdir(MINODE *pmip, char *bpath);
int tst_bit(char *buf, int bit);
int set_bit(char *buf, int bit);
int decFreeInodes(int dev);
int decFreeBlocks(int dev);
int ialloc(int dev);
int balloc(int dev);
int enter_name(MINODE *pip, int ino, char *name);
int file_creat();
// int kcreat(MINODE *pmip, char *bpath);

// link_unlink
int link();
int unlink();
int symlink();

// rmdir
int rmdir();
int checkDirIsEmpty(INODE *ip);
int rm_child(MINODE *pmip, char *name);
int idalloc(int dev, int ino);
int clr_bit(char *buf, int bit);
int incFreeInodes(int dev);
int bdalloc(int dev, int bno);
int incFreeBlocks(int dev);
