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
	int i, pid;
 	if(1 >= argc) return 0;

 	dev1 = open("/dev/dm510-0", O_RDWR);

	const char * msg = argv[1];
 	const size_t size = strlen(msg);
 	printf("Writing : %s\n", msg);
	//write(dev1, msg, size);

	pid = fork();
	write(dev1, msg, size);
	dev2 = open("/dev/dm510-1", O_RDONLY);

  	char * buf = malloc(size*sizeof(*read));
	ssize_t b = read(dev2, buf, size);

	printf("reading from p%d: %s\n", pid, buf);
	printf("\n");

	exit(pid);
  	return 0;
}
