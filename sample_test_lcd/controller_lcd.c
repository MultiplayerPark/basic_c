/*
 * File Name : controller_lcd.c
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
 * File Description : lcd test controller(Linux-3.0/i.MX6)
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
#include <sys/mman.h>
#include <linux/fb.h>

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

#define DRV_LCD_DEVICE	"/dev/fb"

static int displayFd = 0;
static struct fb_var_screeninfo fbVarInfo;
static struct fb_fix_screeninfo fbFixInfo;

int OpenLcdDriver(void)
{
	if( access(DRV_LCD_DEVICE, F_OK) != 0 )
	{
		perror("access()");
		return -1;
	}

	if( (displayFd = open(DRV_LCD_DEVICE,O_RDWR)) < 0 )
	{
		perror("open()");
		return -1;
	}

	if( ioctl(displayFd, FBIOGET_VSCREENINFO, &fbVarInfo) < 0 )
	{
		perror("ioctl()");
		close(displayFd);
		displayFd = 0;
		return -1;
	}

	if( ioctl(displayFd, FBIOGET_FSCREENINFO, &fbFixInfo) < 0 )
	{
		perror("ioctl()");
		close(displayFd);
		displayFd = 0;
		return -1;
	}

	system("export QWS_DISPLAY=LinuxFb:mmWidth=80:mmHeight=130:/dev/fb0");

	printf("[admin] open lcd info ---------------\r\n");
	printf("| screen width  = %d\r\n",fbVarInfo.xres);
	printf("| screen height = %d\r\n",fbVarInfo.yres);
	printf("| bit per pixel = %d\r\n",fbVarInfo.bits_per_pixel);
	printf("| line length   = %d\r\n",fbFixInfo.line_length);
	printf("-------------------------------------\r\n");

	return 1;
}

int CloseLcdDriver(void)
{
	if( displayFd <= 0 )
		return 0;

	close(displayFd);
	displayFd = 0;

	return 1;
}

int DisplayLcd(unsigned char mode)
{
	unsigned short color = 0;
	unsigned int ndx = 0;
	unsigned int widthIndex = 0;

	if( displayFd <= 0 )
		return -1;

	switch( mode )
	{
		case TESTLCD_MODE_BLACK :
			color = 0x0000;
			break;
		case TESTLCD_MODE_WHITE :
			color = 0xFFFF;
			break;
		case TESTLCD_MODE_RED :
			color = 0xF800;
			break;
		case TESTLCD_MODE_GREEN :
			color = 0x07E0;
			break;
		case TESTLCD_MODE_BLUE :
			color = 0x001F;
			break;
		default :
			printf("unknown mode[%d]\n",mode);
			return -1;
	}

	lseek( displayFd, 0, SEEK_SET );

	for( ndx = 0 ; ndx < fbVarInfo.yres ; ndx++ )
	{
		lseek(displayFd, 0, SEEK_SET);
		lseek(displayFd, (ndx*fbFixInfo.line_length), SEEK_SET);
		for( widthIndex = 0 ; widthIndex < fbVarInfo.xres ; widthIndex++ )
		{
			if( write(displayFd, &color, 2) < 0 )
			{
				perror("write()");
				close(displayFd);
				displayFd = 0;
				return -1;
			}
		}
	}

	return 1;
}
