#ifndef ___EASY_ENCODER_INCLUDE______
#define ___EASY_ENCODER_INCLUDE______

//////////////////////////////////////////////////////////////////////////
//jacky, there is a critical bug in current implements
//CEzEncoder::m_pPicture can not be freed while using avfilter
//////////////////////////////////////////////////////////////////////////

#pragma once

#include "Codec.h"
#include <pthread.h>
#include "RsFilter.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>


typedef struct AV_PARAMS
{
	int nACodecID;
	int nChannels;
	int nBitsPerSample;
	int nSampleRate;
	
	int nVCodecID;
	int nFrameRate;
	int nVBitRate;
	int nGopSize;
	int nImageWidth;
	int nImageHeight;
}AV_PARAMS;

class CEzEncoder : public CCodec
{
public:
	CEzEncoder();
	virtual ~CEzEncoder();

public:
	virtual int			VideoEncodeFrame(uint8_t* pSrcData, int nSrcBytes, int nSrcPixFmt, int nSrcImageWidth, int nSrcImageHeight, int nDstCodecID, int nDstImageWidth, int nDstImageHeight, int nLevel, int nBitRate, int nFrameRate, int nGopSize, int nStreamType, unsigned long long llTimestamp);
	virtual int			AudioEncodeFrame(uint8_t* pSrcData, int nSrcBytes, int nSrcPixFmt, int nSrcChannel, int nSrcBitsPerSample, int nSrcSamplePerSec, int nDstCodecID, int nDstChannels, int nDstBitsPerSample, int nDstSamplePerSec, int nDstBitRate, int nStreamType, unsigned long long llTimestamp);

public:
	BOOL_				CreateAviFile(char* pFileName, int nACodecID, int nSampleRate, int nBitsPerSample, int nChannels, int nVCodecID, int nVBitRate, int nFrameRate, int nImageWidth, int nImageHeight, int nGopSize);
	void				Write_Audio_Frame(uint8_t* pData, int nSize);
	void				Write_Video_Frame(uint8_t* pData, int nSize, int isKeyFrame);

//video
private:
	BOOL_				VideoEncoderInitialize(int nCodecID, int nImageWidth, int nImageHeight, int nLevel, int nBitRate, int nFrameRate, int nGopSize, int nStreamType);
	BOOL_				VideoEncoderDestroy();
	//Generate H.264 RTP SDP
	char *				extradata2psets_h264(AVCodecContext *c);
	const uint8_t *		ff_avc_find_startcode(const uint8_t *p, const uint8_t *end);
	char *				av_base64_encode(char * buf, int buf_len, const uint8_t * src, int len);
	//Generate MPEG4 RTP SDP
	char *				extradata2psets_mpeg4(AVCodecContext *c);
	AVFrame *			alloc_picture(PixelFormat pix_fmt, int width, int height);
	AVCodec*			m_pCodecVE;
	AVCodecContext*		m_pContextVE;
	int					m_nVCodecID;
	int					m_nStreamType;
	uint8_t*			m_pVideoEncBuffer;
	AVFrame*			m_pPicture;
	SwsContext*			m_pSwsContext;
	int					m_nSwsSrcWidth;
	int					m_nSwsSrcHeight;

//audio
private:
	BOOL_				AudioEncoderInitialize(int nCodecID, int nChannel, int nBitsPerSample, int nSampleRate, int nBitRate);
	BOOL_				AudioEncoderDestroy();
	AVCodec*			m_pACodec;
	AVCodecContext*		m_pAContext;
	uint8_t*			m_pAudioBuffer;
	uint8_t*			m_pAudioEncBuffer;
	ReSampleContext	*	m_pReSampleCtx;
	int16_t*			m_pReSampleBuffer;
	uint8_t*			m_pFIFOBuffer;

	long				m_nFIFOReadOffset;
	long				m_nFIFOWriteOffset;

private:
	AVStream*			Add_Audio_Stream(AVFormatContext* pFmtCtx);
	AVStream*			Add_Video_Stream(AVFormatContext* pFmtCtx);
	AVFormatContext*	m_pFormatCtx;
	AVStream*			m_pAudioStream;
	AVStream*			m_pVideoStream;
	AVPacket			m_Packet;

	//////////////////////////////////////////////////////////////////////////
	//Filter
	CRsFilter*			m_pRsFilter;
	//////////////////////////////////////////////////////////////////////////
//public:
//	static char			TAG[32];
};

#ifdef __cplusplus
}
#endif

#endif
