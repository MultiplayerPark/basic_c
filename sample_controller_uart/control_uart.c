/*
 * File Name : control_uart.c
 *
 * Copyright 2021 by pwk
 *
 * Developer : PWK (pwk10000@naver.com)
 *
 * Classify : Application
 *
 * Version : 1.00
 *
 * Created Date : 2021-12-24
 *
 * File Description : uart controller(Linux-3.0/i.MX6 Solo)
 *
 * Release List
 * 2021-12-24 : 1st Release
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <semaphore.h>
#include <pthread.h>
#include <poll.h>
#include <sys/ioctl.h>

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include "control_uart.h"

// ---------------------------------------------------------------------------------------------------- //
// Debugging Option...											//
// ---------------------------------------------------------------------------------------------------- //
#define xxx_DEBUG

#ifdef xxx_DEBUG
#define dlp(fmt,args...) printf("[%s,%s(),%d]"fmt,__FILE__,__FUNCTION__,__LINE__,## args)
#else
#define dlp(fmt,args...)
#endif
// ---------------------------------------------------------------------------------------------------- //

// UART INFORMATIONS ... ------------------------------------------------------------------------------ //
// UART1(COM1) :: TERMINAL PORT
// UART2(COM2) :: 115200 bps
// UART3(COM3) :: 9600 bps
// ---------------------------------------------------------------------------------------------------- //

#define INT_UART_NUM	2
#define DRV_UART_BASENAME	"/dev/ttymxc"

static struct pollfd pollFd[INT_UART_NUM];
static sem_t semIntUart[INT_UART_NUM];

static int SendUartData(int fd, char* data, unsigned int length)
{
	int ret = 0;

	if( fd <= 0 )
		return -1;

	if( (data == NULL) || (length == 0) )
		return -1;

	if( (ret = write(fd,data,length)) <= 0 )
		return -1;
	else
	{
		if( length == (unsigned int)ret )
			return 1;
		else
		{
			printf("[UART] failure to complete for write\n");
			return 0;
		}
	}
}

int SendUart1Data(char* data, unsigned int length)
{
	return SendUartData(pollFd[0], data, length);
}

int SendUart2Data(char* data, unsigned int length)
{
	return SendUartData(pollFd[1], data, length);
}

static int* ThreadUartReceiver(void)
{
	struct termios newtio[INT_UART_NUM];
	struct termios oldtio[INT_UART_NUM];
	int timeOut = 1*1000;
	unsigned int readLength = 0;
	char receiveData[8192] = {0,};
	unsigned int cnt = 0;
	int retPoll = 0;
	int baudrate = 0;
	int uartFd = 0;
	char drvName[56] = {0,};
	int* ret;

	memset( &newtio, 0x00, sizeof(struct termios) );
	memset( &oldtio, 0x00, sizeof(struct termios) );

	for( cnt = 0 ; cnt < INT_UART_NUM ; cnt++ )
	{
		memset( drvName, 0x00, sizeof(drvName) );
		sprintf( drvName, "%s%d",DRV_UART_BASENAME, cnt+1 );
		uartFd = 0;
		uartFd = open(drvName,O_RDWR|O_NOCTTY);

		if( uartFd < 0 )
		{
			perror("open() ");
			exit(1);
		}
		else
		{
			printf("[UART] %s driver open success\n",drvName);
			pollFd[cnt].fd = uartFd;
			pollFd[cnt].events = POLLIN;
		}

		sem_init(&semIntUart[cnt],0,1);
		baudrate = 0;

		if( cnt == 0 )
			baudrate = B115200;
		else if( cnt == 1 )
			baudrate = B9600;

		memset( &newtio, 0x00, sizeof(struct termios) );
		memset( &oldtio, 0x00, sizeof(struct termios) );

		tcgetattr(pollFd[cnt].fd,&oldtio[cnt]);

		memcpy( &newtio[cnt], &oldtio[cnt], sizeof(struct termios) );

		newtio[cnt].c_cflag = baudrate | CS8 | CLOCAL | CREAD;
		newtio[cnt].c_iflag = IGNPAR;
		newtio[cnt].c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
		newtio[cnt].c_oflag = 0;
		tcflush(pollFd[cnt].fd,TCIFLUSH);
		tcsetattr(pollFd[cnt].fd,TCSANOW,&newtio[cnt]);
	}

	printf("[UART] Initialize ok\n");

	while(1)
	{
		retPoll = poll(pollFd, INT_UART_NUM, timeOut);

		if( retPoll == -1 )
		{
		}

		for( cnt = 0 ; cnt < INT_UART_NUM ; cnt++ )
		{
			if( pollFd[cnt].revents & POLLIN )
			{
				memset( receiveData, 0x00, sizeof(receiveData) );
				readLength = read(pollFd[cnt].fd,receiveData,8192);

				if( readLength > 0 )
				{
					// receive ok
					retPoll--;
				}
			}
		}

		usleep( 100*1000 );
	}

	pthread_exit(0);

	return ret;
}

int StartUartProcess(void)
{
	pthread_t uartProcess;

	if( pthread_create(&uartProcess,NULL,(void*)ThreadUartReceiver,NULL) < 0 )
	{
		printf("#### thread create error : ThreadUartReceiver");
		exit(1);
	}
	else
		pthread_detach(uartProcess);

	return 1;
}
