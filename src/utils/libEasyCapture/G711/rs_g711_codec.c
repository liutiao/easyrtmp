#include <stdio.h>

#include "rs_g711_codec.h"
#include "EzG711.h"

static int g_nDecHandleCount = 0;
static int g_nEncHandleCount = 0;

void rs_g711_init_decoder(void* pDecHandle)
{
	g_nDecHandleCount ++;
	pDecHandle = &g_nDecHandleCount;
}

int rs_g711_decoder(unsigned char* pInData, int nInSize, short* pOutData, void* pDecHandle)
{
	return ALawDecode((short*)pOutData, pInData, nInSize);
}

void rs_g711_release_decoder(void* pDecHandle)
{
	// free decoder resources
	if (pDecHandle)
	{
		g_nDecHandleCount --;
		pDecHandle = NULL;
	}
}

void rs_g711_init_encoder(void* pEncHandle)
{
	g_nEncHandleCount ++;
	pEncHandle = &g_nEncHandleCount;
}

int rs_g711_encoder(short *pInData, int nInSize, unsigned char *pOutData, void* pEncHandle)
{
	return ALawEncode(pOutData, (short*)pInData, nInSize);
}

void rs_g711_release_encoder(void* pEncHandle)
{
	// free encoder resources
	if (pEncHandle)
	{
		g_nEncHandleCount --;
		pEncHandle = NULL;
	}
}
