#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "arch/x86/include/generated/uapi/asm/unistd_64.h"

int main(int argc, char** argv) {

	char* in = "Hello Worl33d!";
	char msg[5];
	int msglen;

	/* SEND a message containing in*/
	syscall(__NR_dm510_msgbox_put, in, strlen(in) +1 );

	/* READ message */
	msglen = syscall(__NR_dm510_msgbox_get, msg, 5);
	printf("%d\n", msglen);

	return 0;
}
