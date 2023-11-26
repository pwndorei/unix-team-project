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

extern int shmid;
extern int msgqid;

typedef struct{
	long mtype;
	int mtext[1];
}msgbuf;

const char* dat[4] = { "p1.dat",
					  "p2.dat",
					  "p3.dat",
					  "p4.dat" };

int fd = -1;//fd for p*.dat
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
	int data[2] = { 0, };
	int nbyte = 0;
	int i, msgi, msgid[4];
	msgbuf msg;
	//open p*.dat file
	fd = open(dat[id], O_RDONLY);
	if (fd == -1)
	{
		exit(E_OPEN_FILE);
	}

	if (mode == MODE_CLOR)
	{
		shm_addr = shmat(shmid, NULL, 0);
		while (1)
		{
			nbyte = read(fd, &data, sizeof(int) * 2);
			if (nbyte == -1)
			{
				exit(E_READ_FILE);
			}
			else if (!nbyte)//end-of-file
			{
				close(fd);
				shmdt(shm_addr);
				//signal to parent
				break;
			}
			shm_addr[id] = data[0];//write data
			shm_addr[id + NODENUM] = data[1];
			kill(parent, SIGUSR1);//send SIGUSR1 to parent, notify "data is written!"
			raise(SIGSTOP);//stop until parent's SIGCONT
		}
	}
	else if (mode == MODE_SVOR)
	{
		for (i = 0; i < 4; i++)  // creating msg queue #1 ~ #4, for server #1 ~ #4
		{
			msgid[i] = msgget(msgqid) + i, 0644 | IPC_CREAT | IPC_EXCL);
			if (msgid[i] == -1)
			{
				perror("Msg queue");
				exit(-1);
			}
		}

		fd = open(dat[id], O_RDONLY);
		if (fd == -1)
		{
			perror("Open file:");
			exit(-1);
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
	}
	exit(SUCCESS);
}
