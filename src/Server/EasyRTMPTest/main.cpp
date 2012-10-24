//#include "stdafx.h" 
#include <stdint.h>
#include <inttypes.h>

#ifdef WIN32
#include <math.h>
#endif

#pragma comment(lib, "ws2_32.lib")

#pragma comment(lib, "zlibd.lib")
#pragma comment(lib, "librtmp.lib")
#pragma comment(lib, "libx264.lib")
//#pragma comment(lib, "libx264-114.lib")
//#pragma comment(lib, "libx264.dll.a")
//#pragma comment(lib, "libx264.a")
#pragma comment(lib, "libeay32.lib")
//#pragma comment(lib, "polarssl.lib")
#pragma comment(lib, "ssleay32.lib")
/*
#pragma comment(lib, "../../dbin/zlibd.lib")
#pragma comment(lib, "../../dbin/librtmp.lib")
#pragma comment(lib, "../../dbin/libx264.lib")
//#pragma comment(lib, "../../dbin/libx264-114.lib")
//#pragma comment(lib, "../../dbin/libx264.dll.a")
//#pragma comment(lib, "../../dbin/libx264.a")
#pragma comment(lib, "../../dbin/libeay32.lib")
#pragma comment(lib, "../../dbin/polarssl.lib")
#pragma comment(lib, "../../dbin/ssleay32.lib")
*/
extern "C"  
{ 
#include "../utils/x264/common/common.h" 
#include "../utils/x264/common/cpu.h" 
#include "../utils/x264/x264.h" 
#include "../utils/x264/encoder/set.h" 
} 
#include "../utils/librtmp/rtmp_sys.h" 
#include "../utils/librtmp/log.h" 
#include "../utils/librtmp/amf.h" 

#include "CameraDS.h" 

#pragma comment(lib, "winmm.lib")

//转换矩阵 
#define MY(a,b,c) (( a*  0.2989  + b*  0.5866  + c*  0.1145)) 
#define MU(a,b,c) (( a*(-0.1688) + b*(-0.3312) + c*  0.5000 + 128)) 
#define MV(a,b,c) (( a*  0.5000  + b*(-0.4184) + c*(-0.0816) + 128)) 
//大小判断 
#define DY(a,b,c) (MY(a,b,c) > 255 ? 255 : (MY(a,b,c)  0 ? 0 : MY(a,b,c))) 
#define DU(a,b,c) (MU(a,b,c) > 255 ? 255 : (MU(a,b,c)  0 ? 0 : MU(a,b,c))) 
#define DV(a,b,c) (MV(a,b,c) > 255 ? 255 : (MV(a,b,c)  0 ? 0 : MV(a,b,c))) 

void ConvertYCbCr2BGR(unsigned char *pYUV,unsigned char *pBGR,int iWidth,int iHeight); 


void ConvertRGB2YUV(int WIDTH,int HEIGHT,unsigned char *RGB,unsigned char *YUV)
{
	//在位图中颜色存放顺序是BGR
	typedef struct tagRGBPOINT {
		BYTE rgbBlue;   //该颜色的蓝色分量
		BYTE rgbGreen;  //该颜色的绿色分量
		BYTE rgbRed;    //该颜色的红色分量
	} RGBPOINT;

	RGBPOINT point;

	BYTE r;
	BYTE g;
	BYTE b;

	double Y;
	double U;
	double V;

	BYTE bY;
	BYTE bU;
	BYTE bV;

	int n = WIDTH*HEIGHT;
	unsigned char *lpY = YUV;
	unsigned char *lpU = YUV + n;
	unsigned char *lpV = (YUV + n + (n >> 2));

	int pos = 0; 
	int i, j;
	for (i = (HEIGHT - 1); i > -1; i--)
	{
		for (j = 0; j < WIDTH; j++)
		{
			memcpy(&point,RGB + pos, sizeof(RGBPOINT));
			pos += sizeof(RGBPOINT);
			//BmpFile.Read((LPVOID) &point, sizeof(RGBPOINT));

			b = point.rgbBlue;
			g = point.rgbGreen;
			r = point.rgbRed;

			Y = (0.257 * r) + (0.504 * g) + (0.098 * b) +  16.0;
			U = -(0.148 * r) - (0.291 * g) + (0.439 * b) + 128.0;
			V = (0.439 * r) - (0.368 * g) - (0.071 * b) + 128.0;

			Y += 0.5;
			U += 0.5;
			V += 0.5;

			if (Y < 0.0) Y = 0.0;
			if (Y > 255.0) Y = 255.0;
			if (U < 0.0) U = 0.0;
			if (U > 255.0)U = 255.0;
			if (V < 0.0) V = 0.0;
			if (V > 255.0) V = 255.0;

			bY = (BYTE) Y;   
			bU = (BYTE) U;   
			bV = (BYTE) V;   

			lpY[i * WIDTH + j] = bY;
			if (((i + 1) % 2) == 0) {
				if ((j % 2) == 0)
					lpV[(i / 2) * (WIDTH / 2) + j / 2] = bV;
			} else {
				if ((j % 2) == 0)
					lpU[(i / 2) * (WIDTH / 2) + j / 2] = bU;                       
			}
		}   
	}
}

int InitSockets() 
{ 
#ifdef WIN32 
    WORD version; 
    WSADATA wsaData; 
    version = MAKEWORD(1, 1); 
    return (WSAStartup(version, &wsaData) == 0); 
#else 
    return TRUE; 
#endif 
} 
inline void CleanupSockets() 
{ 
#ifdef WIN32 
    WSACleanup(); 
#endif 
} 
#define HEX2BIN(a)      (((a)&0x40)?((a)&0xf)+9:((a)&0xf)) 
int hex2bin(char *str, char **hex) 
{ 
    char *ptr; 
    int i, l = strlen(str); 
    if (l & 1) 
        return 0; 
    *hex = (char *)malloc(l/2); 
    ptr = *hex; 
    if (!ptr) 
        return 0; 
    for (i=0; i<l; i+=2) 
        *ptr++ = (HEX2BIN(str[i]) << 4) | HEX2BIN(str[i+1]); 
    return l/2; 
} 
char * put_byte( char *output, uint8_t nVal ) 
{ 
    output[0] = nVal; 
    return output+1; 
} 
char * put_be16(char *output, uint16_t nVal ) 
{ 
    output[1] = nVal & 0xff; 
    output[0] = nVal >> 8; 
    return output+2; 
} 
char * put_be24(char *output,uint32_t nVal ) 
{ 
    output[2] = nVal & 0xff; 
    output[1] = nVal >> 8; 
    output[0] = nVal >> 16; 
    return output+3; 
} 
char * put_be32(char *output, uint32_t nVal ) 
{ 
    output[3] = nVal & 0xff; 
    output[2] = nVal >> 8; 
    output[1] = nVal >> 16; 
    output[0] = nVal >> 24; 
    return output+4; 
} 
char *  put_be64( char *output, uint64_t nVal ) 
{ 
    output=put_be32( output, nVal >> 32 ); 
    output=put_be32( output, nVal ); 
    return output; 
} 
char * put_amf_string( char *c, const char *str ) 
{ 
    uint16_t len = strlen( str ); 
    c=put_be16( c, len ); 
    memcpy(c,str,len); 
    return c+len; 
} 
char * put_amf_double( char *c, double d ) 
{ 
    *c++ = AMF_NUMBER;  /* type: Number */ 
    { 
        unsigned char *ci, *co; 
        ci = (unsigned char *)&d; 
        co = (unsigned char *)c; 
        co[0] = ci[7]; 
        co[1] = ci[6]; 
        co[2] = ci[5]; 
        co[3] = ci[4]; 
        co[4] = ci[3]; 
        co[5] = ci[2]; 
        co[6] = ci[1]; 
        co[7] = ci[0]; 
    } 
    return c+8; 
} 
int main(int argc, char * argv[]) 
{ 
    if (argc<2)  
    { 
        RTMP_LogPrintf("RTMP_URL IS NULL!!!\n"); 
        //return -1; 
    } 
    if (!InitSockets()) 
    { 
        RTMP_LogPrintf("InitSockets Error!\n"); 
        return -1; 
    } 
    RTMP_LogPrintf("InitSockets!\n"); 
	int nHeight = 240; 
	int nWidth = 320; 
    RTMP_LogPrintf("Camera Open Scuess,Picture Size[%2dx%d]\n",nWidth,nHeight); 
    RTMP_debuglevel = RTMP_LOGINFO; 
    RTMP *r; 
    //char uri[]="rtmp://221.9.244.4/live/jltv"; 
    char uri[]="rtmp://127.0.0.1/live/0_320x240"; 
    r= RTMP_Alloc(); 
    RTMP_Init(r); 
    RTMP_SetupURL(r, (char*)uri); 
    RTMP_EnableWrite(r); 
    RTMP_Connect(r, NULL); 
    RTMP_ConnectStream(r,0); 
    unsigned char szNalBuffer[1024*32]; 
    unsigned char szBodyBuffer[1024*32]; 
    x264_nal_t  *p264Nal; 
    int         i264Nal; 
    x264_param_t p264Param; 
    x264_picture_t * p264Pic; 
    x264_t *p264Handle;  
    p264Pic  = new x264_picture_t(); 
    memset(p264Pic,0,sizeof(x264_picture_t)); 

    x264_param_default(&p264Param);  //set default param 
    p264Param.i_threads	= 1; 
    p264Param.i_width		= nWidth;   //set frame width 
    p264Param.i_height		= nHeight;  //set frame height 
    //baseline level 1.1
    p264Param.b_cabac		= 0;  
    p264Param.i_bframe		= 0; 
    p264Param.b_interlaced	= 0; 
    p264Param.rc.i_rc_method = X264_RC_ABR;//X264_RC_CQP 
    p264Param.i_level_idc	= 21; 
    p264Param.rc.i_bitrate = 200; 
    p264Param.i_fps_num	= 30; 
    p264Param.i_keyint_max	= p264Param.i_fps_num*3; 
	p264Param.i_keyint_max	= p264Param.i_fps_num; 
	p264Handle = x264_encoder_open(&p264Param);
    if(p264Handle == NULL) 
    {
        fprintf( stderr, "x264_encoder_open failed/n" ); 
        return -2; 
    } 
    bs_t bs={0}; 
    x264_picture_alloc(p264Pic, X264_CSP_YV12, p264Param.i_width, p264Param.i_height); 
    p264Pic->i_type = X264_TYPE_AUTO; 
    x264_picture_t pic_out; 
    RTMPPacket packet={0}; 
    memset(&packet,0,sizeof(RTMPPacket)); 
    packet.m_nChannel = 0x04; 
    packet.m_headerType = RTMP_PACKET_SIZE_LARGE; 
    packet.m_nTimeStamp = 0; 
    packet.m_nInfoField2 = r->m_stream_id; 
    packet.m_hasAbsTimestamp = 0; 
    packet.m_body =(char *) szBodyBuffer; 
    char * szTmp=(char *)szBodyBuffer; 
    packet.m_packetType = RTMP_PACKET_TYPE_INFO; 
    szTmp=put_byte(szTmp, AMF_STRING ); 
    szTmp=put_amf_string(szTmp, "@setDataFrame" ); 
    szTmp=put_byte(szTmp, AMF_STRING ); 
    szTmp=put_amf_string(szTmp, "onMetaData" ); 
    szTmp=put_byte(szTmp, AMF_OBJECT ); 
    szTmp=put_amf_string( szTmp, "author" ); 
    szTmp=put_byte(szTmp, AMF_STRING ); 
    szTmp=put_amf_string( szTmp, "" ); 
    szTmp=put_amf_string( szTmp, "copyright" ); 
    szTmp=put_byte(szTmp, AMF_STRING ); 
    szTmp=put_amf_string( szTmp, "" ); 
    szTmp=put_amf_string( szTmp, "description" ); 
    szTmp=put_byte(szTmp, AMF_STRING ); 
    szTmp=put_amf_string( szTmp, "" ); 
    szTmp=put_amf_string( szTmp, "keywords" ); 
    szTmp=put_byte(szTmp, AMF_STRING ); 
    szTmp=put_amf_string( szTmp, "" ); 
    szTmp=put_amf_string( szTmp, "rating" ); 
    szTmp=put_byte(szTmp, AMF_STRING ); 
    szTmp=put_amf_string( szTmp, "" ); 
    szTmp=put_amf_string( szTmp, "presetname" ); 
    szTmp=put_byte(szTmp, AMF_STRING ); 
    szTmp=put_amf_string( szTmp, "Custom" ); 
    szTmp=put_amf_string( szTmp, "width" ); 
    szTmp=put_amf_double( szTmp, p264Param.i_width ); 
    szTmp=put_amf_string( szTmp, "width" ); 
    szTmp=put_amf_double( szTmp, p264Param.i_width ); 
    szTmp=put_amf_string( szTmp, "height" ); 
    szTmp=put_amf_double( szTmp, p264Param.i_height ); 
    szTmp=put_amf_string( szTmp, "framerate" ); 
    szTmp=put_amf_double( szTmp, (double)p264Param.i_fps_num / p264Param.i_fps_den ); 
    szTmp=put_amf_string( szTmp, "videocodecid" ); 
    szTmp=put_byte(szTmp, AMF_STRING ); 
    szTmp=put_amf_string( szTmp, "avc1" ); 
    szTmp=put_amf_string( szTmp, "videodatarate" ); 
    szTmp=put_amf_double( szTmp, p264Param.rc.i_bitrate );  
    szTmp=put_amf_string( szTmp, "avclevel" ); 
    szTmp=put_amf_double( szTmp, p264Param.i_level_idc );  
    szTmp=put_amf_string( szTmp, "avcprofile" ); 
    szTmp=put_amf_double( szTmp, 0x42 );  
    szTmp=put_amf_string( szTmp, "videokeyframe_frequency" ); 
    szTmp=put_amf_double( szTmp, 3 );  
    szTmp=put_amf_string( szTmp, "" ); 
    szTmp=put_byte( szTmp, AMF_OBJECT_END ); 
    packet.m_nBodySize=szTmp-(char *)szBodyBuffer; 
    RTMP_SendPacket(r,&packet,1); 
	//////////////////////////////////////////////////////////////////////////
    packet.m_packetType = RTMP_PACKET_TYPE_VIDEO;   /* VIDEO */ 
	memset(&szBodyBuffer, 0, sizeof(szBodyBuffer));
    szBodyBuffer[ 0]=0x17; 
    szBodyBuffer[ 1]=0x00; 
    szBodyBuffer[ 2]=0x00; 
    szBodyBuffer[ 3]=0x00; 
    szBodyBuffer[ 4]=0x00; 
    szBodyBuffer[ 5]=0x01; 
    szBodyBuffer[ 6]=0x42; 
    szBodyBuffer[ 7]=0xC0; 
    szBodyBuffer[ 8]=0x15; 
    szBodyBuffer[ 9]=0x03; 
    szBodyBuffer[10]=0x01; 
    szTmp=(char *)szBodyBuffer+11; 
    short slen=0; 
    bs_init(&bs,szNalBuffer,16);//初始话bs 
    x264_sps_write(&bs, p264Handle->sps);//读取编码器的SPS 
    slen=bs.p-bs.p_start+1;//spslen（short） 
    slen=htons(slen); 
    memcpy(szTmp,&slen,sizeof(short)); 
    szTmp+=sizeof(short); 
    *szTmp=0x67; 
    szTmp+=1; 
    memcpy(szTmp,bs.p_start,bs.p-bs.p_start); 
    szTmp+=bs.p-bs.p_start; 
    *szTmp=0x01; 
    szTmp+=1; 
    bs_init(&bs,szNalBuffer,16);//初始话bs 
    x264_pps_write(&bs, p264Handle->pps);//读取编码器的PPS 
    slen=bs.p-bs.p_start+1;//spslen（short） 
    slen=htons(slen); 
    memcpy(szTmp,&slen,sizeof(short)); 
    szTmp+=sizeof(short); 
    *szTmp=0x68; 
    szTmp+=1; 
    memcpy(szTmp,bs.p_start,bs.p-bs.p_start); 
    szTmp+=bs.p-bs.p_start; 
    packet.m_nBodySize=szTmp-(char *)szBodyBuffer; 
    RTMP_SendPacket(r,&packet,0); 
    unsigned int nTimes=0; 
    unsigned int oldTick=GetTickCount(); 
    unsigned int newTick=0; 

	packet.m_nTimeStamp=0; 
     
	memset(&szBodyBuffer, 0, sizeof(szBodyBuffer));

	CoInitialize(NULL); 
	CCameraDS camera; 
	if (!camera.OpenCamera(0, false, 320, 240)) 
	{ 
		RTMP_LogPrintf("Open Camera Error\n"); 
		return -1; 
	} 
	nHeight=camera.GetHeight(); 
	nWidth=camera.GetWidth(); 

	while(true) 
    { 
        szBodyBuffer[ 0]=0x17; 
        szBodyBuffer[ 1]=0x01; 
        szBodyBuffer[ 2]=0x00; 
        szBodyBuffer[ 3]=0x00; 
        szBodyBuffer[ 4]=0x42; 
        unsigned char * szTmp=szBodyBuffer+5; 
        unsigned  char * pNal=szNalBuffer; 
        nTimes++; 
        int nFramsInPack=0;
        while(true)
        {
            nFramsInPack++;
            unsigned char * pCameraBuf = camera.QueryFrame();
            if (!pCameraBuf)
            {
                return -1;
            } 
			//rotate rgb buffer;
            //for(int ii=0;ii<nHeight;ii++) 
			//	memcpy(szRGBBuffer+(nWidth*3)*(nHeight-ii-1),pCameraBuf+(nWidth*3)*ii,nWidth*3); 
            //ConvertRGB2YUV(nWidth,nHeight,szRGBBuffer,p264Pic->img.plane[0]); 
			ConvertRGB2YUV(nWidth,nHeight,pCameraBuf,p264Pic->img.plane[0]); 

			if( x264_encoder_encode( p264Handle, &p264Nal, &i264Nal, p264Pic ,&pic_out) < 0 ) 
            { 
                fprintf( stderr, "x264_encoder_encode failed/n" ); 
            } 
            for( int i = 0; i < i264Nal; i++ ) 
            { 
                int i_size; 
                int i_data; 
                i_data = 1024*32; 
				if( ( i_size = x264_nal_encode( pNal, &i_data, 1, &p264Nal[i] ) ) > 0 ) 
				//x264_nal_encode( p264Handle, pNal, &p264Nal[i]);
                {
                    if ((pNal[4]&0x60)==0) 
                    { 
                        continue; 
                    } 
                    if (pNal[4]==0x67) 
                    { 
                        continue; 
                    } 
                    if (pNal[4]==0x68) 
                    { 
                        continue; 
                    } 
                    memmove(pNal,pNal+4,i_size-4); 
                    pNal+=i_size-4; 
                } 
				else
				{
					printf ("what the hell...\n");
					break;
				}
            } 
            unsigned int nSize=pNal-szNalBuffer; 
            packet.m_nBodySize=nSize+9; 
            if (i264Nal>1) 
            { 
                szBodyBuffer[ 0]=0x17; 
            } 
            else 
            { 
                szBodyBuffer[ 0]=0x27; 
            } 
            put_be32((char *)szBodyBuffer+5,nSize); 
			int n = pNal-szNalBuffer;
            memcpy(szBodyBuffer+9,szNalBuffer,n); 

			printf("RTMP Send Bytes= %d\n", nSize);

            RTMP_SendPacket(r,&packet,0); 
            Sleep(20); 
            newTick=GetTickCount(); 
//RTMP_LogStatus("/rInfo NAUL Type:0x%02x size: %5d Tick:%03d %03d",szNalBuffer[0], nSize,33-nSleep,GetTickCount()-oldTick+nSleep); 
            packet.m_headerType = RTMP_PACKET_SIZE_MEDIUM; 
            packet.m_nTimeStamp+=newTick-oldTick; 
            oldTick=newTick; 
            break; 
        } 
    } 
    return 0; 
} 

