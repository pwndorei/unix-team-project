#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include "project.h"


extern key_t key; //declared in common.c
int cnt = 0; //for client-oriented, check chunk completion
int order = 0;//for client-oriented, id of server to resume execution(read chunk)
int mode = -1;//Client-Oriented(MODE_CLOR) or Server-Oriented(MODE_SVOR)
pid_t servers[NODENUM] = {0,};
pid_t clients[NODENUM] = {0,};

int
main()
{
}


//signal handlers

void
client_write_complete(int sig)
{
		//SIGUSR1 Handler
		cnt += 1;

		if(cnt == NODENUM)
		{
				kill(servers[order], SIGCONT);//resume server process
				order = (order + 1) % NODENUM;
				cnt = 0;
		}
}

void
server_read_complete(int sig)
{
		//SIGUSR2 Handler
		int i = 0;

		for(i = 0;i < NODENUM;i++)//resume all clients
		{
				kill(clients[i], SIGCONT);
		}
}

void
shutdown(int sig)
{
		int i = 0;
		for(i = 0; i < NODENUM; i++)
		{
				if(servers[i])
				{
						kill(servers[i], SIGINT);
						waitpid(servers[i], NULL, 0);
						servers[i] = 0;
				}
		}

		for(i = 0; i < NODENUM; i++)
		{
				if(clients[i])
				{
						kill(clients[i], SIGINT);
						waitpid(clients[i], NULL, 0);
						clients[i] = 0;
				}
		}
		puts("bye");
		exit(0);
}

void
on_child_exit(int sig)
{
		//SIGCHLD handler
		//SA_NOCLDSTOP -> ignore children's SIGSTOP & SIGCONT

		int i = 0;
		int exit_code = 0;
		pid_t exited_child = wait(&exit_code);
		for(i = 0;i < NODENUM; i++)
		{
				if(clients[i] == exited_child)
				{
						clients[i] = 0;
						break;
				}
		}
		for(i = 0;i < NODENUM; i++)
		{
				if(clients[i]) return;
		}
		//no return in for-loop -> all clients are terminated
		//terminated servers
		for(i = 0; i < NODENUM; i++)
		{
				if(servers[i])
				{
						kill(servers[i], SIGINT);
						waitpid(servers[i], NULL, 0);
						servers[i] = 0;
				}
		}
		
}

int
client_oriented_io()
{
		//register signal handler for SIGUSR1(from client), SIGUSR2(from server)

		struct sigaction act = {0,};

		act.sa_handler = client_write_complete;
		sigaction(SIGUSR1, &act, NULL);

		act.sa_handler = server_read_complete;
		sigaction(SIGUSR2, &act, NULL);

		act.sa_handler = shutdown;
		sigaction(SIGINT, &act, NULL);

		act.sa_handler = on_child_exit;
		act.sa_flags = SA_NOCLDSTOP;
		sigaction(SIGCHLD, &act, NULL);

		create_shm();//create shared memory

		return 0;
}

int
server_oriented_io()
{

		struct sigaction act = {0,};

		act.sa_handler = shutdown;
		sigaction(SIGINT, &act, NULL);

		act.sa_handler = on_child_exit;
		act.sa_flags = SA_NOCLDSTOP;
		sigaction(SIGCHLD, &act, NULL);

		return 0;
}
