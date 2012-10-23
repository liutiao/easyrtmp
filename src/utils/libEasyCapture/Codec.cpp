#include "Codec.h"

#include "FFmpegCodec.h"
#include "EzEncoder.h"
#include "G711ACodec.h"
//#include "G726Codec.h"

CCodec::CCodec() : m_pLogHandler(NULL)
{
	m_dwCodecError			= 0;
	m_fpLogReport			= NULL;

	m_pVideoParam			= NULL;
	m_pAudioParam			= NULL;

	m_fpLogReport			= NULL;

	m_fpVideoDecodeCallback	= NULL;
	m_fpAudioDecodeCallback	= NULL;

	m_fpVideoEncodeCallback	= NULL;
	m_fpAudioEncodeCallback = NULL;

	m_bTextOSD				= FALSE_;
	m_bTimeOSD				= FALSE_;
}

CCodec::~CCodec()
{

}

void CCodec::SetOSDParam(BOOL_ bTextOSD, BOOL_ bTimeOSD, const char* strOSDFilter)
{
	m_bTextOSD		= bTextOSD;
	m_bTimeOSD		= bTimeOSD;
	m_strOSDFilter	= strOSDFilter;
}

//////////////////////////////////////////////////////////////////////////
//codec related definitions
//////////////////////////////////////////////////////////////////////////
string CCodec::GetCodecNameByCodecID(int cCodecID)
{
	string szCodecName = "libffcodec";
/*
	if (cCodecID == VTYPE_FFMPEG_MPEG2 || cCodecID == VTYPE_XVID_MPEG4_I420
		|| cCodecID == VTYPE_FFMPEG_MPEG4 || cCodecID == VTYPE_FFMPEG_H264
		|| cCodecID == ATYPE_PCM_MULAW
		|| cCodecID == ATYPE_ADPCM_G726_24 || cCodecID == ATYPE_ADPCM_G726_32
		|| cCodecID == ATYPE_ADPCM_G726_40 || cCodecID == ATYPE_ADPCM_IMA_WAV
		|| cCodecID == ATYPE_FFMPEG_MP2 || cCodecID == ATYPE_FFMPEG_MP3
		|| cCodecID == ATYPE_FFMPEG_AAC)
	{
		szCodecName =  "libezencoder";
	}
	else if (cCodecID == VTYPE_H264_HIK)
	{
		szCodecName = "libhikcodec";
	}
	else if (cCodecID == VTYPE_VIVOTEK_MPEG4)
	{
		szCodecName = "libvivocodec";
	}
	else
	{
		szCodecName =  "libffcodec";
	}
*/
	return szCodecName;
}

BOOL_ CCodec::Create(std::string strLibrary, CCodec** pCodec)
{
	if (strLibrary.compare("libezencoder") == 0)
	{
		*pCodec = new CEzEncoder();
	}
	else if (strLibrary.compare("libffcodec") == 0)
	{
		*pCodec = new CFFmpegCodec();
	}
//	else if (strLibrary.compare("libg726") == 0)
//	{
//		*pCodec = new CG726Codec();
//	}
	else if (strLibrary.compare("libg711a") == 0)
	{
		*pCodec = new CG711ACodec();
	}
	else
	{
		*pCodec = new CEzEncoder();
	}

	if (*pCodec)
		return TRUE_;
	else
		return FALSE_;
}

void CCodec::SetLogReport(void* pHandler, RsLogReport fpLogReport)
{
	m_pLogHandler = pHandler;
	m_fpLogReport = fpLogReport;
}

std::string CCodec::GetCodecName()
{
	return m_szCodecName;
}

void CCodec::ResetCodec()
{

}

unsigned long CCodec::GetLastError()
{
	return m_dwCodecError;
}

int CCodec::AudioDecodeFrame(int nCodecID, unsigned char* pEncBuffer, int nEncSize, int nChannel, int nBitRate, int nBitsPerSample, int nSampleRate)
{
	return CODEC_SUCCEED;
}

int CCodec::VideoDecodeFrame(int nCodecID, unsigned char* pEncBuffer, int nEncSize, int nImageWidth, int nImageHeight)
{
	return CODEC_SUCCEED;
}

int	CCodec::VideoEncodeFrame(unsigned char* pSrcData, int nSrcBytes, int nSrcPixFmt, int nSrcImageWidth, int nSrcImageHeight, int nDstCodecID, int nDstImageWidth, int nDstImageHeight, int nLevel, int nBitRate, int nFrameRate, int nGopSize, int nStreamType, unsigned long long llTimestamp)
{
	return CODEC_SUCCEED;
}

int	CCodec::AudioEncodeFrame(unsigned char* pSrcData, int nSrcBytes, int nSrcPixFmt, int nSrcChannel, int nSrcBitsPerSample, int nSrcSamplePerSec, int nDstCodecID, int nDstChannels, int nDstBitsPerSample, int nDstSamplePerSec, int nDstBitRate, int nStreamType, unsigned long long llTimestamp)
{
	return CODEC_SUCCEED;
}

void CCodec::SetVideoDecodeCallback(void* pParam, VIDEODECODECALLBACK fpVideoDecodeCallback)
{
	m_pVideoParam = pParam;
	m_fpVideoDecodeCallback = fpVideoDecodeCallback;
}

void CCodec::SetAudioDecodeCallback(void* pParam, AUDIODECODECALLBACK fpAudioDecodeCallback)
{
	m_pAudioParam = pParam;
	m_fpAudioDecodeCallback = fpAudioDecodeCallback;
}

void CCodec::SetVideoEncodeCallback(void* pParam, VIDEOENCODECALLBACK fpVideoEncodeCallback)
{
	m_pVideoParam = pParam;
	m_fpVideoEncodeCallback = fpVideoEncodeCallback;
}

void CCodec::SetAudioEncodeCallback(void* pParam, AUDIOENCODECALLBACK fpAudioEncodeCallback)
{
	m_pAudioParam = pParam;
	m_fpAudioEncodeCallback = fpAudioEncodeCallback;
}
