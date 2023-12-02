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
extern int msgid[4];
static int msgi = 0;
extern int client_pipe[2];
extern int mode;//from project.c, MODE_CLOR | MODE_SVOR

static pid_t parent;
static int* shm_addr = NULL;
static int fd = -1;//fd for p*.dat

/*
 * signal for sync (Client-Oriented)
 * client: SIGUSR1 to parent after writing 2 int at shm for 1 chunk
 * parent: receive SIGUSR1, increase count from 0 to 4, if(count == 4) all client write to shm(chunk i complete) send signal to server i & reset count
 * client: raise(SIGSTOP), wait for parent's SIGUSR1
 * server: read chunk & write to file, send SIGUSR2 to parent
 * parent: send SIGUSR1 to all client, resume execution
*/

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
		else if(mode == MODE_SVOR)
		{

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

		nbytes = read(fd, data, 8);
		if(nbytes == 0)
		{
				puts("client: EOF");
				exit(0);
		}
		shm_addr[id] = data[0];//write data
		shm_addr[id + NODENUM] = data[1];

		lock.l_type = F_WRLCK;
		lock.l_len = 0;
		lock.l_start = 0;
		lock.l_whence = SEEK_SET;
		fcntl(client_pipe[WREND], F_SETLKW, &lock);
#ifdef DEBUG
		puts("client worker: get lock");
#endif

		write(client_pipe[WREND], "\0", 1);

		lock.l_type = F_UNLCK;
		lock.l_len = 0;
		lock.l_start = 0;
		lock.l_whence = SEEK_SET;
#ifdef DEBUG
		puts("client worker: release lock");
#endif
		fcntl(client_pipe[WREND], F_SETLKW, &lock);

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
		nbytes = read(fd, data, 8);
		shm_addr[id] = data[0];//write data
		shm_addr[id + NODENUM] = data[1];
		kill(-getpid(), SIGUSR1);
		
		if(nbytes == 0)
		{
				kill(parent, SIGINT);
				exit(0);
		}

		for(i = 0; i < NODENUM-1 ; i++)
		{
				do
				{
						nbytes = read(client_pipe[RDEND], &buf, 1);
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

		kill(parent, SIGUSR1);

}


void
do_client_task(int mode)
{
		struct sigaction act = {0,};
		parent = getppid();
		int data[2] = {0,};
		int nbyte = 0;
		int msgi = 0;
		int i = 0;
		struct flock lock = {.l_whence=SEEK_SET, .l_len=0, .l_start=0};
		long buf = 0;
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
				sigaction(SIGUSR1, &act, NULL);

				shm_addr = shmat(shmid, NULL, 0);
				if(shm_addr == (void*)-1)
				{
						perror("shmat");
						exit(-1);
				}
				if(id == 0)//leader
				{
						act.sa_handler = client_leader;
						sigaction(SIGUSR2, &act, NULL);
						act.sa_handler = SIG_IGN;
						sigaction(SIGUSR1, &act, NULL);
				}

				while(1)
				{
						pause();

				}
				exit(0);
		}

		else if(mode == MODE_SVOR)
		{
			while (1)  // send data to msg queue #1 ~ #4
			{
				msgbuf msg;

				nbyte = read(fd, &data, sizeof(int) * 2);
				if (nbyte == -1)
				{
					perror("Read file");
					exit(-1);
				}
				else if (!nbyte)//end-of-file
				{
					close(fd);  // close client's own file
					//signal to parent
					break;
				}
				// send two data
				msg.mtext[0] = data[0];
				msg.mtype = id;
				msgsnd(msgid[msgi], &msg, sizeof(int), 0);
				msg.mtext[0] = data[1];
				msg.mtype = id + NODENUM;
				msgsnd(msgid[msgi++], &msg, sizeof(int), 0);
				msgi %= NODENUM;

				kill(parent, SIGUSR1);//send SIGUSR1 to parent, notify "data is written!"
				raise(SIGSTOP);//stop until parent's SIGCONT
	
			}
			exit(0);//no return!, do_client_task is called inside of for-loop with fork() common.c:24
			//parent's SIGCHLD handler will kill servers
		}
}
