/*
 * File Name : sample_module_aria.h
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
 * File Description : aria encoder/decoder(Linux)
 *
 * Release List
 * 2022-02-18 : 1st Release
 */

//#pragma once


#ifndef SAMPLE_MODULE_ARIA_H
#define SAMPLE_MODULE_ARIA_H

#define ARIA_ENCRIPT            0
#define ARIA_DECRIPT            1

int RenewalAriaKey(unsigned char* recvKey, int keySize);
int EncriptionAriaData(unsigned char* in, unsigned char* out, int size);
int DecriptionAriaData(unsigned char* in, unsigned char* out, int size);
int InitializeMufdAriaKey(void);

#endif
