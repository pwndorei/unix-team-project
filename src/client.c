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

#define RDEND 0
#define WREND 1

static const char *dat[4] = {
		"p1.dat", 
		"p2.dat", 
		"p3.dat", 
		"p4.dat"
};

extern int id;// node's id, common.c
extern int shmid;
extern int msgid[NODENUM];
static int msgi = 0;
extern int client_pipe[2];
extern int mode;//from project.c, MODE_CLOR | MODE_SVOR

static pid_t parent;
static int* shm_addr = NULL;
static int fd = -1;//fd for p*.dat
static int data[2] = {0,};
static int nbyte = 0;

struct timeval io_start;
long rwtime;
long commtime;
struct timeval rwstart;
struct timeval rwend;
struct timeval commstart;
struct timeval commend;

/*
 * signal for sync (Client-Oriented)
 * client: SIGUSR1 to parent after writing 2 int at shm for 1 chunk
 * parent: receive SIGUSR1, increase count from 0 to 4, if(count == 4) all client write to shm(chunk i complete) send signal to server i & reset count
 * client: raise(SIGSTOP), wait for parent's SIGUSR1
 * server: read chunk & write to file, send SIGUSR2 to parent
 * parent: send SIGUSR1 to all client, resume execution
*/

/*
 * signal for sync (Server-Oriented)
 * client: When a client gets SIGUSR1, it sends message to message queue #order. At first cycle, raise(SIGUSR1) itself to start the task.
 * server: Waits until a message comes to server's own message queue (blocked by msgrcv()), then receives the message. every 8 rcvs, sends SIGUSR2 to parent.
 * parent: When the parent gets SIGUSR2, sends SIGUSR1 to every clients to fill the message queue. 
 * shutdown(c): When any of clients reaches EOF, closes own file descripter and sends SIGINT to parent, exit.
 * shutdown(p): When the parent gets SIGINT, wait all clients to be exited, then sends SIGINT to every servers, and wait until they all exited.
 * shutdown(p): + The parent will never react to any SIGINT, using func signal(SIGINT, do_nothing);
 * shutdown(s): When a server gets SIGINT, closes its own file descripter and message queue, exit.
*/


static ssize_t
write_lock(int fd, const void* buf, size_t count)
{		//write with lock
		struct flock lock = {.l_whence=SEEK_SET, .l_len=0, .l_start=0};
		ssize_t nbytes = 0;

		lock.l_type = F_WRLCK;
		lock.l_len = 0;
		lock.l_start = 0;
		lock.l_whence = SEEK_SET;
		fcntl(client_pipe[WREND], F_SETLKW, &lock);
#ifdef DEBUG
		puts("client worker: get lock");
#endif

#ifdef TIMES
	gettimeofday(&rwstart, NULL);
#endif
		nbytes = write(fd, buf, count);//write
#ifdef TIMES
	gettimeofday(&rwend, NULL);
	rwtime += rwend.tv_sec - rwstart.tv_sec;
#endif

		lock.l_type = F_UNLCK;
		lock.l_len = 0;
		lock.l_start = 0;
		lock.l_whence = SEEK_SET;
#ifdef DEBUG
		puts("client worker: release lock");
#endif
		fcntl(client_pipe[WREND], F_SETLKW, &lock);

		return nbytes;
}


static void
shutdown(int sig)
{
		//SIGINT handler

		if(mode == MODE_CLOR)
		{
				shmdt(shm_addr);
				close(fd);//p*.dat
				close(client_pipe[RDEND]);
				close(client_pipe[WREND]);		
		}
		exit(0);
}

static void
client_worker(int sig)
{
		//SIGUSR1 Handler
#ifdef DEBUG
		puts("client: worker");
#endif

		struct flock lock = {.l_whence=SEEK_SET, .l_len=0, .l_start=0};
		int nbytes = 0;
		int data[2] = {0,};

		nbytes = read(fd, data, 8);//read file data
		if(nbytes == 0)
		{
				puts("client: EOF");
				exit(0);
		}
#ifdef TIMES
	gettimeofday(&rwstart, NULL);
#endif
		shm_addr[id] = data[0];//write data
		shm_addr[id + NODENUM] = data[1];
#ifdef TIMES
	gettimeofday(&rwend, NULL);
	rwtime += rwend.tv_sec - rwstart.tv_sec;
#endif
		write_lock(client_pipe[WREND], "\0", 1);//notify write completion via clients pipe

}

static void
client_leader(int sig)
{
#ifdef DEBUG
		puts("client: leader");
#endif
		int nbytes = 0;
		int data[2] = {0,};
		int i = 0;
		char buf;
		nbytes = read(fd, data, 8);//read file data
#ifdef TIMES
	gettimeofday(&rwstart, NULL);
#endif
		shm_addr[id] = data[0];//write data
		shm_addr[id + NODENUM] = data[1];
#ifdef TIMES
	gettimeofday(&rwend, NULL);
	rwtime += rwend.tv_sec - rwstart.tv_sec;
	commtime += rwend.tv_sec - rwstart.tv_sec;
#endif

		kill(-getpid(), SIGUSR1);//SIGUSR1 to all other clients -> invoke sighandler(client_worker)
		
		if(nbytes == 0)
		{
				kill(parent, SIGINT);
				exit(0);
		}

		for(i = 0; i < NODENUM-1 ; i++)
		{
				do
				{
#ifdef TIMES
	gettimeofday(&rwstart, NULL);
#endif
						nbytes = read(client_pipe[RDEND], &buf, 1);//read data from clients pipe
#ifdef TIMES
	gettimeofday(&rwend, NULL);
	rwtime += rwend.tv_sec - rwstart.tv_sec;
#endif
#ifdef DEBUG
						if(nbytes == -1)
						{
								perror("client leader: read return -1 ");
						}
#endif
				}while(nbytes == -1);
		}

#ifdef DEBUG
		puts("client: chunk write complete");
#endif

		//notify chunk write completion to parent via signal
		kill(parent, SIGUSR1);

}

void
svor_client(int sig)
{
		// SIGUSR1 handler for SVOR
		// When a client receives SIGUSR1, it does its read-and-send task.
		struct msqid_ds buf;
		msgbuf msg;

#ifdef TIMES
	gettimeofday(&rwstart, NULL);
#endif
		nbyte = read(fd, &data, sizeof(int) * 2);//read data from own file
#ifdef TIMES
	gettimeofday(&rwend, NULL);
	rwtime += rwend.tv_sec - rwstart.tv_sec;
#endif

		if (nbyte == -1)
		{
			perror("Read file");
			exit(-1);
		}
		else if (!nbyte)//end-of-file
		{
				close(fd);  // close client's own file
				kill(parent, SIGINT); // kill SIGINT to tell the parent that client is going to exit.
				exit(0);
		}
		// send two data
		msg.mtext[0] = data[0];
		msg.mtype = id + 1;
#ifdef TIMES
	gettimeofday(&commstart, NULL);
#endif
		msgsnd(msgid[msgi], &msg, sizeof(int), 0); // send msg to msg queue #msgi
#ifdef TIMES
	gettimeofday(&commend, NULL);
	commtime += commend.tv_sec - commstart.tv_sec;
#endif
		msg.mtext[0] = data[1];
		msg.mtype = id + NODENUM + 1;
#ifdef TIMES
	gettimeofday(&commstart, NULL);
#endif
		msgsnd(msgid[msgi], &msg, sizeof(int), 0);
#ifdef TIMES
	gettimeofday(&commend, NULL);
	commtime += commend.tv_sec - commstart.tv_sec;
#endif
		msgi++;
		msgi %= NODENUM;  // go to next message queue.
}

void
do_client_task(int mode)  // Client's task. Automatically started when a client process is generated.
{
		struct sigaction act = {0,};
		parent = getppid();
		int i = 0;
		char buf;
	
		struct flock lock = {.l_whence=SEEK_SET, .l_len=0, .l_start=0};
		//open p*.dat file
		fd = open(dat[id], O_RDONLY);

		act.sa_handler = shutdown;
		act.sa_flags = 0;
		sigaction(SIGINT, &act, NULL);

		if(fd == -1)
		{
				perror(dat[id]);
				exit(-1);
		}
		if (mode == MODE_CLOR)
		{
				act.sa_handler = client_worker;
				act.sa_flags = SA_NODEFER;
				sigaction(SIGUSR1, &act, NULL);//signal handler for client worker

				shm_addr = shmat(shmid, NULL, 0);//allocate shared memory
				if(shm_addr == (void*)-1)
				{
						perror("shmat");
						exit(-1);
				}
				if(id == 0)//leader
				{
						act.sa_handler = client_leader;
						sigaction(SIGUSR2, &act, NULL);//signal handler for client leader
						act.sa_handler = SIG_IGN;
						sigaction(SIGUSR1, &act, NULL);
						for(i = 0; i < NODENUM-1; i++)
						{
#ifdef TIMES
	gettimeofday(&rwstart, NULL);
#endif
								read(client_pipe[RDEND], &buf, 1);//read from clients pipe -> sync with client workers
#ifdef TIMES
	gettimeofday(&rwend, NULL);
	rwtime += rwend.tv_sec - rwstart.tv_sec;
#endif
						}
						raise(SIGUSR2);//start working
				}
				else
				{
						write_lock(client_pipe[WREND], "\0", 1);//write to clients pipe -> notice to client leader that initialization is finished
				}

				while(1)
				{
						pause();

				}
				exit(0);
		}

		else if(mode == MODE_SVOR)
		{	
			act.sa_handler = svor_client;
			sigaction(SIGUSR1, &act, NULL);  // allocate SIGUSR1 handler.
			raise(SIGUSR1);  // raise SIGUSR1 itself, so that the client can start the task stably.
			while (1)
			{
					pause(); 
					// Wait for SIGUSR1 or SIGINT from parent. No break, SIGINT handler will kill the client.
			}
			exit(0);//no return!, do_client_task is called inside of for-loop with fork() common.c:24
			//parent's SIGINT handler will kill servers
		}
}
