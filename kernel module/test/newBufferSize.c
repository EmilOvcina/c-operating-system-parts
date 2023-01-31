#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include "../ioctl_commands.h"

int main(int argc, char const *argv[]) {
 	int dev1, dev2;
	int result;
 	if(1 >= argc) return 0;

 	dev1 = open("/dev/dm510-0", O_RDWR);
 	dev2 = open("/dev/dm510-1", O_RDWR);

 	const char * msg = argv[1];
 	const size_t size = strlen(msg);
 	printf("Writing : %s\n", msg);

	result = write(dev2,msg,size);

	printf("previous buffer size: %d\n", ioctl(dev1, GET_BUFFER_SIZE));
	ioctl(dev1,SET_BUFFER_SIZE, 1028);
	printf("new buffer size: %d\n", ioctl(dev1, GET_BUFFER_SIZE));

 	if(0 > result){
   		printf("Message size %lu is larger then buffer size %d\n", size, ioctl(dev1,GET_BUFFER_SIZE));
		printf("errorcode: %d", result);
    		return 0;
  	}

  	char * buf = malloc(size*sizeof(*read));
  	ssize_t a = read(dev1,buf,size);

  	printf("Read from buf1: %s\n",buf);

  	return 0;
}
