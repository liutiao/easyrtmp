//////////////////////////////////////////////////////////////////////////
//EzStreamSvr
//Author: JackyHwei
//Web site: http://www.rosoo.net
//////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EzCaptureA.h"
#include "RsWaveIN.h"

#include <list>
using namespace std;

list<CRsWaveIN*> g_lstAudioCapture;

BOOL_ __stdcall EZCA2_Init()
{
	return TRUE_;
}

BOOL_ __stdcall EZCA2_Terminate()
{
	list<CRsWaveIN*>::iterator iter = g_lstAudioCapture.begin();
	while (iter != g_lstAudioCapture.end())
	{
		CRsWaveIN* p = *iter;
		if (p)
		{
			delete p;
			p = 0;
		}

		iter ++;
	}
	g_lstAudioCapture.clear();

	return TRUE_;
}

BOOL_ __stdcall EZCA2_CreateInstance(void** hInstance, void* pParent)
{
	CRsWaveIN* p = new CRsWaveIN(pParent);
	if (p)
	{
		*hInstance = (void*)p;
		g_lstAudioCapture.push_back(p);
		return TRUE_;
	}

	return FALSE_;
}

BOOL_ __stdcall EZCA2_DestroyInstance(void* hInstance)
{
	bool ret = FALSE_;
	CRsWaveIN* p = reinterpret_cast<CRsWaveIN*>(hInstance);
	list<CRsWaveIN*>::iterator iter = g_lstAudioCapture.begin();

	while (iter != g_lstAudioCapture.end())
	{
		if (p == *iter)
		{
			g_lstAudioCapture.remove(p);
			delete p;
			ret = TRUE_;
			break;
		}
		iter ++;
	}

	return ret;
}

BOOL_ __stdcall EZCA2_InitCapture(void* hInstance, AUDIO_PROFILE* pFormat, RAWAUDIO_CALLBACK fpCapAudioCB)
{
	CRsWaveIN* p = reinterpret_cast<CRsWaveIN*>(hInstance);

	if (p->InitCapture(pFormat, fpCapAudioCB))
		return TRUE_;

	return FALSE_;
}

BOOL_ __stdcall EZCA2_StartCapture(void* hInstance)
{
	CRsWaveIN* p = reinterpret_cast<CRsWaveIN*>(hInstance);
	if (p->StartCapture())
		return TRUE_;

	return FALSE_;
}

BOOL_ __stdcall EZCA2_StopCapture(void* hInstance)
{
	CRsWaveIN* p = reinterpret_cast<CRsWaveIN*>(hInstance);
	if (p->StopCapture())
		return TRUE_;

	return FALSE_;
}
