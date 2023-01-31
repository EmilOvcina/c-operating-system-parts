#include "stddef.h"
#include "linux/slab.h"
#include "linux/uaccess.h"
#include "linux/kernel.h"
#include "linux/unistd.h"

typedef struct _msg_t msg_t;

struct _msg_t{
	msg_t* previous;
	int length;
	char* message;
};

static msg_t *bottom = NULL;
static msg_t *top = NULL;

unsigned long flags;

asmlinkage
int dm510_msgbox_put(char* buffer, int length) {

	if(length < 0) { 
		return -22; //invalid argument
	}

	msg_t* msg = kmalloc(sizeof(msg_t), GFP_KERNEL);
	msg->previous = NULL;
	msg->length = length;
	msg->message = kmalloc(length, GFP_KERNEL);

	if(msg != NULL) {
		if(msg->message == NULL) {
			kfree(msg);
			return -12; //out of memory
		}

		if(access_ok(buffer, length) != 0) {
			if(copy_from_user(msg->message, buffer, length) != 0) {
				kfree(msg->message);
				kfree(msg);
				return -90; //message too long
			}
		} else {
			kfree(msg->message);
			kfree(msg);
			return -1; //operation not permitted
		}

		local_irq_save(flags); //concurrency - disable and save
		if(bottom == NULL) {
			bottom = msg;
			top = msg;
		} else {
			/*not empty stack*/
			msg->previous = top;
			top = msg;
		}
		local_irq_restore(flags); // reenable and restore
	} else {
		return -12; //out of memory
	}
	return 0;
}

asmlinkage
int dm510_msgbox_get(char* buffer, int length) {
	local_irq_save(flags); //save and disable
	if(top != NULL) {
		msg_t* msg = top;
		int mlength = msg->length;
		top = msg->previous;
		if(mlength > length){
			kfree(msg->message);
			kfree(msg);
			return -90; //message too long
		}
		
		local_irq_restore(flags);
		/* copy message */
		if(access_ok(buffer, mlength)) {
			if(copy_to_user(buffer, msg->message, mlength) != 0) {
				kfree(msg->message);
				kfree(msg);
				return -90; //message too long
			}
		} else {
			kfree(msg->message);
			kfree(msg);
			return -1; //operation not permitted
		}

		/*free */
		kfree(msg->message);
		kfree(msg);

		return (mlength - 1);
	}
	local_irq_restore(flags);
	return -6; //no such device or address
}
