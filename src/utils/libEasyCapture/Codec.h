#ifndef _____________EZSTREAMSVR_CODEC_INCLUDE
#define _____________EZSTREAMSVR_CODEC_INCLUDE

//////////////////////////////////////////////////////////////////////////
//jacky, there is a critical bug in current implements
//CEzEncoder::m_pPicture can not be freed while using avfilter
//////////////////////////////////////////////////////////////////////////

#include <rscommon.h>
#include <string>
using namespace std;

typedef void (*VIDEODECODECALLBACK)(void* pParam, unsigned char* pDecBuffer, int nDecSize, int nImageWidth, int nImageHeight, int nPixelFormat);
typedef void (*AUDIODECODECALLBACK)(void* pParam, unsigned char* pDecBuffer, int nDecSize, int nChannel, int nBitRate, int nBitsPerSample, int nSampleRate);

typedef void (*VIDEOENCODECALLBACK)(int nStreamType, void* pParam, unsigned char* pEncBuffer, int nEncSize, int nCodecID,int nImageWidth,int nImageHeight, int nFrameType, int nFrameNumber, const char* szExtraData, unsigned long long llTimestamp);
typedef void (*AUDIOENCODECALLBACK)(int nStreamType, void* pParam, unsigned char* pEncBuffer, int nEncSize, int nCodecID,int nChannel, int nBitsPerSample, int nSamplePerSec, const char* szExtraData, unsigned long long llTimestamp);

#define VIDEO_QUEUE_SIZE	(256*1024)
#define VIDEO_DECODE_SIZE	(1280*1024*2)
#define VIDEO_ENCODE_SIZE	(512*1024)

#define AUDIO_QUEUE_SIZE	(128*1024)
#define AUDIO_DECODE_SIZE	(32*1024)
#define AUDIO_ENCODE_SIZE	(32*1024)

enum CODEC_MODE
{
	AV_CODEC_NONE = 0,
	AUDIO_DECODE = 1,
	AUDIO_ENCODE = 2,
	VIDEO_DECODE = 3,
	VIDEO_ENCODE = 4
};

enum UNICODECSTATUS
{
	CODEC_SUCCEED			= 0,	//decode/encode one frame complete
	CODEC_NOTENOUGH_DATA	= 1,	//don't decode one frame, my be not enough data
	CODEC_INVALID_DATA		= 2,	//invalid data
	CODEC_INIT_ERROR		= 3,	//initialize error
	CODEC_ERROR				= 4,	//decode/encode error
	CODEC_SKIP_DATA			= 5
};
//
//#ifdef LIBEZCODEC_EXPORTS
//#	define LIBEZCODEC_API __declspec(dllexport)
//#else
//#	define LIBEZCODEC_API __declspec(dllimport)
//#endif

//class LIBEZCODEC_API CCodec
class CCodec
{
public:
	CCodec();
	virtual ~CCodec();

public:
	/***
	return: UNICODECSTATUS
	***/
	virtual int			AudioDecodeFrame(int nCodecID, unsigned char* pEncBuffer, int nEncSize, int nChannel, int nBitRate, int nBitsPerSample, int nSampleRate);
	virtual int			AudioEncodeFrame(unsigned char* pSrcData, int nSrcBytes, int nSrcPixFmt, int nSrcChannel, int nSrcBitsPerSample, int nSrcSamplePerSec, int nDstCodecID, int nDstChannels, int nDstBitsPerSample, int nDstSamplePerSec, int nDstBitRate, int nStreamType, unsigned long long llTimestamp);

	virtual int			VideoDecodeFrame(int nCodecID, unsigned char* pEncBuffer, int nEncSize, int nImageWidth, int nImageHeight);
	virtual int			VideoEncodeFrame(unsigned char* pSrcData, int nSrcBytes, int nSrcPixFmt, int nSrcImageWidth, int nSrcImageHeight, int nDstCodecID, int nDstImageWidth, int nDstImageHeight, int nLevel, int nBitRate, int nFrameRate, int nGopSize, int nStreamType, unsigned long long llTimestamp);

	virtual void		ResetCodec();

public:
	void				SetVideoDecodeCallback(void* pParam, VIDEODECODECALLBACK fpVideoDecodeCallback);
	void				SetVideoEncodeCallback(void* pParam, VIDEOENCODECALLBACK fpVideoEncodeCallback);
	void				SetAudioDecodeCallback(void* pParam, AUDIODECODECALLBACK fpAudioDecodeCallback);
	void				SetAudioEncodeCallback(void* pParam, AUDIOENCODECALLBACK fpAudioEncodeCallback);
	void				SetLogReport(void* pHandler, RsLogReport fpLogReport);

	string				GetCodecNameByCodecID(int cCodecID);
	static BOOL_			Create(std::string strLibrary, CCodec** pCodec);
	std::string			GetCodecName();

	unsigned long		GetLastError();

	std::string			m_szCodecName;
	unsigned long		m_dwCodecError;
	void*				m_pVideoParam;
	void*				m_pAudioParam;

	RsLogReport			m_fpLogReport;
	void*				m_pLogHandler;
	char				TAG[32];

	VIDEODECODECALLBACK m_fpVideoDecodeCallback;
	AUDIODECODECALLBACK m_fpAudioDecodeCallback;

	VIDEOENCODECALLBACK m_fpVideoEncodeCallback;
	AUDIOENCODECALLBACK m_fpAudioEncodeCallback;
	//////////////////////////////////////////////////////////////////////////
	//Filter
	void				SetOSDParam(BOOL_ bTextOSD, BOOL_ bTimeOSD, const char* strOSDFilter);
	BOOL_				m_bTextOSD;
	BOOL_				m_bTimeOSD;
	std::string			m_strOSDFilter;
};

#endif
