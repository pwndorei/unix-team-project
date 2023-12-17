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
int order = 0;//id of server to resume execution(read chunk)
int mode = -1;//Client-Oriented(MODE_CLOR) or Server-Oriented(MODE_SVOR)
pid_t servers[NODENUM] = {0,};
pid_t clients[NODENUM] = {0,};
extern int client_pipe[2];
extern int msgid[NODENUM];

struct timeval io_start;

int
main()
{
#ifdef DEBUG
		printf("parent pid: %d\n",getpid());
#endif

		create_source_data();

#ifdef TIMES
	start_timer(&io_start);
#endif
		client_oriented_io();
		
#ifdef TIMES
	start_timer(&io_start);
#endif
		server_oriented_io();
		
}


//signal handlers

void
do_nothing(int sig)
{
	// do nothing
}

void
client_write_complete(int sig)
{
		//SIGUSR1 Handler
#ifdef DEBUG
		puts("parent: SIGUSR1 caught");
#endif

		kill(servers[order], SIGUSR1);//resume server process
		// printf("killed SIGUSR to server #%d\n", order);
		order = (order + 1) % NODENUM;
		
}

void
server_read_complete(int sig)
{
		//SIGUSR2 Handler
#ifdef DEBUG
		puts("parent: server complete reading, wake all clients");
#endif
		if (mode == MODE_CLOR)
				kill(clients[0], SIGUSR2);
		else if (mode == MODE_SVOR)
		{	
				for (int i = 0; i < NODENUM; i++)
						kill(clients[i], SIGUSR1);
		}

}

static void
shutdown(int sig)
{
		int i = 0;
		puts("shutdown");

		signal(SIGINT, do_nothing);

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
				if(clients[i] && mode == MODE_SVOR)
				{
						kill(clients[i], SIGINT);
						waitpid(clients[i], NULL, 0);
						clients[i] = 0;
				}
		}
		puts("bye");
		exit(0);
}


int
client_oriented_io()
{

		mode = MODE_CLOR;

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

		//sleep(1);
		//kill(clients[0], SIGUSR2);


		while(1)
		{
				pause();
		}

		return 0;

}

int
server_oriented_io()
{

		mode = MODE_SVOR;

		int i;
		struct sigaction act = {0,};

		act.sa_handler = shutdown;
		sigaction(SIGINT, &act, NULL);

		act.sa_handler = client_write_complete;
		sigaction(SIGUSR1, &act, NULL);

		act.sa_handler = server_read_complete;
		sigaction(SIGUSR2, &act, NULL);

		create_msg_queue();
		gen_node(clients, NODENUM, do_client_task, MODE_SVOR);
		gen_node(servers, NODENUM, do_server_task, MODE_SVOR);

		while(1)
		{
				pause();
		}
  
		return 0;
}
