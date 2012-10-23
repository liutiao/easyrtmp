// AudioCapture.h: interface for the CRsWaveIN class.
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_WAVEIN_H__01416D75_968C_4FAF_96B0_346799F0FB3A__INCLUDED_)
#define AFX_WAVEIN_H__01416D75_968C_4FAF_96B0_346799F0FB3A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <rscommon.h>

#include <mmsystem.h>
#include <dsound.h>

#include "DSoundCapture.h"

class CRsWaveIN  
{
public:
	CRsWaveIN(void* p);
	virtual ~CRsWaveIN();

public:
	bool InitCapture(AUDIO_PROFILE* pFormat, RAWAUDIO_CALLBACK fpCapAudioCB);

	bool StartCapture();
	bool StopCapture();
	
protected:
	static DWORD		s_dwInstance;
	RAWAUDIO_CALLBACK	m_fpCapAudioCB;
	void*				m_pParent;
	AUDIO_PROFILE		m_vAudioFormat;
//	WAVEHDR*			m_pWaveHdr;

	
private:
	static BOOL		stcWAVCALLBACK(LPVOID lpParent,LPVOID lpUser,LPBYTE pBuffer,int nBytes);
	CDSoundCapture*	m_pDSoundCapture;
};

#endif // !defined(AFX_WAVEIN_H__01416D75_968C_4FAF_96B0_346799F0FB3A__INCLUDED_)
