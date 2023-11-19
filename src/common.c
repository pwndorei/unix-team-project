#include <unistd.h>
#include "project.h"

int
gen_node(pid_t pids[], int n)
{
		//create n children, return value for their ID
		int i = 0;
		pid_t pid = 0;
		for(;i<n;i++)
		{
				pid = fork();
				if(!pid) return i; //if child process -> return i
				pids[i] = pid;
		}

		return -1; // parent return -1
}
