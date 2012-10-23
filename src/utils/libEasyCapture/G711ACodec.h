#pragma once
#include "codec.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "G711/rs_g711_codec.h"

class CG711ACodec : public CCodec
{
public:
	CG711ACodec(void);
	virtual ~CG711ACodec(void);

public:
	virtual int			AudioEncodeFrame(unsigned char* pSrcData, int nSrcBytes, int nSrcPixFmt, int nSrcChannel, int nSrcBitsPerSample, int nSrcSamplePerSec, int nDstCodecID, int nDstChannels, int nDstBitsPerSample, int nDstSamplePerSec, int nDstBitRate, int nStreamType, unsigned long long llTimestamp);

private:
	void* m_pEncHandle;
	void* m_pDecHandle;
	unsigned char* m_pEncBuffer;
	unsigned char* m_pDecBuffer;
};

#ifdef __cplusplus
}
#endif
