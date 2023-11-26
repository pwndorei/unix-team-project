#include <stdio.h>
#include <signal.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "project.h"

extern int shmid;
extern int id;//in common.c, id for server
static const char *dat[4] = {
		"node1.dat",
		"node2.dat",
		"node3.dat",
		"node4.dat"
};
static int fd = -1;
int* shm_addr = NULL;

void
do_server_task(int mode)
{
		pid_t parent = getppid();
		int data[2] = {0,};
		int nbyte = 0;
		//open node*.dat file
		fd = open(dat[id], O_WRONLY | O_CREAT);
		if(fd == -1)
		{
				exit(E_OPEN_FILE);
		}

		if(mode == MODE_CLOR)
		{
				shm_addr = shmat(shmid, NULL, 0);
				while(1)
				{
						raise(SIGSTOP);//stop until parent's SIGCONT
						write(fd, shm_addr, CHKSIZE);// write data
						kill(parent, SIGUSR2);//send SIGUSR2 to parent, notify "complete reading!"
				}
		}
		else if(mode == MODE_SVOR)
		{
		}
		exit(SUCCESS);//no return!, do_client_task is called inside of for-loop with fork() common.c:24
}
