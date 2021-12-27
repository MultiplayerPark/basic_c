/*
 * File Name : control_uart.h
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
 * File Description : uart controller(Linux)
 *
 * Release List
 * 2021-12-24 : 1st Release
 */

//#pragma once

#ifndef CONTROL_UART_H
#define CONTROL_UART_H

int SendUart1Data(char* data, unsigned int length);
int SendUart2Data(char* data, unsigned int length);
int StartUartProcess(void);

#endif 
