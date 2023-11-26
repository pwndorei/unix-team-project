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

#define NODENUM  4
#define CHKSIZE 0x20// 32bytes(8 int)

#define MODE_CLOR 0
#define MODE_SVOR 1

//exit status, LSB 0 -> client, 1 -> server
#define SUCCESS 0
#define E_OPEN_FILE 2
#define E_READ_FILE 4
#define E_WRITE_FILE 8
//error(shared memory) 0x2?
#define E_INVALID_SHMID 0x20
#define E_SHMAT_FAILED 0x22
//error(message queue) 0x4?



int create_source_data();
int client_oriented_io();
int server_oriented_io();

//for initialize Nodes
int gen_node(pid_t[], int, int(*)());

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
void shutdown(int);//SIGINT handler

//sighandler for client

