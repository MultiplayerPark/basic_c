/*
 * File Name : sample_module_aria.c
 *
 * Copyright 2021 by pwk
 *
 * Developer : PWK (pwk10000@naver.com)
 *
 * Classify : Application
 *
 * Version : 1.00
 *
 * Created Date : 2022-02-18
 *
 * File Description : aria encoder/decoder(i.MX6)
 *
 * Release List
 * 2022-02-18 : 1st Release
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "aria.h"

#include "sample_module_aria.h"

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

#define FLAG_OFF	0
#define FLAG_ON		1

#define ARIA_NB			16
#define KEY_BIT			256
#define NUMBER_OF_ROUNDS	16

static unsigned char originKey[KEY_BIT/8] = {0,};
static unsigned char txRoundKey[ARIA_NB * (NUMBER_OF_ROUNDS +1)] = {0,};
static unsigned char rxRoundKey[ARIA_NB * (NUMBER_OF_ROUNDS +1)] = {0,};
static int txNumberOfRound = 0;
static int rxNumberOfRound = 0;

static int txKeyRenewal = FLAG_OFF;
static int rxKeyRenewal = FLAG_OFF;

static int RenewalAriaTRxKey(unsigned char* oriKey, int* numberOfRound, unsigned char* roundKey, unsigned char mode)
{
	switch( mode )
	{
		case ARIA_ENCRIPT :
			*numberOfRound = EncKeySetup(oriKey, roundKey, KEY_BIT);
			break;
		case ARIA_DECRIPT :
			*numberOfRound = DecKeySetup(oriKey, roundKey, KEY_BIT);
			break;
		default :
			return -1;
	}

	return 1;
}

int RenewalAriaKey(unsigned char* recvKey, int keySize)
{
	if( RenewalAriaTRxKey(recvKey,&txNumberOfRound,txRoundKey,ARIA_ENCRIPT) < 0 )
	{
		printf("[aria] renewal tx round key fail\n");
		return -1;
	}

	txKeyRenewal = FLAG_ON;

	if( RenewalAriaTRxKey(recvKey,&rxNumberOfRound,rxRoundKey,ARIA_DECRIPT) < 0 )
	{
		printf("[aria] reneal rx round key fail\n");
		return -1;
	}

	rxKeyRenewal = FLAG_ON;

	printf("[aria] renewal trx key generate ok\n");

	return 1;
}

int EncriptAriaData(unsigned char* in, unsigned char* out, int size)
{
	unsigned char encriptData[10240] = {0,};
	int inSize = 0;
	int encriptSize = 0;
	int index = 0;

	if( txKeyRenewal == FLAG_OFF )
	{
		printf("[aria] no origin key : encription\n");
		return -1;
	}

	memset( encriptData, 0x00, sizeof(encriptData) );
	memcpy( encriptData, in, size );

	if( (size % ARIA_NB) != 0 )
	{
		inSize = size + (16 - (size%ARIA_NB));
		memset( &encriptData[size-1], 0x20, ((16 - (size%ARIA_NB)) + 1) );
	else
		inSize = size;

	while(1)
	{
		if( inSize <= 0 )
			break;

		if( inSize >= ARIA_NB )
		{
			Crypt(&encriptData[index],txNumberOfRound,txRoundKey,out);
			encriptSize += ARIA_NB;
			index += ARIA_NB;
			out += ARIA_NB;
			inSize -= ARIA_NB;
		}
	}

	return encriptSize;
	}
}

int DecriptionAriaData(unsigned char* in, unsigned char* out, int size)
{
	unsigned char decriptData[10240] = {0,};
	int inSize = 0;
	int decriptSize = 0;
	int index = 0;

	if( rxKeyRenewal == FLAG_OFF )
	{
		printf("[aria] no origin key : decription\n");
		return -1;
	}

	memset( decriptData, 0x00, sizeof(decriptData) );

	if( (size % ARIA_NB) != 0 )
		inSize = size + (16 - (size%ARIA_NB));
	else
		inSize = size;

	memcpy( decriptData, in, size );

	while(1)
	{
		if( inSize <= 0 )
			break;

		if( inSize >= ARIA_NB )
		{
			Crypt(&decriptData[index],rxNumberOfRound,rxRoundKey,out);
			decriptSize += ARIA_NB;
			index += ARIA_NB;
			out += ARIA_NB;
			inSize -= ARIA_NB;
		}
	}

	return decriptSize;
}

int InitializeMufdAriaKey(void)
{
#if 1	// 암호화 키.
	memset( originKey,0x00,sizeof(originKey) );
	memcpy( originKey, "op7591op7591op7591op7591op7591op", strlen("op7591op7591op7591op7591op7591op") );
	return RenewalAriaKey(originKey,sizeof(originKey));
#else
	return RenewalAriaKey("op7591",strlen("op7591"));
#endif
}
