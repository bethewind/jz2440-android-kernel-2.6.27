/*
* aaron
*/

#include <fcntl.h>
#include <stdio.h>
#include <poll.h>

int main(int argc, char** argv)
{
	int fd;
	fd = open("/dev/key0", O_RDWR);

	if (fd == -1)
	    printf("open failed!");

	int val =0;
	struct pollfd fds[1];
	fds[0].fd = fd;
	fds[0].events =POLLIN;
	int result = poll(fds, 1, 5000);
	if (result == 0)
	    printf("time off");
	else {
        read(fd,&val,4);
        printf("key:%d", val);
    }

	return 0;
}




























