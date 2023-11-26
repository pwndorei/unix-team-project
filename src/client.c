#include <stdio.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "project.h"

static const char *dat[4] = {
		"p1.dat", 
		"p2.dat", 
		"p3.dat", 
		"p4.dat"
};

static int fd = -1;//fd for p*.dat
extern int id;// node's id, common.c
static int* shm_addr = NULL;

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
		raise(SIGSTOP);//wait until server ready
		fd = open(dat[id], O_RDONLY);
		if(fd == -1)
		{
				perror(dat[id]);
				exit(-1);
		}

		fd = open(dat[id], O_RDONLY);
		if (fd == -1)
		{
				shm_addr = shmat(shmid, NULL, 0);
				if(shm_addr == (void*)-1)
				{
						perror("shmat");
						exit(-1);
				}
				while(1)
				{
						nbyte = read(fd, &data, sizeof(int)*2);
#ifdef DEBUG
						printf("client[%d]: data->%d,%d\n", id, data[0], data[1]);
#endif
						if(nbyte == -1)
						{
								exit(-1);
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
#ifdef DEBUG
						printf("client[%d]: SIGUSR1 sent to parent(%d)\n", id, parent);
#endif
						raise(SIGSTOP);//stop until parent's SIGCONT
				}
		}

		msgi = 0;

		while (1)  // send data to msg queue #1 ~ #4
		{
			nbyte = read(fd, &data, sizeof(int) * 2);
			if (nbyte == -1)
			{
				perror("Read file");
				exit(-1);
			}
			else if (!nbyte)//end-of-file
			{
				close(fd);  // close client's own file
				for (i = 0; i < 4; i++)  // delete four msg queues
					msgctl(msgid[i], IPC_RMID, (struct msqid_ds*)NULL);

				//signal to parent
				break;
			}
			// send two data
			msg.mtext[0] = data[0];
			msg.mtype = id;
			msgsnd(msgid[msgi], msg, sizeof(int), 0);
			msg.mtext[0] = data[1];
			msg.mtype = id + NODENUM;
			msgsnd(msgid[msgi++], msg, sizeof(int), 0);
			msgi %= NODENUM;

			kill(parent, SIGUSR1);//send SIGUSR1 to parent, notify "data is written!"
			raise(SIGSTOP);//stop until parent's SIGCONT

		}
		exit(0);//no return!, do_client_task is called inside of for-loop with fork() common.c:24
		//parent's SIGCHLD handler will kill servers
}
