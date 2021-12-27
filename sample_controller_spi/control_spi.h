/*
 * File Name : control_spi.h
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
 * File Description : spi controller(Linux)
 *
 * Release List
 * 2021-12-24 : 1st Release
 */

//#pragma once

#ifndef CONTROL_SPI_H
#define CONTROL_SPI_H


int OpenSpiDriver(unsigned char channel, unsigned char bits, unsigned int speed);
int CloseSpiDriver(unsigned char channel);
int WriteSpi(unsigned char channel, unsigned char* writeData, unsigned int writeLen);
int ReadSpi(unsigned char channel, unsigned char* readData, unsigned int readLen);

#endif
