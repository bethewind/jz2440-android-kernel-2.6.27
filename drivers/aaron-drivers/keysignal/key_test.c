/*
* aaron
*/

#include <fcntl.h>
#include <stdio.h>
#include <poll.h>
#include <signal.h>
#include <unistd.h>
int fd;
void sighandler (int signum)
{
	 printf("sighandler");
    int val =0;

    read(fd,&val,4);
    printf("key:%d", val);
	val = 1;
	write(fd,&val,4);
}

int main(int argc, char** argv)
{
    int Oflags;
    signal(SIGIO, sighandler);

	fd = open("/dev/key0", O_RDWR);

	if (fd == -1)
	    printf("open failed!");

	fcntl(fd, F_SETOWN, getpid());

    Oflags = fcntl(fd, F_GETFL);

    fcntl(fd, F_SETFL, Oflags | FASYNC);

    while (1) {
        sleep(1000);
    }



	return 0;
}

