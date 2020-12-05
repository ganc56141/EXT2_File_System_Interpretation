//NAME: JIAXUAN XUE,Felix Gan
//EMAIL: nimbusxue1015@g.ucla.edu,felixgan@g.ucla.edu
//ID: 705142227,205127385

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <limits.h>
#include <time.h>
#include <getopt.h>
#include <sys/stat.h>
#include <stdint.h>
#include "ext2_fs.h"

#define SUPERBLOCKOFFSET 1024
#define GROUP_INDEX 0

int fd;
struct ext2_super_block superblock;
__u32 s_log_block_size;
struct ext2_group_desc* table;          // pointer to a table containing group_descriptor
struct ext2_group_desc group_descriptor;

// declare functions
void exit_on_error(char *reason);
void DirectoryEntries(__u32 inode_num, struct ext2_inode inode);
void IndirectBlockReferences(__u32 inode_num, int level, int block_num, int level_offset);
void inode(__u32 inode_index);
char* GMT_time(__u32 time);
void print_free_block();
void InodesSummary();


// generic error function
void exit_on_error(char *reason)
{
        fprintf(stderr, "%s error: %s\n", reason, strerror(errno));
        exit(1);
}

int power(int a, int b)
{
    for (int i = 0; i < b-1; i++) a*=a;
    return a;
}

void DirectoryEntries(__u32 inode_num, struct ext2_inode inode) {
    for (int i = 0; i < 12; i++) {
        __u32 byte_offset = 0;
        while (inode.i_block[i] != 0 && byte_offset < s_log_block_size) {
            struct ext2_dir_entry dir;
            if (pread(fd, &dir, sizeof(dir), (inode.i_block[i] - 1) * s_log_block_size + SUPERBLOCKOFFSET + byte_offset) < 0)
                exit_on_error("failed to read the Directory Entries");

            if (dir.inode != 0){
                fprintf(stdout,"DIRENT,%d,%d,%d,%d,%d,'%s'\n",inode_num,byte_offset,dir.inode,dir.rec_len,dir.name_len,dir.name);
            }
                
            byte_offset += dir.rec_len;
        }
    
    }
}

void IndirectBlockReferences(__u32 inode_num, int level, int block_num, int level_offset){
    __u32 *block = malloc(s_log_block_size*sizeof(__u32));
    
    int indirect_offset = s_log_block_size*block_num;
    
    if (pread(fd, block, s_log_block_size, indirect_offset) < 0)
        exit_on_error("failed to read for indirect entry reference");

    for (__u32 i = 0 ; i < s_log_block_size/4 ; i++) {
        if(block[i] != 0){
            
            fprintf(stdout,"INDIRECT,%d,%d,%d,%d,%u\n",inode_num,level,level_offset,block_num,block[i]);
                
            if (level != 1) {
                IndirectBlockReferences(inode_num, level-1, block[i], level_offset);
            }
        }
        
        if(level == 2){
            level_offset+=256;
        }else if(level == 3){
            level_offset+= 256*256;
        }else{
            level_offset++;
        }
    }
    free(block);
}

void inode(__u32 inode_index)
{
        
        struct ext2_inode this_inode;
        // group_descriptor = group, bg_inode_table is the block number of the inode table
        int offset = SUPERBLOCKOFFSET + s_log_block_size * (group_descriptor.bg_inode_table-1) + (inode_index-1) * sizeof(this_inode);
        if ( pread(fd, &this_inode, sizeof(struct ext2_inode), offset) < 0 ) exit_on_error("pread");

    if (this_inode.i_mode != 0 && this_inode.i_links_count != 0) {
        // check type
        char type;
        if ((this_inode.i_mode & 0xF000) == 0x4000) type = 'd';
        else if ((this_inode.i_mode & 0xF000) == 0xA000) type = 's';
        else if ((this_inode.i_mode & 0xF000) == 0x8000) type = 'f';
        else type = '?';

        // need to check? nah, I'll trust the calling function
        printf("INODE,%i,%c,%o,%i,%i,%i,%s,%s,%s,%i,%i", inode_index, type, this_inode.i_mode & 0x0FFF, this_inode.i_uid, this_inode.i_gid, this_inode.i_links_count,
                    GMT_time(this_inode.i_ctime), GMT_time(this_inode.i_mtime), GMT_time(this_inode.i_atime),this_inode.i_size, this_inode.i_blocks);
        
        // For ordinary files (type 'f') and directories (type 'd') the next fifteen fields are block addresses
        if (type == 'f' || type =='d' || (type == 's' && this_inode.i_size > 60)) for (int i = 0; i < 15; i++) printf(",%d", this_inode.i_block[i]);
        
        fprintf(stdout, "\n");
        
        if (type == 'f' || type =='d')
        {
                if (type =='d') DirectoryEntries(inode_index, this_inode);
                for (int i = 0; i < 3; i++)
                {
                        long offset = 12;
                        for (int j = 1; j < i+1; j++) offset += power(256, j);
                        if (this_inode.i_block[12+i] != 0) IndirectBlockReferences(inode_index, i+1, this_inode.i_block[12+i], offset);
                }
        }
        
    }
        
}

char* GMT_time(__u32 time) {
        // initializing time structure
 struct tm* t_struct;
        time_t lt = time;
        t_struct = gmtime(&lt);
        char* time_buf = (char*)malloc(sizeof(char)*30);
       
	// printing formatted time
//	sprintf(time_buf, "%02d/%02d/%02d %02d:%02d:%02d",
//                t_struct->tm_mon,
//                t_struct->tm_mday,
//                t_struct->tm_year,
//                t_struct->tm_hour,
//                t_struct->tm_min,
//                t_struct->tm_sec);
    strftime(time_buf, 24, "%m/%d/%y %H:%M:%S", t_struct);
        
   return time_buf;
}
  

void print_free_block()
{
        int offset = SUPERBLOCKOFFSET + (group_descriptor.bg_block_bitmap - 1) * s_log_block_size;
        char* byte_array = malloc(sizeof(char) * s_log_block_size);
        if ( pread(fd, byte_array, s_log_block_size, offset) < 0 ) exit_on_error("pread with block free bitmap");
        
        int block_index = 1;
        for (__u32 i = 0; i < s_log_block_size; i++) 
                for (int j = 0; j < 8; j++) 
                {
                        if (~byte_array[i] & (1 << j)) fprintf(stdout, "BFREE,%d\n", block_index);
                        block_index++;
                }     
        free(byte_array);
}

int block_offset(int num) {
    return (num - 1) * (int) s_log_block_size + SUPERBLOCKOFFSET;
}

void InodesSummary() {
        char* byte_array = malloc(superblock.s_inodes_per_group / 8 * sizeof(char));
        if (pread(fd, byte_array, superblock.s_inodes_per_group / 8, (group_descriptor.bg_inode_bitmap - 1) * s_log_block_size + SUPERBLOCKOFFSET) < 0) exit_on_error("failed to read for Inodes summary");

    __u32 inode_index = 1;

    for (__u32 i = 0; i < superblock.s_inodes_per_group / 8; i++){
                for (int j = 0; j < 8; j++){
                    if (~byte_array[i] & (1 << j)){
                        fprintf(stdout, "IFREE,%d\n", inode_index);
                    }else{
                        inode(inode_index);
                    }
                    inode_index++;
                }
    }
        free(byte_array);
    
 
}

int main(int argc, char *argv[])
{
       if (argc != 2) exit_on_error("Unrecognized arguments");
       if ((fd = open(argv[1], O_RDONLY)) < 0) exit_on_error("Error: Failed to open the image");         //opening image


       // defining global variables

       s_log_block_size = EXT2_MIN_BLOCK_SIZE << superblock.s_log_block_size;          // calculating blocksize

       long group_size = sizeof(group_descriptor);             // define size of ONE group
       long offset = (unsigned)SUPERBLOCKOFFSET + s_log_block_size;     // define offset to get to start of block containing group descriptor
       table = malloc(group_size);

       if ( pread(fd, table, group_size, offset) < 0 ) exit_on_error("pread");
       group_descriptor = table[0];


        //print superblock
       
       if (pread(fd, &superblock, sizeof(struct ext2_super_block),SUPERBLOCKOFFSET) < 0){
               exit_on_error("failed to read the superblock");
       }

       if (superblock.s_magic != EXT2_SUPER_MAGIC){
               exit_on_error("failed to verify the superblock");
       }
       
       fprintf(stdout, "SUPERBLOCK,%d,%d,%d,%d,%d,%d,%d\n",superblock.s_blocks_count,superblock.s_inodes_count,
               s_log_block_size,superblock.s_inode_size,superblock.s_blocks_per_group,superblock.s_inodes_per_group,
               superblock.s_first_ino);
    
        //print group summary
        fprintf(stdout, "GROUP,%d,%d,%d,%d,%d,%d,%d,%d\n",GROUP_INDEX,superblock.s_blocks_count,superblock.s_inodes_count,
            group_descriptor.bg_free_blocks_count,group_descriptor.bg_free_inodes_count,group_descriptor.bg_block_bitmap,
            group_descriptor.bg_inode_bitmap,group_descriptor.bg_inode_table);
    
        //print free blocks
        print_free_block();
    
        //print inodes summary
        InodesSummary();
        
        free(table);
        
        exit(0);
}
