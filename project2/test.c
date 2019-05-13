#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>

#include "queue.h"

//test with sudo ./test pop


int main(int argc, char *argv){

    int fd;
    
    if (argv[1] == '0'){ //getting arguments from the command line
		fd = open("dev/queue0", O_RDWR); //open queue0
		
		if (fd == -1){
			perror("cannot access device");
			exit(EXIT_FAILURE);
		}
		
		else{ //if we open it appropriately the ioctl function that we modified will be called.
			ioctl(fd, POP_IO);
		}
	}
	close (fd);

    return 0;
}
