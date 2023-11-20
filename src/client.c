#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "project.h"

extern int shmid;
const char *dat[4] = {"p1.dat", 
					  "p2.dat", 
					  "p3.dat", 
					  "p4.dat"};

int fd = -1;//fd for p*.dat
extern int id;// node's id, common.c
int* shm_addr = NULL;

/*
 * signal for sync (Client-Oriented)
 * client: SIGUSR1 to parent after writing 2 int at shm for 1 chunk
 * parent: receive SIGUSR1, increase count from 0 to 4, if(count == 4) all client write to shm(chunk i complete) send signal to server i & reset count
 * client: pause(), wait for parent's SIGUSR1
 * server: read chunk & write to file, send SIGUSR2 to parent
 * parent: send SIGUSR1 to all client, resume execution
*/

void
do_client_task(int mode)
{
		//open p*.dat file
		fd = open(dat[id], O_RDONLY);
		if(fd == -1)
		{
				exit(E_OPEN_FILE);
		}

		if(mode == MODE_CLOR)
		{
				shm_addr = shmat(shmid, NULL, 0);
		}
		else if(mode == MODE_SVOR)
		{
		}
		exit(SUCCESS);
}

