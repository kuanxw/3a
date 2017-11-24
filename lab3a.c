/* NAME: Kuan Xiang Wen, Amir Saad
 * EMAIL: kuanxw@g.ucla.edu, arsaad@g.ucla.edu
 * ID: 004461554, 604840359
 */

//Include the following libraries
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h> 
#include <assert.h>
#include <math.h>
#include <time.h>

//Special header file
#include "ext2_fs.h"

//Global constants
const int EXIT_SUCC = 0;
const int EXIT_BAD_ARG = 1;
const int EXIT_ERROR = 2;
const int BASE_OFFSET = 1024;


//Global variables
static int image = -1; //File system image file descriptor
static int outfd = -1; //Output file descriptor
static unsigned int block_s = 0; //block lbyte_offset
static unsigned int num_groups = 0; //number of groups
int* inode_bitmap;

struct ext2_super_block sb; //super block struct
struct ext2_group_desc* group_desc; //group struct for group descriptors

#define BLOCK_OFFSET(block) (BASE_OFFSET + (block-1) * block_s)

//Function to format the time
void time_format(uint32_t timestamp, char* buffer) {
  time_t epoch = timestamp;
  struct tm ts = *gmtime(&epoch);
  strftime(buffer, 80, "%m/%d/%y %H:%M:%S", &ts);
}

//Free memory
void free_memory() {
  if(group_desc != NULL) {
    free(group_desc);
  } 
  if(inode_bitmap != NULL) {
    free(inode_bitmap);
  }
  close(image);
  close(outfd);
}


//Function to print error message to stderr
void print_error(const char* msg, int code) {
  fprintf(stderr, "%s\n %s\n", msg, strerror(code));
}

//superblock analysis
void superblock() {
  //Read superblock located at byte offset 1024 from the beginning
  int status0 = pread(image, &sb, sizeof(struct ext2_super_block), 1024);
  int test0 = errno;
  
  //Check if failed to read image
  if(status0 == -1) {
    print_error("Error could not read image", test0);
    exit(EXIT_ERROR);
  }

  //Check if we have an ext2 file system
  /*  if(sb.s_magic != EXT2_SUPER_MAGIC) {
    fprintf(stderr, "Error! Not an ext2 file system\n");
    exit(EXIT_ERROR);
    }*/
  
  block_s = EXT2_MIN_BLOCK_SIZE << sb.s_log_block_size;

  //Add summary to csv
  printf("SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", sb.s_blocks_count, sb.s_inodes_count, block_s, sb.s_inode_size, sb.s_blocks_per_group, sb.s_inodes_per_group, sb.s_first_ino);
}

//group analysis
void group() {
  num_groups = 1 + (sb.s_blocks_count - 1) / sb.s_blocks_per_group;
  unsigned int descr_list_size = num_groups * sizeof(struct ext2_group_desc);

  //Allocate memory for group descriptors
  group_desc = malloc(descr_list_size);
  int status1 = errno;

  //Check if malloc failed
  if(group_desc == NULL) {
    print_error("Error! malloc failed", status1);
    exit(EXIT_ERROR);
  }

  //Read image
  int test2 = pread(image, group_desc, descr_list_size, BASE_OFFSET + block_s);
  int status2 = errno;

  //Check if we failed to read image
  if(test2 == -1) {
    print_error("Error reading image file", status2);
    exit(EXIT_ERROR);
  }

  //Remainder variables for group blocks and inodes
  uint32_t blocks_r = sb.s_blocks_count;
  uint32_t blocks_in_group = sb.s_blocks_per_group;
  uint32_t inodes_r = sb.s_inodes_count;
  uint32_t inodes_in_group = sb.s_inodes_per_group;

  //Loop through groups
  unsigned int i;
  
  for(i = 0; i < num_groups; i++) {
    if(blocks_r < sb.s_blocks_per_group) {
      blocks_in_group = blocks_r;
    }

    if(inodes_r < sb.s_inodes_per_group) {
      inodes_in_group = inodes_r;
    }

    //Add to summary csv
    printf("GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n", i, blocks_in_group, inodes_in_group, group_desc[i].bg_free_blocks_count, group_desc[i].bg_free_inodes_count, group_desc[i].bg_block_bitmap, group_desc[i].bg_inode_bitmap, group_desc[i].bg_inode_table);

    //Update block and inode remainder variables
    blocks_r -= sb.s_blocks_per_group;
    inodes_r -= sb.s_inodes_per_group;
  }
}

//free block analysis
void free_block() {
  unsigned i;

  //Iterate through each group
  for(i = 0; i < num_groups; i++) {
    unsigned long k;
    for(k = 0; k < block_s; k++) {
      //Read one byte at a time from the block's bitmap
      uint8_t byte;
      int status3 = pread(image, &byte, 1, (block_s * group_desc[i].bg_block_bitmap) + k);
      int test3 = errno;
      //Check if the pread failed
      if(status3 == -1) {
        print_error("Error! Failed to read image file", test3);
        exit(EXIT_ERROR);
      }

      int mask = 1;
      unsigned long j;
      //Examine the byte bit by bit, with zero indicating a free block
      for(j = 0; j < 8; j++) {
        if((byte & mask) == 0) {
          fprintf(stdout, "BFREE,%lu\n", i * sb.s_blocks_per_group + k*8 + j + 1);
        }
        mask <<= 1;
      }
    }
  }
}

//free inode analysis
void free_inode() {
  unsigned long i;
  //Iterate through each group
  inode_bitmap = malloc(sizeof(uint8_t) * num_groups * block_s);
  //Check if malloc failed
  if(inode_bitmap == NULL) {
    fprintf(stderr, "Error! malloc failed");
    exit(EXIT_ERROR);
  }

  for(i = 0; i < num_groups; i++) {
    unsigned long j;
    
    for(j = 0; j < block_s; j++) {
      //Read one byte at a time from the block's bitmap
      uint8_t byte;
      int status4 = pread(image, &byte, 1, (block_s * group_desc[i].bg_inode_bitmap) + j);
      int test4 = errno;
      
      //Check if pread failed
      if(status4 == -1) {
        print_error("Error! Failed to read image", test4);
        exit(EXIT_ERROR);
      }

      inode_bitmap[i + j] = byte;
      int mask = 1;
      unsigned long k;

      //Examine the byte bit by bit, with zero indicating a free block
      for(k = 0; k < 8; k++) {
        if((byte & mask) == 0) {
          fprintf(stdout, "IFREE,%lu\n", i * sb.s_inodes_per_group + j*8 + k + 1);
        }
        mask <<= 1;
      }
    }
  }
}
/*
//indirect block analysis
void indirect_block(int owner_inode_num, uint32_t block_num, int level, uint32_t current_offset) {
  uint32_t num_entries = block_s / sizeof(uint32_t);
  uint32_t entries[num_entries];

  memset(entries, 0, sizeof(entries));

  int status5 = pread(image, entries, block_s, BLOCK_OFFSET(block_num));
  int test5 = errno;

  if(status5 == -1) {
    print_error("Error! Failed to read image", test5);
    exit(EXIT_ERROR);
  }

  assert(level == 1 || level == 2 || level == 3);

  unsigned int i;
  for(i = 0; i < num_entries; i++) {
    if(entries[i] != 0) {
      printf("INDIRECT,%u,%u,%u,%u,%u\n", owner_inode_num, level, current_offset, block_num, entries[i]);

      if(level == 1) {
        current_offset++;
      } 
      else if(level == 2 || level == 3) {
        current_offset += level == 2 ? 256 : 65536;
        indirect_block(owner_inode_num, entries[i], level - 1, current_offset);
      }
    }
  }
}*/

//Analyze indirect block references in a directory. Returns logical block offset of inner values
int scan_dir_indirects(struct ext2_inode* inode, int inode_num, uint32_t block_num, unsigned int lbyte_offset, int level) {
  uint32_t num_entries = block_s/sizeof(uint32_t);
  uint32_t entries[num_entries];
  memset(entries, 0, sizeof(entries));
  
  int status7 = pread(image, entries, block_s, BLOCK_OFFSET(block_num));
  int test7 = errno;

  if(status7 == -1) {
    print_error("Error! Failed to read image", test7);
    exit(EXIT_ERROR);
  }

  unsigned char block[block_s];
  //struct ext2_dir_entry* entry;

  unsigned int i;
  int lblock_offset=0;
  
  for(i = 0; i < num_entries; i++) {
    if(entries[i] != 0) {
      if(level == 2 || level == 3) {
        if(lblock_offset == 0) lblock_offset = scan_dir_indirects(inode, inode_num, entries[i], lbyte_offset, level - 1);
        else scan_dir_indirects(inode, inode_num, entries[i], lbyte_offset, level - 1);
      }
      int status8 = pread(image, block, block_s, BLOCK_OFFSET(entries[i]));
      int test8 = errno;

      if(status8 == -1) {
        print_error("Error! Failed to read image", test8);
      }

      /*entry = (struct ext2_dir_entry*) block;
	
      while((lbyte_offset < inode->i_size) && entry->file_type) {
        char file_name[EXT2_NAME_LEN + 1];
        memcpy(file_name, entry->name, entry->name_len);
        file_name[entry->name_len] = 0;

        if(entry->inode != 0) {
          printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n", inode_num, lbyte_offset, entry->inode, entry->rec_len, entry->name_len, file_name);
        }
        lbyte_offset += entry->rec_len;
        entry = (void*) entry + entry->rec_len;
      }*/
	  fprintf(stderr,"Try:%d\n",BLOCK_OFFSET(entries[i]));
      fprintf(stdout,"INDIRECT,%d,%d,%d,%d,%d\n",inode_num,level,lblock_offset,block_num,entries[i]+1);
    }
  }
  return lblock_offset;
}

//directory and file analysis
void directory(struct ext2_inode* inode, int inode_num) {
  unsigned int lbyte_offset = 0;//Used only in Directory, but declared outside for convenience for scan_dir_indirects()
  if(S_ISDIR(inode->i_mode)){ // if(file_type == 'd')
    unsigned char b[block_s];
    struct ext2_dir_entry* entry;

    //Analyze direct blocks
    unsigned int i;
    for(i = 0; i < EXT2_NDIR_BLOCKS; i++) {
      int status5 = pread(image, b, block_s, BLOCK_OFFSET(inode->i_block[i]));
      int test5 = errno;

      if(status5 == -1) {
        print_error("Error! Failed to read image", test5);
        exit(EXIT_ERROR);
      }

      entry = (struct ext2_dir_entry*) b;

      while((lbyte_offset < inode->i_size) && entry->file_type) {
        char file_name[EXT2_NAME_LEN + 1];
        memcpy(file_name, entry->name, entry->name_len);

        file_name[entry->name_len] = 0;

        if(entry->inode != 0) {
          printf("DIRENT,%d,%d,%d,%d,%d,'%s'\n", inode_num, lbyte_offset, entry->inode, entry->rec_len, entry->name_len, file_name);
        }
        lbyte_offset += entry->rec_len;
        entry = (void*) entry + entry->rec_len;
      }
    }
  }

  if(inode->i_block[EXT2_IND_BLOCK] != 0) {
    scan_dir_indirects(inode, inode_num, inode->i_block[EXT2_IND_BLOCK], lbyte_offset, 1);
  }
  if(inode->i_block[EXT2_DIND_BLOCK] != 0) {
    scan_dir_indirects(inode, inode_num, inode->i_block[EXT2_DIND_BLOCK], lbyte_offset, 2);
  }
  if(inode->i_block[EXT2_TIND_BLOCK] != 0) {
    scan_dir_indirects(inode, inode_num, inode->i_block[EXT2_TIND_BLOCK], lbyte_offset, 3);
  }
}

//inode summary analysis
void inode_summary() {
  unsigned int inode_num;
  struct ext2_inode inode;

  unsigned int i;

  for(i = 0; i < num_groups; i++) {
    //Read the root directory first, then set to the first non-reserved inode
    for(inode_num = 2; inode_num < sb.s_inodes_count; inode_num++) {
      int valid_inode = 1;
      int inode_found = 0;

      unsigned int j;
      for(j = 0; j < block_s; j++) {
        uint8_t byte = inode_bitmap[i + j];
        int mask = 1;

        int k;
        for(k = 0; k < 8; k++) {
          unsigned long current_inode_num = i * sb.s_inodes_per_group + j * 8 + k + 1;
          if(current_inode_num == inode_num) {
            inode_found = 1;

            if((byte & mask) == 0) {
              valid_inode = 0;
            }
            break;
          }
          mask <<= 1;
          if(inode_found) {
            break;
          }
        }
      }

      if(!inode_found) {
        continue;
      }

      if(!valid_inode) {
        continue;
      }

      off_t offset = BLOCK_OFFSET(group_desc[i].bg_inode_table) + (inode_num - 1) * sizeof(struct ext2_inode);

      int status6 = pread(image, &inode, sizeof(struct ext2_inode), offset);
      int test6 = errno;

      if(status6 == -1) {
        print_error("Error! Failed to read image", test6);
        exit(EXIT_ERROR);
      }


      //Acquire inode file format
      char file_type = '?';
      if(S_ISDIR(inode.i_mode)) {
        file_type = 'd';
      } else if(S_ISREG(inode.i_mode)) {
        file_type = 'f';
      } else if(S_ISLNK(inode.i_mode)) {
        file_type = 's';
      }


      //Get permissions
      uint16_t mode = inode.i_mode & (0x01C0 | 0x0038 | 0x0007);

      //Get the time it was created
      char i_ctime[80];
      char i_mtime[80];
      char i_atime[80];

      time_format(inode.i_ctime, i_ctime);
      time_format(inode.i_mtime, i_mtime);
      time_format(inode.i_atime, i_atime);

      printf("INODE,%d,%c,%o,%u,%u,%u,%s,%s,%s,%u,%u", inode_num, file_type, mode, inode.i_uid, inode.i_gid, inode.i_links_count, i_ctime, i_mtime, i_atime, inode.i_size, inode.i_blocks);

      unsigned int u;
      //Print addresses
      for(u = 0; u < EXT2_N_BLOCKS; u++) {
        printf(",%u", inode.i_block[u]);
      }
      printf("\n");
	  
      if(file_type == 'd' || file_type == 'f') {
        //assert(S_ISDIR(inode.i_mode));
        directory(&inode, inode_num);
      }
		
      //If currently reading root directory, go to thr first non-reserved inode for the 
      //next iteration
      if(inode_num == 2) {
        inode_num = sb.s_first_ino - 1;
      }
    }
  }
}


//Main function
int main(int argc, char** argv) {
  //Check for valid number of arguments
  if(argc != 2) {
    fprintf(stderr, "Error: Invalid number of arguments provided\n");
    exit(EXIT_BAD_ARG);
  }  


  //Open file system image for analysis
  const char* file_system_img = argv[1];
  image = open(file_system_img, O_RDONLY);
  int status = errno;

  //Check if failed to open file
  if(image == -1) {
    print_error("Error: Could not open file", status);
    exit(EXIT_ERROR);
  }
  outfd = creat("out.csv",0666);
  status = errno;
  
  if(outfd == -1){
    print_error("Error: Failed to create output file", status);
    exit(EXIT_ERROR);
  }
  
  //Call routines to perform analysis
  superblock();
  group();
  free_block();
  free_inode();
  inode_summary();

  free_memory();
  //Finish, exit success
  exit(EXIT_SUCC);
}