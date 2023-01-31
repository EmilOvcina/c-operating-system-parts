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

int i;

  if(1 >= argc) return 0;
  dev1 = open("/dev/dm510-0", O_RDWR|O_NONBLOCK);
  dev2 = open("/dev/dm510-1", O_RDWR|O_NONBLOCK);

  const char * msg = "1";
  const size_t size = strlen(msg);
size_t final = 1;
  printf("Writing : %s\n", msg);

for(i = 0; i < 2048; i++) {
  result = write(dev1,msg,size);
  if(0 > result){
    printf("Message size %lu is larger then buffer size %d\n", size,
		 ioctl(dev1,GET_BUFFER_SIZE));

	printf("errorcode: %d\n", result);
    return 0;
  }
final++;
}

printf("wrting done");

  char * buf = malloc(final*sizeof(*read));
  ssize_t a = read(dev2,buf,final);

  printf("Read %ld from buffer: %s\n",a,buf);

free(buf);

  return 0;
}
