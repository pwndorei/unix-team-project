#include <stdio.h>
#include <signal.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/type.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "project.h"

/*
* 
*/

extern int id;
extern const int msg[4];  // msg queue #1 ~ #4
const char* dat[4] = { "p1.dat","p2.dat","p3.dat","p4.dat" };
int fd = -1;

typedef struct {
	long mtype;
	int mtext[1];
}msgbuf;

void do_server_task(int mode)
{
	pid_t parent = getppid();
	int data[2] = { 0, };
	int msgid[4];
	int nbyte = 0;
	msgbuf msg;
	int i, msgi;

	if (mode == MODE_CLOR)  // client task
	{
		for (i = 0; i < 4; i++)  // creating msg queue #1 ~ #4, for server #1 ~ #4
		{
			msgid[i] = msgget(7777 + i, 0644 | IPC_CREAT | IPC_EXCL);
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




}
