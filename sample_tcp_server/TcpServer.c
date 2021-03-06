/*
 * File Name : TcpServer.c
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
 * File Description : TCP Echo Server(Linux)
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

#include <fcntl.h>
#include <unistd.h>

#include <poll.h>
#include <pthread.h>

#include <semaphore.h>

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

typedef struct st_clientInfo_t {
	int cliFd;
	unsigned int cliIpAddress;
	unsigned int cliPort;
}__attribute__((packed))clientInfo_t;

#define SAMPLE_TCP_PORT1	5000
#define SAMPLE_TCP_PORT2	5002
#define CLIENT_MAX_NUM		30
#define BUF_MAX_SIZE		8192

#define FLAG_OFF		0
#define FLAG_ON			1

static clientInfo_t clientInfoPort1[CLIENT_MAX_NUM];
static clientInfo_t clientInfoPort2[CLIENT_MAX_NUM];

static sem_t semInfo[2];
// ---------------------------------------------------------------------------------------------------- //
// SENDER

int SendTcpDataFromServer(const unsigned int port, const int ipAddress, char* data, unsigned int dataSize)
{
	unsigned int loop = 0;
	int fd = 0;

	if( (data == NULL) || (dataSize <= 0) )
		return -1;

	switch( port )
	{
		case SAMPLE_TCP_PORT1 :
			sem_wait(&semInfo[0]);
			for( loop = 0 ; loop < CLIENT_MAX_NUM ; loop++ )
			{
				if( ipAddress == clientInfoPort1[loop].cliIpAddress )
				{
					fd = clientInfoPort1[loop].cliFd;
					if( send(fd, data, dataSize, MSG_DONTWAIT) < 0 )
						clientInfoPort1[loop].cliIpAddress = 0xFFFFFFFF;

					break;
				}
			}
			sem_post(&semInfo[0]);
			break;
		case SAMPLE_TCP_PORT2 :
			sem_wait(&semInfo[1]);
			for( loop = 0 ; loop < CLIENT_MAX_NUM ; loop++ )
			{
				if( ipAddress == clientInfoPort2[loop].cliIpAddress )
				{
					fd = clientInfoPort2[loop].cliFd;
					if( send(fd, data, dataSize, MSG_DONTWAIT) < 0 )
						clientInfoPort2[loop].cliIpAddress = 0xFFFFFFFF;

					break;
				}
			}
			sem_post(&semInfo[2]);
			break;
		default :	// ????????? ???????????? ?????? ??????, ?????? ???????????? CLIENT IP??? ?????? ??????.
			sem_wait(&semInfo[0]);
			for( loop = 0 ; loop < CLIENT_MAX_NUM ; loop++ )
			{
				if( ipAddress == clientInfoPort1[loop].cliIpAddress )
				{
					fd = clientInfoPort1[loop].cliFd;
					if( send(fd, data, dataSize, MSG_DONTWAIT) < 0 )
						clientInfoPort1[loop].cliIpAddress = 0xFFFFFFFF;

					break;
				}
			}
			sem_post(&semInfo[0]);

			sem_wait(&semInfo[1]);
			for( loop = 0 ; loop < CLIENT_MAX_NUM ; loop++ )
			{
				if( ipAddress == clientInfoPort2[loop].cliIpAddress )
				{
					fd = clientInfoPort2[loop].cliFd;
					if( send(fd, data, dataSize, MSG_DONTWAIT) < 0 )
						clientInfoPort2[loop].cliIpAddress = 0xFFFFFFFF;

					break;
				}
			}
			sem_post(&semInfo[2]);
			break;
	}

	return 1;
}


// ---------------------------------------------------------------------------------------------------- //
// RECEIVER

static int* ThreadTcpServer(int localPort)
{
	socklen_t saddrLen;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	clientInfo_t* clientInfo;
	int sockCnt = 0;
	int sockOpt = 0;
	int timeOut = 1000;
	struct pollfd pollFd[CLIENT_MAX_NUM];
	int loop, subLoop, sockFd, listenFd;
	int retPoll = 0, retRcv = 0;
	int* retThread;
	char temp[BUF_MAX_SIZE] = {0,};
	char receiveData[BUF_MAX_SIZE] = {0,};
	unsigned char duplicateFlag = FLAG_OFF;
	sem_t* semControl = NULL;

	listenFd = socket(PF_INET, SOCK_STREAM, 0);	// create socket
	if( listenFd == -1 )
	{
		printf("[TCP SERVER|%d] :: ",localPort);
		perror("socket()");
		exit(1);
	}

	/* SO_REUSEADDR
	* ?????? PORT??? ?????? Socket bind ?????? ?????? ??????(?????? ???????????? ??? ??????)
	* ?????? bind??? ????????? Socket ????????? ????????? ??? ??? ????????? ??????
	*/
	sockOpt = 1;
	if( setsockopt(listenFd, SOL_SOCKET, SO_REUSEADDR, &sockOpt, sizeof(sockOpt)) == -1 )
	{
		printf("[TCP SERVER|%d] :: ",localPort);
		perror("setsockopt()");
		exit(1);
	}

	memset( &serv_addr, 0x00, sizeof(struct sockaddr_in) );
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(localPort);

	if( bind(listenFd, (struct sockaddr*)&serv_addr,sizeof(struct sockaddr_in)) == -1 )
	{
		printf("[TCP SERVER|%d] :: ",localPort);
		perror("bind()");
		exit(1);
	}

	dlp("bind() :: port<%d> ok\n",localPort);

	if( listen(listenFd, CLIENT_MAX_NUM) == -1 )
	{
		printf("[TCP SERVER|%d] :: ",localPort);
		perror("listen()");
		exit(1);
	}

	// poll() :: listener socket fd ??????
	pollFd[sockCnt].fd = listenFd;
	pollFd[sockCnt++].events = POLLIN;	// poll event : receive(request connection)

	switch( localPort )
	{
		case 5000 :
			clientInfo = clientInfoPort1;
			sem_init(&semInfo[0],0,1);
			semControl = &semInfo[0];
			break;
		case 5002 :
			clientInfo = clientInfoPort2;
			sem_init(&semInfo[1],0,1);
			semControl = &semInfo[1];
			break;
		default :
			return retThread;
	}

	for( loop = 0 ; loop < CLIENT_MAX_NUM ; loop++ )
		memset( &clientInfo[loop], 0x00, sizeof(clientInfo_t) );


	dlp("tcp server :: port<%d> enable!\n",localPort);

	while(1)
	{
		retPoll = poll(pollFd, sockCnt, timeOut);	// fd ??????

		if( pollFd[0].revents & POLLIN )	// 1st pollfd :: listener(connection)
		{
			saddrLen = sizeof(struct sockaddr_in);
			if( (sockFd = accept(pollFd[0].fd, (struct sockaddr*)&cli_addr, &saddrLen)) == -1 )
			{
				printf("[TCP SERVER|%d] :: ",localPort);
				perror("accept()");
				continue;
			}

			// ?????? ??????
			{
				duplicateFlag = FLAG_OFF;
				for( loop = 1 ; loop < CLIENT_MAX_NUM ; loop++ )
				{
					// ???????????? ??? ????????? ???????????? ??????. ?????? ????????? ????????????, ??? ????????? ????????????.
					// ??? ??? ?????? ????????? ????????? ???????????? ?????????... ??? ?????? ???????????? ?????????????????? ??? ????????? ????????? ?????????.
					if( clientInfo[loop].cliIpAddress == inet_addr(inet_ntoa(cli_addr.sin_addr)) )
					{
						sem_wait(semControl);
						shutdown(clientInfo[loop].cliFd, SHUT_RDWR);
						memset( temp, 0x00, sizeof(temp) );
						sprintf( temp, "TCP SERVER<%d> Duplicate Client...[%s]",localPort,inet_ntoa(cli_addr.sin_addr) );
						printf("%s\n",temp);
						// shutdown -> close. "graceful exit"
						close(clientInfo[loop].cliFd);

						memset( &pollFd[loop], 0x00, sizeof(struct pollfd) );
						pollFd[loop].fd = sockFd;
						pollFd[loop].events = (POLLIN|POLLHUP|POLLERR);	// ?????? | ???????????? | ??? ??? ?????? ??????
						memset( &clientInfo[loop], 0x00, sizeof(clientInfo_t) );
						clientInfo[loop].cliFd = sockFd;
						clientInfo[loop].cliIpAddress = inet_addr(inet_ntoa(cli_addr.sin_addr));
						clientInfo[loop].cliPort = ntohs( ((struct sockaddr_in*)&cli_addr)->sin_port );
						memset( temp, 0x00, sizeof(temp) );
						sprintf( temp, "TCP SERVER<%d> Connection ok[%s][%d]",localPort,inet_ntoa(cli_addr.sin_addr),clientInfo[loop].cliPort );
						printf("%s\n",temp);
						duplicateFlag = FLAG_ON;
						sem_post(semControl);
						break;
					}
				}
			}

			// ??? ????????? ??????.
			{
				if( duplicateFlag == FLAG_OFF )
				{
					if( sockCnt < CLIENT_MAX_NUM )
					{
						pollFd[sockCnt].fd = sockFd;
						pollFd[sockCnt].events = (POLLIN | POLLHUP | POLLERR);
						memset( &clientInfo[sockCnt], 0x00, sizeof(clientInfo_t) );
						clientInfo[sockCnt].cliFd = sockFd;
						clientInfo[sockCnt].cliIpAddress = inet_addr(inet_ntoa(cli_addr.sin_addr));
						clientInfo[sockCnt].cliPort=  ntohs( ((struct sockaddr_in*)&cli_addr)->sin_port );
						memset( temp, 0x00, sizeof(temp) );
						sprintf( temp, "TCP SERVER<%d> Connection ok[%s][%d]",localPort,inet_ntoa(cli_addr.sin_addr),clientInfo[loop].cliPort );
						printf("%s\n",temp);
						sockCnt++;
					}
					else
					{
						printf("TCP SERVER<%d> Connection Full!!!\n",localPort);
						shutdown(sockFd, SHUT_RDWR);
						close(sockFd);
					}
				}
			}

			continue;	// ?????? ????????? ?????? ???, ????????? poll ????????? ??????...
		}

		// Sender ?????? ???????????? ????????? ?????? ?????? ??????. Receiver?????? ?????? ????????????.
		// client??? ?????? ?????? ??? ??? ????????? ??? ????????? ?????????. ???????????? ???????????????, sender?????? ?????? ??????????????? ?????? ???.
		for( loop = 1 ; loop < sockCnt ; loop++ )
		{
			if( clientInfo[loop].cliIpAddress == 0xFFFFFFFF )	// Clear...
			{
				sem_wait(semControl);
				shutdown(clientInfo[loop].cliFd, SHUT_RDWR);
				close(clientInfo[loop].cliFd);
				if( (sockCnt == 0) || (sockCnt == 1) )
				{
					// ????????? ??? ?????? ??????... ?????? ??? ?????? ??????..
				}
				else if( sockCnt == 2 )
				{
					pollFd[sockCnt].fd = -1;
					memset( &clientInfo[loop], 0x00, sizeof(clientInfo_t) );
					memset( &pollFd[loop], 0x00, sizeof(struct pollfd) );
					clientInfo[loop].cliFd = -1;
					--sockCnt;
					sem_post(semControl);
					break;
				}
				else
				{
					for( subLoop = loop ; subLoop < (sockCnt-1) ; subLoop++ )	// ?????? ??????????????? ?????????. ???????????? ?????? ??????.
					{
						memset( &pollFd[subLoop], 0x00, sizeof(struct pollfd) );
						memset( &clientInfo[subLoop], 0x00, sizeof(clientInfo_t) );
						memcpy( &pollFd[subLoop], &pollFd[subLoop+1], sizeof(struct pollfd) );
						memcpy( &clientInfo[subLoop], &clientInfo[subLoop+1], sizeof(clientInfo_t) );
					}

					pollFd[sockCnt].fd = -1;
					memset( &pollFd[sockCnt], 0x00, sizeof(struct pollfd) );
					memset( &clientInfo[sockCnt], 0x00, sizeof(clientInfo_t) );
					clientInfo[sockCnt].cliFd = -1;
					--sockCnt;
				}

				loop--;
				retPoll--;
				sem_wait(semControl);
				continue;
			}

			if( pollFd[loop].fd < 0 )
				continue;

			if( pollFd[loop].revents & POLLIN )
			{
				retRcv = recv(pollFd[loop].fd, receiveData, BUF_MAX_SIZE, 0);

				if( retRcv == -1 )	// ????????? ??????
				{
					sem_wait(semControl);
					shutdown(clientInfo[loop].cliFd, SHUT_RDWR);
					close(clientInfo[loop].cliFd);
					if( (sockCnt == 0) || (sockCnt == 1) )
					{
						// ????????? ??? ?????? ??????... ?????? ??? ?????? ??????..
					}
					else if( sockCnt == 2 )
					{
						pollFd[sockCnt].fd = -1;
						memset( &clientInfo[loop], 0x00, sizeof(clientInfo_t) );
						memset( &pollFd[loop], 0x00, sizeof(struct pollfd) );
						clientInfo[loop].cliFd = -1;
						--sockCnt;
						sem_post(semControl);
						break;
					}
					else
					{
						for( subLoop = loop ; subLoop < (sockCnt-1) ; subLoop++ )	// ?????? ??????????????? ?????????. ???????????? ?????? ??????.
						{
							memset( &pollFd[subLoop], 0x00, sizeof(struct pollfd) );
							memset( &clientInfo[subLoop], 0x00, sizeof(clientInfo_t) );
							memcpy( &pollFd[subLoop], &pollFd[subLoop+1], sizeof(struct pollfd) );
							memcpy( &clientInfo[subLoop], &clientInfo[subLoop+1], sizeof(clientInfo_t) );
						}

						pollFd[sockCnt].fd = -1;
						memset( &pollFd[sockCnt], 0x00, sizeof(struct pollfd) );
						memset( &clientInfo[sockCnt], 0x00, sizeof(clientInfo_t) );
						clientInfo[sockCnt].cliFd = -1;
						--sockCnt;
					}

					loop--;
					sem_post(semControl);
				}
				else if( retRcv == 0 )	// ?????? ??????
				{
					sem_wait(semControl);
					close(clientInfo[loop].cliFd);
					if( (sockCnt == 0) || (sockCnt == 1) )
					{
						// ????????? ??? ?????? ??????... ?????? ??? ?????? ??????..
					}
					else if( sockCnt == 2 )
					{
						pollFd[sockCnt].fd = -1;
						memset( &clientInfo[loop], 0x00, sizeof(clientInfo_t) );
						memset( &pollFd[loop], 0x00, sizeof(struct pollfd) );
						clientInfo[loop].cliFd = -1;
						--sockCnt;
						break;
					}
					else
					{
						for( subLoop = loop ; subLoop < (sockCnt-1) ; subLoop++ )	// ?????? ??????????????? ?????????. ???????????? ?????? ??????.
						{
							memset( &pollFd[subLoop], 0x00, sizeof(struct pollfd) );
							memset( &clientInfo[subLoop], 0x00, sizeof(clientInfo_t) );
							memcpy( &pollFd[subLoop], &pollFd[subLoop+1], sizeof(struct pollfd) );
							memcpy( &clientInfo[subLoop], &clientInfo[subLoop+1], sizeof(clientInfo_t) );
						}

						pollFd[sockCnt].fd = -1;
						memset( &pollFd[sockCnt], 0x00, sizeof(struct pollfd) );
						memset( &clientInfo[sockCnt], 0x00, sizeof(clientInfo_t) );
						clientInfo[sockCnt].cliFd = -1;
						--sockCnt;
					}

					loop--;
					sem_post(semControl);
				}
				else if( retRcv > 0 )
				{
					// ????????? ?????? ??????. echo
					SendTcpDataFromServer(localPort,clientInfo[loop].cliIpAddress,receiveData, retRcv);
				}
				else
				{
				}
				retPoll--;
			}
			else if( pollFd[loop].revents & POLLERR )	// UNKNOWN ERROR
			{
	// close
				retPoll--;
			}
			else if( pollFd[loop].revents & POLLHUP )	// Stream Socket perr closed connection, or shutdown writing half of connection
			{
	// close
				retPoll--;
			}
		}


		usleep( 100*1000 );
	}

	switch( localPort )
	{
		case 5000 :
			sem_destroy(&semInfo[0]);
			break;
		case 5002 :
			sem_destroy(&semInfo[1]);
			break;
		default :
			return retThread;
	}

	pthread_exit(0);

	return retThread;
}

// ---------------------------------------------------------------------------------------------------- //
// INIT

int StartTcpServer(void)
{
	pthread_t tcpServer1;
	pthread_t tcpServer2;

	if( pthread_create(&tcpServer1, NULL, (void*)ThreadTcpServer, (void*)SAMPLE_TCP_PORT1) < 0 )
	{
		printf("#### thread create error : TCP SERVER1");
		exit(1);
	}
	else
		pthread_detach(tcpServer1);

	if( pthread_create(&tcpServer2, NULL, (void*)ThreadTcpServer, (void*)SAMPLE_TCP_PORT2) < 0 )
	{
		printf("#### thread create error : TCP SERVER2");
		exit(1);
	}
	else
		pthread_detach(tcpServer2);


	return 1;
}


