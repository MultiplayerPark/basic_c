/*
 * File Name : TcpClient.c
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
 * File Description : TCP Echo Client(Linux)
 *
 * Release List
 * 2021-12-24 : 1st Release
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>

#include <fcntl.h>
#include <unistd.h>

#include <poll.h>

#include <pthread.h>
#include <assert.h>

#include "TcpClient.h"

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

enum {
	TCP_CLIENT_NO1=0,
	TCP_CLIENT_NO2,
	TCP_CLIENT_NO3
};

typedef struct st_tcpFlagInform {
	int connectInform;
	int waitTimeCnt;
}__attribute__((packed))tcpFlagInform_t;

#define TCP_CLIENT_NO	TCP_CLIENT_NO3+1
#define BUF_MAX_SIZE	8192
#define TIMEOUT_RECONN	100

#define KEY_INPUT_IP	0
#define KEY_INPUT_PORT	1

#define FLAG_OFF	0
#define FLAG_ON		1

static struct pollfd pollTcpClient[TCP_CLIENT_NO];

static int ScanKeyboardData(const int type)
{
	int keyData = 0;
	char keyboard[32] = {0,};
	unsigned int k = 0;
	char ch = 0;
	struct sockaddr_in addr_inet;

	memset( &addr_inet, 0x00, sizeof(struct sockaddr_in) );

	while(1)
	{
		printf("+-------------------------------------------------+\r\n");

		switch( type )
		{
			case KEY_INPUT_IP :
				printf(" input data(ex>158.181.17.205) : ");
				break;
			case KEY_INPUT_PORT :
				printf(" input data(ex> 5000) : ");
				break;
		}

		memset( keyboard, 0x00, sizeof(keyboard) );
		ch = 0;
		k = 0;

		do {
			scanf("%c",&ch);
			keyboard[k++] = ch;
			printf("%c",ch);
		}while(ch != 0x0A );

		if( ch == 0x1B )
			break;
		else if( ch == 0x0A )
		{
			if( type == KEY_INPUT_IP )
			{
				if( !inet_aton(keyboard,&addr_inet.sin_addr) )
					printf("Invalid IP Address\n");
				else
				{
					keyData = addr_inet.sin_addr.s_addr;
					break;
				}
			}
			else if( type == KEY_INPUT_PORT )
			{
				keyData = atoi(keyboard);
				break;
			}
		}

		system("clear");
	}

	return keyData;
}

int SendTcpDataFromClient(int sockNo, char* data, unsigned int dataSize)
{
	int fd = 0;

	if( (data == NULL) || (dataSize == 0) )
		return -1;

	fd = pollTcpClient[sockNo].fd;

	if( fd > 0 )
		return write(fd,data,dataSize);
	else
		return -1;
}

static int ConnectionTcp(int* sockFd, struct sockaddr_in* targetInform, struct pollfd* pollInform, int servIP, int servPORT)
{
	struct linger solinger;

	if( (*sockFd = socket(PF_INET,SOCK_STREAM,0)) < 0 )
	{
		printf("[TCP CLIENT] :: ");
		perror("socket()");
		return -1;
	}

	memset( targetInform, 0x00, sizeof(struct sockaddr_in) );
	targetInform->sin_family = AF_INET;
	targetInform->sin_addr.s_addr = htonl(servIP);
	targetInform->sin_port = htons(servPORT);

	solinger.l_onoff = 1;
	solinger.l_linger = 0;
	if( setsockopt(*sockFd,SOL_SOCKET,SO_LINGER,&solinger,sizeof(struct linger)) == -1 )
	{
		printf("[TCP CLIENT] :: ");
		perror("setsockopt() ");
		return -1;
	}

	if( connect(*sockFd, (struct sockaddr*)targetInform, sizeof(struct sockaddr_in)) < 0 )
	{
		printf("[TCP CLIENT] :: ");
		perror("connect() ");
		close(*sockFd);
		return -1;
	}

	memset( pollInform, 0x00, sizeof(struct pollfd) );
	pollInform->fd = *sockFd;
	pollInform->events = (POLLIN | POLLERR | POLLHUP);

	return 1;
}

static int* ThreadTcpClient(int sockNo)
{
	struct sockaddr_in targetAddr;
	int sockFd = 0;
	int* retThread;
	int timeOut = 0;
	int retPoll = 0;
	int retRcv = 0;
	char receiveData[BUF_MAX_SIZE] = {0,};
	int servIpAddress = 0;
	int servPort = 0;
	tcpFlagInform_t tcpFlagInform;

	memset( &tcpFlagInform, 0x00, sizeof(tcpFlagInform_t) );

	servIpAddress = ScanKeyboardData(KEY_INPUT_IP);
	servPort = ScanKeyboardData(KEY_INPUT_PORT);

	while(1)
	{
		if( tcpFlagInform.connectInform == FLAG_OFF )
		{
			if( tcpFlagInform.waitTimeCnt == 0 )
			{
				if( ConnectionTcp(&sockFd, &targetAddr, &pollTcpClient[sockNo],servIpAddress,servPort) < 0 )
				{
					tcpFlagInform.connectInform = FLAG_OFF;
					sockFd = -1;
				}
				else
				{
					tcpFlagInform.connectInform = FLAG_ON;
				}

				tcpFlagInform.waitTimeCnt = TIMEOUT_RECONN;

			}
		}
		else
		{
			retPoll = poll(&pollTcpClient[sockNo], 1, timeOut);

			if( retPoll == -1 )
				continue;
			else if( retPoll == 0 )	// timeout...
			{
			}

			if( pollTcpClient[sockNo].revents & POLLIN )
			{
				retRcv = recv(pollTcpClient[sockNo].fd,receiveData,BUF_MAX_SIZE,0);

				if( retRcv < 0 )	// Incorrecting shutdown
				{
					shutdown(pollTcpClient[sockNo].fd,SHUT_RDWR);
					close(pollTcpClient[sockNo].fd);
					tcpFlagInform.connectInform = FLAG_OFF;
					tcpFlagInform.waitTimeCnt = TIMEOUT_RECONN;
					pollTcpClient[sockNo].fd = -1;
				}
				else if( retRcv == 0 )	// Graceful shutdown
				{
					close(pollTcpClient[sockNo].fd);
					tcpFlagInform.connectInform = FLAG_OFF;
					tcpFlagInform.waitTimeCnt = TIMEOUT_RECONN;
					pollTcpClient[sockNo].fd = -1;
				}
				else	// receive data
				{
				}
			}
			else if( pollTcpClient[sockNo].revents & POLLERR )	// unknown error
			{
			}
			else if( pollTcpClient[sockNo].revents & POLLHUP )	// Stream socket peer closed connection, or shutdown writing half of connection
			{
			}
		}

		tcpFlagInform.waitTimeCnt--;

		usleep( 100*1000 );
	}

	pthread_exit(0);

	return retThread;
}

int StartTcpClient(void)
{
	pthread_t tcpClient1;
	pthread_t tcpClient2;
	pthread_t tcpClient3;

	if( pthread_create(&tcpClient1,NULL,(void*)ThreadTcpClient,(void*)TCP_CLIENT_NO1) < 0 )
	{
		printf("#### thread create error : ThreadTcpClient");
		exit(1);
	}
	else
		pthread_detach(tcpClient1);

	if( pthread_create(&tcpClient2,NULL,(void*)ThreadTcpClient,(void*)TCP_CLIENT_NO2) < 0 )
	{
		printf("#### thread create error : ThreadTcpClient");
		exit(1);
	}
	else
		pthread_detach(tcpClient2);

	if( pthread_create(&tcpClient3,NULL,(void*)ThreadTcpClient,(void*)TCP_CLIENT_NO3) < 0 )
	{
		printf("#### thread create error : ThreadTcpClient");
		exit(1);
	}
	else
		pthread_detach(tcpClient3);

	return 1;
}
