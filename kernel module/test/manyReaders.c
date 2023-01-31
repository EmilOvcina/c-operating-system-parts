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
	int i;
 	if(1 >= argc) return 0;

 	dev1 = open("/dev/dm510-0", O_RDWR);

	for(i = 0; i <= 11; i++) {
 		dev2 = open("/dev/dm510-1", O_RDONLY);
		if(dev2 < 0) {
			printf("Couldn't open reader, too many readers");
			return 0;
		}
		printf("dm510-1 reader opened: %d\n", i);
	}
 	const char * msg = argv[1];
 	const size_t size = strlen(msg);
 	printf("Writing : %s\n", msg);
	write(dev2,msg,size);

  	char * buf = malloc(size*sizeof(*read));
  	ssize_t a = read(dev1,buf,size);

  	printf("Read from buf1: %s\n",buf);

  	return 0;
}
