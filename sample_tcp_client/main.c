#include <stdio.h>
#include <unistd.h>

#include "TcpClient.h"

int main(void)
{
	StartTcpClient();
	while(1)
	{
		usleep(100*1000);
	}

	return 1;
}
