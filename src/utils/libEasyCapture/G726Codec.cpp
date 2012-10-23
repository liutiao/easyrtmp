#include "G726Codec.h"
#include "g726.h"

#pragma comment(lib, "g726lib.lib")

CG726Codec::CG726Codec(void) :m_pOutBuffer(NULL)
{
}

CG726Codec::~CG726Codec(void)
{
	if (m_pOutBuffer)
	{
		delete[] m_pOutBuffer;
		m_pOutBuffer = NULL;
	}
}

int CG726Codec::AudioEncodeFrame(unsigned char* pSrcData, int nSrcBytes, int nSrcPixFmt, int nSrcChannel, int nSrcBitsPerSample, int nSrcSamplePerSec, int nDstCodecID, int nDstChannels, int nDstBitsPerSample, int nDstSamplePerSec, int nDstBitRate, int nStreamType)
{
	/*
	if (!m_pOutBuffer) m_pOutBuffer = new char[1024];

	if (nSrcBytes != 960)
	{
		//printf("Audio Input Invalid, Size=%d", nSrcBytes);
		return -1;
	}
	g726_Encode((unsigned char*)pSrcData,m_pOutBuffer);

	if (m_fpAudioEncodeCallback)
		m_fpAudioEncodeCallback(nStreamType, 
			m_pAudioParam, 
			(unsigned char*)m_pOutBuffer, 
			nSrcBytes/8, 
			nDstCodecID, 
			nSrcChannel,
			nSrcBitsPerSample, 
			nSrcSamplePerSec,
			"");
	*/
	return 0;
}

string CG726Codec::GetCodecName()
{
	return "libg726";
}
