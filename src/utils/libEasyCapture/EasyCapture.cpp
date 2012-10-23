#include "stdafx.h"
#include "EasyCapture.h"

//#ifdef _DEBUG
//static int av_log_level = AV_LOG_INFO;
//#else
static int av_log_level = AV_LOG_WARNING;
//#endif

#ifdef _USE_WAVEIN
#include "../libRsWaveIN/EzCaptureA.h"

#ifdef _DEBUG
#pragma comment(lib, "../bin/libRsWaveINd.lib")
#pragma comment(lib, "../bin/libEasySourced.lib")
#else
#pragma comment(lib, "../bin/libRsWaveIN.lib")
#pragma comment(lib, "../bin/libEasySource.lib")
#endif
#endif

CEasyCapture::CEasyCapture()
{
	m_pAudio		= NULL;
	m_pEnc_a		= NULL;
	m_pV4R			= NULL;

#ifdef _DEBUG
//	av_log_set_callback(ddddd);
#endif
	strcpy(TAG, "EasyCapture");
}

CEasyCapture::~CEasyCapture(void)
{
	StopVideoCapture();
	StopAudioCapture();
}

BOOL CEasyCapture::Start()
{
	Stop();

	//1. Start Audio
	StartAudioCapture(m_vDeviceInfo.m_wChannelNumber);

	//2. Start Video
//	string strOSD;
//	BOOL_ bOSDText = FALSE_;
//	BOOL_ bOSDTime = FALSE_;

	return StartVideoCapture(m_vDeviceInfo.m_wChannelNumber, m_pOSDFilter, m_bOSDText, m_bOSDTime);
}

BOOL_ CEasyCapture::Stop()
{
	StopAudioCapture();
	StopVideoCapture();

	return TRUE_;
}

BOOL_ CEasyCapture::CheckReceiveStatus()
{
	return TRUE;
}

BOOL_ CEasyCapture::StartVideoCapture(int nChannel, const char* osd, BOOL_ bOSD, BOOL_ bOSDTime)
{
	StopVideoCapture();

	m_pV4R = new CV4R();
	m_pV4R->SetupVideoParam(m_vMediaProfile.vVideoProfile.nCodecID
		, (int)m_vMediaProfile.vVideoProfile.dFPS
		, m_vMediaProfile.vVideoProfile.nBitRate
		, m_vMediaProfile.vVideoProfile.nGOP
		, m_vMediaProfile.vVideoProfile.nWidth
		, m_vMediaProfile.vVideoProfile.nHeight
		, m_vMediaProfile.vVideoProfile.nLevel
		);
	m_pV4R->SetLogCallback(m_pLogParam, m_fpLogReport);

	if (!m_pV4R->StartCaptureV(nChannel, osd, bOSD, bOSDTime, cbRawVideoCB, cbFrameCB, this))
		return FALSE_;

	return TRUE_;
}

BOOL_ CEasyCapture::StopVideoCapture()
{
	if (m_pV4R)
	{
		m_pV4R->StopCaptureV();
		delete m_pV4R;
		m_pV4R = NULL;
	}

	return TRUE_;
}

void CEasyCapture::cbRawVideoCB(void* pParam, uint8_t* buf, int nBytes, int w, int h, int nPixFmt, int64_t llTimeStamp)
{
	CEasyCapture* p = (CEasyCapture*)pParam;
	if (p->m_fpRawVideoCB)
		p->m_fpRawVideoCB(pParam, buf, nBytes, w, h, nPixFmt, llTimeStamp);

	p->m_pV4R->InputVideoRaw(buf, nBytes, w, h, nPixFmt, llTimeStamp, 0);
}

void CEasyCapture::cbFrameCB(void* pParam, uint8_t* pFrame, int nBytes, int nCodecID, int vDuringAlarm, int16_t vFrameType, int bStretchMode, int w, int h, int32_t ts)
{
	CEasyCapture* p = (CEasyCapture*)pParam;
	if (p->m_fpFrameVCB)
		p->m_fpFrameVCB(pParam, pFrame, nBytes, nCodecID, vDuringAlarm, vFrameType, bStretchMode, w, h, ts);
}

//////////////////////////////////////////////////////////////////////////
//AUdio Capture
//////////////////////////////////////////////////////////////////////////
BOOL_ CEasyCapture::StartAudioCapture(int nIndex)
{
	BOOL_ bResult = FALSE_;
	if (m_fpLogReport)
		m_fpLogReport(m_pLogParam, "", LOGLEVEL0, "I", TAG, "Start AudioCapture");
#ifdef _USE_WAVEIN
	if (!m_pAudio)
	{
		EZCA2_CreateInstance(&m_pAudio, this);
	}
	if (m_pAudio)
	{
		/*
		STREAMPROFILE* pProfile			= &m_vStreamProfile;
		if (pProfile)
		{
			m_vAudioParam.nChannels		= pProfile->iChannels;
			m_vAudioParam.nBitsPerSample= pProfile->iBitsPerSample;
			m_vAudioParam.nSamplePerSec	= pProfile->vTracks[RTP_TRACK_AUDIO].nClockRate;
			m_vAudioParam.nWaveInSize	= pProfile->iBytesPerCapture;
		}
		else
		{
			m_vAudioParam.nChannels		= 1;
			m_vAudioParam.nBitsPerSample= 16;
			m_vAudioParam.nSamplePerSec	= 8000;
			m_vAudioParam.nWaveInSize	= 320;
		}
		m_vAudioParam.nChannels		= m_vMediaProfile.vAudioProfile.nChannels;
		m_vAudioParam.nBitsPerSample= m_vMediaProfile.vAudioProfile.nBitsPerSample;
		m_vAudioParam.nSamplePerSec	= m_vMediaProfile.vAudioProfile.nSampleRate;
		m_vAudioParam.nWaveInSize	= m_vMediaProfile.vAudioProfile.nBytesPerCapture;
		*/

		memcpy(&m_vAudioParam, &m_vMediaProfile.vAudioProfile, sizeof(m_vAudioParam));

		//m_pAudio->Connect(0);
		EZCA2_InitCapture(m_pAudio, &m_vAudioParam, cbAudioCallbackRaw);

		bResult = EZCA2_StartCapture(m_pAudio);
		if (bResult)
		{
			if (m_fpLogReport)
				m_fpLogReport(m_pLogParam, "", LOGLEVEL0, "I", TAG, "Start Capture Audio Successfully!");
		}
		else
		{
			if (m_fpLogReport)
				m_fpLogReport(m_pLogParam, "", LOGLEVEL0, "E", TAG, "Start Capture Audio Failed!");
		}
	}
#else
	if (!m_pAudio)
	{
		EZCA_CreateInstance(&m_pAudio, this);
	}
	if (m_pAudio)
	{
		//m_pAudio->Connect(0);
		return EZCA_StartCapture(m_pAudio, nIndex, &m_vAudioParam, stcCaptureCB, NULL);
	}
#endif

	if (bResult)
	{
//		if (m_cbEventCB)
//			m_cbEventCB(m_lpEventParam, EVENT_AUDIO_OPENED, NULL, 0, 0, 0);
	}

	return bResult;
}

void CEasyCapture::StopAudioCapture()
{
	if (m_pAudio)
	{
#ifdef _USE_WAVEIN
		EZCA2_StopCapture(m_pAudio);
		EZCA2_DestroyInstance(m_pAudio);
		EZCA2_Terminate();
#else
		EZCA_DestroyInstance(m_pAudio);
#endif
		m_pAudio = NULL;
	}
}

void CEasyCapture::cbRawAudioCB(void* pParam, uint8_t* buf, int nBytes, int channels, int bitsPerSample, int sampleRate, int nRawFmt, int64_t llTimeStamp)
{
	//RS_OUTPUT("Audio Captured Size = %d\n", nDecSize);
	CEasyCapture* p = (CEasyCapture*)pParam;

	if (!p->m_vMediaProfile.bHasAudio) return;

	if (NULL == p->m_pEnc_a)
	{
		if (p->m_vMediaProfile.vAudioProfile.nCodecID == CODEC_ID_ADPCM_G726)
			CCodec::Create("libg726", &p->m_pEnc_a);
		else if (p->m_vMediaProfile.vAudioProfile.nCodecID == CODEC_ID_PCM_ALAW)
			CCodec::Create("libg711a", &p->m_pEnc_a);
		else
			CCodec::Create("libezencoder", &p->m_pEnc_a);
		if (p->m_pEnc_a)
			p->m_pEnc_a->SetAudioEncodeCallback(p, stcAudioEncodeCallback);
	}

	if (!p->m_pEnc_a) return;
	int nResult;
	nResult = p->m_pEnc_a->AudioEncodeFrame(
		buf, 
		nBytes, 
		nRawFmt,
		channels, bitsPerSample, sampleRate, 
		p->m_vMediaProfile.vAudioProfile.nCodecID, 
		p->m_vMediaProfile.vAudioProfile.nChannels,
		p->m_vMediaProfile.vAudioProfile.nBitsPerSample, 
		p->m_vMediaProfile.vAudioProfile.nSampleRate,
		p->m_vMediaProfile.vAudioProfile.nBitRate, //m_vStreamProfile.iChannels*m_vStreamProfile.iBitsPerSample*m_vStreamProfile.iSamplePerSec,
		p->m_vMediaProfile.vAudioProfile.nProfileID, 
		llTimeStamp);

	if (nResult == CODEC_ERROR)
	{
		printf("VideoEncodeFrame error\n");
	}
}

void CEasyCapture::cbAudioCallbackRaw(void* p, uint8_t* pBuf, int nBytes, int channels, int bitsPerSample, int sampleRate, int nRawFmt, int64_t lTimestamp)
{
	cbRawAudioCB(p, pBuf, nBytes, channels, bitsPerSample, sampleRate, nRawFmt, lTimestamp);
}

void CEasyCapture::stcAudioEncodeCallback(int ch, void* pParam, unsigned char* pEncBuffer, int nEncSize, int nCodecID,int nChannel, int nBitsPerSample, int nSamplePerSec, const char* szExtraData, unsigned long long llTimestamp)
{
	CEasyCapture* pStream = (CEasyCapture*) pParam;
	if (!pStream)
		return;
	pStream->cbAudioCallbackEnc(ch, pEncBuffer, nEncSize, nCodecID, nChannel, nBitsPerSample, nSamplePerSec, szExtraData, llTimestamp);
}

void CEasyCapture::cbAudioCallbackEnc(int ch, unsigned char* pEncBuffer, int nEncSize, int nCodecID,int nChannel, int nBitsPerSample, int nSamplePerSec, const char* szExtraData, unsigned long long llTimestamp)
{
	int nBitRate = 0;
	if (m_fpFrameACB)
		m_fpFrameACB(this, pEncBuffer, nEncSize, nCodecID, nBitsPerSample, nSamplePerSec, nChannel, nBitRate, llTimestamp);
}
