// V4R.cpp: implementation of the CV4R class.
//
//////////////////////////////////////////////////////////////////////

//#include "stdafx.h"
//#include "libmp4v2test.h"
#include "RsCameraDS.h"
#include "V4R.h"

//#ifdef _DEBUG
//#pragma comment(lib, "../../../bin_debug/libEzCodec.lib")
//#else
//#pragma comment(lib, "../../../bin/libEzCodec.lib")
//#endif

#ifdef WIN32
#pragma comment(lib, "pthreadVC2.lib")
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "swscale.lib")
#endif

CV4R::CV4R() : m_pLogHandler(NULL), m_pCamera(NULL), m_pEnc_v(NULL), m_cbLogReport(NULL)
{
	m_nCodecID		= CODEC_ID_H264;
	m_bProperties	= FALSE_;
	m_bCapturing	= FALSE_;

	strcpy(TAG, "V4R");

	m_fpRawVideoCB	= NULL;
	m_fpFrameCB		= NULL;
	m_pParamCB		= NULL;
	pthread_mutex_init(&m_vV4RMutex, NULL);
	pthread_cond_init(&m_vV4REvent, NULL);
	int joinable 		= PTHREAD_CREATE_DETACHED;
	int s = pthread_attr_init(&m_vThreadAttr);
	if (s != 0)
	{
		//printf("1111 adfasdf\n");
		//handle_error_en(s, "pthread_attr_init");
	}
	s = pthread_attr_setdetachstate(&m_vThreadAttr, joinable);
	if (s != 0)
	{
		//printf("pthread_attr_setdetachstate failed:%s\n", strerror(s));
		//handle_error_en(s, "pthread_attr_setstacksize");
	}
}

CV4R::~CV4R()
{
	StopCaptureV();

	pthread_mutex_destroy(&m_vV4RMutex);
	pthread_cond_destroy(&m_vV4REvent);
	pthread_attr_destroy(&m_vThreadAttr);
}

BOOL_ CV4R::SetupVideoParam(int nCodec, int nFrameRate, int nBitRate, int nGOP, int nWidth, int nHeight, int nLevel)
{
	m_nCodecID		= nCodec;
	m_nFrameRate	= nFrameRate;
	m_nBitRate		= nBitRate;
	m_nGOP			= nGOP;
	m_nWidth		= nWidth;
	m_nHeight		= nHeight;
	m_nProfileLevel	= nLevel;

	return TRUE_;
}

void CV4R::SetLogCallback(void* pParent, RsLogReport stcLogReport)
{
	m_pLogHandler	= pParent;
	m_cbLogReport	= stcLogReport;
}

BOOL_ CV4R::StartCaptureV(int nChannelNo, const char* osd, BOOL_ bOSD, BOOL_ bOSDTime, RAWVIDEO_CALLBACK fpRawVideoCB, FRAME_CALLBACK fpFrameCB, void* pParam)
{
	BOOL_ ret;
	m_pCamera = new CRsCameraDS();
	ret = m_pCamera->OpenCamera(nChannelNo, m_nWidth, m_nHeight, m_nFrameRate, m_bProperties);
	if (!ret )
	{
		if (m_cbLogReport)
			m_cbLogReport(m_pLogHandler, "", 0, "E", TAG, "Capture Camera %d Failed(%d:%d)", m_nWidth, m_nHeight);
		return FALSE_; 
	}

	m_bCapturing				= TRUE_;

	int res = pthread_create(&m_tThreadCapture, &m_vThreadAttr, ThreadCapture, this);
	if (res == 0)
		ret = TRUE_;

	if (m_cbLogReport)
		m_cbLogReport(m_pLogHandler, "", LOGLEVEL0, "I", TAG,"Video Capture thread started successfully(FPS=%d)!", m_nFrameRate);

	memset(&m_vFrameRateEmu, 0, sizeof(FRAMERATE_EMU));
	m_vFrameRateEmu.rate_emu	= 1;
	m_vFrameRateEmu.per_frame	= 1000000000LL/m_nFrameRate;
	m_nChannelNo				= nChannelNo;
	m_bTextOSD					= bOSD;
	m_bTimeOSD					= bOSDTime;
	m_fpRawVideoCB				= fpRawVideoCB;
	m_fpFrameCB					= fpFrameCB;
	m_pParamCB					= pParam;
	if (osd)
		m_strTextOSD			= osd;
	else
		m_strTextOSD			= "RG4 Media Systems - RG4.NET";

	return ret;
}

BOOL_ CV4R::StopCaptureV()
{
	m_bCapturing = FALSE_;

	pthread_cond_signal(&m_vV4REvent);

	Sleep(200);

//	pthread_cancel(m_tThreadCapture);

	if (m_pCamera)
	{
		m_pCamera->CloseCamera();

		//void* ret = NULL;
		//pthread_join(m_tThreadCapture, &ret);

		delete m_pCamera;
		m_pCamera = NULL;
	}

	if (m_cbLogReport)
		m_cbLogReport(m_pLogHandler, "", LOGLEVEL0, "I", TAG,"Capture thread stopped!");

	if (m_pEnc_v)
	{
		delete m_pEnc_v;
		m_pEnc_v = NULL;
	}

	return TRUE_;
}
/*
int64_t CV4R::getutime(void)
{
#if HAVE_GETRUSAGE
	struct rusage rusage;

	getrusage(RUSAGE_SELF, &rusage);
	return (rusage.ru_utime.tv_sec * 1000000LL) + rusage.ru_utime.tv_usec;
#elif HAVE_GETPROCESSTIMES
	HANDLE proc;
	FILETIME c, e, k, u;
	proc = GetCurrentProcess();
	GetProcessTimes(proc, &c, &e, &k, &u);
	return ((int64_t) u.dwHighDateTime << 32 | u.dwLowDateTime) / 10;
#else
	return av_gettime();
#endif
}

int32_t CV4R::usleep(uint64_t usec)
{
	int32_t n = 1;
#if 0
	struct timeval tv;
	tv.tv_sec = (long)(usec / 1000000);
	tv.tv_usec= (long)(usec - tv.tv_sec * 1000000);
	struct fd_set fd;
	FD_ZERO(&fd);
	FD_SET(0, &fd);
	n = select(1, &fd, NULL, NULL, &tv);
#else
//	Sleep(((usec/1000)/5)*5);
	if (usec > 1000000)
		usec = 1000000;
	DWORD dwSleep = (DWORD)(usec/1000);
	Sleep(dwSleep);
//	OutputDebugStr("Sleep = %d\n", dwSleep);
#endif

	return n;
}

void CV4R::bktr_getframe(uint64_t per_frame)
{
	uint64_t curtime;

	curtime = av_gettime();
	if (!m_vFrameRateEmu.last_frame_time || ((m_vFrameRateEmu.last_frame_time + per_frame) > curtime))
	{
			if (!usleep(m_vFrameRateEmu.last_frame_time + per_frame + per_frame / 8 - curtime))
			{
				//if (!nsignals)
				//	TRACE("SLEPT NO signals - %d microseconds late\n", (int)(av_gettime() - m_vFrameRateEmu.last_frame_time - per_frame));
			}
	}
	//nsignals = 0;
	m_vFrameRateEmu.last_frame_time = curtime;
}
*/

void* CV4R::ThreadCapture(void* param)
{
	CV4R* p	= (CV4R*)param;
	unsigned char* pFrame;

	int w			= p->m_pCamera->GetWidth();		//m_vStreamProfile.wImageWidth;
	int h			= p->m_pCamera->GetHeight();	//m_vStreamProfile.wImageHeight;
	p->m_nWidth		= w;
	p->m_nHeight	= h;
	int s			= w*h*3;

	// frame rate emulation
	int64_t start	= av_gettime();

	timespec vTime,vLastTime;
	vTime.tv_sec = (long)time(NULL);
	vTime.tv_nsec = (long)p->m_vFrameRateEmu.per_frame;
	vLastTime.tv_sec = (long)time(NULL);
	vLastTime.tv_nsec = (long)p->m_vFrameRateEmu.per_frame;

	if (p->m_cbLogReport)
		p->m_cbLogReport(p->m_pLogHandler, "", LOGLEVEL0, "I", p->TAG, "Capture Thread Started!");

	while (1)
	{
		if (!p->m_bCapturing)
			break;

		//FIXME jacky, there maybe time span issues here
		if (time(NULL) != vLastTime.tv_sec)
		{
			vTime.tv_sec	= (long)time(NULL);
			if (vTime.tv_nsec + (long)p->m_vFrameRateEmu.per_frame >= 1000000000LL)
			{
				vTime.tv_nsec	= vTime.tv_nsec + (long)p->m_vFrameRateEmu.per_frame - 1000000000LL;
				vTime.tv_sec += 1;
			}
			else
				vTime.tv_nsec	= vTime.tv_nsec + (long)p->m_vFrameRateEmu.per_frame;
		}
		else
		{
			vTime.tv_sec	= (long)time(NULL);
			if (vTime.tv_nsec + p->m_vFrameRateEmu.per_frame >= 1000000000LL)
			{
				vTime.tv_nsec	= vTime.tv_nsec + (long)p->m_vFrameRateEmu.per_frame - 1000000000LL;
				vTime.tv_sec += 1;
			}
			else
				vTime.tv_nsec	= vTime.tv_nsec + (long)p->m_vFrameRateEmu.per_frame;
		}


		pthread_mutex_lock(&p->m_vV4RMutex);
		int nErr = pthread_cond_timedwait(&p->m_vV4REvent, &p->m_vV4RMutex, &vTime);
		if (nErr == ETIMEDOUT)
		{
			pFrame = p->m_pCamera->QueryFrame();

			if (p->m_cbLogReport)
				p->m_cbLogReport(p->m_pLogHandler, "", LOGLEVEL6, "I", p->TAG, "Capture Video Callback!");

			//p->cbVideoCallbackRaw(0, pFrame, s, w, h, PIX_FMT_BGR24, GetTickCount(), 1);
			if (pFrame)
				p->cbVideoCallbackRaw(0, pFrame, s, w, h, PIX_FMT_BGR24, clock(), 1);

			pthread_mutex_unlock(&p->m_vV4RMutex);
			vLastTime.tv_sec	= vTime.tv_sec;
			vLastTime.tv_nsec	= vTime.tv_nsec;
			continue;
		}
		pthread_mutex_unlock(&p->m_vV4RMutex);

		if (p->m_cbLogReport)
			p->m_cbLogReport(p->m_pLogHandler, "", LOGLEVEL3, "I", p->TAG, "Capture Thread Terminating!");

		break;
	}

	if (p->m_cbLogReport)
		p->m_cbLogReport(p->m_pLogHandler, "", LOGLEVEL0, "I", p->TAG, "Capture Thread Terminated!");

	pthread_detach(pthread_self());

	return NULL;
}

void CV4R::InputVideoRaw(unsigned char *pDecBuffer, int nDecSize, int nImageWidth, int nImageHeight, int nPixelFormat, unsigned long long llTimeStamp,BOOL_ bKeyFrame)
{
	if (m_cbLogReport)
		m_cbLogReport(m_pLogHandler, "", LOGLEVEL4, "I", TAG,"Input video %d x %d , %d\n", nImageWidth, nImageHeight, nDecSize);

	//FPS control
//	clock_t finish_encode = clock();
//	double duration_encode = fabs((double)(finish_encode - m_tStartTime_Encode) / CLOCKS_PER_SEC) + m_fOverTime_Encode;
//	double fSecondsPerFrame_encode = 1.0 / double(m_nFrameRate);

//	if (duration_encode >= fSecondsPerFrame_encode)
//	if (m_bAutoEncode)
	{
		if (NULL == m_pEnc_v)
		{
			if (m_cbLogReport)
				m_cbLogReport(m_pLogHandler, "", LOGLEVEL4, "I", TAG, "Opening Video Encoder Codec...");
			CCodec::Create("libezencoder", &m_pEnc_v);
			if (m_pEnc_v)
			{
				m_pEnc_v->SetLogReport(m_pLogHandler, m_cbLogReport);
				m_pEnc_v->SetVideoEncodeCallback(this, CV4R::stcVideoEncodeCallback);
				m_pEnc_v->SetOSDParam(m_bTextOSD, m_bTimeOSD, m_strTextOSD.c_str());
			}
		}

		if (!m_pEnc_v) return;
		if (!m_bCapturing) return;

		int nResult;
		nResult = m_pEnc_v->VideoEncodeFrame(
			pDecBuffer, 
			nDecSize, 
			nPixelFormat, 
			nImageWidth, 
			nImageHeight,
			m_nCodecID, 
			m_nWidth,
			m_nHeight, 
			m_nProfileLevel,
			m_nBitRate, 
			m_nFrameRate, 
			m_nGOP, 
			m_nChannelNo, 
			llTimeStamp);

		if (nResult == CODEC_ERROR)
		{
			if (m_cbLogReport)
				m_cbLogReport(m_pLogHandler, "", LOGLEVEL4, "I", TAG, "VideoEncodeFrame failed\n");
		}
//		m_fOverTime_Encode = duration_encode - fSecondsPerFrame_encode;
//		if (m_fOverTime_Encode >= fSecondsPerFrame_encode)
//			m_fOverTime_Encode = fSecondsPerFrame_encode;
//		m_tStartTime_Encode = clock();
	}
}

void CV4R::stcVideoEncodeCallback(int nStreamType, void* pParam, unsigned char* pEncBuffer, int nEncSize,
										   int nCodecID,int nImageWidth,int nImageHeight, int keyframe, int nFrameNumber, 
										   const char* szExtraData, unsigned long long llTimestamp)
{
	CV4R* p = (CV4R*)pParam;

	//p->RTMPSendV(keyframe, pEncBuffer, nEncSize);
	p->cbVideoCallbackEnc(nStreamType, pParam, pEncBuffer, nEncSize, nCodecID, nImageWidth, nImageHeight, keyframe, nFrameNumber, szExtraData, llTimestamp);
}

void CV4R::cbVideoCallbackRaw(int ch, unsigned char *pDecBuffer, int nDecSize, int nImageWidth, int nImageHeight, int nPixelFormat, unsigned long long llTimeStamp,BOOL_ bKeyFrame)
{
	//RS_OUTPUT("cbVideoCallbackRaw\n");
	if (m_fpRawVideoCB)
		m_fpRawVideoCB(m_pParamCB, pDecBuffer, nDecSize, nImageWidth, nImageHeight, nPixelFormat, (int)llTimeStamp);
}

void CV4R::cbVideoCallbackEnc(int ch, void* pParam, unsigned char* pEncBuffer, int nEncSize,int nCodecID,int nImageWidth,int nImageHeight, int keyframe, int nFrameNumber, const char* szExtraData, unsigned long long llTimestamp)
{
	//RS_OUTPUT("cbVideoCallbackEnc\n");
	if (m_fpFrameCB)
		m_fpFrameCB(m_pParamCB, pEncBuffer, nEncSize, nCodecID, 0, keyframe==1, 0, nImageWidth, nImageHeight, (int)llTimestamp);
}
