#if !defined(AFX_FFMPEGCODEC_H__1CDA3DA4_9D23_4591_BB5C_DE19E09A5EB9__INCLUDED_)
#define AFX_FFMPEGCODEC_H__1CDA3DA4_9D23_4591_BB5C_DE19E09A5EB9__INCLUDED_

#include "Codec.h"

#include <pthread.h>
#pragma comment(lib, "pthreadVC2.lib")

#ifdef __cplusplus
extern "C" {
#endif 

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libswscale/swscale.h>

//class CColorSpace;
class CFFmpegCodec : public CCodec
{
public:
	CFFmpegCodec();
	virtual ~CFFmpegCodec();

//for test
public:
	virtual int			AudioEncodeFrame_OLD(unsigned char* pSrcData, int nSrcBytes, int nSrcPixFmt, int nSrcChannel, int nSrcBitsPerSample, int nSrcSamplePerSec, int nDstCodecID, int nDstChannels, int nDstBitsPerSample, int nDstSamplePerSec, int nDstBitRate, int nStreamType, unsigned long long llTimestamp);
	virtual int			VideoDecodeFrame(int nCodecID, unsigned char* pEncBuffer, int nEncSize, int nImageWidth, int nImageHeight);
	virtual int			VideoEncodeFrame(unsigned char* pSrcData, int nSrcBytes, int nSrcPixFmt, int nSrcImageWidth, int nSrcImageHeight, int nDstCodecID, int nDstImageWidth, int nDstImageHeight, int nLevel, int nBitRate, int nFrameRate, int nGopSize, int nStreamType, unsigned long long llTimestamp);

	virtual int			AudioDecodeFrame(int nCodecID, unsigned char* pEncBuffer, int nEncSize, int nChannel, int nBitRate, int nBitsPerSample, int nSampleRate);
	virtual int			AudioEncodeFrame(unsigned char* pSrcData, int nSrcBytes, int nSrcPixFmt, int nSrcChannel, int nSrcBitsPerSample, int nSrcSamplePerSec, int nDstCodecID, int nDstChannels, int nDstBitsPerSample, int nDstSamplePerSec, int nDstBitRate, int nStreamType, unsigned long long llTimestamp);

public:
	void				ResetCodec();

private:
	BOOL_				AudioEncoderInitialize(int nCodecID, int nChannel, int nBitRate, int nBitsPerSample, int nSampleRate);
	BOOL_				AudioEncoderInitialize_new(int nCodecID, int nChannel, int nBitRate, int nBitsPerSample, int nSampleRate);
	BOOL_				AudioEncoderDestroy();
	BOOL_				AudioDecoderInitialize(int nCodecID, int nChannel, int nBitRate, int nBitsPerSample, int nSampleRate);
	BOOL_				AudioDecoderDestroy();

	BOOL_				VideoDecoderInitialize(int nCodecID, int nWidth, int nHeight);
	BOOL_				VideoDecoderDestroy();
	BOOL_				VideoEncoderInitialize(int nCodecID, int nWidth, int nHeight, int nLevel, int nBitRate, int nFrameRate, int nGopSize, int nStreamType);
	BOOL_				VideoEncoderDestroy();

	//Generate H.264 RTP SDP
	char *				extradata2psets_h264(AVCodecContext *c);
	const uint8_t *		ff_avc_find_startcode(const uint8_t *p, const uint8_t *end);
	char *				av_base64_encode(char * buf, int buf_len, const uint8_t * src, int len);
	//Generate MPEG4 RTP SDP
	char *				extradata2psets_mpeg4(AVCodecContext *c);

	AVFrame *			alloc_picture(int pix_fmt, int width, int height);
private:
	int					m_nCodecType;

	AVCodec*			m_pCodecAD;
	AVCodec*			m_pCodecAE;
	AVCodecContext*		m_pContextAD;
	AVCodecContext*		m_pContextAE;

	AVCodec*			m_pCodecVD;
	AVCodec*			m_pCodecVE;
	AVCodecContext*		m_pContextVD;
	AVCodecContext*		m_pContextVE;

	int					m_nCodecID;
	int					m_nStreamType;

    AVFrame*			m_pPicture;

	unsigned char*		m_pAudioDecBuffer;
	unsigned char*		m_pVideoDecBuffer;
	unsigned char*		m_pAudioEncBuffer;
	unsigned char*		m_pVideoEncBuffer;
	
	unsigned char*		m_pVideoBuffer;
	unsigned long		m_dwVideoWriteIndex;
	unsigned long		m_dwVideoReadIndex;

	unsigned char*		m_pAudioBuffer;
	unsigned long		m_dwAudioWriteIndex;
	unsigned long		m_dwAudioReadIndex;
	
	int16_t*			m_pSample;
	int					m_nAudioFrameSize;

	BOOL_				m_bNeedInterleave;
	int					m_nInterleaveCount;
	AVFrame*			m_pYUVFrame1;
	AVFrame*			m_pYUVFrame2;
	AVFrame*			m_pYUVMergeFrame;
	AVFrame*			m_pDeinterlace;

	SwsContext*			m_pSwsContext;

//	CColorSpace*		m_pColorSpace;
//public:
//	static char			TAG[32];
};

#ifdef __cplusplus
}
#endif

#endif // !defined(AFX_FFMPEGCODEC_H__1CDA3DA4_9D23_4591_BB5C_DE19E09A5EB9__INCLUDED_)
