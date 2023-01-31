/* Prototype module for second mandatory DM510 assignment */
#ifndef __KERNEL__
#  define __KERNEL__
#endif
#ifndef MODULE
#  define MODULE
#endif

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/wait.h>
/* #include <asm/uaccess.h> */
#include <linux/uaccess.h>
#include <linux/semaphore.h>
/* #include <asm/system.h> */
#include <asm/switch_to.h>

#include <linux/cdev.h>
#include <linux/sched/signal.h>
#include "ioctl_commands.h"

/* Prototypes - this would normally go in a .h file */
static int dm510_open( struct inode*, struct file* );
static int dm510_release( struct inode*, struct file* );
static ssize_t dm510_read( struct file*, char*, size_t, loff_t* );
static ssize_t dm510_write( struct file*, const char*, size_t, loff_t* );
long dm510_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

#define DEVICE_NAME "dm510_dev" /* Dev name as it appears in /proc/devices */
#define MAJOR_NUMBER 254
#define MIN_MINOR_NUMBER 0
#define MAX_MINOR_NUMBER 1

/* end of what really should have been in a .h file */

#define init_MUTEX(_m) sema_init(_m,1) //for semaphore initiation
#define BUFFER_COUNT 2 // amount of bounding buffers

/* file operations struct */
static struct file_operations dm510_fops = {
	.owner   = THIS_MODULE,
	.read    = dm510_read,
	.write   = dm510_write,
	.open    = dm510_open,
	.release = dm510_release,
        .unlocked_ioctl   = dm510_ioctl
};

//Bounding buffer struct
struct Buffer {
	char *buffer, *end;
	char *rp, *wp;
	int buffersize;
};

/* Struct of pipe */
struct pipe {
	wait_queue_head_t inq, outq;
	struct Buffer* write_buffer;
	struct Buffer* read_buffer;
	int nreaders, nwriters;
	struct fasync_struct *async_queue;
	struct semaphore semaphore;
	struct cdev cdev;
};

/*First device*/
dev_t first = MKDEV(MAJOR_NUMBER, MIN_MINOR_NUMBER);

/* All devices and buffers*/
static struct pipe* devices;
static struct Buffer buffers[BUFFER_COUNT];

/*parameters, per buffer*/
int bufferSize = 2048;
int maxReaders = 10;

typedef struct Buffer Buffer;

/*Setup a cdev*/
int cdev_setup(struct pipe* dev, int index) {
	int errno, devno = first + index;
	cdev_init(&dev->cdev, &dm510_fops);
	dev->cdev.owner = THIS_MODULE;
	errno = cdev_add(&dev->cdev, devno, 1);
	if(errno) {
		printk("cdev_setup failed");
	}
	return 0;
}

/*Initialize buffer*/
int init_buffer(Buffer* b) {
	void* check;
	check = kmalloc(bufferSize * sizeof(*b->buffer), GFP_KERNEL);
	if(!check)
		return -ENOMEM; //out of memory

	/*setup buffer pointers*/
	b->buffersize = bufferSize;
	b->wp = b->rp = b->buffer = check;
	b->end = b->buffer + b->buffersize;
	return 0;
}

/* called when module is loaded */
int dm510_init_module( void ) {
	/* initialization code belongs here */

	int i, result;
	int totalSize = (int) (DEVICE_COUNT) * sizeof(struct pipe);

	result = register_chrdev_region(first, DEVICE_COUNT, DEVICE_NAME);
	if(result < 0) {
		printk(KERN_NOTICE "unable to get region");
		return 0;
	}

	devices = kmalloc(totalSize, GFP_KERNEL);
	if(devices == NULL) {
		unregister_chrdev_region(first, DEVICE_COUNT);
		return 0;
	}

	memset(devices, 0, totalSize);

	for(i = 0; i < BUFFER_COUNT; i++) {
		result = init_buffer(buffers + i);
		if(result < 0) return -15;
	}

	for(i = 0; i < DEVICE_COUNT; i++) {
		init_waitqueue_head(&(devices[i].inq));
		init_waitqueue_head(&(devices[i].outq));
		init_MUTEX(&devices[i].semaphore);
		devices[i].read_buffer = buffers + (i % BUFFER_COUNT);
		devices[i].write_buffer = buffers + ((i+1) % BUFFER_COUNT);
		cdev_setup(devices + i, i);
	}

	printk(KERN_INFO "DM510: Hello from your device!\n");
	return 0;
}

/*Frees char buffer in Buffer*/
int free_buffer(struct Buffer* buff) {
	kfree(buff->buffer);
	buff->buffer = NULL;
	return 0;
}

/* Called when module is unloaded */
void dm510_cleanup_module( void ) {
	/* clean up code belongs here */
	int i;

	if(!devices)
		return;

	for(i = 0; i < DEVICE_COUNT; i++) {
		if(devices[i].write_buffer) cdev_del(&devices[i].cdev);
	}

	for(i = 0; i < BUFFER_COUNT; i++) {
		if(buffers[i].buffer)
			free_buffer(buffers+i);
	}

	kfree(devices);
	unregister_chrdev_region(first, DEVICE_COUNT);

	printk(KERN_INFO "DM510: Module unloaded.\n");
}


/* Called when a process tries to open the device file */
static int dm510_open( struct inode *inode, struct file *filp ) {
	/* device claiming code belongs here */
	struct pipe* dev;
	dev = container_of(inode->i_cdev, struct pipe, cdev);
	filp->private_data = dev;

	if(down_interruptible(&dev->semaphore)) {
		printk(KERN_ERR "down_interrupted");
		return -ERESTARTSYS; //restart system
	}

	if(filp->f_mode & FMODE_READ) {
		if(dev->nreaders >= maxReaders) { //check if too many readers
			up(&dev->semaphore);
			return -ERESTARTSYS;
		}else {
			dev->nreaders++;
		}
	}
	if(filp->f_mode & FMODE_WRITE){
		if(dev->nwriters >= 1) { //check if too many writers
			up(&dev->semaphore);
			return -ERESTARTSYS;
		} else {
			dev->nwriters++;
		}
	}
	/*semaphore unlock*/
	up(&dev->semaphore);

	return nonseekable_open(inode, filp);
}

int fasync_p(int fd, struct file *filp, int mode) {
	struct pipe* dev = filp->private_data;
	return fasync_helper(fd, filp, mode, &dev->async_queue);
}


/* Called when a process closes the device file. */
static int dm510_release( struct inode *inode, struct file *filp ) {
	/* device release code belongs here */
	struct pipe* dev = filp->private_data;

	down(&dev->semaphore);
	fasync_p(-1, filp, 0);
	if(filp->f_mode & FMODE_READ && dev->nreaders) dev->nreaders--;
	if(filp->f_mode & FMODE_WRITE && dev->nwriters) dev->nwriters--;
	up(&dev->semaphore);

	printk(KERN_INFO "\n");
	printk(KERN_INFO "\n");
	return 0;
}

/*reads from a buffer*/
int bufferRead(struct pipe* dev, struct Buffer* buf, char* text, size_t count) {
	size_t size = count;
	if(buf->wp > buf->rp) { //read to wp
		size = min(size, (size_t)(buf->wp - buf->rp));
		if(copy_to_user(text, buf->rp, size)) {
			up(&dev->semaphore);
			return -EFAULT; //Bad address
		}
		buf->rp += size; //move pointer
	} else { //if rp needs to wrap around (wp < rp)
		size = min(size, (size_t)(buf->end - buf->rp));
		if(copy_to_user(text, buf->rp, size)) { //read to the end of buffer
			up(&dev->semaphore);
			return -EFAULT; //Bad address
		}
		count -= size;
		if(count > 0) { //still something to read
			size = min(count, (size_t)(buf->wp - buf->buffer));
			buf->rp = buf->buffer; // wrap
			if(copy_to_user(text, buf->rp, size)) { //reading the last data
				up(&dev->semaphore);
				return -EFAULT; //Bad address
			}
		}
		buf->rp += size;
	}
	return 0;
}


/* Called when a process, which already opened the dev file, attempts to read from it. */
static ssize_t dm510_read( struct file *filp,
    char *buf,      /* The buffer to fill with data     */
    size_t count,   /* The max number of bytes to read  */
    loff_t *f_pos)  /* The offset in the file           */
{
	struct pipe* dev = filp->private_data;

	/* read code belongs here */
	if(down_interruptible(&dev->semaphore)) {
		return -ERESTARTSYS;
	}

	while (dev->read_buffer->rp == dev->read_buffer->wp) { //nothing to read

		up(&dev->semaphore);
		if (filp->f_flags & O_NONBLOCK) {
			return -EAGAIN; //Try again
		}

		/*Wait event interruptible*/
		if(wait_event_interruptible(dev->inq, (dev->read_buffer->rp != dev->read_buffer->wp))) {
			return -ERESTARTSYS;
		}

		if(down_interruptible(&dev->semaphore)) {
			return -ERESTARTSYS;
		}
	}

	bufferRead(dev, dev->read_buffer, (char*)buf, count);

	up(&dev->semaphore);

	/*wake up other processes*/
	wake_up_interruptible(&dev->outq);

	return 0; //return number of bytes read
}

/*Returns amount of space free in buffer*/
int spacefree(struct Buffer* buf) {
	if(buf->rp == buf->wp)
		return buf->buffersize - 1;
	return ((buf->rp + buf->buffersize - buf->wp) % buf->buffersize) - 1;
}

/*Waits if buffer is full else return 0*/
int getWriteSpace(struct pipe* dev, struct file *filp) {
	Buffer* wb = dev->write_buffer;
	while(spacefree(wb) == 0) { //full
		DEFINE_WAIT(wait);

		printk(KERN_INFO "BUFFER FULL");

		up(&dev->semaphore);
		if(filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		prepare_to_wait(&dev->outq, &wait, TASK_INTERRUPTIBLE);
		if(spacefree(wb) == 0) {
			schedule();
		}
		finish_wait(&dev->outq, &wait);

		if (signal_pending(current))
			return -ERESTARTSYS;
		if(down_interruptible(&dev->semaphore))
			return -ERESTARTSYS;
	}
	return 0;
}

/*Writes to a buffer*/
int writeToBuffer(struct pipe* dev, struct Buffer* buf, char* text, size_t size) {
	size_t newSize = min(size, (size_t)spacefree(buf));
	
	if(buf->wp < buf->rp) { //something to read
		newSize = min(newSize, (size_t)(buf->rp - buf->wp - 1));
		if(copy_from_user(buf->wp, text, newSize)) {
			up(&dev->semaphore);
			return -EFAULT;
		}
		/*Move pointer*/
		buf->wp += newSize;
	} else { //wrap around if rp > wp
		newSize = min(newSize, (size_t)(buf->end - buf->wp));
		if(copy_from_user(buf->wp, text, newSize)) { //write to end
			up(&dev->semaphore);
			return -EFAULT;
		}
		size-=newSize;
		if(size < 0){ //more to write
			newSize = min(size, (size_t)(buf->rp - buf->buffer) % buf->buffersize); //to make sure there is enough space to write on
			if(copy_from_user(buf->wp, text, newSize)) { //write to end
				up(&dev->semaphore);
				return -EFAULT;
			}

			buf->wp = buf->buffer;
		} 
		buf->wp += newSize;
	}
	return newSize;
}

/* Called when a process writes to dev file */
static ssize_t dm510_write( struct file *filp,
    const char *buf,/* The buffer to get data from      */
    size_t count,   /* The max number of bytes to write */
    loff_t *f_pos )  /* The offset in the file           */
{
	/* write code belongs here */
	struct pipe* dev = filp->private_data;
	int result;

	if(down_interruptible(&dev->semaphore)) {
		return -ERESTARTSYS;
	}

	if(count > buffers->buffersize) {
		return -ENOMEM;//message too long
	}

	result = getWriteSpace(dev, filp);
	if(result) //check if buffer is full
		return result;

	count = writeToBuffer(dev, dev->write_buffer,(char*) buf, count);

	/*wake up*/
	wake_up_interruptible(&dev->inq);

	up(&dev->semaphore);

	/*send signal to readers*/
	if(dev->async_queue)
		kill_fasync(&dev->async_queue, SIGIO, POLL_IN);

	printk(KERN_INFO "spacefree= %d\n", spacefree(dev->write_buffer));

	return count; //return number of bytes written
}

/*Resize the buffer*/
int resizeBuffer(struct Buffer* buffer, size_t newSize) {
	void* newMemory;
	size_t difference = 0;

	char* oldMem = buffer->buffer;
	newMemory = kmalloc(newSize * sizeof(*buffer->buffer), GFP_KERNEL);

	if(!newMemory)
		return -ENOMEM; //no memory error

	if (buffer->wp < buffer->rp) { //writepointer is closest to buffer start, it has wrapped
		size_t diff2 = buffer->wp - buffer->buffer; //amount of bytes not read before read pointer
		difference = buffer->end - buffer->rp; //amount of bytes not read

		memcpy(newMemory, buffer->rp, difference);

		buffer->rp = newMemory;

		memcpy(buffer->rp + difference, buffer->buffer, diff2);

		buffer->wp = buffer->rp + difference + diff2;
	} else if (buffer->rp < buffer->wp){ //Readpointer is closest to buffer start
		difference = buffer->wp - buffer->rp; // amount of bytes not read
		memcpy(newMemory, buffer->rp, difference);

		buffer->rp = newMemory;
		buffer->wp = buffer->rp + difference;
	} else { //empty buffer
		buffer->wp = buffer->rp = newMemory;
	}

	buffer->buffer = newMemory;
	buffer->buffersize = newSize;
	kfree(oldMem);
	return 0;
}

/* called by system call icotl */
long dm510_ioctl(
    struct file *filp,
    unsigned int cmd,   /* command passed from the user */
    unsigned long arg ) /* argument of the command */
{
	struct pipe* dev = filp->private_data;
	/* ioctl code belongs here */
	printk(KERN_INFO "DM510: ioctl called.\n");

	switch(cmd) {
		case GET_BUFFER_SIZE:
			return buffers->buffersize;
		case SET_BUFFER_SIZE: {
			int i;

			if(down_interruptible(&dev->semaphore)) {
				return -ERESTARTSYS; //restart;
			}

			for(i = 0;i < BUFFER_COUNT; i++) { //check if possible
				int used = buffers[i].buffersize - spacefree(buffers + i);
				if(used > arg) { //buffer argument is too small
					printk(KERN_INFO "argument is too small");
					up(&dev->semaphore);
					return -EINVAL; //inavlid argument
				}
			}
			for(i = 0;i < BUFFER_COUNT; i++) {
				resizeBuffer(buffers + i, arg);
			}

			up(&dev->semaphore);
		}
			break;
		case GET_MAXREADERS_COUNT: //Not used
			return maxReaders;
		case SET_MAXREADERS_COUNT: //Not used, example only
			maxReaders = arg;
			break;
	}

	return -EINVAL; //invalid argument
}

module_init( dm510_init_module );
module_exit( dm510_cleanup_module );

MODULE_AUTHOR( "Emil Ovcina, Ognjen Andric & Markus Tang-Jensen" );
MODULE_LICENSE( "GPL" );
