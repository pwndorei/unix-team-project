#include <stdio.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "project.h"

extern int shmid;
extern int msgid[NODENUM];
extern int id;//in common.c, id for server
extern int mode;//from project.c, indicate client/server-oriented mode

static const char *dat[4] = {
		"node1.dat",
		"node2.dat",
		"node3.dat",
		"node4.dat"
};
static int fd = -1;
static int* shm_addr = NULL;
static int ser_buf[CHKSIZE] = {0, };
static pid_t parent;

struct timeval io_start;
long rwtime;
struct timeval rwstart;
struct timeval rwend;

static void
read_chunk_shm(int sig)
{
#ifdef TIMES
	gettimeofday(&rwstart, NULL);
#endif
		//SIGUSR1 handler
		write(fd, shm_addr, CHKSIZE);// write data
#ifdef TIMES
	gettimeofday(&rwend, NULL);
	rwtime += rwend.tv_sec - rwstart.tv_sec;
#endif
#ifdef DEBUG
		puts("server: read complete");
#endif
		kill(parent, SIGUSR2);//send SIGUSR2 to parent, notify "complete reading!"
}

static void
shutdown(int sig)
{
		//SIGINT handler

		if(mode == MODE_CLOR)
		{
				close(fd);
				shmdt(shm_addr);
#ifdef TIMES
	stop_timer(&io_start, "CLOR I/O");
	printf("rwtime = %ld\n", rwtime);
#endif	
		}

		else if(mode == MODE_SVOR)
		{
					close(fd);  // close own file
					msgctl(msgid[id], IPC_RMID, NULL);  // close own message queue
					printf("message queue #%d closed.\n", id);
#ifdef TIMES
	stop_timer(&io_start, "SVOR I/O");
	printf("rwtime = %ld\n", rwtime);
#endif
		}
		exit(0);
}


void
do_server_task(int mode)
{
		parent = getppid();
		int data[2] = {0,};
		int nbyte = 0;
		struct sigaction act = {0,};
		fd = open(dat[id], O_WRONLY | O_CREAT | O_TRUNC, 0644);  // open p*.dat.
		act.sa_handler = shutdown;
		if(fd == -1)
		{
				perror(dat[id]);
				exit(-1);
		}

		if(mode == MODE_CLOR)
		{
				sigaction(SIGINT, &act, NULL);
				act.sa_handler = read_chunk_shm;
				act.sa_flags = 0;
				sigaction(SIGUSR1, &act, NULL);

				shm_addr = shmat(shmid, NULL, 0);
				if(shm_addr == (void*)-1)
				{
						perror("shmat");
						exit(-1);
				}
				while(1)
				{
						pause();
				}
		}
		else if(mode == MODE_SVOR)
		{
		    static msgbuf msg;
			sigset_t mask;
			sigfillset(&mask);
			sigdelset(&mask, SIGINT);
			act.sa_mask = mask;
			sigaction(SIGINT, &act, NULL);  // ignore other signals while shutdown

			while(1)
			{
				for (int i = 0; i < 8; i++)  // automatically starts
				{
					nbyte = msgrcv(msgid[id], &msg, sizeof(int), -8, 0);  // receive messages from own msg queue.
					if (nbyte == -1)
					{
						perror("msgrcv");
						printf("server #%d\n", id);
						exit(-1);
					}
					ser_buf[msg.mtype - 1] = msg.mtext[0];  // write data to buffer of a chunk! 'mtype' of msg is data's index of the chunk.
				} 
#ifdef TIMES
	gettimeofday(&rwstart, NULL);
#endif
				write(fd, ser_buf, CHKSIZE);  // write data to file
#ifdef TIMES
	gettimeofday(&rwend, NULL);
	rwtime += rwend.tv_sec - rwstart.tv_sec;
#endif
				kill(parent, SIGUSR2);  // every 8 receives, kill SIGUSR2 to parent to tell "This server's task has done. Go to the next server!"

			}
		}
		exit(0);//no return!, do_client_task is called inside of for-loop with fork() common.c:24
}
