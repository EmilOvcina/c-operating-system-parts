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

  result = write(dev1,msg,size);
  if(0 > result){
    printf("Message size %lu is larger then buffer size %d\n", size,
		 ioctl(dev1,GET_BUFFER_SIZE));

	printf("errorcode: %d\n", result);
    return 0;
  }

  char * buf = malloc(size*sizeof(*read));
  ssize_t a = read(dev2,buf,size);

  printf("Read from buffer: %s\n",buf);

  return 0;
}
