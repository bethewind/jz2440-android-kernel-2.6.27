/*
* aaron
*/

#include <fcntl.h>
#include <stdio.h>
int main(int argc, char** argv)
{
	int fd;
	fd = open("/dev/led0", O_RDWR);

	if (fd == -1)
	    printf("open faled");

	if (argc != 2) {
	    printf("Usage:\n");
	    printf("%s <on|off>", argv[0]);
	}
	int val;
	if(strcmp(argv[1], "on") == 0) {
	    val = 1;
	} else {
	    val = 0;
	}

	write(fd,&val,4);

	return 0;
}




























