// DSoundCapture.cpp: implementation of the CDSoundCapture class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DSoundCapture.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


HRESULT CDSoundCapture::InitDirectSound(GUID* pDeviceGuid )
{
	HRESULT hr;
	
	ZeroMemory( g_aPosNotify, sizeof(DSBPOSITIONNOTIFY) *  (this->m_nNotifyNumber + 1) );
	g_dwCaptureBufferSize = 0;
	g_dwNotifySize = 0;

	// Create IDirectSoundCapture using the preferred capture device
	if(pDeviceGuid)
		hr = DirectSoundCaptureCreate( pDeviceGuid, (LPDIRECTSOUNDCAPTURE*)&g_pDSCapture, NULL ) ;
	else
		hr = DirectSoundCaptureCreate( &DSDEVID_DefaultCapture, (LPDIRECTSOUNDCAPTURE*)&g_pDSCapture, NULL ) ;
	
	if( FAILED( hr ) )
	{
		this->m_error= ERROR_MEDIA_AUDIO_COM;
		return hr;
	}
	
	return S_OK;
}

HRESULT CDSoundCapture::FreeDirectSound()
{
	// Release DirectSound interfaces
	LPDIRECTSOUNDNOTIFY notify = (LPDIRECTSOUNDNOTIFY)g_pDSNotify;
	SAFE_RELEASE( notify );
	g_pDSNotify = 0;
	LPDIRECTSOUNDCAPTUREBUFFER dsb = (LPDIRECTSOUNDCAPTUREBUFFER)g_pDSBCapture;
	SAFE_RELEASE( dsb );
	g_pDSBCapture = 0;
	LPDIRECTSOUNDCAPTURE dsc = (LPDIRECTSOUNDCAPTURE)g_pDSCapture;
	SAFE_RELEASE( dsc );  
	g_pDSCapture = 0;

	return S_OK;
}


HRESULT CDSoundCapture::InitNotifications()
{
	HRESULT hr;  
	
	if( NULL == g_pDSBCapture )
	{
		this->m_error = ERROR_MEDIA_AUDIO_COM;
		return E_FAIL;
	}
	LPDIRECTSOUNDCAPTUREBUFFER capture = (LPDIRECTSOUNDCAPTUREBUFFER)g_pDSBCapture;
	
	// Create a notification event, for when the sound stops playing
	if( FAILED( hr = capture->QueryInterface( IID_IDirectSoundNotify,  
		(VOID**)&g_pDSNotify ) ) )
	{
		this->m_error = ERROR_MEDIA_AUDIO_COM;
		return hr;
	}
	
	// Setup the notification positions
	DSBPOSITIONNOTIFY * ar_notify = (DSBPOSITIONNOTIFY*)g_aPosNotify;
	for( INT i = 0; i < this->m_nNotifyNumber; i++ )
	{
		ar_notify[i].dwOffset = (g_dwNotifySize * i) + g_dwNotifySize - 1;
		ar_notify[i].hEventNotify = g_hNotificationEvent;   
	}
    
	// Tell DirectSound when to notify us. the notification will come in the from  
	// of signaled events that are handled in WinMain()
	LPDIRECTSOUNDNOTIFY notify = (LPDIRECTSOUNDNOTIFY)g_pDSNotify;
	if( FAILED( hr = notify->SetNotificationPositions( this->m_nNotifyNumber,  
		ar_notify ) ) )
	{
		this->m_error = ERROR_MEDIA_AUDIO_COM;
		return hr;
	}
	
	return S_OK;
}

HRESULT CDSoundCapture::RecordCapturedData()  
{
	HRESULT hr;
	VOID* pbCaptureData = NULL;
	DWORD dwCaptureLength;
	VOID* pbCaptureData2 = NULL;
	DWORD dwCaptureLength2;
	DWORD dwReadPos;
	DWORD dwCapturePos;
	LONG lLockSize;
	
	if( NULL == g_pDSBCapture )
	{
		this->m_error = ERROR_MEDIA_AUDIO_CAPTURE;
		return S_FALSE;
	}
	LPDIRECTSOUNDCAPTUREBUFFER capture = (LPDIRECTSOUNDCAPTUREBUFFER)g_pDSBCapture;
	
	if( FAILED( hr = capture->GetCurrentPosition( &dwCapturePos, &dwReadPos ) ) )
	{
		this->m_error = ERROR_MEDIA_AUDIO_CAPTURE;
		return hr;
	}
	
	lLockSize = dwReadPos - g_dwNextCaptureOffset;
	if( lLockSize < 0 )
		lLockSize += g_dwCaptureBufferSize;
	
	// Block align lock size so that we are always write on a boundary
	lLockSize -= (lLockSize % g_dwNotifySize);
	
	if( lLockSize == 0 )
	{
		this->m_error = ERROR_MEDIA_AUDIO_CAPTURE;
		return S_FALSE;
	}
	
	// Lock the capture buffer down
	if( FAILED( hr = capture->Lock( g_dwNextCaptureOffset, lLockSize,  
		&pbCaptureData, &dwCaptureLength,  
		&pbCaptureData2, &dwCaptureLength2, 0L ) ) )
	{
		this->m_error = ERROR_MEDIA_AUDIO_CAPTURE;
		return hr;
	}
	
	int totallen = dwCaptureLength+dwCaptureLength2;
	
	//jacky, move to class member data
	//PBYTE data= new BYTE[totallen];
	if (!data) data = new BYTE[192000];
	
	memcpy(data,pbCaptureData,dwCaptureLength);
	
	// Move the capture offset along
	g_dwNextCaptureOffset += dwCaptureLength;  
	g_dwNextCaptureOffset %= g_dwCaptureBufferSize; // Circular buffer
	
	if( pbCaptureData2 != NULL )
	{
		memcpy(data+dwCaptureLength,pbCaptureData2,dwCaptureLength2);
		
		// Move the capture offset along
		g_dwNextCaptureOffset += dwCaptureLength2;  
		g_dwNextCaptureOffset %= g_dwCaptureBufferSize; // Circular buffer
	}

	// Unlock the capture buffer
	capture->Unlock( pbCaptureData, dwCaptureLength, pbCaptureData2, dwCaptureLength2 );
	
	if(this->m_proc)
	{
		this->m_proc(this->m_parent,this,data,totallen);
	}

	//jacky, move to class member data
	//if(data)
	//	delete data;

	return S_OK;
}

WAVCALLBACK CDSoundCapture::SetCallbackProc(WAVCALLBACK proc,LPVOID parent)
{
	WAVCALLBACK backup = this->m_proc;
	this->m_parent = parent;
	this->m_proc = proc;
	return backup;
}

WAVCALLBACK CDSoundCapture::GetCallbackProc()
{
	return this->m_proc;
}

HRESULT CDSoundCapture::CreateCaptureBuffer( int nSamplesPerSec,int nChannels,int wBitsPerSample, int dwNotifySize)
{
	WAVEFORMATEX wfx;
	ZeroMemory( &wfx, sizeof(WAVEFORMATEX) );  
	wfx.wFormatTag = (WORD) WAVE_FORMAT_PCM;  
	wfx.nChannels = nChannels;  
	wfx.nSamplesPerSec = nSamplesPerSec;  
	wfx.wBitsPerSample = wBitsPerSample;  
	wfx.nBlockAlign = (WORD) (wfx.wBitsPerSample / 8 * wfx.nChannels);
	wfx.nAvgBytesPerSec = (DWORD) (wfx.nSamplesPerSec * wfx.nBlockAlign);
	
	WAVEFORMATEX * pwfxInput = &wfx;
	
	HRESULT hr;
	DSCBUFFERDESC dscbd;
	
	LPDIRECTSOUNDNOTIFY notify = (LPDIRECTSOUNDNOTIFY)g_pDSNotify;
	SAFE_RELEASE( notify);
	g_pDSNotify = 0;
	
	LPDIRECTSOUNDCAPTUREBUFFER dsb = (LPDIRECTSOUNDCAPTUREBUFFER)g_pDSBCapture;
	SAFE_RELEASE( dsb );
	g_pDSBCapture = 0;
	
	//////////////////////////////////////////////////////////////////////////
	// Set the notification size
	//jacky, my g.729 codec can only encode 160 bytes PCM data, so change it to MAX as 160 bytes
	//g_dwNotifySize = max( 1024, pwfxInput->nAvgBytesPerSec / 8 );
#if 0
	g_dwNotifySize = max( 160, pwfxInput->nAvgBytesPerSec / 100 );
#else
	g_dwNotifySize = max( 160, dwNotifySize);
#endif
	g_dwNotifySize -= g_dwNotifySize % pwfxInput->nBlockAlign;   
	
	// Set the buffer sizes  
	g_dwCaptureBufferSize = g_dwNotifySize * this->m_nNotifyNumber;
	
	// Create the capture buffer
	ZeroMemory( &dscbd, sizeof(dscbd) );
	dscbd.dwSize = sizeof(dscbd);
	dscbd.dwBufferBytes = g_dwCaptureBufferSize;
	dscbd.lpwfxFormat = pwfxInput; // Set the format during creatation
	if(g_pDSCapture==NULL)
	{
		this->m_error = ERROR_MEDIA_AUDIO_COM;
		return S_FALSE;
	}
	LPDIRECTSOUNDCAPTURE capture = (LPDIRECTSOUNDCAPTURE )g_pDSCapture;
	if( FAILED( hr = capture->CreateCaptureBuffer( &dscbd,  
		(LPDIRECTSOUNDCAPTUREBUFFER*)&g_pDSBCapture,  
		NULL ) ) )
	{
		this->m_error = ERROR_MEDIA_AUDIO_BUFFER;
		return hr;
	}
	
	g_dwNextCaptureOffset = 0;
	
	if( FAILED( hr = InitNotifications() ) )
		return hr;
	
	return S_OK;
}

CDSoundCapture::CDSoundCapture(void)
{
	//jacky
	data = NULL;

	g_hNotificationEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
	g_pDSCapture = NULL;
	g_pDSBCapture = NULL;
	g_pDSNotify = NULL;
	g_aPosNotify = NULL;
	m_hThread = NULL;
	m_dwThreadID = 0;
	m_bRecording = false;
	m_nNotifyNumber = 1;
	m_del = false;
	m_proc = NULL;
	m_error = 0;
}

CDSoundCapture::~CDSoundCapture(void)
{
	Exit();
	m_del = true;
	if (data)
	{
		delete[] data;
		data = NULL;
	}
}

void CDSoundCapture::Exit()
{
	Stop(TRUE);
	FreeDirectSound();
	if(g_aPosNotify)
	{
		delete g_aPosNotify;
		g_aPosNotify = NULL;
	}
	if(g_hNotificationEvent!=NULL)
	{
		CloseHandle(g_hNotificationEvent);
		g_hNotificationEvent = NULL;
	}
}

DWORD WINAPI CDSoundCapture::ThreadStop(LPVOID p)
{
	Sleep(200);
	CDSoundCapture* pParent = (CDSoundCapture*)p;
	LPDIRECTSOUNDCAPTUREBUFFER dsb = (LPDIRECTSOUNDCAPTUREBUFFER)pParent->g_pDSBCapture;

	if( !pParent->m_del && pParent->g_pDSBCapture && !FAILED( dsb->Stop() ) )
		pParent->RecordCapturedData();
	return 0;
}


DWORD WINAPI CDSoundCapture::ThreadRecord(LPVOID p)
{
	bool bDone = false;
	CDSoundCapture* pParent = (CDSoundCapture*)p;
	
	MSG msg;
	while(!bDone)
	{
		DWORD dwResult = MsgWaitForMultipleObjects( 1, &pParent->g_hNotificationEvent,FALSE, INFINITE, QS_ALLEVENTS );
		switch( dwResult )
		{
		case WAIT_OBJECT_0 + 0:
			if(FAILED(pParent->RecordCapturedData()))
			{
				if(pParent->GetCallbackProc()!=NULL)
					pParent->GetCallbackProc()(pParent->m_parent,NULL,NULL,NULL);
				bDone = true;
			}
			break;
		case WAIT_OBJECT_0 + 1:
			while( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
			{
				if( msg.message == WM_QUIT )
				{
					return 0;
				}
			}
			break;
		}
	}
	pParent->Stop(TRUE);
	return 0;
}

bool CDSoundCapture::Init(int nNotifyNum)
{
	this->m_nNotifyNumber = nNotifyNum;
	if(this->g_aPosNotify)
		delete this->g_aPosNotify;

	g_aPosNotify = new DSBPOSITIONNOTIFY[this->m_nNotifyNumber+1];
	return !FAILED(this->InitDirectSound(NULL));
}

bool CDSoundCapture::Start(int nSamplesPerSec,int nChannels,int wBitsPerSample, int dwNotifySize)
{
	bool bReturn = false;
	try
	{
		if(FAILED(this->CreateCaptureBuffer(nSamplesPerSec,nChannels,wBitsPerSample, dwNotifySize)))
			throw 1;
		if(this->m_hThread==NULL)
		{
			this->m_hThread = ::CreateThread(NULL,0,CDSoundCapture::ThreadRecord,(LPVOID)this,0,&this->m_dwThreadID);
			if(this->m_hThread==NULL)
			{
				this->m_error = ERROR_MEDIA_AUDIO_THREAD;
				throw 1;
			}
		}
		LPDIRECTSOUNDCAPTUREBUFFER dsb = (LPDIRECTSOUNDCAPTUREBUFFER)g_pDSBCapture;
		if(FAILED(dsb->Start(DSCBSTART_LOOPING)))
		{
			this->m_error = ERROR_MEDIA_AUDIO_DEVICE;
			throw 1;
		}
		this->m_bRecording = true;
		bReturn = true;
	}
	catch(int)
	{
		this->m_bRecording = false;
	}
	return bReturn;
}

int CDSoundCapture::GetLastError(){return m_error;}

bool CDSoundCapture::IsRecording(){return m_bRecording;}

void CDSoundCapture::Stop(BOOL KillThread/*=FALSE*/,BOOL Delay/*=FALSE*/)
{
	this->m_bRecording = false;
	if(KillThread)
	{
		if(this->m_hThread!=NULL)
		{
			PostThreadMessage( this->m_dwThreadID, WM_QUIT, 0, 0 );
			WaitForSingleObject( this->m_hThread, INFINITE );
			CloseHandle( this->m_hThread );
			this->m_hThread = NULL;
		}
	}
	if(!Delay)
	{
		if( NULL == g_pDSBCapture )
			return;
		LPDIRECTSOUNDCAPTUREBUFFER dsb = (LPDIRECTSOUNDCAPTUREBUFFER)g_pDSBCapture;
		if( FAILED( dsb->Stop() ) )
			return;
		RecordCapturedData();
	}
	else
		CreateThread(NULL,0,CDSoundCapture::ThreadStop,(LPVOID)this,0,NULL);
}

