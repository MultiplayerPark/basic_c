#include <stdio.h>
#include <unistd.h>

#include "WavPlayer.h"

int main(void)
{
	StartWavePlayer();
	while(1)
	{
		usleep(100*1000);
	}

	return 1;
}
