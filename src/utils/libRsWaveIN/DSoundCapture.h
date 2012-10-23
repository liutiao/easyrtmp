
#if !defined(AFX_DSOUNDCAPTURE_H__7D49D10C_BC3E_4130_A4BE_5436A7CE136F__INCLUDED_)
#define AFX_DSOUNDCAPTURE_H__7D49D10C_BC3E_4130_A4BE_5436A7CE136F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <mmsystem.h>
#include <dsound.h>

#define MAX(a,b)        ( (a) > (b) ? (a) : (b) )

#define SAFE_FREE(p)	{ if(p) { free(p);     (p)=NULL; } }
#define SAFE_RELEASE(p){if (p){(p)->Release(); (p) = NULL;}}

enum AUDIO_CAPTURE_ERROR
{
	ERROR_MEDIA_AUDIO_THREAD = 1000,
	ERROR_MEDIA_AUDIO_DEVICE,
	ERROR_MEDIA_AUDIO_BUFFER,
	ERROR_MEDIA_AUDIO_CAPTURE,
	ERROR_MEDIA_AUDIO_COM,
};

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dsound.lib")

typedef BOOL (*WAVCALLBACK)(LPVOID,LPVOID,LPBYTE,int);

class CDSoundCapture  
{
public:
	CDSoundCapture();
	virtual ~CDSoundCapture();

protected:
	bool m_bRecording;
	int m_error;
	HANDLE m_hThread;
	DWORD m_dwThreadID;
	WAVCALLBACK m_proc;
	int m_nNotifyNumber;
public:
	void* g_pDSCapture ;
	void* g_pDSBCapture ;
	void* g_pDSNotify ;
	//WAVEFORMATEX g_wfxInput;
	void* g_aPosNotify;
	HANDLE g_hNotificationEvent;  
	DWORD g_dwCaptureBufferSize;
	DWORD g_dwNextCaptureOffset;
	DWORD g_dwNotifySize;
public:
	static DWORD WINAPI ThreadRecord(LPVOID);
	static DWORD WINAPI ThreadStop(LPVOID);
private:
	HRESULT InitDirectSound(GUID* pDeviceGuid );
	HRESULT FreeDirectSound();
	HRESULT InitNotifications();
	HRESULT RecordCapturedData();
	HRESULT CreateCaptureBuffer( int nSamplesPerSec,int nChannels,int wBitsPerSample, int dwNotifySize);
public:
	LPVOID m_parent;
	bool m_del;
	int GetLastError();
	bool IsRecording();
	bool Start(int nSamplesPerSec,int nChannels,int wBitsPerSample, int dwNotifySize);
	void Stop( BOOL KillThread = FALSE,BOOL Delay = FALSE);
	bool Init(int nNotifyNum = 50);
	void Exit();
	WAVCALLBACK SetCallbackProc(WAVCALLBACK proc,LPVOID parent);
	WAVCALLBACK GetCallbackProc();

	//jacky, move to class member for callback data;
	PBYTE data;
};

#endif // !defined(AFX_DSOUNDCAPTURE_H__7D49D10C_BC3E_4130_A4BE_5436A7CE136F__INCLUDED_)
