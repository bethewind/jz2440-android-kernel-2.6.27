/*
* aaron
*/

#include <fcntl.h>
#include <stdio.h>
int main(int argc, char** argv)
{
	int fd;
	fd = open("/dev/key0", O_RDWR);

	if (fd == -1)
	    printf("open failed!");

	int val =0;
	while (1) {
	    read(fd,&val,4);
	    printf("key:%d\n", val);
	}

	return 0;
}




























