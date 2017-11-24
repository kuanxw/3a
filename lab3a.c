/* NAME: Kuan Xiang Wen, Amir Saad
 * EMAIL: kuanxw@g.ucla.edu, arsaad@g.ucla.edu
 * ID: 004461554, 604840359
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include "ext2_fs.h"

//Error handling
extern int errno;
int errVar;

//Input/Output files
char* imgFile;
int imgfd;
int outfd;

//Read all the information into relevant structs, then prints them into CSV format
void write_ext2_info(){
	pread();
	
}

//Main: Includes in/out file handling
int main(int argc, char **argv){
	if(argc != 2){
		fprintf(stderr,"Usage: ./lab3c IMGFILE\n");
		exit(1);
	}
	imgFile = argv[1];
	imgfd = open(imgFile, O_RDONLY);
	if(imgfd < 0){
		errVar = errno;
		fprintf(stderr,"Failed to open input file:  %s\n", strerror(errVar));
		exit(1);
	}
	outfd = creat("out.csv",0666);
	if(outfd < 0){
		errVar = errno;
		fprintf(stderr,"Failed to create output file:  %s\n", strerror(errVar));
		exit(1);
	}
	
	write_ext2_info();
	
	close(outfd);
	close(imgfd);
    exit(0);
}
