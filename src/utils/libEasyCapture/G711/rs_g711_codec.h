#ifndef _______EASY_V2IP_ROSOO_VA_G711_INCLDUE
#define _______EASY_V2IP_ROSOO_VA_G711_INCLDUE

void rs_g711_init_decoder(void* pDecHandle);
int rs_g711_decoder(unsigned char* pInData, int nInSize, short* pOutData, void* pDecHandle);
void rs_g711_release_decoder(void* pDecHandle);

void rs_g711_init_encoder(void* pEncHandle);
int rs_g711_encoder(short *pInData, int nInSize, unsigned char *pOutData, void* pEncHandle);
void rs_g711_release_encoder(void* pEncHandle);

#endif
