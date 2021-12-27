/*
 * File Name : control_lcd.h
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
 * File Description : lcd test controller(Linux)
 *
 * Release List
 * 2021-12-24 : 1st Release
 */

//#pragma once

#ifndef CONTROL_LCD_H
#define CONTROL_LCD_H

#define TESTLCD_MODE_BLACK	0
#define TESTLCD_MODE_WHITE	1
#define TESTLCD_MODE_RED	2
#define TESTLCD_MODE_GREEN	3
#define TESTLCD_MODE_BLUE	4

int OpenLcdDriver(void);
int CloseLcdDriver(void);
int DisplayLcd(unsigned char mode);

#endif
