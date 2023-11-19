#include <fcntl.h>
#include <unistd.h>

#define NODENUM  4

int create_source_data();
int client_oriented_io();
int server_oriented_io();
int gen_node(pid_t*, int);
