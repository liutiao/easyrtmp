//////////////////////////////////////////////////////////////////////////
//EzStreamSvr
//Author: JackyHwei
//Web site: http://www.rosoo.net
//////////////////////////////////////////////////////////////////////////

#if !defined(__EZSTREAMSVR_CAPTURE_AUDIO_BY_WAVEIN_INCLUDE______)
#define __EZSTREAMSVR_CAPTURE_AUDIO_BY_WAVEIN_INCLUDE______

#include <rscommon.h>

#define EZSTREAMSVR_CAPTURE_A_API __declspec(dllexport)

EZSTREAMSVR_CAPTURE_A_API BOOL_	__stdcall EZCA2_Init();
EZSTREAMSVR_CAPTURE_A_API BOOL_	__stdcall EZCA2_Terminate();

EZSTREAMSVR_CAPTURE_A_API BOOL_	__stdcall EZCA2_CreateInstance(void** hInstance, void* pParent);
EZSTREAMSVR_CAPTURE_A_API BOOL_	__stdcall EZCA2_DestroyInstance(void* hInstance);

EZSTREAMSVR_CAPTURE_A_API BOOL_	__stdcall EZCA2_InitCapture(void* hInstance, AUDIO_PROFILE* pFormat, RAWAUDIO_CALLBACK fpCapAudioCB);

EZSTREAMSVR_CAPTURE_A_API BOOL_	__stdcall EZCA2_StartCapture(void* hInstance);
EZSTREAMSVR_CAPTURE_A_API BOOL_	__stdcall EZCA2_StopCapture(void* hInstance);

#endif
