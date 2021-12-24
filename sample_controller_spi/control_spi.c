/*
 * File Name : control_spi.c
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
 * File Description : spi controller(Linux-3.0/i.MX6)
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
#include <linux/spi/spidev.h>

#include <semaphore.h>

#include "control_spi.h"

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

#define SPI_TRX_LENGTH	32

typedef struct st_controlSpi {
	int fdSpi;
	unsigned char mode;
	unsigned char bits;
	unsigned short delay;
	unsigned int speed;
	char txBuf[SPI_TRX_LENGTH];
	char rxBuf[SPI_TRX_LENGTH];
	sem_t semSpi;
}__attribute__((packed))controlSpi_t;

#define DRV_SPI_CH	"/dev/spidev"

static controlSpi_t controlSpi;
static struct spi_ioc_transfer spiInform;

static int SetSpiBitPerWord(unsigned char channel, unsigned char bits)
{
	int ret = 0;

	if( controlSpi.fdSpi <= 0 )
		return -1;

	if( (ret = ioctl(controlSpi.fdSpi, SPI_IOC_WR_BITS_PER_WORD, &bits)) < 0 )
		perror("spi - ioctl() ");

	return ret;
}

static int GetSpiBitPerWord(unsigned char channel, unsigned char* bits)
{
	int ret = 0;

	if( controlSpi.fdSpi <= 0 )
		return -1;

	if( (ret = ioctl(controlSpi.fdSpi, SPI_IOC_RD_BITS_PER_WORD, &bits)) < 0 )
		perror("spi - ioctl() ");

	return ret;
}

static int SetSpiSpeed(unsigned char channel, unsigned int speed)
{
	int ret = 0;

	if( controlSpi.fdSpi <= 0 )
		return -1;

	if( (ret = ioctl(controlSpi.fdSpi, SPI_IOC_WR_MAX_SPEED_HZ, &speed)) < 0 )
		perror("spi - ioctl() ");

	return ret;
}

static int GetSpiSpeed(unsigned char channel, unsigned int* speed)
{
	int ret = 0;

	if( controlSpi.fdSpi <= 0 )
		return -1;

	if( (ret = ioctl(controlSpi.fdSpi, SPI_IOC_RD_MAX_SPEED_HZ, speed)) < 0 )
		perror("spi - ioctl() ");

	return ret;
}

int OpenSpiDriver(unsigned char channel, unsigned char bits, unsigned int speed)
{
	char drvName[56] = {0,};
	unsigned char mode = 0;

	memset( drvName, 0x00, sizeof(drvName) );

	switch( channel )	// driver...
	{
		case 0 :
			sprintf( drvName, "%s0.0", DRV_SPI_CH );
			break;
		case 1 :
			sprintf( drvName, "%s0.1", DRV_SPI_CH );
			break;
		case 4 :
			sprintf( drvName, "%s1.0", DRV_SPI_CH );
			break;
		default :
	}

	memset( controlSpi, 0x00, sizeof(controlSpi_t) );
	if( (controlSpi.fdSpi = open(drvName, O_RDWR)) < 0 )
	{
		printf("[SPI|%d] error :: ");
		perror("open() ");
		return -1;
	}

	sem_init( &controlSpi.semSpi,0,1 );

	if( ioctl(controlSpi.fdSpi,SPI_IOC_WR_MODE,&mode) < 0 )
	{
		perror("ioctl() ");
	}

	if( SettingSpiBitPerWord(channel,bits) < 0 )
		return -1;

	GetSpiBitPerWord(channel,&controlSpi.bits);

	if( controlSpi.bits != bits )
	{
		printf("set SPI bit per word :: fail\n");
		return -1;
	}

	if( SetSpiSpeed(channel,speed) < 0 )
		return -1;

	if( controlSpi.speed != speed )
	{
		printf("set SPI speed :: fail\n");
		return -1;
	}

	// minimun 1 usec --> 실제 오실로스코프 측정으로는 9usec... 왜 그런지는 공부를 더 해봐야 겠음...
	controlSpi.delay = 1;

	printf("=[SPI INFORMATION] ===================\n");
	printf("| Channel : %d\n",channel);
	printf("| Bit per Word : %d\n",controlSpi.bits);
	printf("| Speed : %d\n",controlSpi.speed);
	printf("======================================\n");

	return 1;
}

int CloseSpiDriver(unsigned char channel)
{
	if( controlSpi.fdSpi > 0 )
	{
		close(controlSpi.fdSpi);
		sem_destroy(&controlSpi.semSpi);
		controlSpi.fdSpi = 1;
	}

	return 1;
}

int WriteSpi(unsigned char channel, unsigned char* writeData, unsigned int writeLen)
{
	int ret = 0;

	if( controlSpi.fdSpi < 0 )
		return -1;

	if( (writeData == NULL) || (writeLen == 0) )
		return -1;

	sem_wait(&controlSpi.semSpi);

	memset( controlSpi.txBuf, 0x00, SPI_TRX_LENGTH );
	memcpy( controlSpi.txBuf, writeData, writeLen );
	memset( &spiInform, 0x00, sizeof(struct spi_ioc_transfer) );

	spiInform.tx_buf = (unsigned long)controlSpi.txBuf;
	spiInform.len = writeLen;
	spiInform.delay_usecs = controlSpi.delay;
	spiInform.speed_hz = controlSpi.speed;
	spiInform.bits_per_word = controlSpi.bits;

	if( (ret = ioctl(controlSpi.fdSpi, SPI_IOC_MESSAGE(1), &spiInform)) < 0 )
	{
		perror("spi - ioctl() ");
	}

	sem_post(&controlSpi.semSpi);

	return ret;
}

int ReadSpi(unsigned char channel, unsigned char* readData, unsigned int readLen)
{
	int ret = 0;

	if( controlSpi.fdSpi < 0 )
		return -1;

	sem_wait(&controlSpi.semSpi);

	memset( controlSpi.txBuf, 0x00, SPI_TRX_LENGTH );
	memset( controlSpi.rxBuf, 0x00, SPI_TRX_LENGTH );
	memcpy( controlSpi.txBuf, readData, readLen );
	memcpy( controlSpi.rxBuf, readData, readLen );
	memset( readData, 0x00, readLen );

	spiInform.tx_buf = (unsigned long)controlSpi.txBuf;
	spiInform.rx_buf = (unsigned long)controlSpi.rxBuf;
	spiInform.len = readLen;
	spiInform.delay_usecs = controlSpi.delay;
	spiInform.speed_hz = controlSpi.speed;
	spiInform.bits_per_word = controlSpi.bits;

	if( (ret = ioctl(controlSpi.fdSpi, SPI_IOC_MESSAGE(1), &spiInform)) < 0 )
		perror("spi - ioctl() ");


	memcpy( readData, controlSpi.rxBuf, readLen );

	sem_post(&controlSpi.fdSpi);

	return 1;
}
