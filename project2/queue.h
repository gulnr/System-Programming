#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

#ifndef QUEUE_MAJOR
#define QUEUE_MAJOR 0   
#endif

#ifndef QUEUE_NR_DEVS
#define QUEUE_NR_DEVS 3   
#endif


/* for text list*/
struct queue_qset {
	char *element; 
	struct queue_qset *next;
};


struct queue_dev {
	struct queue_qset *head;
    struct queue_qset *tail;
	struct queue_qset *data;  /* Pointer to text list*/
	bool first_device;
	unsigned long size;       /* amount of data stored here */
	unsigned int access_key;  /* used by sculluid and scullpriv */
	struct semaphore sem;     /* mutual exclusion semaphore     */
	struct cdev cdev;	  /* Char device structure		*/
	struct queue_dev *next; /*Pointer to next device*/
};


/* Prototypes for shared functions */
int queue_p_init(dev_t dev);
void queue_p_cleanup(void);
int queue_access_init(dev_t dev);


int queue_trim(struct queue_dev *dev);
ssize_t queue_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t queue_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
long queue_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);


/* Use 'k' as magic number */
#define QUEUE_IOC_MAGIC  'k'


#define POP _IO(QUEUE_IOC_MAGIC, 0)


#endif /* _QUEUE_H_ */
