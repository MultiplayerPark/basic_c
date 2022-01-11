#ifndef __RTP_CONTROL_H__
#define __RTP_CONTROL_H__
//[*]-------------------------------------------------------------------------------------------------------------------------------------------------------------[*]
#include "mediastreamer2/mediastream.h"
//[*]-------------------------------------------------------------------------------------------------------------------------------------------------------------[*]
typedef enum	{
    MODULE_STATUS_UNINIT = 0,
    MODULE_STATUS_INIT,    
    MODULE_STATUS_PLAY,
    MODULE_STATUS_STOP
}	_ModuleStatus;

bool_t          RtpInit(void);
bool_t          RtpExit(void);

bool_t          RtpPlay(char *pAddress, int Port, bool_t IsMultiCast, int Jitter);
bool_t          RtpStop(void);

void            SetRtpLogLevel(int Level);
void            SetSoundLevel(int Percent);
int             GetSoundLevel(void);

void            SetRtpJitter(int Jitter);
void			SetOssHandle(int OssHandle);
void			SetIoctlFunction(OssIoctl IoctlFunction);
void			SetWriteFunction(OssWrite WriteFunction);
void            RtpStatusReset(void);

uint64_t        GetPacketLoss(void);
uint64_t        GetPacketLate(void);
uint64_t        GetPacketBad(void);
uint64_t        GetPacketDiscard(void);
double          GetRecvBandwidth(void);
const char*     GetModuleVer(void);
_ModuleStatus   GetModuleStatus(void);
//[*]-------------------------------------------------------------------------------------------------------------------------------------------------------------[*]
#endif
