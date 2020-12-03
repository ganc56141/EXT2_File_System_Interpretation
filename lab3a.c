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

int fd;
struct ext2_super_block superblock;
unsigned int block_size;


int main(int argc, char* argv[]) {
    if (argc != 2) {
            fprintf(stderr, "Unrecognized arguments\n");
            exit(1);
    }
    
    if (fd = open(argv[1], O_RDONLY) < 0) {
            fprintf(stderr, "Error: Failed to open the image: %s\n", strerror(errno));
            exit(1);
    }
    
    
}
