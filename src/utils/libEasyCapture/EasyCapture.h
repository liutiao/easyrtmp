#ifndef __RG4_VIDEO_CAPTURE_INSTANCE_H_____
#define __RG4_VIDEO_CAPTURE_INSTANCE_H_____

#include "../libEasySource/BaseSource.h"
//#include "../../common/EzStreamSvr/EzBase.h"
//#include "../../utils/libV4R/V4R.h"
#include "V4R.h"

#define _USE_WAVEIN
#ifdef LIBEASYCAPTURE_EXPORTS
#	define LIBEASYCAPTURE_API __declspec(dllexport)
#else
#	define LIBEASYCAPTURE_API __declspec(dllimport)
#endif

#pragma once

class LIBEASYCAPTURE_API CEasyCapture : public CBaseVideo
{
public:
	CEasyCapture();
	virtual ~CEasyCapture(void);

public:
	virtual BOOL_		Start();
	virtual BOOL_		Stop();

	virtual BOOL_		CheckReceiveStatus();

	static void			cbFrameCB(void* pParam, uint8_t* pFrame, int nBytes, int nCodecID, int vDuringAlarm, int16_t vFrameType, int bStretchMode, int w, int h, int32_t ts);
	static void			cbRawVideoCB(void* pParam, uint8_t* buf, int nBytes, int w, int h, int nPixFmt, int64_t llTimeStamp);

	//////////////////////////////////////////////////////////////////////////
	//Audio
	BOOL_				StartAudioCapture(int nIndex);
	void				StopAudioCapture();
	static void			cbRawAudioCB(void* pParam, uint8_t* buf, int nBytes, int channels, int bitsPerSample, int sampleRate, int nRawFmt, int64_t llTimeStamp);
#ifdef _USE_WAVEIN
	static void			cbAudioCallbackRaw(void* p, uint8_t* pBuf, int nBytes, int channels, int bitsPerSample, int sampleRate, int nRawFmt, int64_t llTimeStamp);
#endif
	AUDIO_PROFILE		m_vAudioParam;
	void*				m_pAudio;
	static void			stcAudioEncodeCallback(int ch, void* pParam, unsigned char* pEncBuffer, int nEncSize, int nCodecID,int nChannel, int nBitsPerSample, int nSamplePerSec, const char* szExtraData, unsigned long long llTimestamp);
	void				cbAudioCallbackEnc(int ch, unsigned char* pEncBuffer, int nEncSize, int nCodecID,int nChannel, int nBitsPerSample, int nSamplePerSec, const char* szExtraData, unsigned long long llTimestamp);

private:
	BOOL_				StartVideoCapture(int nChannel, const char* osd, BOOL_ bOSD, BOOL_ bOSDTime);
	BOOL_				StopVideoCapture();

public:
	CV4R*				m_pV4R;
	CCodec*				m_pEnc_a;
};

#endif //__RG4_VIDEO_CAPTURE_INSTANCE_H_____
