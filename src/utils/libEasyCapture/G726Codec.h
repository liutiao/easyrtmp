#ifndef __EASY_STREAM_SERVER_INCLUDE_AUDIO_G726
#define __EASY_STREAM_SERVER_INCLUDE_AUDIO_G726

#pragma once

#include "Codec.h"

class CG726Codec : public CCodec
{
public:
	CG726Codec(void);
	~CG726Codec(void);

public:
//	virtual int		AudioDecodeFrame(int nCodecID, unsigned char* pEncBuffer, int nEncSize, int nChannel, int nBitRate, int nBitsPerSample, int nSampleRate);
	virtual int		AudioEncodeFrame(unsigned char* pSrcData, int nSrcBytes, int nSrcPixFmt, int nSrcChannel, int nSrcBitsPerSample, int nSrcSamplePerSec, int nDstCodecID, int nDstChannels, int nDstBitsPerSample, int nDstSamplePerSec, int nDstBitRate, int nStreamType);
	virtual string	GetCodecName();

private:
	char*	m_pOutBuffer;
};

#endif
