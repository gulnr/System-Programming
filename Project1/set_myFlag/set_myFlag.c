#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <asm/errno.h>

asmlinkage long sys_set_myFlag(pid_t pid, int flag){
	
	if((current->cred)->uid == 0){
		struct task_struct *process_ptr = find_task_by_vpid(pid); //return the pointer of pid process
		
		if(process_ptr != NULL){// there is process
			if(flag == 0 || flag == 1){ //valid flag values
				process_ptr-> myFlag = flag;
				return 0;
			}else{ 
				return EINVAL; //invalid value 
			}
		}else{
			return ESRCH; //no such process
		}
	}else{
		return EPERM;
	}

}
