/*
 * File Name : control_tca8418.c
 *
 * Copyright 2021 by pwk
 *
 * Developer : PWK (pwk10000@naver.com)
 *
 * Classify : Application
 *
 * Version : 1.00
 *
 * Created Date : 2022-02-17
 *
 * File Description : key matrix chip controller-tca8418(Linux-3.0/i.MX6)
 *
 * Release List
 * 2022-02-17 : 1st Release
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <unistd.h>

#include "control_tca8418.h"

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

#define KEYMTX_CHIP_ADDR	0x34

#define KEYMTX_REG_CFG		0x01	// Configuration Register
#define KEYMTX_REG_INT_STAT	0x02	// Interrupt Clear Register
#define KEYMTX_REG_LCK_EC	0x03	// Interrupt Quantity Register

#define KEYMTX_REG_KEY_EVENTA	0x04	// Key Info Data EVENT 'A' ~ 'J'
#define KEYMTX_REG_KEY_EVENTB	0x05
#define KEYMTX_REG_KEY_EVENTC	0x06
#define KEYMTX_REG_KEY_EVENTD	0x07
#define KEYMTX_REG_KEY_EVENTE	0x08
#define KEYMTX_REG_KEY_EVENTF	0x09
#define KEYMTX_REG_KEY_EVENTG	0x0A
#define KEYMTX_REG_KEY_EVENTH	0x0B
#define KEYMTX_REG_KEY_EVENTI	0x0C
#define KEYMTX_REG_KEY_EVENTJ	0x0D

#define KEYMTX_REG_KP_GPIO1	0x1D	// Select GPIO or Keypad...(ROW0~7)
#define KEYMTX_REG_KP_GPIO2	0x1E	// Select GPIO or Keypad...(COL0~7)
#define KEYMTX_REG_KP_GPIO3	0x1F	// Select GPIO or Keypad...(COL8~9)

#define KEYMTX_REG_GPIEVT1	0x20	// ROW 0 ~ 7 GPI EVENT
#define KEYMTX_REG_GPIEVT2	0x21	// COL 0 ~ 7 GPI EVENT
#define KEYMTX_REG_GPIEVT3	0x22	// COL 8 ~ 9 GPI EVENT

#define KEYMTX_REG_GPIODIR1	0x23	// ROW 0 ~ 7 DIRECTION
#define KEYMTX_REG_GPIODIR2	0x24	// COL 0 ~ 7 DIRECTION
#define KEYMTX_REG_GPIODIR3	0x25	// COL 8 ~ 9 DIRECTION

#define KEYMTX_REG_INTEN1	0x1A	// ROW 0 ~ 7 INTERRUPT ENABLE
#define KEYMTX_REG_INTEN2	0x1B	// COL 0 ~ 7 INTERRUPT ENABLE
#define KEYMTX_REG_INTEN3	0x1C	// COL 8 ~ 9 INTERRUPT ENABLE
// ---------------------------------------------------------------------------------------------------- //

#define FLAG_OFF	0
#define FLAG_ON		1

#define DRV_GPIO_KEY_INT	"/dev/gpio_int"

// GPIO INT FUNCTIONs...
static int fdKeyInt;

static int CheckGpioInt(void)
{
	char buff[4] = {0,};

	if( fdKeyInt < 0 )
		return -1;


	memset( buff, 0x00, sizeof(buff) );

	if( read(fdKeyInt, buff, 4) < 0 )
		return FLAG_OFF;
	else
		return FLAG_ON;
}

static int OpenKeyMtxInt(void)
{
	char drvName[56] = {0,};

	memset( drvName, 0x00, sizeof(drvName) );
	memcpy( drvName, DRV_GPIO_KEY_INT, strlen(DRV_GPIO_KEY_INT) );

	if( (fdKeyInt = open(drvName, O_RDWR|O_NONBLOCK)) < 0 )
	{
		perror("open() ");
		exit(1);
	}

	dlp("Driver [%s] open success\n",drvName);

	return 1;
}

// ---------------------------------------------------------------------------------------------------- //

// IIC READ / WRITE FUNCTIONs...
static int WriteKeyMtxData(const char writeReg, const char data)
{
	return 1;
}

static int ReadKeyMtxData(const char readReg, char* data)
{
	return 1;
}

// ---------------------------------------------------------------------------------------------------- //


int OpenKeyMatrix(void)
{
	unsigned char data = 0;
	char keyEventFlag = FLAG_OFF;
	char eventClearData = 0x01;

	if( WriteKeyMtxData(KEYMTX_REG_CFG, 0x15) < 0 )
	{
		dlp("key_mtx initialize step 1 : fail\n");
		return -1;
	}

	// h/w 구성에 따라 설정.
	if( WriteKeyMtxData(KEYMTX_REG_KP_GPIO1, 0xFF) < 0 )
	{
		dlp("key_mtx initialize step 2 : fail\n");
		return -1;
	}

	if( WriteKeyMtxData(KEYMTX_REG_KP_GPIO2, 0x0F) < 0 )
	{
		dlp("key_mtx initialize step 3 : fail\n");
		return -1;
	}

	if( WriteKeyMtxData(KEYMTX_REG_KP_GPIO3, 0x00) < 0 )
	{
		dlp("key_mtx initialize step 2 : fail\n");
		return -1;
	}

	// 최초 OPEN 시, INT CLEAR
	if( ReadKeyMtxData(KEYMTX_REG_INT_STAT, (char*)&data) < 0 )
		dlp("key_mtx : iic read fail\n");

	if( (data & 0x01) == 0x01 )
	{
		data = 0xFF;
		keyEventFlag = FLAG_ON;
	}

	if( WriteKeyMtxData(KEYMTX_REG_INT_STAT, eventClearData) < 0 )
		dpl("key_mtx : iic write fail\n");

	return 1;
}

int CheckKeyMatrixData(keyMtxInfo_t* mtxInfo, int* bootFlag)
{
	unsigned int loop = 0;
	unsigned int cnt = 0;
	unsigned char data = 0;
	char keyEventFlag = FLAG_OFF;
	char eventClearData = 0x01;


	if( CheckGpioInt() == FLAG_ON )	// DETECT GPIO INTERRUPT 
	{
		data = 0x00;
		if( ReadKeyMtxData(KEYMTX_REG_INT_STAT, (char*)&data) < 0 )
			dlp("key_mtx : iic read fail\n");

		if( (data & 0x01) == 0x01 )	// KEY EVENT INT
		{
			data = 0xFF;
			keyEventFlag = FLAG_ON;
		}
		else	// the other event int... clear
		{
			if( WriteKeyMtxData(KEYMTX_REG_INT_STAT, 0xFF) < 0 )
				dlp("key_mtx : iic write fail\n");

			return -1;
		}

		if( keyEventFlag == FLAG_ON )
		{
			// GET KEY INFO QUANTITY
			if( ReadKeyMtxData(KEYMTX_REG_LCK_EC, (char*)&data) < 0 )
				dlp("key_mtx : iic read fail\n");

			// CLEAR INTERRUPT
			if( WriteKeyMtxData(KEYMTX_REG_INT_STAT, eventClearData) < 0 )
				dlp("key_mtx : iic write fail\n");

			cnt = data;
			dlp("key_mtx : key event cnt[%d]\n",cnt);

			if( cnt >= 10 )	// 이벤트는 MAX10.
			{
				dlp("key_mtx : event over[%d]\n",cnt);
				for( loop = 0 ; loop < cnt ; loop++ )
					ReadKeyMtxData(KEYMTX_REG_KEY_EVENTA, (char*)&data);

				if( WriteKeyMtxData(KEYMTX_REG_INT_STAT, 0xFF) < 0 )
					dlp("key_mtx : iic write fail\n");

				return -1;
			}
			else if( cnt == 0 )	// 0개 인데, INT 발생하는 경우. 그냥 CLEAR
			{
				if( WriteKeyMtxData(KEYMTX_REG_INT_STAT, 0xFF) < 0 )
					dlp("key_mtx : iic write fail\n");

				return -1;
			}

			for( loop = 0 ; loop < cnt ; loop++ )
			{
				data = 0x00;
				if( ReadKeyMtxData(KEYMTX_REG_KEY_EVENTA, (char*)&data) < 0 )
				{
					dlp("key_mtx : iic read fail\n");
					continue;
				}
			}

			data = 0x00;
			keyEventFlag = FLAG_OFF;
		}
	}

	return 1;
}
