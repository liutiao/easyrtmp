// AudioCapture.cpp: implementation of the CRsWaveIN class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "RsWaveIN.h"

#include "math.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#pragma comment(lib,"Winmm")
#pragma comment(lib,"dsound.lib")
#pragma comment(lib,"dxguid.lib")

DWORD CRsWaveIN::s_dwInstance = 0;

CRsWaveIN::CRsWaveIN(void* p)
{
	m_fpCapAudioCB					= 0;
	m_pParent						= p;
	m_vAudioFormat.nBitsPerSample	= 16;
	m_vAudioFormat.nChannels		= 1;
	m_vAudioFormat.nSampleRate		= 8000;
	m_vAudioFormat.nCodecID			= 0;

//	m_pWaveHdr						= NULL;
	s_dwInstance					++;

	m_pDSoundCapture				= new CDSoundCapture();
}

CRsWaveIN::~CRsWaveIN()
{
	StopCapture();

	if (m_pDSoundCapture)
	{
		delete m_pDSoundCapture;
		m_pDSoundCapture = NULL;
	}
}

bool CRsWaveIN::InitCapture(AUDIO_PROFILE* pFormat, RAWAUDIO_CALLBACK fpCapAudioCB)
{
	memcpy(&m_vAudioFormat, pFormat, sizeof(m_vAudioFormat));
	m_fpCapAudioCB = fpCapAudioCB;

	if (!m_pDSoundCapture)
		m_pDSoundCapture = new CDSoundCapture();

	if (!m_pDSoundCapture->Init())
		return false;

	m_pDSoundCapture->SetCallbackProc(stcWAVCALLBACK, this);

	return true;
}

bool CRsWaveIN::StartCapture()
{
	bool bRet=false;

	if (!m_pDSoundCapture)
		return false;

	bRet = m_pDSoundCapture->Start(m_vAudioFormat.nSampleRate, m_vAudioFormat.nChannels, m_vAudioFormat.nBitsPerSample
		, m_vAudioFormat.nBytesPerCapture);

	return bRet;
}

bool CRsWaveIN::StopCapture()
{
	if (!m_pDSoundCapture) return true;

	m_pDSoundCapture->Stop();

	return true;
}

BOOL CRsWaveIN::stcWAVCALLBACK(LPVOID lpParent,LPVOID lpUser,LPBYTE pBuffer,int nBytes)
{
	CRsWaveIN* pParent = (CRsWaveIN*)lpParent;

	//(void* p, char* pBuf, int nSize, unsigned long lTimestamp, RAWAUDIOINFO* pFormat)
	if (pParent->m_fpCapAudioCB)
	{
		DWORD dwTime = timeGetTime();
		dwTime = clock();

		//void (*RAWAUDIO_CALLBACK)	(void* pParam, uint8_t* buf, int nBytes, int channels, int bitsPerSample, int sampleRate
		//, int nRawFmt, int64_t llTimeStamp);
		pParent->m_fpCapAudioCB(pParent->m_pParent, pBuffer, nBytes
			, pParent->m_vAudioFormat.nChannels
			, pParent->m_vAudioFormat.nBitsPerSample
			, pParent->m_vAudioFormat.nSampleRate
			, pParent->m_vAudioFormat.nCodecID
			, dwTime);
		//pParent->m_fpCapAudioCB(pParent->m_pParent, (char*)pBuffer, nBytes, dwTime, &pParent->m_vAudioFormat);
	}

	return TRUE;
}