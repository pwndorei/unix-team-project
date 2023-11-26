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
static int* shm_addr = NULL;

void
do_server_task(int mode)
{
		pid_t parent = getppid();
		int data[2] = {0,};
		int nbyte = 0;
		fd = open(dat[id], O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if(fd == -1)
		{
				perror(dat[id]);
				exit(-1);
		}

		if(mode == MODE_CLOR)
		{
				shm_addr = shmat(shmid, NULL, 0);
				if(shm_addr == (void*)-1)
				{
						perror("shmat");
						exit(-1);
				}
				while(1)
				{
#ifdef DEBUG
						printf("server[%d]: wait\n", id);
#endif
						raise(SIGSTOP);//stop until parent's SIGCONT
						write(fd, shm_addr, CHKSIZE);// write data
#ifdef DEBUG
						printf("server[%d]: read complete\n", id);
#endif
						kill(parent, SIGUSR2);//send SIGUSR2 to parent, notify "complete reading!"
				}
		}
		else if(mode == MODE_SVOR)
		{

		}
		exit(0);//no return!, do_client_task is called inside of for-loop with fork() common.c:24
}
