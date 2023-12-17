#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/time.h>

#define NODENUM  4
#define CHKSIZE 0x20// 32bytes(8 int)

#define MODE_CLOR 0
#define MODE_SVOR 1

#ifdef TIMES
#define TIMER_START(start)  \
		gettimeofday(&start,NULL);
#define TIMER_END(start, end, total) \
		gettimeofday(&end, NULL);\
		total += end.tv_sec - start.tv_sec;
		
#else
#define TIME_START(timer) /*do nothing*/
#define TIME_END(start, end, total) /*do nothing*/
#endif


typedef struct _msg
{
		long mtype;
		int mtext[1];
}msgbuf;




int create_source_data();
int client_oriented_io();
int server_oriented_io();

//for initialize Nodes
void gen_node(pid_t[], int, void(*)(int), int);

void gen_key();

//for IPC
//initialize(called before generating nodes)
void create_shm();
void create_msg_queue();

//for client
void do_client_task(int);


//for server
void do_server_task(int);


//sighandler for parent
void client_write_complete(int);//SIGUSR1 handler
void server_read_complete(int);//SIGUSR2 handler


//time measurement function
void start_timer(struct timeval *timer);
void stop_timer(struct timeval *start_time, const char *label);


extern struct timeval io_start;
extern long rwtime = 0;
