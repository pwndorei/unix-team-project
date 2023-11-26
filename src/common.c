#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <errno.h>
#include "project.h"

key_t key = 0; // key, used when creating/getting shm, msg queue
int shmid = 0; //shared memory id
int msgid[NODENUM] = {0, };
const int proj_id = 0x41;//temp id
int id;//id for node


void
gen_node(pid_t pids[], int n, void(*task)(int), int mode)
{
		//create n children, return value for their ID
		int i = 0;
		pid_t pid = 0;
		for(;i<n;i++)
		{
				pid = fork();
				if(!pid)
				{
						id = i;//extern int id at server.c & client.c
						task(mode);//do_client_task or do_server_task, no return(just exit)
						exit(-1);//Hoxy Molla
				}
				pids[i] = pid;
				setpgid(pid, pids[0]);//set group id -> first client/server
		}
}

void
gen_key()
{
		if(key) return;

		key = ftok("./", proj_id);
		//if absolute path == relative path -> same key
}

void
create_shm()
{
		shmid = shmget(key, CHKSIZE, IPC_CREAT | 0600);

		if(shmid == -1 && errno != EEXIST)
		{
				perror("shmget failed");
				exit(-1);
		}
}
