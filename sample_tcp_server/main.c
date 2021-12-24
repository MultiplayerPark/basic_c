#include <stdio.h>
#include <unistd.h>

#include "TcpServer.h"

int main(void)
{
	StartTcpServer();
	while(1)
	{
		usleep(100*1000);
	}

	return 1;
}
