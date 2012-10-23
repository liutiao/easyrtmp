#ifndef __EZ_BASIC_DEFINITION_INCLUDE___
#define __EZ_BASIC_DEFINITION_INCLUDE___

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#include <libavcodec/avcodec.h>
}
#endif


#ifndef RS_OUTPUT
#ifdef WIN32
#define RS_OUTPUT TRACE
#else
#define RS_OUTPUT printf
#endif
#endif

#ifndef BOOL_
//typedef int BOOL_;
#define BOOL_	int
#define FALSE_	0
#define TRUE_	1
#endif

enum STREAM_SOURCE_TYPE
{
	STREAM_SOURCE_TYPE_CAPTURE = 0,
	STREAM_SOURCE_TYPE_RELAY,
	STREAM_SOURCE_TYPE_DVR,
};

enum LOGLEVEL
{
	LOGLEVEL0 	= 0,
	LOGLEVEL1 	= 1,
	LOGLEVEL2	= 2,
 	LOGLEVEL3	= 3,
	LOGLEVEL4	= 4,
	LOGLEVEL5	= 5,
	LOGLEVEL6	= 6,
	LOGLEVEL7	= 7,
	LOGLEVEL8	= 8,
	LOGLEVEL9	= 9
};

typedef void (*RsLogReport)(void* pHandler, const char* szLogName, int nLogLevel, const char* szLogType, const char* szProgramID, const char* pszFormat, ...);

#define IMAGEBUFFERSIZE				512*1024
#define IMAGECODECSIZE				1920*1080*3

#define MAX_AUDIO_SIZE				192000
#define MAX_RAW_AUDIO_SIZE			192000
#define MAX_VIDEO_SIZE				256*1024
#define MAX_RAW_VIDEO_SIZE			1920*1080*3

#define MAX_RTSP_COMMAND_BUFFER		4*1024
#define MAX_RTP_BUFFER_SIZE			1400
//#define RTSPHEADERSIZE			4
//#define RTPHEADERSIZE				12

#define COMMAND_BUFFERSIZE			32*1024
//#define IMAGERECEIVESIZE			128*1024
//#define AUDIOBUFFERSIZE			32*1024
//#define AUDIORECEIVESIZE			8*1024


enum EZ_CODEC
{
	//CODEC_ID_G729		= 0x31000,
	CODEC_ID_G722		= 0x31001,
	CODEC_ID_G722_1,
	CODEC_ID_G723,
	CODEC_ID_DVI4,
	CODEC_ID_LPC,
	CODEC_ID_CN,
	CODEC_ID_G728,

	CODEC_ID_CelB		= 0x32001,
	CODEC_ID_nv,

};

// FrameType
enum FRAMETYPE
{ 
	PTYPE_INTRA			=0,
	PTYPE_INTER			=1
};

//#ifndef TYPE_DEVICE_INFO
typedef struct T_DEVICE_INFO
{
	STREAM_SOURCE_TYPE	m_nStreamType;
	uint16_t			m_wConnectionType;
	uint16_t			m_wProtocalType;
	char				m_szProductLib[32];
	uint16_t			m_wCameraType;
	uint16_t			m_wDomeType;
	uint16_t			m_wNTSCPAL;
	char				m_szDispServerIP[64];
	uint16_t			m_wDispServerPort;
	char				m_szServerIP[64];
	uint16_t			m_wServerPort;
	char				m_szCameraIP[64];
	uint16_t			m_wCameraPort;
	uint32_t			m_wDeviceID;
	uint16_t			m_wChannelNumber;
	uint16_t			m_wCompressionType;
	uint16_t			m_wImageSize;
	uint16_t			m_wRotation;
	char				m_szLocation[32];
	char				m_szNodeKey[32];
	char				m_szAccount[16];
	char				m_szPassword[16];
	char				m_szAuthUser[16];
	char				m_szAuthPass[16];
	char				m_szConnParam[256];
} DEVICE_INFO;
//#define TYPE_DEVICE_INFO
//#endif

//#ifndef TYPE_SYSTEMTIME_
typedef struct T_SYSTEMTIME {
	unsigned short wYear;
	unsigned short wMonth;
	unsigned short wDayOfWeek;
	unsigned short wDay;
	unsigned short wHour;
	unsigned short wMinute;
	unsigned short wSecond;
	unsigned short wMilliseconds;
} SYSTEMTIME_;
//#define TYPE_SYSTEMTIME_
//#endif

typedef struct T_VIDEO_PROFILE
{
	int nCodecID;
	int nBitRate;
	int nWidth;
	int nHeight;
	int nSampleRate;
	int nGOP;
	int nLevel;
//	int	iRawType;
	//for capture only
	unsigned long	hWnd;
	double	dTimestamp;
	double	dFPS;

	unsigned long	lReserved1;
	unsigned long	lReserved2;
	unsigned long	dwReserved;

}VIDEO_PROFILE;

typedef struct T_AUDIO_PROFILE
{
	int nCodecID;
	int nChannels;
	int nBitsPerSample;
	int nSampleRate;
	int nBitRate;
	int nBytesPerCapture;
	int nProfileID;//useless...
	double	dTimestamp;
	double	dFPS;
}AUDIO_PROFILE;

typedef struct T_MEDIA_PROFILE
{
	int bHasVideo;
	int bHasAudio;
	VIDEO_PROFILE vVideoProfile;
	AUDIO_PROFILE vAudioProfile;
}MEDIA_PROFILE;

typedef void (*PROFILE_CALLBACK)	(void* pParam, MEDIA_PROFILE* pMediaProfile);
typedef void (*FRAME_CALLBACK)		(void* pParam, uint8_t* pFrame, int nBytes, int nCodecID, int vDuringAlarm, int16_t vFrameType, int bStretchMode, int w, int h, int32_t ts);
typedef void (*FRAME_CALLBACK_A)	(void* pParam, uint8_t* pFrame, int nBytes, int nCodecID, int nBitsPerSample, int nSampleRate, int nChannel, int nBitRate, int32_t ts);
typedef void (*RAWVIDEO_CALLBACK)	(void* pParam, uint8_t* buf, int nBytes, int w, int h, int nPixFmt, int64_t llTimeStamp);
typedef void (*RAWAUDIO_CALLBACK)	(void* pParam, uint8_t* buf, int nBytes, int channels, int bitsPerSample, int sampleRate, int nRawFmt, int64_t llTimeStamp);
typedef void (*RTPKT_CALLBACK)		(void* pParam, uint8_t* pPkt, int nBytes, int nTCP, int nPaddingType, uint8_t* pPadding, int nPaddingSize);

typedef void (*COMMAND_CALLBACK)	(void* pParam, const char* xml, int result);
typedef void (*EVENT_CALLBACK)		(void* pParam, int channelType, int channelID, const char* eventType);

#define VIDEO_IDENTIFIER			"VIDEOXX"
#define COMMAND_IDENTIFIER			"ARGUSXX"
#define COMMAND_IDENTIFIER_Y		"ARGUSXY"
#define RECORD_IDENTIFIER			"UNISVRY"
#define CLIENT_IDENTIFIER			"CLIENTX"

typedef struct T_VIDEOHEADER
{
	char			cCommandID[8];
	char			cDuringAlarm;
	int				cVideoType;
	unsigned short	wFrameType;			//	0,1,2,3,....,0,1,2,3,.....
	unsigned long	dwFrameIndex;		//	1,2,3,4,....,N
	unsigned long	dwTotalFrameNum;	//	N
	unsigned long	dwFrameOffset;		
	unsigned long	dwFrameSize;
	unsigned short	wImageWidth;
	unsigned short	wImageHeight;
	BOOL_			bStretchMode;
	SYSTEMTIME_		stDateTime;
	//WORD		wRecordSize;
} VIDEOHEADER;

typedef struct T_VIDEOHEADER2
{
	char			cCommandID[7];
	char			cMDValue;
	char			cDuringAlarm;
	int				cVideoType;
	unsigned short	wFrameType;			//	0,1,2,3,....,0,1,2,3,.....
	unsigned long	dwFrameIndex;		//	1,2,3,4,....,N
	unsigned long	dwTotalFrameNum;	//	N
	unsigned long	dwFrameOffset;		
	unsigned long	dwFrameSize;
	unsigned short	wImageWidth;
	unsigned short	wImageHeight;
	BOOL_			bStretchMode;
	SYSTEMTIME_		stDateTime;
} VIDEOHEADER2;

// ConnectStatus
enum
{
	CONNECT_START			= 0,
	CONNECT_READY			= 1,
	CONNECT_NOT_READY		= 2,
	CONNECT_SENDING			= 3,
	CONNECT_NOT_SEND		= 4
};

#endif

