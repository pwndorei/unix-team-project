#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include "project.h"

const char *dat[4] = {"p1.dat", 
					  "p2.dat", 
					  "p3.dat", 
					  "p4.dat"};

int
create_source_data()
{
		int fd[4] = {0, };
		
		int i = 0;

		int data = 0;

		for(i = 0; i < 4; i++)
		{
				fd[i] = open(dat[i], O_CREAT | O_WRONLY, 0666);

				if(fd[i] == -1)
				{
						perror("open error");
				}
		}

		for(i = 0; i < 0x100000; i++, data++)
		{
				write(fd[i%4], &data, 4);
		}


		for(i = 0; i < 4; i++)
		{
				close(fd[i]);
		}

		return 0;
}
