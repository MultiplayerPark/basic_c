/*
 * File Name : control_iic.c
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <semaphore.h>

#include "control_iic.h"

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

// 채널 갯수만큼 생성해야한다. sample이라 하나만 Open한다 가정하고 작성.
static int fdIic = 0;
static sem_t semIic;

#define DRV_IIC_NAME	"/dev/i2c-"

#define MASK_8BIT	0xFF
#define SHIFT_BIT8	8

int OpenIicDriver(unsigned char channel)
{
	char drvName[56] = {0,};

	memset( drvName, 0x00, sizeof(drvName) );

	sprintf( drvName, "%s%d",DRV_IIC_NAME,channel+1 );

	sem_init(semIic,0,1);

	if( (fdIic = open(drvName,O_RDWR)) < 0 )
	{
		printf("[IIC|%d] open\n",channel);
		perror("open() ");
		return -1;
	}

	return 1;
}

int CloseIicDriver(unsigned char channel)
{
	if( fdIic > 0 )
	{
		close(fdIic);
		fdIic = -1;
		sem_destroy(&semIic);
	}
}

int WriteIic(iicData_t* writeData)
{
	struct i2c_rdwr_ioctl_data controlIic;
	struct i2c_msg iicInform[2];
	unsigned int cnt = 0;
	unsigned char buf[256] = {0,};
	unsigned int len = 0;
	int ret = 0;

	if( writeData->dataLen > 253 )
	{
		printf("[IIC|%d] write : data size over[%d]\n",writeData->dataLen);
		return -1;
	}

	sem_wait(&semIic);

	memset( &controlIic, 0x00, sizeof(controlIic) );
	memset( &iicInform[0], 0x00, sizeof(struct i2c_msg) );
	memset( &iicInform[1], 0x00, sizeof(struct i2c_msg) );
	memset( buf, 0x00, sizeof(buf) );

	switch( writeData->regAddrSize )
	{
		case 1 :
			buf[0] = (writeData->regAddr & MASK_8BIT);
			memcpy( &buf[1], writeData->data, writeData->dataLen );
			len = writeData->dataLen + 1;
			break;
		case 2 :
			buf[0] = (writeData->regAddr >> SHIFT_BIT8) & MASK_8BIT;
			buf[1] = (writeData->regAddr & MASK_8BIT);
			memcpy( &buf[2], writeData->data, writeData->dataLen );
			len = writeData->dataLen + 2;
			break;
		default :
			return -1;
	}

	iicInform[0].addr = writeData->slaveAddress;	// chip address
	iicInform[0].flags = 0;
	iicInform[0].len = len;
	iicInform[0].buf = buf;

	controlIic.msg = &iicInform[0];
	controlIic.nmsgs = 1;

	for( cnt = 0 ; cnt < CNT_RETRY ; cnt++ )
	{
		if( (ret = ioctl(fdIic,I2C_RDWR,&controlIic)) == -1 )
		{
			printf("[IIC|%d] write : ",writeData->iicChannel);
			perror("ioctl() ");
		}
		else
			break;
	}

	// 3회 실패 시, 클럭핀을 GPIO로 변경 후, 강제로 클럭을 튕긴다.
	// SLAVE가 클럭을 올바르기 인지하지 못하는 경우(8bit), 강제로 맞춰서 살려낸다.
	//( cnt == CNT_RETRY )
		//nctionIicClkReset(writeData->iicChannel);

	sem_post(&semIic);

	return ret;
}

char ReadIic(iicData_t* readData)
{
	struct i2c_rdwr_ioctl_data controlIic;
	struct i2c_msg iicInform[2];
	unsigned int cnt = 0;
	unsigned char buf[256] = {0,};
	unsigned int len = 0;
	int ret = 0;

	if( readData->dataLen > 253 )
	{
		printf("[IIC|%d] read : data size over[%d]\n",readData->dataLen);
		return -1;
	}

	sem_wait(&semIic);

	memset( &controlIic, 0x00, sizeof(controlIic) );
	memset( &iicInform[0], 0x00, sizeof(struct i2c_msg) );
	memset( &iicInform[1], 0x00, sizeof(struct i2c_msg) );
	memset( buf, 0x00, sizeof(buf) );

	switch( readData->regAddrSize )
	{
		case 1 :
			buf[0] = (readData->regAddr & MASK_8BIT);
			memcpy( &buf[1], readData->data, readData->dataLen );
			len = readData->dataLen + 1;
			break;
		case 2 :
			buf[0] = (readData->regAddr >> SHIFT_BIT8) & MASK_8BIT;
			buf[1] = (readData->regAddr & MASK_8BIT);
			memcpy( &buf[2], readData->data, readData->dataLen );
			len = readData->dataLen + 2;
			break;
		default :
			return -1;
	}

	iicInform[0].addr = readData->slaveAddress;
	iicInform[0].flags = 0;
	iicInform[0].len = len;
	iicInform[0].buf = buf;

	iicInform[1].addr = readData->slaveAddress;
	iicInform[1].flags = I2C_M_RD;
	iicInform[1].len = readData->dataLen;
	iicInform[1].buf = readData->data;

	controlIic.msgs = &iicInform[0];
	controlIic.nmsgs = 2;

	for( cnt = 0 ; cnt < CNT_RETRY ; cnt++ )
	{
		if( (ret = ioctl(fdIic,I2C_RDWR,&controlIic)) == -1 )
		{
			printf("[IIC|%d] read : ",readData->iicChannel);
			perror("ioctl() ");
		}
		else
			break;
	}

	// 3회 실패 시, 클럭핀을 GPIO로 변경 후, 강제로 클럭을 튕긴다.
	// SLAVE가 클럭을 올바르기 인지하지 못하는 경우(8bit), 강제로 맞춰서 살려낸다.
	//( cnt == CNT_RETRY )
		//nctionIicClkReset(readData->iicChannel);

	sem_post(&semIic);

	return ret;
}
