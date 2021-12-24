/*
 * File Name : control_iic.h
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
 * File Description : iic controller(Linux)
 *
 * Release List
 * 2021-12-24 : 1st Release
 */

//#pragma once

#ifndef CONTROL_IIC_H
#define CONTROL_IIC_H

typedef struct st_iicData_t {
	unsigned char iicChannel;
	unsigned char slaveAddress;
	unsigned short regAddr;
	unsigned char regAddrSize;
	unsigned char data[256];
	unsigned int dataLen;
}__attribute__((packed))iicData_t;

int OpenIicDriver(unsigned char channel);
int CloseIicDriver(unsigned char channel);

int WriteIic(iicData_t* writeData);
char ReadIic(iicData_t* readData);

#endif
