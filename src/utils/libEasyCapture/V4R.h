//////////////////////////////////////////////////////////////////////////
//jacky, there is a critical bug in current implements
//CEzEncoder::m_pPicture can not be freed while using avfilter
//////////////////////////////////////////////////////////////////////////

#ifndef __VIDEO_FOR_RG4_MEDIA_SYSTEMS_H____
#define __VIDEO_FOR_RG4_MEDIA_SYSTEMS_H____

#include <stdint.h>
#include <math.h>
#include <pthread.h>
#include <rscommon.h>

#include "Codec.h"
/*
#ifdef LIBV4R_EXPORTS
#	define LIBV4R_API __declspec(dllexport)
#else
#	define LIBV4R_API __declspec(dllimport)
#endif
*/
#pragma once

#ifdef __cplusplus
extern "C" {
#endif
	
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>

class CRsCameraDS;
//class LIBV4R_API CV4R
class CV4R
{
public:
	CV4R();
	virtual ~CV4R();

public:
	//////////////////////////////////////////////////////////////////////////
	BOOL_			StartCaptureV(int nChannel, const char* osd, BOOL_ bOSD, BOOL_ bOSDTime, RAWVIDEO_CALLBACK fpRawVideoCB, FRAME_CALLBACK fpFrameCB, void* pParam);
	BOOL_			StopCaptureV();
	//////////////////////////////////////////////////////////////////////////
	BOOL_			SetupVideoParam(int nCodec, int nFrameRate, int nBitRate, int nGOP, int nWidth, int nHeight, int nLevel);
	void			SetLogCallback(void* pParent, RsLogReport stcLogReport);
	//////////////////////////////////////////////////////////////////////////
	void			InputVideoRaw(unsigned char *pDecBuffer, int nDecSize, int nImageWidth, int nImageHeight, int nPixelFormat, unsigned long long llTimeStamp,BOOL_ bKeyFrame);
	//////////////////////////////////////////////////////////////////////////
	//Log
	RsLogReport		m_cbLogReport;
	void*			m_pLogHandler;
	char			TAG[32];
	//////////////////////////////////////////////////////////////////////////
	//OSD
	BOOL_			m_bTimeOSD;
	BOOL_			m_bTextOSD;
	std::string		m_strTextOSD;
	//////////////////////////////////////////////////////////////////////////
	//FPS
//	DOUBLE			m_fOverTime_Encode;
//	clock_t			m_tStartTime_Encode;
	BOOL_			m_bCapturing;
	int				m_nChannelNo;
	pthread_mutex_t	m_vV4RMutex;
	pthread_cond_t 	m_vV4REvent;
	pthread_attr_t 	m_vThreadAttr;
	//////////////////////////////////////////////////////////////////////////
	RAWVIDEO_CALLBACK m_fpRawVideoCB;
	FRAME_CALLBACK	m_fpFrameCB;
	void*			m_pParamCB;

public:
	//jacky, why if i declare these interfaces as pure virtual functions and implement them in an 
	//overloaded class can not work in RELEASE version??????it's really a shit!
	//virtual void	cbVideoCallbackRaw(int ch, unsigned char *pDecBuffer, int nDecSize, int nImageWidth, int nImageHeight, int nPixelFormat, unsigned long long llTimeStamp,BOOL_ bKeyFrame) = 0;
	//virtual void	cbVideoCallbackEnc(int ch, void* pParam, unsigned char* pEncBuffer, int nEncSize,int nCodecID,int nImageWidth,int nImageHeight, int keyframe, int nFrameNumber, const char* szExtraData, unsigned long long llTimestamp) = 0;
	virtual void	cbVideoCallbackRaw(int ch, unsigned char *pDecBuffer, int nDecSize, int nImageWidth, int nImageHeight, int nPixelFormat, unsigned long long llTimeStamp,BOOL_ bKeyFrame);
	virtual void	cbVideoCallbackEnc(int ch, void* pParam, unsigned char* pEncBuffer, int nEncSize,int nCodecID,int nImageWidth,int nImageHeight, int keyframe, int nFrameNumber, const char* szExtraData, unsigned long long llTimestamp);

protected:
	static void		stcVideoEncodeCallback(int nStreamType, void* pParam, unsigned char* pEncBuffer, int nEncSize,int nCodecID,int nImageWidth,int nImageHeight, int keyframe, int nFrameNumber, const char* szExtraData, unsigned long long llTimestamp);
	//void			EncodeVideo(unsigned char *pDecBuffer, int nDecSize, int nImageWidth, int nImageHeight, int nPixelFormat, unsigned long long llTimeStamp,BOOL_ bKeyFrame);

private:
	//////////////////////////////////////////////////////////////////////////
	//Frame rate emulation
	/*
	int64_t			getutime(void);
	int32_t			usleep(uint64_t usec);
	void			bktr_getframe(uint64_t per_frame);
	*/
	typedef struct FRAMERATE_EMU
	{
		int32_t		rate_emu;
		int64_t		per_frame;
		int64_t		last_frame_time;
		int64_t		largest_pts;
	}FRAMERATE_EMU;
	FRAMERATE_EMU	m_vFrameRateEmu;

	CCodec*			m_pEnc_v;

	BOOL_			m_bProperties;
//	BOOL_			m_bAutoEncode;

	CRsCameraDS*	m_pCamera;
	int				m_nCodecID;
	int				m_nProfileLevel;
	int				m_nFrameRate;
	int				m_nBitRate;
	int				m_nGOP;
	int				m_nWidth;
	int				m_nHeight;

	pthread_t		m_tThreadCapture;
	static void*	ThreadCapture(void* param);
};

#ifdef __cplusplus
}
#endif

#endif // __VIDEO_FOR_RG4_MEDIA_SYSTEMS_H____
