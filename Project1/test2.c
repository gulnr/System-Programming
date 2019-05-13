#include <stdio.h>
#include <stdlib.h>
#include <asm/errno.h>
#include <unistd.h>
#include <string.h>

#define set_myFlag 355

int main(){
    
    int f, i, check_return, flag, tmp;

    printf("FIRST -> getpid(): %d, getppid(): %d \n", getpid(), getppid());

    for(i = 0; i < 5; i++){
        f = fork();

        if(i == 4){
            if(f == 0){//child process
                flag = 1;
                
                int check_return;
                printf("PROCESS CALLS THE EXIT -> getpid(): %d , getppid(): %d \n", getpid(), getppid());
                check_return = syscall(set_myFlag, getpid(), flag);//try to set flag to child process
                    
                printf("PROCESS CALLS THE EXIT -> Return value of set_myFlag: %s\n", strerror(check_return));

                sleep(5);
                printf("The child that calls the exit call was died!\n");
                exit(0);
            }
        }

        if(f == 0){
            printf("Waiting process child: getpid(): %d , getppid(): %d \n", getpid(), getppid());
            while(1);
        }
    }
    
    sleep(10);
    return 0;
}