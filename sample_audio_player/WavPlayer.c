/*
 * File Name : WavPlayer.c
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
 * File Description : Wave Player Module(Linux/ALSA)
 *
 * Release List
 * 2021-12-24 : 1st Release
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/stat.h>

#include <pthread.h>
#include <semaphore.h>

#include <alsa/asoundlib.h>
#include <alsa/pcm.h>

#include "WavPlayer.h"

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

typedef struct st_audioInform {
	char waveFileName[256];
	unsigned int wavFileNameSize;
	unsigned char flag;
}__attribute__((packed))audioInform_t;

typedef struct st_pcmInfo {
	char			pcmDeviceName[64];
	snd_pcm_t		*pcmHandle;
	snd_pcm_hw_params_t	*pcmHwParams;
	snd_pcm_sw_params_t	*pcmSwParams;
	snd_pcm_format_t	format;
	unsigned int		nChannels;
	unsigned int		sampleRate;
	unsigned int		periodTime;
	unsigned int		bufferTime;
	snd_pcm_uframes_t	bufferSize;
	snd_pcm_uframes_t	periodSize;
	unsigned int		nPeriods;
	unsigned char*		pcmFramesData;
	char			waveFileName[256];
	unsigned int		waveNameSize;
	unsigned int		waveFileSize;
	int			status;
	int			needStop;
}__attribute__((packed))pcmInfo_t;

typedef struct st_waveHeader {
	unsigned char	riffID[4];
	unsigned int	riffLen;
	unsigned char	waveFormat[4];
	unsigned char	fmtID[4];
	unsigned int	fmtLen;
	unsigned short	audioFormat;
	unsigned short	nChannels;
	unsigned int	sampleRate;
	unsigned int	byteRate;
	unsigned short	nBlockAlign;
	unsigned short	nBitPerSample;
	unsigned char	wavID[4];
	unsigned int	rawDataLen;
}__attribute__((packed))waveHeader_t;

#define WAV_NAME_MAXSIZE	256

#define WAV_FIELD_RIFF    "RIFF"
#define WAV_FIELD_FORMAT  "WAVE"
#define WAV_FIELD_FMT     "fmt "
#define WAV_FIELD_DATA    "data"
#define AUDIO_FORMAT_PCM  1

#define FLAG_OFF 	0
#define FLAG_ON		1

static pcmInfo_t pcmInfo;
static audioInform_t playInform;
static sem_t semWavPlayer;

static int SetPcmHwParams(pcmInfo_t *pInfo)
{
	unsigned int reqSampleRate = 0;
	int nErr = 0;
	snd_pcm_uframes_t       periodSizeMin;
	snd_pcm_uframes_t       periodSizeMax;
	snd_pcm_uframes_t       bufferSizeMin;
	snd_pcm_uframes_t       bufferSizeMax;

	// Choose all parameters
	if( (nErr = snd_pcm_hw_params_any(pInfo->pcmHandle,pInfo->pcmHwParams)) < 0 )
	{
		dlp("<Audio Player> Broken configuration for playback : no configurations available : [%s]\r\n",\
				snd_strerror(nErr));
		return -1;
	}

	// set the interleaved read/write format
	if( (nErr = snd_pcm_hw_params_set_access(pInfo->pcmHandle,pInfo->pcmHwParams, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0 )
	{
		dlp("<Audio Player> Access type not available for playback : [%s]\r\n",snd_strerror(nErr));
		return -1;
	}

	// set the sample format
	if( (nErr = snd_pcm_hw_params_set_format(pInfo->pcmHandle,pInfo->pcmHwParams, pInfo->format)) < 0 )
	{
		dlp("<Audio Player> Sample format not available for playback : [%s]\r\n",snd_strerror(nErr));
		return -1;
	}

	// set the count of channels
	if( (nErr = snd_pcm_hw_params_set_channels(pInfo->pcmHandle,pInfo->pcmHwParams,pInfo->nChannels)) < 0 )
	{
		dlp("<Audio Player> Channels count (%i) not available for playback : [%s]\r\n",pInfo->nChannels,snd_strerror(nErr));
		return -1;
	}

	reqSampleRate = pInfo->sampleRate;
	if( (nErr = snd_pcm_hw_params_set_rate(pInfo->pcmHandle,pInfo->pcmHwParams,pInfo->sampleRate,0)) < 0 )
	{
		dlp("<Audio Player> Rate %iHz not available for playback : [%s]\r\n",pInfo->sampleRate,snd_strerror(nErr));
		return -1;
	}


	if( reqSampleRate != pInfo->sampleRate )
	{
		dlp("Rate doesn't match(request %iHz, get %iHz) : [%s]\r\n",\
				pInfo->sampleRate,reqSampleRate,snd_strerror(nErr));
		return -1;
	}

	//dlp("<Audio Player> Rate Set to %iHz (requested %iHz)\r\n",reqSampleRate,pInfo->sampleRate);

	// set the buffer time
	nErr = snd_pcm_hw_params_get_buffer_size_min(pInfo->pcmHwParams,&bufferSizeMin);
	nErr = snd_pcm_hw_params_get_buffer_size_max(pInfo->pcmHwParams,&bufferSizeMax);
	nErr = snd_pcm_hw_params_get_period_size_min(pInfo->pcmHwParams,&periodSizeMin,NULL);
	nErr = snd_pcm_hw_params_get_period_size_max(pInfo->pcmHwParams,&periodSizeMax,NULL);
	//dlp("<Audio Player> Buffer Size Range from %lu to %lu\r\n",bufferSizeMin,bufferSizeMax);
	//dlp("<Audio Player> Period Size Range from %lu to %lu\r\n",periodSizeMin,periodSizeMax);

	if( pInfo->periodTime > 0 )
	{
		dlp("<Audio Player> Requested period time %u us\r\n",pInfo->periodTime);

		if( (nErr = snd_pcm_hw_params_set_period_time_near(pInfo->pcmHandle,pInfo->pcmHwParams,&pInfo->periodTime,NULL)) < 0)
		{
			dlp("<Audio Player> Unable to set period time %u us for playback : [%s]\r\n",pInfo->periodTime,snd_strerror(nErr));
			return -1;
		}
	}
	if( pInfo->bufferTime > 0 )
	{
		dlp("<Audio Player> Requested buffer time %u us\r\n",pInfo->bufferTime);

		if( (nErr = snd_pcm_hw_params_set_buffer_time_near(pInfo->pcmHandle,pInfo->pcmHwParams,&pInfo->bufferTime,NULL)) < 0)
		{
			dlp("<Audio Player> Unable to set buffer time %u us for playback : [%s]\r\n",pInfo->bufferTime,snd_strerror(nErr));
			return -1;
		}
	}

	// set the buffer size
	if( (!pInfo->bufferTime) && (!pInfo->periodTime) )
	{
		pInfo->bufferSize = bufferSizeMax;

		if( !pInfo->periodTime )
		{
			pInfo->bufferSize = (pInfo->bufferSize / pInfo->nPeriods) * pInfo->nPeriods;
		}
		//dlp("<Audio Player> Using Max buffer size %lu\r\n",pInfo->bufferSize);

		if( (nErr = snd_pcm_hw_params_set_buffer_size_near(pInfo->pcmHandle,pInfo->pcmHwParams,&pInfo->bufferSize)) < 0 )
		{
			dlp("<Audio Player> Unable to set buffer size %lu for playback : [%s]\r\n",pInfo->bufferSize,\
					snd_strerror(nErr));
			return -1;
		}
	}

	if( (!pInfo->bufferTime) || (!pInfo->periodTime) )
	{
		//dlp("<Audio Player> Periods = %u\r\n",pInfo->nPeriods);
		if( (nErr = snd_pcm_hw_params_set_periods_near(pInfo->pcmHandle,pInfo->pcmHwParams,&pInfo->nPeriods,NULL)) < 0 )
		{
			dlp("<Audio Player> Unable to set nPeriods %u for playback : [%s]\r\n",pInfo->nPeriods,snd_strerror(nErr));
			return -1;
		}
	}

	snd_pcm_hw_params_get_buffer_size(pInfo->pcmHwParams,&pInfo->bufferSize);
	snd_pcm_hw_params_get_period_size(pInfo->pcmHwParams,&pInfo->periodSize,NULL);

	//dlp("<Audio Player> was set period_size = %lu\r\n",pInfo->periodSize);
	//dlp("<Audio Player> was set buffer_size = %lu\r\n",pInfo->bufferSize);

	if( (2*pInfo->periodSize) > pInfo->bufferSize )
	{
		dlp("<Audio Player> buffer to small, could not use\r\n");
		return -1;
	}

	// write the parameters to PCM device
	if( (nErr = snd_pcm_hw_params(pInfo->pcmHandle,pInfo->pcmHwParams)) < 0 )
	{
		dlp("<Audio Player> Unable to set hw params for playback : [%s]\r\n",snd_strerror(nErr));
		return -1;
	}

	return 1;
}

static int SetPcmSwParams(pcmInfo_t *pInfo)
{
	int nErr = 0;

	// get the current sw params
	if( (nErr = snd_pcm_sw_params_current(pInfo->pcmHandle,pInfo->pcmSwParams)) < 0 )
	{
		dlp("<Audio Player> Unable to determine current sw params for playback : [%s]\r\n",\
				snd_strerror(nErr));
		return -1;
	}

	// start the transfer when a buffer is full
	if( (nErr = snd_pcm_sw_params_set_start_threshold(pInfo->pcmHandle,pInfo->pcmSwParams,pInfo->bufferSize)) < 0 )
	{
		dlp("<Audio Player> Unable to set start threshold mode for playback : [%s]\r\n",\
				snd_strerror(nErr));
		return -1;
	}

	// allow the transfer when at least period_size frames can be processed
	if( (nErr = snd_pcm_sw_params_set_avail_min(pInfo->pcmHandle,pInfo->pcmSwParams,pInfo->periodSize)) < 0 )
	{
		dlp("<Audio Player> Unable to set avail min for playback : [%s]\r\n",snd_strerror(nErr));
		return -1;
	}

#if 0
	if( (nErr = snd_pcm_sw_params_set_xfer_align(pInfo->pcmHandle,pInfo->pcmSwParams,1)) < 0 )
	{
		dlp("<Audio Player> Unable to set transfer align for playback : [%s]\r\n",snd_strerror(nErr));
		return -1;
	}
#endif

	// write the parameters to the PCM device
	if( (nErr = snd_pcm_sw_params(pInfo->pcmHandle,pInfo->pcmSwParams)) < 0 )
	{
		dlp("<Audio Player> Unable to set sw params for playback : [%s]\r\n",snd_strerror(nErr));
		return -1;
	}

	return 1;
}

int CloseAudioPlayer(void)
{
	snd_pcm_close(pcmInfo.pcmHandle);

	system("rmmod snd_soc_wm8960");
	system("rmmod snd_soc_imx_wm8960");

	return 1;
}

int OpenAudioPlayer(void)
{
	int nErr = 0;

	system("insmod /lib/modules/3.0.35-2666-gbdde708/kernel/sound/soc/codecs/snd-soc-wm8960.ko");
	system("insmod /lib/modules/3.0.35-2666-gbdde708/kernel/sound/soc/imx/snd-soc-imx-wm8960.ko");

	sleep(1);

	snd_pcm_hw_params_alloca(&pcmInfo.pcmHwParams);
	snd_pcm_sw_params_alloca(&pcmInfo.pcmSwParams);

	strcpy(pcmInfo.pcmDeviceName, "default");
	pcmInfo.format = SND_PCM_FORMAT_S16;
	pcmInfo.nChannels = 1;    // MONO
	pcmInfo.sampleRate = 48000;       // 48KHz
	pcmInfo.periodTime = 0;
	pcmInfo.bufferTime = 0;
	pcmInfo.nPeriods = 4;

	if( (nErr = snd_pcm_open(&pcmInfo.pcmHandle, pcmInfo.pcmDeviceName, SND_PCM_STREAM_PLAYBACK, 0)) < 0 )
	{
		dlp("<Audio Player> snd_pcm_open error : [%s]\r\n",snd_strerror(nErr));
		return -1;
	}

	if( (nErr = SetPcmHwParams(&pcmInfo)) < 0 )
	{
		dlp("<Audio Player> Setting fo hw parameters failed\r\n");
		snd_pcm_close(pcmInfo.pcmHandle);
		return -1;
	}

	if( (nErr = SetPcmSwParams(&pcmInfo)) < 0 )
	{
		dlp("<Audio Player> Setting of sw parameters failed\r\n");
		snd_pcm_close(pcmInfo.pcmHandle);
		return -1;
	}

	return 1;
}

static int CheckWavFile(pcmInfo_t *pInfo, int wavFd)
{
	waveHeader_t wavHeader;

	if( read(wavFd, &wavHeader, sizeof(waveHeader_t)) < (int)sizeof(waveHeader_t) )
	{
		fprintf(stderr,"Invalid WAV file : [%s]\r\n",pInfo->waveFileName);
		return -1;
	}

	// Check RIFF ID...
	if( memcmp(wavHeader.riffID, WAV_FIELD_RIFF, 4) != 0 )
	{
		fprintf(stderr,"Not a WAV file : [%s]\r\n",pInfo->waveFileName);
		return -1;
	}

	// Check WAVE FORMAT...
	if( memcmp(wavHeader.waveFormat, WAV_FIELD_FORMAT,4) != 0 )
	{
		fprintf(stderr,"Not a WAV file : [%s]\r\n",pInfo->waveFileName);
		return -1;
	}

	// check fmt field
	if( memcmp(wavHeader.fmtID, WAV_FIELD_FMT, 4) != 0 )
	{
		fprintf(stderr,"Not a fmt field : [%s]\r\n",pInfo->waveFileName);
		return -1;
	}

	// Check Audio Format
	if( wavHeader.audioFormat != AUDIO_FORMAT_PCM )
	{
		fprintf(stderr,"Not a PCM Type[%d] : [%s]\r\n",wavHeader.audioFormat,pInfo->waveFileName);
		return -1;
	}

	switch( wavHeader.nChannels )
	{
		case 1 :
			break;
		case 2 :
			fprintf(stderr,"%s is stereo stream\r\n",pInfo->waveFileName);
			return -1;
		default :
			fprintf(stderr,"%s is not a mono or stereo stream[%d channels]\r\n",pInfo->waveFileName,wavHeader.nChannels);
			return -1;
	}

	if( pInfo->sampleRate != wavHeader.sampleRate )
	{
		fprintf(stderr,"%s's Sample Rate doesn't match (%d)\r\n",pInfo->waveFileName,wavHeader.sampleRate);
		return -1;
	}

	if( wavHeader.nBitPerSample != 16 )
	{
		fprintf(stderr,"Unsupported sample format bits %d for %s\r\n",wavHeader.nBitPerSample,pInfo->waveFileName);
		return -1;
	}
#if 0
	if( memcmp(wavHeader.wavID,WAV_FIELD_DATA, 4) != 0 )
	{
		fprintf(stderr,"Not Data field[%s] : [%s]\r\n",wavHeader.wavID,pInfo->waveFileName);
		return -1;
	}
#endif

#if 0
	dlp("<Audio Player> WAV Header Info ------\r\n");
	dlp("<Audio Player> sample_rate = %d, channels = %d, chunk_size = %d\r\n",\
			wavHeader.sampleRate,wavHeader.nChannels,wavHeader.rawDataLen);
	dlp("<Audio Player> ----------------------\r\n");
#endif

	{
		struct stat fileInfo;
		int retCheck = 0;

		if( (retCheck = stat(pInfo->waveFileName, &fileInfo)) == -1 )
		{
			dlp("stat() error \r\n");
		}
		// File Size Get...
		pInfo->waveFileSize = (fileInfo.st_size - sizeof(waveHeader_t));
	}

	if( pInfo->waveFileSize != wavHeader.rawDataLen )
	{
		//dlp("Real File Size[%d] vs Wave Header size info[%d]\r\n",pInfo->waveFileSize,wavHeader.rawDataLen);
		// Save Log : WAV Header 의 Size 정보가 실제 파일 사이즈보다 큰 경우 -> 음원이 깨진 경우이다.
	}

	close(wavFd);

	return 1;
}

static int ReadWavFile(unsigned short *buf, int channels, unsigned int offset, int bufSize)
{
	static FILE *wavFp = NULL;
	int size = 0;

	if( offset >= pcmInfo.waveFileSize )
	{
		pcmInfo.needStop = FLAG_ON;
		pcmInfo.status = FLAG_OFF;
		return 1;
	}

	if( !offset )
	{
		if( wavFp )
			fclose(wavFp);

		wavFp = fopen(pcmInfo.waveFileName, "r");
		if( !wavFp )
			return -1;
		if( fseek(wavFp, sizeof(waveHeader_t), SEEK_SET) < 0 )
			return -1;
	}

	if( (unsigned int)(offset + bufSize) > pcmInfo.waveFileSize )
		bufSize = pcmInfo.waveFileSize - offset;

	bufSize /= pcmInfo.nChannels;

	for( size = 0 ; size < bufSize ; size += 2 )
	{
		int chn = 0;
		for(chn = 0 ; (unsigned int)chn < pcmInfo.nChannels ; chn++ )
		{
			if( chn == channels )
			{
				if( fread(buf, 2, 1, wavFp) != 1 )
				{
					return size;
				}
			}
			else
			{
				*buf = 0;
				fclose(wavFp);
			}
			buf++;
		}
		if( pcmInfo.needStop == FLAG_ON )
			break;
	}

	return size;
}

static int RecoveryPcm(snd_pcm_t *pHandle, int err)
{
	if( err == -EPIPE )       // under-run
	{
		if( (err = snd_pcm_prepare(pHandle)) < 0 )
		{
			dlp("Can't recovery from underrun, prepare failed : [%s]\r\n",snd_strerror(err));
			return 0;
		}
	}
	else if( err == -ESTRPIPE )
	{
		// wait until the suspend flag is released
		while( (err = snd_pcm_resume(pHandle)) == -EAGAIN )
		{
			if( pcmInfo.needStop == FLAG_ON )
				break;
			sleep(1);
		}

		if( err < 0 )
		{
			if( (err = snd_pcm_prepare(pHandle)) < 0 )
				dlp("<Audio Player> Can't recovery from suspend, prepare failed : [%s]\r\n",snd_strerror(err));
		}

		return 0;
	}

	return err;
}

static int WritePcmBuffer(snd_pcm_t *pHandle, unsigned char *ptr, int cptr)
{
	int nErr = 0;

	while( cptr > 0 )
	{
		if( pcmInfo.needStop == FLAG_ON )
		{
			dlp("<Audio Player> Stop Play\r\n");
			break;
		}

		nErr = snd_pcm_writei(pHandle, ptr, cptr);

		if( nErr == -EAGAIN )
			continue;

		if( nErr < 0 )
		{
			//dlp("<Audio Player> write error : %s\r\n",snd_strerror(nErr));
			if( RecoveryPcm(pHandle,nErr) < 0 )
			{
				dlp("<Audio Player> Recovery failed\r\n");
				return -1;
			}
			break;
		}

		ptr += snd_pcm_frames_to_bytes(pHandle, nErr);
		cptr -= nErr;
	}

	return nErr;
}

static int WritePcmDevice(snd_pcm_t *pHandle, unsigned int channels, unsigned char *frames)
{
	int nErr = 0;
	unsigned int num = 0;
	int bufSize = snd_pcm_frames_to_bytes(pHandle,pcmInfo.periodSize);

	while( (nErr = ReadWavFile((unsigned short *)frames, channels, num, bufSize)) > 0 )
	{
		num += nErr;

		if( (nErr = WritePcmBuffer(pHandle, frames, snd_pcm_bytes_to_frames(pHandle, nErr*pcmInfo.nChannels))) < 0 )
		{
			break;
		}

		if( pcmInfo.needStop == FLAG_ON )
		{
			break;
		}
	}

	if( pcmInfo.bufferSize > num )
	{
		snd_pcm_drain(pHandle);
		snd_pcm_prepare(pHandle);
	}

	return nErr;
}


static int PlayWaveFile(audioInform_t *pInfo)
{
	int nErr = 0;
	unsigned int chn = 0;
	int wavFd = 0;
	int num = 0;
	int loop = 1;

	if( pInfo->waveFileName == NULL )
		return -1;

	memset( pcmInfo.waveFileName, 0x00, 256 );
	memcpy( pcmInfo.waveFileName, pInfo->waveFileName, pInfo->wavFileNameSize );
	printf("<Audio Player> File Name : [%s]\r\n",pcmInfo.waveFileName);
	pcmInfo.waveFileSize = pInfo->wavFileNameSize;

	pcmInfo.needStop = 0;

	if( (wavFd = open(pcmInfo.waveFileName,O_RDONLY)) < 0 )
	{
		fprintf(stderr,"Cannot open WAV file : [%s]\r\n",pcmInfo.waveFileName);
		return -1;
	}

	lseek(wavFd,0,SEEK_SET);

	for( chn = 0 ; chn < pcmInfo.nChannels ; chn++ )
	{
		if( CheckWavFile(&pcmInfo,wavFd) < 0 )
		{
			close(wavFd);
			dlp("<Audio Player> Wave File Check Failed\r\n");
			return -1;
		}
	}

	pcmInfo.status = FLAG_ON;

	if( (pcmInfo.pcmFramesData = (unsigned char *) calloc(snd_pcm_frames_to_bytes(pcmInfo.pcmHandle,pcmInfo.periodSize),sizeof(char))) == NULL )
	{
		dlp("<Audio Player> No enough memory\r\n");
		snd_pcm_close(pcmInfo.pcmHandle);
		return -1;
	}

	for( num = 0 ; !loop||num<loop ; loop++ )
	{
		for( chn = 0 ; chn < pcmInfo.nChannels ; chn++ )
		{
			if( (nErr = WritePcmDevice(pcmInfo.pcmHandle, chn, pcmInfo.pcmFramesData)) < 0 )
			{
				//dlp("<Audio Player> Transfer failed\r\n");
			}

			// 강제 종료...
			if( pcmInfo.needStop == FLAG_ON )
			{
				//dlp("<Audio Player> Stop Command\r\n");
				break;
			}
		}

		if( pcmInfo.needStop == FLAG_ON )
		{
			//printf("<Audio Player> Stop Command\r\n");
			free(pcmInfo.pcmFramesData);
			pcmInfo.pcmFramesData = NULL;
			pcmInfo.status = FLAG_OFF;
			return -2;
		}
	}

	free(pcmInfo.pcmFramesData);
	pcmInfo.pcmFramesData = NULL;
	pcmInfo.status = FLAG_OFF;

	return 1;
}

int* ThreadAudioPlayer(void)
{
        int* ret;
	audioInform_t setInform;

	memset( &setInform, 0x00, sizeof(audioInform_t) );

        printf("[admin] thread create ok - ThreadAudioPlayer\r\n");

	if( OpenAudioPlayer() < 0 )
	{
		printf("AUDIO PLAYER OPEN FAIL\n");
		exit(1);
	}

        while(1)
        {

		sem_wait(&semWavPlayer);
		if( playInform.flag == FLAG_ON )
		{
			memset( &setInform, 0x00, sizeof(audioInform_t) );
			memcpy( &setInform, &playInform, sizeof(audioInform_t) );
			PlayWaveFile(&setInform);
		}
		sem_post(&semWavPlayer);

		if( pcmInfo.needStop == FLAG_ON )
			pcmInfo.needStop = FLAG_OFF;

		usleep( 100*1000 );
	}

	return ret;
}

int CheckAudioPlayer(void)
{
	return pcmInfo.status;
}

int StopAudioPlayer(void)
{
	if( pcmInfo.needStop == FLAG_OFF )
		pcmInfo.needStop = FLAG_ON;

	return 1;
}

int PlayOnWaveFile(char* fileName, unsigned int fileNameSize)
{
	if( (fileName == NULL) || (fileNameSize == 0) )
		return -1;

	sem_wait(&semWavPlayer);

	memset( &playInform, 0x00, sizeof(audioInform_t) );
	memcpy( playInform.waveFileName, fileName, fileNameSize );
	playInform.wavFileNameSize = fileNameSize;
	playInform.flag = FLAG_ON;
	sem_post(&semWavPlayer);

	return 1;
}

int StartWavePlayer(void)
{
	pthread_t wavePlayer;

	sem_init(&semWavPlayer,0,1);

	if( pthread_create(&wavePlayer,NULL,(void*)ThreadAudioPlayer,NULL)<0 )
	{
		printf("#### thread create error : ThreadAudioPlayer");
		exit(1);
	}

	return 1;
}
