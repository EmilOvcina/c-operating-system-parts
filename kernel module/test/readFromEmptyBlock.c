#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include "../ioctl_commands.h"

int main(int argc, char* argv[]) {

	int reader = open("/dev/dm510-0", O_RDONLY|O_NONBLOCK);
	char buffer[10];

	int error = read(reader, buffer, 10);
	if(0 > error) {
		printf("Couldn't not read %d\n", error);
	} else {
		printf("message: %s\n", buffer);
	}

	return 0;
}
