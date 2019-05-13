#include <stdio.h>
#include <stdlib.h>
#include <asm/errno.h>
#include <unistd.h>
#include <string.h>


#define set_myFlag 355


int main(){
	
	int flag = 1;

	int check_return;
	printf("getpid(): %d , getppid(): %d \n", getpid(), getppid());
	check_return = syscall(set_myFlag, getpid(), flag);//set flag value to mother process
		
	printf("Return value of set_myFlag: %s\n", strerror(check_return));
	
	int f;
	f = fork();
	printf("f value: %d\n", f);
	
	if(f == 0){ //child process
		printf("Child pid: %d child parent pid:%d\n", getpid(), getppid());

		return 0;
	}else if(f < 0){
		printf("Return value: %s\n", strerror(f));
		printf("getpid(): %d , getppid(): %d \n", getpid(), getppid());
	}else{
		printf("Parent pid: %d\n", getpid());
	}
	
	return 0;
}