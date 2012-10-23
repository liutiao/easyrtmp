#include "G711ACodec.h"


CG711ACodec::CG711ACodec(void)
{
	m_pDecHandle = NULL;
	m_pEncHandle = NULL;
	m_pDecBuffer = NULL;
	m_pEncBuffer = NULL;
}

CG711ACodec::~CG711ACodec(void)
{
	if (m_pDecBuffer)
	{
		delete[] m_pDecBuffer;
		m_pDecBuffer = NULL;
	}
	if (m_pEncBuffer)
	{
		delete[] m_pEncBuffer;
		m_pEncBuffer = NULL;
	}
}

int CG711ACodec::AudioEncodeFrame(unsigned char* pSrcData, int nSrcBytes, int nSrcPixFmt, int nSrcChannel, int nSrcBitsPerSample, int nSrcSamplePerSec, int nDstCodecID, int nDstChannels, int nDstBitsPerSample, int nDstSamplePerSec, int nDstBitRate, int nStreamType, unsigned long long llTimestamp)
{
	if (!m_pEncHandle)
	{
		rs_g711_init_encoder(m_pEncHandle);
	}
	if (!m_pEncBuffer)
		m_pEncBuffer = new unsigned char[32*1024];

	int n = rs_g711_encoder((short*)pSrcData, nSrcBytes, m_pEncBuffer, m_pEncHandle);
	if (n > 0)
	{
		if (m_fpAudioEncodeCallback)
			m_fpAudioEncodeCallback(nStreamType, 
			m_pAudioParam, 
			(unsigned char*)m_pEncBuffer, 
			n, 
			nDstCodecID, 
			nSrcChannel,
			nSrcBitsPerSample, 
			nSrcSamplePerSec,
			"", llTimestamp);
	}

	return 0;
}
