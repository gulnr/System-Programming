#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/cred.h>
#include <linux/uidgid.h>
#include <linux/kernel.h>	/* printk() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/file.h>  
#include <asm/switch_to.h>		/* cli(), *_flags */
#include <asm/uaccess.h>	/* copy_*_user */
#include <linux/string.h>

#include "queue.h"		/* local definitions */

MODULE_AUTHOR("");
MODULE_LICENSE("Dual BSD/GPL");

int queue_major =   QUEUE_MAJOR;
int queue_minor =   0;
int queue_nr_devs = QUEUE_NR_DEVS;	/* number of bare scull devices */

module_param(queue_major, int, S_IRUGO);
module_param(queue_minor, int, S_IRUGO);
module_param(queue_nr_devs, int, S_IRUGO);


struct queue_dev *queue_devices;	/* allocated in scull_init_module */

/*
 * Empty out the scull device; must be called with the device
 * semaphore held.
 */

int queue_trim(struct queue_dev *dev)
{
	struct queue_qset *next, *next2, *dptr, *tmp_head;

	for (dptr = dev->data; dptr; dptr = next) {
		if (dptr != NULL) { //if there is element in the list
            tmp_head = dptr; 
            
            while((tmp_head != NULL)){ //through the element list
                next2 = tmp_head->next;
                kfree(tmp_head);
                tmp_head = next2;
            }
		}
		next = dptr->next;
		kfree(dptr);
	}

	dev->data = NULL;
    dev->head = NULL;
    dev->tail = NULL;
	return 0;
}



int queue_open(struct inode *inode, struct file *filp)
{
    struct queue_dev *dev;

    dev = container_of(inode->i_cdev, struct queue_dev, cdev);
    filp->private_data = dev;
 
    return 0;
}


int queue_release(struct inode *inode, struct file *filp)
{
	return 0;
}



/* Data management: read and write */

ssize_t queue_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos){
    struct queue_qset *tmp_head, *next2;
    struct queue_dev *dev = filp->private_data; 
    char *ptr;
    ssize_t retval = 0;
    int total_size = 0;
    char* temp;
    int count_len=0;
	printk(KERN_INFO "	READ: 	count %zu\n",count);
        
    if(dev->first_device){
        printk(KERN_INFO "	READ: first queue can not read \n"); 
    }
    else{

        tmp_head = dev->head; //pointer to the list text
		
        //calculate the total byte by iterating over the list of device
        while(tmp_head != NULL){ //through the element list
            next2 = tmp_head->next;
			ptr=tmp_head->element;
			count_len=0;
            while(*ptr){
				*++ptr;
				count_len++;
			}
			printk(KERN_INFO "READ: count_len %d\n",count_len);
			total_size = total_size + count_len-1;
			printk(KERN_INFO "READ: total_size %d\n",total_size);
            tmp_head = next2;
        }
		 

		
        temp = kmalloc(total_size, GFP_KERNEL);

        //char str[total_size];

        strcpy(temp, "");

        tmp_head = dev->head; //pointer to the list text
        while(tmp_head != NULL){
            next2 = tmp_head->next;
            strcat(temp, tmp_head->element);
            tmp_head = next2;
        }
        *f_pos += total_size;
		retval = total_size; 

        //temp = &str[0];

        if (copy_to_user(buf, temp, total_size)) { //int copy_to_user(void *dst, const void *src, unsigned int size);
            retval = -EFAULT;
        }

    }


    return retval;
}


ssize_t queue_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos){
    struct queue_dev *dev = filp->private_data;
    ssize_t retval = 0;
    char* temp;

    if(dev->first_device == 1){
        printk(KERN_WARNING "This device can not write\n");
    }else{
        //create a node for element
        struct queue_qset *dptr;
        dptr = kmalloc(sizeof(struct queue_qset), GFP_KERNEL);

        //retval = -ENOMEM;

        temp = kmalloc(count, GFP_KERNEL);

        if (copy_from_user(temp, buf, count)) {
            retval = -EFAULT;
        }


        *f_pos += count;
        retval = count;
		printk(KERN_INFO "	WRITE: 	count %zu\n",count);
		int j=0;
        while(j<count){
			printk(KERN_INFO "	WRITE: 	temp %c\n",temp[j]);
			j++;
		}
		
        if(dev->data == NULL){ // there is no text list in the device
            dev->head = dptr;
            dev->tail = dptr;
            dev->data = dptr;
            dptr->element = temp;
            dptr->next = NULL;
        }else{ // add the element to end of the list
            dev->tail->next = dptr;
            dev->tail = dptr;
            dptr->element = temp;
            dptr->next = NULL;
        }
        /*manually test for write -> head, tail and currentnode
        int k=0;
        while(k<count){
			printk(KERN_INFO "	WRITE: 	dptr->element %c\n",(dptr->element)[k]);
			k++;
		}
		k=0;
        while(k<count){
			printk(KERN_INFO "	WRITE: 	dptr->element %c\n",(dev->head->element)[k]);
			k++;
		}
		k=0;
        while(k<count){
			printk(KERN_INFO "	WRITE: 	dptr->element %c\n",(dev->tail->element)[k]);
			k++;
		}
        */
    }
	//kfree(temp);
    return retval;
}


 /* The ioctl() implementation */
 /*            POP             */
 
 long queue_ioctl(struct file *filp, unsigned int cmd, unsigned long arg){
    int retval = 0;

    struct queue_dev *dev = filp->private_data;
    struct queue_qset *tmp_head;
    struct queue_dev *dev_tmp;

    switch(cmd) {
        case POP:
			if (! capable (CAP_SYS_ADMIN))
				return -EPERM;

            if(dev->first_device == 1){
                dev_tmp = dev->next;

                while(dev_tmp->data == NULL){ //if the list is empty, iterate through devices
                    dev_tmp = dev_tmp->next;
                }

                //do pop
                tmp_head = dev_tmp->head;
                dev_tmp->head = dev_tmp->head->next;
                dev_tmp->data = dev_tmp->head->next;

                kfree(tmp_head);
            }
            else{
                printk(KERN_WARNING "This device can not do pop operation\n");
            }
            break;
    }

    return retval;

 }

 struct file_operations queue_fops = {
	.owner =    THIS_MODULE,
	.read =     queue_read,
	.write =    queue_write,
	.unlocked_ioctl = queue_ioctl,
	.open =     queue_open,
	.release =  queue_release,
};


/*
 * The cleanup function is used to handle initialization failures as well.
 * Thefore, it must be careful to work correctly even if some of the items
 * have not been initialized
 */

void queue_cleanup_module(void)
{
	
    int i;
    dev_t devno = MKDEV(queue_major, queue_minor);

    if (queue_devices) {
        for (i = 0; i < queue_nr_devs; i++) {
            queue_trim(queue_devices + i);
            cdev_del(&queue_devices[i].cdev);
        }
    kfree(queue_devices);
    }

    unregister_chrdev_region(devno, queue_nr_devs);
    printk(KERN_INFO "Goodbye, world 5\n");
}


int queue_init_module(void)
{
    int result, i;
    int err;
    dev_t devno = 0;
    struct queue_dev *dev;
	printk(KERN_INFO "queue: my minor number %d  \n", queue_minor);
	if(queue_minor >= queue_nr_devs){
		printk(KERN_INFO "queue: can not exceed %d device \n", queue_nr_devs);
		return -EFAULT;
	}
    if (queue_major) {
        devno = MKDEV(queue_major, queue_minor);
        result = register_chrdev_region(devno, queue_nr_devs, "queue");
    } else {
        result = alloc_chrdev_region(&devno, queue_minor, queue_nr_devs,
                                     "queue");
        queue_major = MAJOR(devno);
    }
    if (result < 0) {
        printk(KERN_WARNING "queue: can't get major %d\n", queue_major);
        return result;
    }

    queue_devices = kmalloc(queue_nr_devs * sizeof(struct queue_dev),
                            GFP_KERNEL);
    if (!queue_devices) {
        result = -ENOMEM;
        goto fail;
    }
    memset(queue_devices, 0, queue_nr_devs * sizeof(struct queue_dev));

    /* Initialize each device. */
    for (i = 0; i < queue_nr_devs; i++) {
        dev = &queue_devices[i];
        if(i == 0){
            dev->first_device = 1;
        }else{
            dev->first_device = 0;
        }
        
        dev->data = dev->tail = dev->head = NULL;
        sema_init(&dev->sem,1);
        devno = MKDEV(queue_major, queue_minor + i);
        cdev_init(&dev->cdev, &queue_fops);
        dev->cdev.owner = THIS_MODULE;
        dev->cdev.ops = &queue_fops;
        err = cdev_add(&dev->cdev, devno, 1);
        dev->next = NULL;
        if (err)
            printk(KERN_NOTICE "Error %d adding queue%d", err, i);
    }

    // create the devices as linked list
    for(i = 0; i < (queue_nr_devs-1); i++){
        dev = &queue_devices[i];
        dev->next = &queue_devices[i+1];
    }
    printk(KERN_INFO "Hello, world 5\n=============\n");
	printk(KERN_INFO "mymajor_number: %d\n", queue_major);
	printk(KERN_INFO "my minor number: %d\n", queue_minor);
	printk(KERN_INFO "the numberof device is: %d\n", queue_nr_devs);

    return 0; /* succeed */

  fail:
    queue_cleanup_module();
    return result;
}


module_init(queue_init_module);
module_exit(queue_cleanup_module);
