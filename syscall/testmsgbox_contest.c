#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "arch/x86/include/generated/uapi/asm/unistd_64.h"

int main(int argc, char** argv) {

	char* in = "Hello Worl33d!";
	char msg[50];
	int msglen;

	/* SEND a message containing in*/
	syscall(__NR_dm510_msgbox_put, in, strlen(in) +1 );

	fork();

	/* READ message */
	msglen = syscall(__NR_dm510_msgbox_get, msg, 50);
	
	printf("%d\n", msglen);

	exit(0);

	return 0;
}
