/*
 * File Name : control_tca8418.h
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
 * File Description : key matrix chip controller(tca8418)(Linux)
 *
 * Release List
 * 2022-02-17 : 1st Release
 */

//#pragma once

int OpenKeyMatrix(void);
int CheckKeyMatrixData(keyMtxInfo_t* mtxInfo, int* bootFlag);

#endif
