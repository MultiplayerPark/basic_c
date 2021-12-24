/*
 * File Name : TcpClient.h
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

//#pragma once

#ifndef TCPSERVER_H
#define TCPSERVER_H

int StartTcpClient(void);

int SendTcpDataFromClient(int sockNo, char* data, unsigned int dataSize);

#endif
