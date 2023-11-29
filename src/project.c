#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include "project.h"
#include <sys/time.h>

extern key_t key; //declared in common.c
int cnt = 0; //for client-oriented, check chunk completion
int order = 0;//for client-oriented, id of server to resume execution(read chunk)
int mode = -1;//Client-Oriented(MODE_CLOR) or Server-Oriented(MODE_SVOR)
pid_t servers[NODENUM] = {0,};
pid_t clients[NODENUM] = {0,};
extern int client_pipe[2];

int
main()
{
#ifdef DEBUG
		printf("parent pid: %d\n",getpid());
#endif

		struct timeval io_start;
		create_source_data();

		start_timer(&io_start);
		client_oriented_io();
		stop_timer(&io_start, "IO");
		
		start_timer(&io_start);
		server_oriented_io();
		stop_timer(&io_start, "IO");
		
}


//signal handlers

void
client_write_complete(int sig)
{
		//SIGUSR1 Handler

#ifdef DEBUG
		puts("parent: SIGUSR1 caught");
#endif

		kill(servers[order], SIGCONT);//resume server process
		order = (order + 1) % NODENUM;
}

void
server_read_complete(int sig)
{
		//SIGUSR2 Handler
#ifdef DEBUG
		puts("parent: server complete reading, wake all clients");
#endif
		kill(clients[0], SIGUSR2);
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
#ifdef DEBUG
						printf("parent: client[%d] exited\n", i);
#endif
						clients[i] = 0;
						break;
				}
		}
		for(i = 0;i < NODENUM; i++)
		{
				if(clients[i]) return;
		}
		//no return in for-loop -> all clients are terminated

#ifdef DEBUG
		puts("parent: all clients exited, kill all servers");
#endif

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
		
		exit(0);
}

int
client_oriented_io()
{
		//register signal handler for SIGUSR1(from client), SIGUSR2(from server)

		struct sigaction act = {0,};

		act.sa_handler = client_write_complete;
		sigaction(SIGUSR1, &act, NULL);

		act.sa_flags = 0;
		sigemptyset(&(act.sa_mask));
		act.sa_handler = server_read_complete;
		sigaction(SIGUSR2, &act, NULL);

		act.sa_flags = 0;
		sigemptyset(&(act.sa_mask));
		act.sa_handler = shutdown;
		sigaction(SIGINT, &act, NULL);

		create_shm();//create shared memory
#ifdef DEBUG
		puts("Start");
#endif

		gen_node(servers, NODENUM, do_server_task, MODE_CLOR);
		pipe(client_pipe);
		gen_node(clients, NODENUM, do_client_task, MODE_CLOR);

		sleep(1);
		kill(clients[0], SIGUSR2);


		while(1)
		{
				pause();
		}

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
