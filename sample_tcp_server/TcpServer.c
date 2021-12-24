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

	listenFd = socket(PF_INET, SOCK_STREAM, 0);	// create socket
	if( listenFd == -1 )
	{
		printf("[TCP SERVER|%d] :: ",localPort);
		perror("socket()");
		exit(1);
	}

	/* SO_REUSEADDR
	* 같은 PORT에 다른 Socket bind 되는 것을 허용(강제 종료하고 재 실행)
	* 기존 bind에 할당된 Socket 자원을 재사용 할 수 있도록 허가
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

	// poll() :: listener socket fd 추가
	pollFd[sockCnt].fd = listenFd;
	pollFd[sockCnt++].events = POLLIN;	// poll event : receive(request connection)

	switch( localPort )
	{
		case 5000 :
			clientInfo = clientInfoPort1;
			break;
		case 5002 :
			clientInfo = clientInfoPort2;
			break;
		default :
			return retThread;
	}

	for( loop = 0 ; loop < CLIENT_MAX_NUM ; loop++ )
		memset( &clientInfo[loop], 0x00, sizeof(clientInfo_t) );


	dlp("tcp server :: port<%d> enable!\n",localPort);

	while(1)
	{
		retPoll = poll(pollFd, sockCnt, timeOut);	// fd 감시

		if( pollFd[0].revents & POLLIN )	// 1st pollfd :: listener(connection)
		{
			saddrLen = sizeof(struct sockaddr_in);
			if( (sockFd = accept(pollFd[0].fd, (struct sockaddr*)&cli_addr, &saddrLen)) == -1 )
			{
				printf("[TCP SERVER|%d] :: ",localPort);
				perror("accept()");
				continue;
			}

			// 중복 관리
			{
				duplicateFlag = FLAG_OFF;
				for( loop = 1 ; loop < CLIENT_MAX_NUM ; loop++ )
				{
					// 중복되는 새 접속이 들어오는 경우. 기존 접속을 종료하고, 새 접속을 수행한다.
					// 알 수 없는 이유로 연결이 끊어지는 경우나... 뭐 그런 경우에는 클라이언트가 재 접속을 요구할 것이다.
					if( clientInfo[loop].cliIpAddress == inet_addr(inet_ntoa(cli_addr.sin_addr)) )
					{
						shutdown(clientInfo[loop].cliFd, SHUT_RDWR);
						memset( temp, 0x00, sizeof(temp) );
						sprintf( temp, "TCP SERVER<%d> Duplicate Client...[%s]",localPort,inet_ntoa(cli_addr.sin_addr) );
						printf("%s\n",temp);
						// shutdown -> close. "graceful exit"
						close(clientInfo[loop].cliFd);

						memset( &pollFd[loop], 0x00, sizeof(struct pollfd) );
						pollFd[loop].fd = sockFd;
						pollFd[loop].events = (POLLIN|POLLHUP|POLLERR);	// 수신 | 장애발생 | 알 수 없는 에러
						memset( &clientInfo[loop], 0x00, sizeof(clientInfo_t) );
						clientInfo[loop].cliFd = sockFd;
						clientInfo[loop].cliIpAddress = inet_addr(inet_ntoa(cli_addr.sin_addr));
						clientInfo[loop].cliPort = ntohs( ((struct sockaddr_in*)&cli_addr)->sin_port );
						memset( temp, 0x00, sizeof(temp) );
						sprintf( temp, "TCP SERVER<%d> Connection ok[%s][%d]",localPort,inet_ntoa(cli_addr.sin_addr),clientInfo[loop].cliPort );
						printf("%s\n",temp);
						duplicateFlag = FLAG_ON;
						break;
					}
				}
			}

			// 새 접속인 경우.
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

			continue;	// 접속 이벤트 발생 시, 빠르게 poll 처리를 위해...
		}

		// Sender 에서 상대방의 종료를 알게 되는 경우. Receiver에서 종료 처리한다.
		// client가 얼마 없을 땐 이 로직이 큰 영향이 없으나. 대량으로 있는경우엔, sender에서 직접 처리하는게 좋을 듯.
		for( loop = 1 ; loop < sockCnt ; loop++ )
		{
			if( clientInfo[loop].cliIpAddress == 0xFFFFFFFF )	// Clear...
			{
				shutdown(clientInfo[loop].cliFd, SHUT_RDWR);
				close(clientInfo[loop].cliFd);
				if( (sockCnt == 0) || (sockCnt == 1) )
				{
					// 접속자 수 관리 실패... 나올 수 없는 경우..
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
					for( subLoop = loop ; subLoop < (sockCnt-1) ; subLoop++ )	// 다른 알고리즘이 있으나. 무식하게 일단 적용.
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
				continue;
			}

			if( pollFd[loop].fd < 0 )
				continue;

			if( pollFd[loop].revents & POLLIN )
			{
				retRcv = recv(pollFd[loop].fd, receiveData, BUF_MAX_SIZE, 0);

				if( retRcv == -1 )	// 비정상 종료
				{
					shutdown(clientInfo[loop].cliFd, SHUT_RDWR);
					close(clientInfo[loop].cliFd);
					if( (sockCnt == 0) || (sockCnt == 1) )
					{
						// 접속자 수 관리 실패... 나올 수 없는 경우..
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
						for( subLoop = loop ; subLoop < (sockCnt-1) ; subLoop++ )	// 다른 알고리즘이 있으나. 무식하게 일단 적용.
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
				}
				else if( retRcv == 0 )	// 정상 종료
				{
					close(clientInfo[loop].cliFd);
					if( (sockCnt == 0) || (sockCnt == 1) )
					{
						// 접속자 수 관리 실패... 나올 수 없는 경우..
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
						for( subLoop = loop ; subLoop < (sockCnt-1) ; subLoop++ )	// 다른 알고리즘이 있으나. 무식하게 일단 적용.
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
				}
				else if( retRcv > 0 )
				{
					// 데이터 정상 수신.
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

	pthread_exit(0);

	return retThread;
}

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


