#include <stdio.h>
#include <signal.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "project.h"

extern int shmid;
static const char *dat[4] = {
		"p1.dat", 
		"p2.dat", 
		"p3.dat", 
		"p4.dat"
};

static int fd = -1;//fd for p*.dat
extern int id;// node's id, common.c
int* shm_addr = NULL;

/*
 * signal for sync (Client-Oriented)
 * client: SIGUSR1 to parent after writing 2 int at shm for 1 chunk
 * parent: receive SIGUSR1, increase count from 0 to 4, if(count == 4) all client write to shm(chunk i complete) send signal to server i & reset count
 * client: raise(SIGSTOP), wait for parent's SIGUSR1
 * server: read chunk & write to file, send SIGUSR2 to parent
 * parent: send SIGUSR1 to all client, resume execution
*/


void
do_client_task(int mode)
{
		pid_t parent = getppid();
		int data[2] = {0,};
		int nbyte = 0;
		//open p*.dat file
		fd = open(dat[id], O_RDONLY);
		if(fd == -1)
		{
				exit(E_OPEN_FILE);
		}

		if(mode == MODE_CLOR)
		{
			
				#ifdef TIMES
        			struct timeval stime,etime;
        			int time_result;
				#endif

				shm_addr = shmat(shmid, NULL, 0);
				while(1)
				{
						nbyte = read(fd, &data, sizeof(int)*2);
						if(nbyte == -1)
						{
								exit(E_READ_FILE);
						}
						else if(!nbyte)//end-of-file
						{
								close(fd);
								shmdt(shm_addr);
								break;
						}
						shm_addr[id] = data[0];//write data
						shm_addr[id + NODENUM] = data[1];
						kill(parent, SIGUSR1);//send SIGUSR1 to parent, notify "data is written!"
						raise(SIGSTOP);//stop until parent's SIGCONT
				}

				#ifdef TIMES
        			gettimeofday(&stime, NULL);
				#endif

				printf("**Program IO, communication and the rest\n");
				printf("**Program IO\n");
				printf("**Program communication\n");

				#ifdef TIMES
        		gettimeofday(&etime, NULL);
        		time_result = etime.tv_usec - stime.tv_usec;
        		printf("Client_oriented_io TIMES == %ld %ld %ld\n", etime.tv_usec, stime.tv_usec, time_result);
				#endif


		}
		else if(mode == MODE_SVOR)
		{
		}
		exit(SUCCESS);//no return!, do_client_task is called inside of for-loop with fork() common.c:24
		//parent's SIGCHLD handler will kill servers
}

