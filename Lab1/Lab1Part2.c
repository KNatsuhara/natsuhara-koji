#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

struct partition {
	u8 drive;             // drive number FD=0, HD=0x80, etc.

	u8  head;             // starting head 
	u8  sector;           // starting sector
	u8  cylinder;         // starting cylinder

	u8  sys_type;         // partition type: NTFS, LINUX, etc.

	u8  end_head;         // end head 
	u8  end_sector;       // end sector
	u8  end_cylinder;     // end cylinder

	u32 start_sector;     // starting sector counting from 0 
	u32 nr_sectors;       // number of of sectors in partition
};

char *dev = "vdisk";
int fd;
    
// read a disk sector into char buf[512]
int read_sector(int fd, int sector, char *buf)
{
    lseek(fd, sector*512, SEEK_SET);  // lssek to byte sector*512
    read(fd, buf, 512);               // read 512 bytes into buf[ ]
}

int main()
{
  struct partition *p;
  char buf[512];

  fd = open(dev, O_RDONLY);   // open dev for READ
  read_sector(fd, 0, buf);    // read in MBR at sector 0    

  p = (struct partition *)(&buf[0x1be]); // p->P1
   
  // print P1's start_sector, nr_sectors, sys_type;
  printf("P1: start_sector=%d\tnr_sectors=%d\tsys_type=%d\n\n", p->start_sector, p->nr_sectors, p->sys_type);
  // Write code to print all 4 partitions;
  printf("\tstart_sector\tend_sector\tnr_sectors\n");
  for (int i = 1; i < 5; i++)
  {
      printf("P%d:\t%d\t\t%d\t\t%d\n", i, p->start_sector, p->start_sector+p->nr_sectors-1, p->nr_sectors);
      p++;
  }

  p = (struct partition *)(&buf[0x1be]); // p->P1

  for (int i = 1; i < 5; i++)
  {
      printf("[%d %x]\t", i, p->sys_type);
      p++;
  }
  printf("\n");

  // ASSUME P4 is EXTEND type:
  p--;                                     // Bringng p back to point to P4
  int extStart = p->start_sector;          // Let  int extStart = P4's start_sector;
  printf("Ext Partition 4: start_sector=%d\n", extStart);     // print extStart to see it;
  printf("Enter any key to continue : ");
  getchar();
  printf("Next localMBR sector=%d\n", extStart);

  int localMBR = extStart;
  int tempExtStart = extStart;
  int numPartition = 5;
  int currStartSect = 0;
  int currEndSect = 0;
  int currNRSect = 0;

  while (p->sys_type != 0) 
  {
      printf("-------------------------------------\n");
      read_sector(fd, localMBR, buf);    // read in MBR at sector
      p = (struct partition *)(&buf[0x1be]); // p->P4
      printf("Entry1 : start_sector=%d\tnr_sectors=%d\n", p->start_sector, p->nr_sectors);
      currStartSect = p->start_sector + extStart; // Gets start_sector for current partition
      currNRSect = p->nr_sectors;
      p++;
      printf("Entry2 : start_sector=%d\tnr_sectors=%d\n", p->start_sector, p->nr_sectors);
      currEndSect = currStartSect+currNRSect-1;
      printf("-------------------------------------\n");
      if (p->start_sector != 0)
      {
          printf("\tstart_sector\tend_sector\tnr_sectors\n");               //Prints out header for sectors
          printf("P%d:\t%d\t\t%d\t\t%d\n\n", numPartition, currStartSect, currEndSect, currNRSect); //Prints out the values of each sector in the partition
          numPartition++;                                                       //Increment the partition number we are on
          printf("Next localMBR sector = %d + %d = %d\n", tempExtStart, p->start_sector, tempExtStart+p->start_sector); //Prints out the location of the next localMBRSector
          printf("Next localMBR sector=%d\n", tempExtStart+p->start_sector);
          localMBR=tempExtStart+p->start_sector;
          extStart = localMBR;                                      //Set/accumulate extStart = localMBR to find the next extended partition
      }
      else
      {
          printf("\tstart_sector\tend_sector\tnr_sectors\n");
          printf("P%d:\t%d\t\t%d\t\t%d\n\n", numPartition, currStartSect, currEndSect, currNRSect);
          printf("End of Extend Partitions\n");
      }
  }
  printf("===================================================\n");
}
