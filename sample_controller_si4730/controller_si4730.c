/*
 * File Name : control_si4730.c
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
 * File Description : si4730(IIC control 2CHANNEL FM MODULE) controller(Linux-3.0/i.MX6)
 *
 * Release List
 * 2021-12-24 : 1st Release
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
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

typedef struct st_iicInform_t {
	unsigned char iicChannel;
	unsigned char slaveAddress;
	unsigned short regAddress;
	unsigned char regAddressSize;
	unsigned char data[256];
	unsigned int dataSize;
}__attribute__((packed))iicInform_t;

static int WriteIicToFm(iicInform_t* writeData)
{
	//return WriteIic();	// call write iic function
}

static int ReadIicFromFm(iicInform_t* readData)
{
	//return ReadIic();	// call read iic function
}

#define SI4730_CHIP_ADDRESS		0x63
#define SI4730_RESPONSE_OK		0x80
#define SI4730_SETTING_FREQ_RESPONSE_OK	0x81

// POWER UP : 0x01 + 0xD0 + 0xB0(0x05:Analog Audio/0xB0:Digital Audio/0xB5:Both)

#define SI4730_CMD_POWER_UP		0x01	// setting power up
#define SI4730_CMD_REQ_REV_INFO		0x10	// Request Revision informations..
#define SI4730_CMD_SET_PROPERTY		0x12	// setting property
#define SI4730_CMD_SET_FM_FREQ		0x20	// setting frequency
#define SI4730_CMD_REQ_FM_FREQ		0x22	// getting frequency

#define SI4730_DATA_POWER_UP_FMR	0xC0	// FM Receiver
#define SI4730_DATA_POWER_UP_AMR	0xC1	// AM Receiver

#define SI4730_DATA_POWER_UP_DA		0xB0	// Digital Audio
#define SI4730_DATA_POWER_UP_AA		0x05	// Analog Audio

#define SI4730_REG_D_SAMPLERATE		0x0104	// parameter : samplerate
#define SI4730_REG_D_OUT_FORMAT		0x0102	// parameter : digital output format
#define SI4730_REG_GPO_IEN		0x0001	// parameter : gpio interrupt setting
#define SI4730_REG_REFCLK_FREQ		0x0002	// parameter : REF CLK
#define SI4730_REG_REFCLK_PRESCALE	0x0202	// parameter : REF CKK prescale
#define SI4730_REG_RX_VOLUME		0x4000	// parameter : rx volume
#define SI4730_REG_FM_DEEMPHASIS	0x1100	// parameter : fm deemphasis
#define SI4730_REG_RX_HARD_MUTE         0x4001	// parameter : rx hard mute
#define SI4730_REG_FM_RSSI_ST_THD       0x1800	// parameter : fm blend rssi stereo threshold
#define SI4730_REG_FM_RSSI_MO_THD       0x1804	// parameter : fm blend rssi mono threshold
#define SI4730_REG_FM_MAX_TUNE_ERR      0x1108	// parameter : fm max tune error
#define SI4730_REG_FM_RSQ_INT_SRC       0x1200	// parameter : fm rsq int source ??
#define SI4730_REG_HARD_MUTE            0x4001	// parameter : hardware mute

static int PowerUpSi4730(char id, unsigned char mode, unsigned char audioType)
{
	iicInform_t initData;
	unsigned int stackCnt = 0;

	memset( &initData, 0x00, sizeof(iicInform_t) );

	switch( id )
	{
		case FM_NUM1 :
		case FM_NUM2 :
			initData.iicChannel = id;	// setting iic channel inform
			break;
		default :
			return -1;
	}

	initData.slaveAddress = SI4730_CHIP_ADDRESS;
	initData.regAddress = SI4730_CMD_POWER_UP;
	initData.regAddressSize = 1;

	switch( mode )
	{
		case FM_RECEIVER :
			initData.data[stackCnt++] = SI4730_DATA_POWER_UP_FMR;
			break;
		case AM_RECEIVER :
			initData.data[stackCnt++] = SI4730_DATA_POWER_UP_AMR;
			break;
		default :
			return -1;
	}

	switch( audioType )
	{
		case FM_DIGITAL_AUDIO :
			initData.data[stackCnt++] = SI4730_DATA_POWER_UP_DA;
			break;
		case FM_ANALOG_AUDIO :
			initData.data[stackCnt++] = SI4730_DATA_POWER_UP_AA;
			break;
		default :
			return -1;
	}

	initData.dataSize = stackCnt;

	WriteIicToFm(initData);

	// wait initialize..
	usleep( 110*1000 );
}
