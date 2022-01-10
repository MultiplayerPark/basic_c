/*
 * File Name : controller_modbus.c
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
 * File Description : modbus controller
 *
 * Release List
 * 2021-12-24 : 1st Release
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <arpa/inet.h>

#include <semaphore.h>
#include <pthread.h>

#include <sys/types.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "include/modbus/modbus.h"
#include "include/modbus/modbus-rtu.h"


// ----------------------------------------------------------------------------------------------------- //
// Debugging Option...																					 //
// ----------------------------------------------------------------------------------------------------- //
#define xxx_DEBUG

#ifdef xxx_DEBUG
#define dlp(fmt,args...) printf("[%s,%s(),%d]"fmt,__FILE__,__FUNCTION__,__LINE__,## args)
#else
#define dlp(fmt,args...)
#endif
// ----------------------------------------------------------------------------------------------------- //


static int* ThreadModbus(void)
{
	int* ret;
	modbus_t* ctx;
	struct timeeval response;
	int length = 0;
	unsigned int loop = 0;
	unsigned int delayCnt = 0;

	ctx = modbus_new_rtu("/dev/ttyS5",9600,'N',8,1);
	if( ctx == NULL )
		fprintf(stderr, "Unable to create the libmodbus context for RTU:%s\n",modbus_strerror(errno));

	printf("new modbus rtu attach\n");

	if( modbus_set_slave(ctx,0x01) == -1 )
	{
		fprintf(stderr, "Unable to set the slave ID in context for RTU:%s\n",modbus_strerror(errno));
		printf("Unable to set the slave ID in context for RTU:%s\n",modbus_strerror(errno));
	}

	modbus_rtu_set_serial_mode(cts,MODBUS_RTU_RS232);
	response.tv_set = 1;
	modbus_set_response_timeout(ctx, 0, 1000);
	modbus_set_error_recovery(ctx,MODBUS_ERROR_RECOVERY_LINK|MODBUS_ERROR_RECOVERY_PROTOCOL);

	if( modbus_connect(ctx) == -1 )
	{
		fprintf(stderr, "Unable to connect:%s\n",modbus_strerror(errno));
		modbus_free(ctx);
	}

	while(1)
	{
		if( delayCnt >= 200 )
		{
			modbus_flush(ctx);
			modbus_set_slave(ctx,0x01);
			ControlSpare1_485Enable(FLAG_ON);
			length = modbus_read_registers(ctx,0x00C8,0x005A,test);
			//length = modbus_read_registers(ctx,0x00D4,0x0008,test);
			usleep( 10*1000 );
			ControlSpare1_485Enable(FLAG_OFF);
			if( length < 0 )
			{
				//fprintf(stderr, "modbus_read_registers fail %s\n",modbus_strerror(errno));
				//printf("modbus_read_registers fail %s\n",modbus_strerror(errno));
			}
			else
			{
				for( loop = 0 ; loop < length ; loop++ )
					printf("[0x%04x]",test[loop]);
			}

			delayCnt = 0;
		}

		delayCnt++;

		usleep( 100*1000 );
	}

	pthread_exit(0);

	return ret;
}




