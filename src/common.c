#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <errno.h>
#include "project.h"
#include <sys/time.h>
#include <sys/msg.h>

key_t key = 0; // key, used when creating/getting shm, msg queue
int shmid = 0; //shared memory id
int msgid[NODENUM] = {0, };
const int proj_id = 0x41;//temp id
int id;//id for node
int client_pipe[2];

void
gen_node(pid_t pids[], int n, void(*task)(int), int mode)
{
		//create n children
		int i = 0;
		pid_t pid = 0;
		for(;i<n;i++)
		{
				pid = fork();
				if(!pid)
				{
						id = i;//extern int id at server.c & client.c
						task(mode);//do_client_task or do_server_task, no return(just exit)
						exit(-1);
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
		gen_key();
		shmid = shmget(key, CHKSIZE, IPC_CREAT | 0600);

		if(shmid == -1 && errno != EEXIST)
		{
				perror("shmget failed");
				exit(-1);
		}
}

void
create_msg_queue()
{		
		int i;
		gen_key();
		for (i = 0; i < NODENUM; i++)
		{
			msgid[i] = msgget(key + i, IPC_CREAT | 0600);
			if (msgid[i] == -1)
			{
				perror("msgget");
				exit(-1);
			}
		}
}

//time measurement function
// 시간 측정을 시작하는 함수
void start_timer(struct timeval *timer) {
    gettimeofday(timer, NULL);
}

// 시간 측정을 종료하고 결과를 출력하는 함수
void stop_timer(struct timeval *start_time, const char *label) {
    struct timeval end_time;
    gettimeofday(&end_time, NULL);

    long time_result = end_time.tv_sec - start_time->tv_sec;
    printf("%s TIMES = %ld - %ld = %ld\n", label, end_time.tv_sec, start_time->tv_sec, time_result);
}
