#include "EzEncoder.h"
#define BYTE uint8_t
#include "DIB24Lib.h"
#pragma comment(lib, "DIB24Lib.lib")

extern pthread_mutex_t	g_vFFMPEGLock;
extern BOOL_ s_bFFMPGELock;

static int av_log_level = AV_LOG_WARNING;

void dddd(void* ptr, int level, const char* fmt, va_list vl)
{
	static int print_prefix=1;
	static int count;
	static char line[1024], prev[1024];
	AVClass* avc= ptr ? *(AVClass**)ptr : NULL;
	if(level >= av_log_level)
		return;
#undef fprintf
	if(print_prefix && avc) {
		_snprintf_s(line, sizeof(line),sizeof(line), "[%s @ %p]", avc->item_name(ptr), ptr);
	}else
		line[0]=0;

	_vsnprintf_s(line + strlen(line), sizeof(line) - strlen(line), sizeof(line)-strlen(line), fmt, vl);

	print_prefix= line[strlen(line)-1] == '\n';
	if(print_prefix && !strcmp(line, prev)){
		count++;
		return;
	}
	if(count>0){
		fprintf(stderr, "    Last message repeated %d times\n", count);
		count=0;
	}
	//    fputs(line, stderr);
	strcat_s(line, sizeof(line), "\n");
	printf(line);
	strcpy_s(prev, sizeof(prev), line);
}

CEzEncoder::CEzEncoder()
{
	strcpy(TAG, "EzEncoder");
	avcodec_init();
    av_register_all();

	if (!s_bFFMPGELock)
	{
		pthread_mutex_init(&g_vFFMPEGLock, NULL);
		s_bFFMPGELock = TRUE_;
	}
#ifdef _DEBUG
//	av_log_set_callback(dddd);
#endif
	//video
	m_pSwsContext		= NULL;
	m_pCodecVE			= NULL;
	m_pContextVE		= NULL;
	m_pVideoEncBuffer	= NULL;
	m_pPicture			= NULL;

	//audio
	m_nFIFOReadOffset	= 0;
	m_nFIFOWriteOffset	= 0;
	m_pAudioEncBuffer	= NULL;
	m_pFIFOBuffer		= NULL;
	m_pAudioBuffer		= NULL;
	m_pReSampleBuffer	= NULL;
	m_pReSampleCtx		= NULL;
	m_pACodec			= NULL;
	m_pAContext			= NULL;

	//av container
	m_pFormatCtx		= NULL;
	m_pAudioStream		= NULL;
	m_pVideoStream		= NULL;

	//filter
#ifdef __RG4_ENABLE_WATERMARK
	m_pRsFilter			= NULL;
#endif
}

CEzEncoder::~CEzEncoder()
{
	VideoEncoderDestroy();
	AudioEncoderDestroy();
}

AVFrame *CEzEncoder::alloc_picture(PixelFormat pix_fmt, int width, int height)
{
	AVFrame *picture;
	uint8_t *picture_buf;
	int size;
	
	picture = avcodec_alloc_frame();
	
	if (!picture)
		return NULL;
	
	size = avpicture_get_size((PixelFormat)pix_fmt, width, height);
	
	picture_buf = (uint8_t *)malloc(size);
	if (!picture_buf)
	{
		av_free(picture);
		return NULL;
	}
	
	avpicture_fill((AVPicture *)picture, picture_buf, pix_fmt, width, height);
	picture->width	= width;
	picture->height = height;
	picture->format = pix_fmt;

	return picture;
}

int CEzEncoder::VideoEncodeFrame(uint8_t* pSrcData, int nSrcBytes, int nSrcPixFmt, int nSrcImageWidth, int nSrcImageHeight, int nDstCodecID, int nDstImageWidth, int nDstImageHeight, int nLevel, int nBitRate, int nFrameRate, int nGopSize, int nStreamType, unsigned long long llTimestamp)
{
	try
	{
		if(!m_pContextVE || m_nVCodecID != nDstCodecID)
		{
			if(!VideoEncoderInitialize(nDstCodecID, nDstImageWidth, nDstImageHeight, nLevel, nBitRate, nFrameRate, nGopSize, nStreamType))
			{
				return -1;
			}
		}

		if (m_pContextVE == NULL) 
		{
			return -1;
		}

		PixelFormat npixFmt = (PixelFormat)nSrcPixFmt;

		if (nSrcImageWidth != m_nSwsSrcWidth || nSrcImageHeight != m_nSwsSrcHeight)
		{
			if (m_pSwsContext)
			{
				sws_freeContext(m_pSwsContext);
				m_pSwsContext = NULL;
			}
		}
		if (!m_pSwsContext)
		{
			m_pSwsContext = sws_getContext(nSrcImageWidth, nSrcImageHeight, npixFmt,
				nDstImageWidth, nDstImageHeight, PIX_FMT_YUV420P, SWS_BICUBLIN, NULL, NULL, NULL);//SWS_BICUBIC
			if (m_pSwsContext == NULL)
			{
				return -1;
			}
			m_nSwsSrcWidth	= nSrcImageWidth;
			m_nSwsSrcHeight = nSrcImageHeight;
		}		

		if (!m_pPicture)
		{
			m_pPicture = alloc_picture(PIX_FMT_YUV420P, nDstImageWidth, nDstImageHeight);
		}

		uint8_t* src[4];
		int srcStride[4];
		if (nSrcPixFmt == PIX_FMT_YUV420P)//YV12
		{
			src[0] = pSrcData;
			src[2] = pSrcData + nSrcImageWidth*nSrcImageHeight;
			src[1] = pSrcData + nSrcImageWidth*nSrcImageHeight*5/4;
			src[3] = NULL;	
			srcStride[0] = nSrcImageWidth;
			srcStride[2] = nSrcImageWidth/2;
			srcStride[1] = nSrcImageWidth/2;
			srcStride[3] = 0;
		}
		else if (nSrcPixFmt == PIX_FMT_YUV420P)//RAW_VIDEO_I420
		{
			src[0] = pSrcData;
			src[1] = pSrcData + nSrcImageWidth*nSrcImageHeight;
			src[2] = pSrcData + nSrcImageWidth*nSrcImageHeight*5/4;
			src[3] = NULL;
			srcStride[0] = nSrcImageWidth;
			srcStride[1] = nSrcImageWidth/2;
			srcStride[2] = nSrcImageWidth/2;
			srcStride[3] = 0;
		}
		else if (nSrcPixFmt == PIX_FMT_BGR24)//RAW_VIDEO_RGB24
		{
			DIB24FlipXY(pSrcData, nSrcImageWidth, nSrcImageHeight);
			src[0] = pSrcData;
			src[1] = NULL;
			src[2] = NULL;
			src[3] = NULL;
			srcStride[0] = nSrcImageWidth*3;
			srcStride[2] = 0;
			srcStride[1] = 0;
			srcStride[3] = 0;
		}
		else if (nSrcPixFmt == PIX_FMT_RGB24)//RAW_VIDEO_RGB24
		{
			DIB24FlipXY(pSrcData, nSrcImageWidth, nSrcImageHeight);
			src[0] = pSrcData;
			src[1] = NULL;
			src[2] = NULL;
			src[3] = NULL;
			srcStride[0] = nSrcImageWidth*3;
			srcStride[2] = 0;
			srcStride[1] = 0;
			srcStride[3] = 0;
		}
		else if (nSrcPixFmt == PIX_FMT_RGB32)//RAW_VIDEO_RGB32
		{
			DIB24FlipXY(pSrcData, nSrcImageWidth, nSrcImageHeight);
			src[0] = pSrcData;
			src[1] = NULL;
			src[2] = NULL;
			src[3] = NULL;
			srcStride[0] = nSrcImageWidth*4;
			srcStride[2] = 0;
			srcStride[1] = 0;
			srcStride[3] = 0;
		}

		int nRet = sws_scale(m_pSwsContext, src, srcStride, 0, nSrcImageHeight, m_pPicture->data, m_pPicture->linesize);
		int out_size;

#ifdef __RG4_ENABLE_WATERMARK
		if (m_pRsFilter)
		{
			m_pRsFilter->FilterFrame(m_pPicture, m_pPicture->width, m_pPicture->height);
		}
#endif
		out_size = avcodec_encode_video(
			m_pContextVE, 
			m_pVideoEncBuffer,
			nDstImageWidth*nDstImageHeight*3/2,
			m_pPicture
			);

		if(out_size < 0)
		{
			VideoEncoderDestroy();
			return 0;
		}
		else if (out_size == 0)
		{
			return 0;
		}
		else if (out_size > 0)
		{
			string szExtraData = "";
			if (m_pContextVE->extradata_size == 0)
			{
				int nStart,nEnd;
				switch (m_pContextVE->codec_id)
				{
				case CODEC_ID_H264:
					{
						if (!m_pContextVE->coded_frame->key_frame)
							break;
						for (int i=0; i<out_size; i++)
						{
							if (m_pVideoEncBuffer[i] == 0x00 && m_pVideoEncBuffer[i+1] ==0x00 && m_pVideoEncBuffer[i+2] == 0x00 && m_pVideoEncBuffer[i+3] ==0x01 && m_pVideoEncBuffer[i+4] ==0x67)
								nStart = i;
							if (m_pVideoEncBuffer[i] == 0x00 && m_pVideoEncBuffer[i+1] ==0x00 && m_pVideoEncBuffer[i+2] == 0x00 && m_pVideoEncBuffer[i+3] ==0x01 && m_pVideoEncBuffer[i+4] ==0x65)
							{
								nEnd = i;
								m_pContextVE->extradata_size = nEnd - nStart;
								break;
							}
						}
						if (m_pVideoEncBuffer[nStart + 4] == 0x67 && m_pContextVE->extradata_size)
						{
							m_pContextVE->extradata = m_pVideoEncBuffer + nStart;
							char *config = NULL;
							config = extradata2psets_h264(m_pContextVE);  //generate SDP
							szExtraData = config;
						}
					}
					break;
				case CODEC_ID_MPEG4:
					{
						if (!m_pContextVE->coded_frame->key_frame)
							break;
						BOOL_ bFoundVOL = FALSE_;
						for (int i=0; i<out_size; i++)
						{
							if (m_pVideoEncBuffer[i] == 0x00 && m_pVideoEncBuffer[i+1] ==0x00 && m_pVideoEncBuffer[i+2] == 0x01 && m_pVideoEncBuffer[i+3] ==0xB0)
							{
								nStart = i;
								// 								FILE* fp = fopen("c:\\MPEG4-VOL.dat", "a+b");
								// 								if (fp)
								// 								{
								// 									fwrite(m_pVideoEncBuffer, sizeof(unsigned char), out_size, fp);
								// 									fclose(fp);
								// 								}
								bFoundVOL = TRUE_;
							}
							if (!bFoundVOL) continue;
							if (m_pVideoEncBuffer[i] == 0x0 && m_pVideoEncBuffer[i+1] ==0x0 && m_pVideoEncBuffer[i+2] == 0x01 && m_pVideoEncBuffer[i+3] ==0xB6)
							{
								nEnd = i;
								//								m_pContextVE->extradata = m_pVideoEncBuffer + nStart;
								m_pContextVE->extradata_size = nEnd - nStart;

								//////////////////////////////////////////////////////////////////////////
								if (m_pContextVE->extradata_size > 65536)
								{
									av_log(m_pContextVE, AV_LOG_ERROR, "Too many extra data!\n");
									break;
								}

								char* config = new char[m_pContextVE->extradata_size*2+2];
								memset(config, 0, m_pContextVE->extradata_size*2);
								for (int k = nStart; k < nEnd; k ++)
								{
									sprintf(config + (k-nStart)*2, "%02X", m_pVideoEncBuffer[k]);
								}
								szExtraData = config;
								delete[] config;
								config = NULL;
								break;
							}
						}
					}
					break;
				}
			}

			if (m_pVideoParam && m_fpVideoEncodeCallback)
			{
				//RS_OUTPUT("--------encode : FrameType=%d, FrameNumber=%d\n", m_pContext->coded_frame->key_frame, m_pContext->frame_number);
				//m_fpVideoEncodeCallback(m_pVideoParam, m_pVideoEncBuffer, out_size, nDstCodecID, nDstImageWidth, nDstImageHeight, m_pContextVE->coded_frame->key_frame);
				m_fpVideoEncodeCallback(nStreamType, m_pVideoParam, m_pVideoEncBuffer, out_size, nDstCodecID,nDstImageWidth, nDstImageHeight, m_pContextVE->coded_frame->key_frame, m_pContextVE->frame_number,szExtraData.c_str(), llTimestamp);
			}
		}
	}
	catch (...)
	{
		if (m_fpLogReport)
			m_fpLogReport(m_pLogHandler, "", 5, "X", TAG, "EzEncoder:video encode error");
		VideoEncoderDestroy();
		return -1;
	}

	return 0;
}

BOOL_ CEzEncoder::VideoEncoderInitialize(int nCodecID, int nImageWidth, int nImageHeight, int nLevel, int nBitRate, int nFrameRate, int nGopSize, int nStreamType)
{
	VideoEncoderDestroy();
	
    //Find the video encoder 
    m_pCodecVE = avcodec_find_encoder((CodecID)nCodecID);
    if (!m_pCodecVE)
		return FALSE_;
	
	if (!m_pContextVE)
	{
		//////////////////////////////////////////////////////////////////////////
		//jacky, to be verfied
		if (nCodecID == CODEC_ID_H264)
			nBitRate /= 1024;

		m_pContextVE = avcodec_alloc_context();
		m_pContextVE->codec_id			= (CodecID)nCodecID;
		m_pContextVE->codec_type		= AVMEDIA_TYPE_VIDEO;
		m_pContextVE->width				= nImageWidth;
		m_pContextVE->height			= nImageHeight;
		m_pContextVE->time_base.den		= nFrameRate;             //frame rate
		m_pContextVE->time_base.num		= 1;
		//m_pContextVE->time_base.den	= int(nFrameRate*100+0.5);
		//m_pContextVE->time_base.num	= 100;
		m_pContextVE->gop_size			= nFrameRate;//nGopSize;
		m_pContextVE->max_b_frames		= 0;
		m_pContextVE->pix_fmt			= PIX_FMT_YUV420P;
		m_pContextVE->bit_rate			= nBitRate;


		if (CODEC_ID_H264 == nCodecID)
		{
			m_pContextVE->has_b_frames		= 0;
			m_pContextVE->thread_count		= 1;

			int p = 5;
			if (p == 1)
			{
				m_pContextVE->profile			= 66;
				if (nLevel == 1)
					m_pContextVE->level			= 11;
				else if (nLevel == 2)
					m_pContextVE->level			= 12;
				else if (nLevel == 3)
					m_pContextVE->level			= 13;
				else
					m_pContextVE->level			= nLevel;

				m_pContextVE->me_range			= 16; 
				m_pContextVE->qmin				= 0; 
				m_pContextVE->qmax				= 69; 
				m_pContextVE->max_qdiff			= 4; 
				m_pContextVE->qcompress			= 0.6f;
				m_pContextVE->cqp				= 0;
				m_pContextVE->directpred		= 1;
				m_pContextVE->b_frame_strategy	= 1;
				m_pContextVE->i_quant_factor	= 0.71f;
				m_pContextVE->scenechange_threshold = 40;

				m_pContextVE->bit_rate			= nBitRate;
				m_pContextVE->rc_min_rate		= nBitRate;
				m_pContextVE->rc_max_rate		= nBitRate;
				m_pContextVE->bit_rate_tolerance= nBitRate;
				m_pContextVE->rc_buffer_size	= nBitRate;
				m_pContextVE->rc_initial_buffer_occupancy = m_pContextVE->rc_buffer_size*3/4;
				m_pContextVE->rc_buffer_aggressivity = (float)1.0;
				m_pContextVE->rc_initial_cplx	= 0.5; 
			}
			else if (p == 2)
			{
				m_pContextVE->profile			= 66;

				m_pContextVE->dct_algo			= 0;
				m_pContextVE->gop_size			= 12; 
				m_pContextVE->me_pre_cmp		= 2;
				m_pContextVE->cqp				= 26;
				m_pContextVE->me_method			= 7;
				m_pContextVE->qmin				= 3;
				m_pContextVE->qmax				= 31;
				m_pContextVE->max_qdiff			= 3;
				m_pContextVE->qcompress			= 0.5;
				m_pContextVE->qblur				= 0.5;
				m_pContextVE->nsse_weight		= 8;
				m_pContextVE->i_quant_factor	= (float)0.8;
				m_pContextVE->b_quant_factor	= 1.25;
				m_pContextVE->b_quant_offset	= 1.25;
			}
			else if (p == 3)
			{
				m_pContextVE->bit_rate			= nBitRate;
				m_pContextVE->rc_min_rate		= nBitRate;
				m_pContextVE->rc_max_rate		= nBitRate;
				m_pContextVE->bit_rate_tolerance= nBitRate;
				m_pContextVE->rc_buffer_size	= nBitRate;
				m_pContextVE->rc_initial_buffer_occupancy = m_pContextVE->rc_buffer_size*3/4;
				m_pContextVE->rc_buffer_aggressivity = (float)1.0;
				m_pContextVE->rc_initial_cplx	= 0.5; 
			}
			else if (p == 4)
			{
				/*
				coder=0
				bf=0
				flags2=-wpred-dct8x8
				wpredp=0
				*/
				m_pContextVE->profile			= 66;
				m_pContextVE->coder_type		= 0;
				m_pContextVE->bframebias		= 0;
				m_pContextVE->flags2			= CODEC_FLAG2_WPRED | CODEC_FLAG2_8X8DCT;
				m_pContextVE->weighted_p_pred	= 0;
				m_pContextVE->level				= 13;

				m_pContextVE->me_range			= 16; 
				m_pContextVE->qmin				= 0; 
				m_pContextVE->qmax				= 69; 
				m_pContextVE->max_qdiff			= 4; 
				m_pContextVE->qcompress			= 0.6f;
				m_pContextVE->cqp				= 0;
			}
			else if (p == 5)
			{
				m_pContextVE->profile			= 66;

				m_pContextVE->me_range			= 16;
				m_pContextVE->qmin				= 10;
				m_pContextVE->qmax				= 51;
				m_pContextVE->max_qdiff			= 4;
				m_pContextVE->qcompress			= 0.6f;

				m_pContextVE->cqp				= 26;
/*
				m_pContextVE->me_range			= 16; 
				m_pContextVE->qmin				= 0; 
				m_pContextVE->qmax				= 69; 
				m_pContextVE->max_qdiff			= 4; 
				m_pContextVE->qcompress			= 0.6f;
				m_pContextVE->cqp				= 0;
				m_pContextVE->directpred		= 1;
				m_pContextVE->b_frame_strategy	= 1;
				m_pContextVE->i_quant_factor	= 0.71f;
				m_pContextVE->scenechange_threshold = 40;
*/
			}
			else if (p == 6)
			{
			}

//			m_pContextVE->bit_rate_tolerance	= nBitRate;
		}
		else
		{
			//CBR
			if (1)
			{
//				m_pContextVE->rc_min_rate	= nBitRate;
//				m_pContextVE->rc_max_rate	= nBitRate;
				m_pContextVE->bit_rate_tolerance= nBitRate;
//				m_pContextVE->rc_buffer_size= nBitRate;
//				m_pContextVE->rc_initial_buffer_occupancy = m_pContextVE->rc_buffer_size*3/4;
//				m_pContextVE->rc_buffer_aggressivity= (float)1.0;
//				m_pContextVE->rc_initial_cplx= 0.5;
			}
			else
			{
				int minbr					= 8*1000;
				int maxbr					= 80*1000;
				m_pContextVE->flags			|= CODEC_FLAG_QSCALE;
				m_pContextVE->rc_min_rate	= minbr;
				m_pContextVE->rc_max_rate	= maxbr; 
				m_pContextVE->bit_rate		= nBitRate;
			}
		}
	}

	//m_pContextVE->flags |= CODEC_FLAG_GLOBAL_HEADER;

    // Open the codec
	pthread_mutex_lock(&g_vFFMPEGLock);
    if (avcodec_open(m_pContextVE, m_pCodecVE) < 0)
	{
		av_free(m_pContextVE);
		m_pContextVE = NULL;
		pthread_mutex_unlock(&g_vFFMPEGLock);
		return FALSE_;
    }
	pthread_mutex_unlock(&g_vFFMPEGLock);

	if (!m_pVideoEncBuffer)
		m_pVideoEncBuffer = new uint8_t[VIDEO_ENCODE_SIZE];
	if (!m_pVideoEncBuffer)
		return FALSE_;

	m_nVCodecID			= nCodecID;
	//m_nStreamType		= nStreamType;
#ifdef __RG4_ENABLE_WATERMARK
	m_pRsFilter			= new CRsFilter();
	if (m_bTextOSD || m_bTimeOSD)
	{
		if (!m_pRsFilter->InitFilter(PIX_FMT_YUV420P, nImageWidth, nImageHeight, nFrameRate, m_pContextVE->sample_aspect_ratio.num/m_pContextVE->sample_aspect_ratio.den, m_bTimeOSD, m_bTextOSD, m_strOSDFilter.c_str()))
		{
			if (m_fpLogReport)
				m_fpLogReport(NULL, "", LOGLEVEL1, "E", TAG, "Initialize Filter Failed!");
			delete m_pRsFilter;
			m_pRsFilter = NULL;
		}
	}
#endif

	return TRUE_;
}

BOOL_ CEzEncoder::VideoEncoderDestroy()
{
	//////////////////////////////////////////////////////////////////////////
	//filter
#ifdef __RG4_ENABLE_WATERMARK
	BOOL_ bFreePicture = TRUE_;
	if (m_pRsFilter)
	{
		bFreePicture = FALSE_;
		delete m_pRsFilter;
		m_pRsFilter = NULL;
	}
#endif
	if (m_pContextVE)
	{
		pthread_mutex_lock(&g_vFFMPEGLock);
		avcodec_close(m_pContextVE);
		av_free(m_pContextVE);
		m_pContextVE = NULL;
		pthread_mutex_unlock(&g_vFFMPEGLock);
	}
	if (m_pPicture)
	{
#ifdef __RG4_ENABLE_WATERMARK
		if (bFreePicture)
#endif
		{
			if (m_pPicture->data[0])
				free(m_pPicture->data[0]);
		}

		av_free(m_pPicture);
		m_pPicture = NULL;
	}
	
	if (m_pSwsContext)
	{
		sws_freeContext(m_pSwsContext);
		m_pSwsContext = NULL;
	}

	if (m_pVideoEncBuffer)
	{
		delete[] m_pVideoEncBuffer;
		m_pVideoEncBuffer = NULL;
	}

	return TRUE_;
}

#define MAX_PSET_SIZE 1024
char *CEzEncoder::extradata2psets_h264(AVCodecContext *c)
{
	char *psets, *p;
	const uint8_t *r;
	const char *pset_string = "; sprop-parameter-sets=";

	if (c->extradata_size > 65536) {
		av_log(c, AV_LOG_ERROR, "Too many extra data!\n");

		return NULL;
	}

	psets = (char*)av_mallocz(MAX_PSET_SIZE);
	if (psets == NULL) {
		av_log(c, AV_LOG_ERROR, "Cannot allocate memory for the parameter sets\n");
		return NULL;
	}
	memcpy(psets, pset_string, strlen(pset_string));
	p = psets + strlen(pset_string);
	r = ff_avc_find_startcode(c->extradata, c->extradata + c->extradata_size);
	while (r < c->extradata + c->extradata_size) {
		const uint8_t *r1;

		while (!*(r++));
		r1 = ff_avc_find_startcode(r, c->extradata + c->extradata_size);
		if (p != (psets + strlen(pset_string))) {
			*p = ',';
			p++;
		}
		if (av_base64_encode(p, MAX_PSET_SIZE - (p - psets), r, r1 - r) == NULL) {
			av_log(c, AV_LOG_ERROR, "Cannot BASE64 encode %td %td!\n", MAX_PSET_SIZE - (p - psets), r1 - r);
			av_free(psets);

			return NULL;
		}
		p += strlen(p);
		r = r1;
	}

	return psets;
}


const uint8_t *CEzEncoder::ff_avc_find_startcode(const uint8_t *p, const uint8_t *end)
{
	const uint8_t *a = p + 4 - ((long)p & 3);

	for( end -= 3; p < a && p < end; p++ ) {
		if( p[0] == 0 && p[1] == 0 && p[2] == 1 )
			return p;
	}

	for( end -= 3; p < end; p += 4 ) {
		uint32_t x = *(const uint32_t*)p;
		//      if( (x - 0x01000100) & (~x) & 0x80008000 ) // little endian
		//      if( (x - 0x00010001) & (~x) & 0x00800080 ) // big endian
		if( (x - 0x01010101) & (~x) & 0x80808080 ) { // generic
			if( p[1] == 0 ) {
				if( p[0] == 0 && p[2] == 1 )
					return p-1;
				if( p[2] == 0 && p[3] == 1 )
					return p;
			}
			if( p[3] == 0 ) {
				if( p[2] == 0 && p[4] == 1 )
					return p+1;
				if( p[4] == 0 && p[5] == 1 )
					return p+2;
			}
		}
	}

	for( end += 3; p < end; p++ ) {
		if( p[0] == 0 && p[1] == 0 && p[2] == 1 )
			return p;
	}

	return end + 3;
}


char *CEzEncoder::av_base64_encode(char * buf, int buf_len, const uint8_t * src, int len)
{
	static const char b64[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	char *ret, *dst;
	unsigned i_bits = 0;
	int i_shift = 0;
	int bytes_remaining = len;

	if (len >= UINT_MAX / 4 ||
		buf_len < len * 4 / 3 + 12)
		return NULL;
	ret = dst = buf;
	while (bytes_remaining) {
		i_bits = (i_bits << 8) + *src++;
		bytes_remaining--;
		i_shift += 8;

		do {
			*dst++ = b64[(i_bits << 6 >> i_shift) & 0x3f];
			i_shift -= 6;
		} while (i_shift > 6 || (bytes_remaining == 0 && i_shift > 0));
	}
	while ((dst - ret) & 3)
		*dst++ = '=';
	*dst = '\0';

	return ret;
}

char *CEzEncoder::extradata2psets_mpeg4(AVCodecContext *c)
{
	char *psets, *p;
	const uint8_t *r;
	const char *pset_string = "; sprop-parameter-sets=";

	if (c->extradata_size > 65536) {
		av_log(c, AV_LOG_ERROR, "Too many extra data!\n");

		return NULL;
	}

	psets = (char*)av_mallocz(MAX_PSET_SIZE);
	if (psets == NULL) {
		av_log(c, AV_LOG_ERROR, "Cannot allocate memory for the parameter sets\n");
		return NULL;
	}
	memcpy(psets, pset_string, strlen(pset_string));
	p = psets + strlen(pset_string);
	r = ff_avc_find_startcode(c->extradata, c->extradata + c->extradata_size);
	while (r < c->extradata + c->extradata_size) {
		const uint8_t *r1;

		while (!*(r++));
		r1 = ff_avc_find_startcode(r, c->extradata + c->extradata_size);
		if (p != (psets + strlen(pset_string))) {
			*p = ',';
			p++;
		}
		if (av_base64_encode(p, MAX_PSET_SIZE - (p - psets), r, r1 - r) == NULL) {
			av_log(c, AV_LOG_ERROR, "Cannot BASE64 encode %td %td!\n", MAX_PSET_SIZE - (p - psets), r1 - r);
			av_free(psets);

			return NULL;
		}
		p += strlen(p);
		r = r1;
	}

	return psets;
}

int	CEzEncoder::AudioEncodeFrame(uint8_t* pSrcData, int nSrcBytes, int nSrcPixFmt, int nSrcChannel, int nSrcBitsPerSample, int nSrcSamplePerSec, int nDstCodecID, int nDstChannels, int nDstBitsPerSample, int nDstSamplePerSec, int nDstBitRate, int nStreamType, unsigned long long llTimestamp)
//int CEzEncoder::AudioEncodeFrame(uint8_t* pSrcData, int nSrcBytes,			   int nSrcChannel, int nSrcBitPerSample, int nSrcSampleRate, audio_format* pDstFormat)
{
	if (NULL == m_pAContext)
	{
		if (!AudioEncoderInitialize(nDstCodecID, nDstChannels, nDstBitsPerSample, nDstSamplePerSec, nDstBitRate))
			return -1;
	}

	//ReSample
	int isize= nSrcBitsPerSample/8;
	int osize= av_get_bits_per_sample_format(m_pAContext->sample_fmt)/8;
	int16_t *out_buffer = NULL;
	int out_size = 0;
	int frame_bytes;
	int ret;

	if (nDstChannels != nSrcChannel || nDstSamplePerSec != nSrcSamplePerSec)
	{
		int resample_size = 0;
		if (NULL == m_pReSampleCtx)
			m_pReSampleCtx = av_audio_resample_init(
			nDstChannels,
			nSrcChannel, 
			nDstSamplePerSec, 
			nSrcSamplePerSec, 
			m_pAContext->sample_fmt, 
			m_pAContext->sample_fmt, 
			16, 10, 0, 0.8);
		if (m_pReSampleCtx)
		{
			if (NULL == m_pReSampleBuffer)
				m_pReSampleBuffer = (short *)malloc(AUDIO_QUEUE_SIZE);
			if (NULL == m_pReSampleBuffer)
				return -1;
			int nb_resamples = audio_resample(m_pReSampleCtx, m_pReSampleBuffer, (short *)pSrcData, nSrcBytes/(nSrcChannel* isize));
			if (nb_resamples > 0)
			{
				resample_size = nb_resamples * m_pAContext->channels * osize;
			}
		}

		out_buffer = m_pReSampleBuffer;
		out_size = resample_size;
	}
	else
	{
		out_buffer = (int16_t *)pSrcData;
		out_size = nSrcBytes;
	}

	/* now encode as many frames as possible */
	if (m_pAContext->frame_size > 1)
	{
		if (NULL == m_pFIFOBuffer)
		{
			m_pFIFOBuffer = new uint8_t[AUDIO_QUEUE_SIZE];
			m_nFIFOWriteOffset = 0;
			m_nFIFOReadOffset = 0;
		}
		if (NULL == m_pFIFOBuffer)
			return -1;

		if(m_nFIFOWriteOffset+out_size >= AUDIO_QUEUE_SIZE)
		{
			long nBufferSize = m_nFIFOWriteOffset-m_nFIFOReadOffset;
			if(nBufferSize + out_size >= AUDIO_QUEUE_SIZE)
			{
				m_nFIFOWriteOffset = 0;
				m_nFIFOReadOffset = 0;
			}
			else
			{
				memmove(m_pFIFOBuffer, m_pFIFOBuffer+m_nFIFOReadOffset, nBufferSize);
				m_nFIFOReadOffset	= 0;
				m_nFIFOWriteOffset	= nBufferSize;
			}
		}

		memcpy(m_pFIFOBuffer+m_nFIFOWriteOffset, out_buffer, out_size);
		m_nFIFOWriteOffset += out_size;

		frame_bytes = m_pAContext->frame_size * osize * m_pAContext->channels;

		while ((m_nFIFOWriteOffset-m_nFIFOReadOffset) >= frame_bytes)
		{
			if (NULL == m_pAudioBuffer)
				m_pAudioBuffer = new uint8_t[AUDIO_QUEUE_SIZE];
			if (NULL == m_pAudioBuffer)
				return -1;

			memcpy(m_pAudioBuffer, m_pFIFOBuffer+m_nFIFOReadOffset, frame_bytes);
			m_nFIFOReadOffset += frame_bytes;

			const int audio_out_size= AUDIO_QUEUE_SIZE;
			ret = avcodec_encode_audio(m_pAContext, m_pAudioEncBuffer, audio_out_size, (short *)m_pAudioBuffer);
			if (ret < 0) {
				return -1;
			}

			if (m_pAudioParam && m_fpAudioEncodeCallback)
			{
				//m_fpAudioEncodeCallback(m_pAudioParam, m_pAudioEncBuffer, ret, (int)m_pAContext->codec_id, m_pAContext->channels, m_pAContext->bits_per_sample, m_pAContext->sample_rate);
#if FFMPEG_0_5
				m_fpAudioEncodeCallback(nStreamType, m_pAudioParam, m_pAudioEncBuffer, out_size, nDstCodecID, m_pAContext->channels, m_pAContext->bits_per_sample, m_pAContext->sample_rate,"", llTimestamp);
#else
				m_fpAudioEncodeCallback(nStreamType, m_pAudioParam, m_pAudioEncBuffer, ret, nDstCodecID, m_pAContext->channels, 0, m_pAContext->sample_rate,"", llTimestamp);
#endif
			}
		}
	}
	else
	{
		int coded_bps = av_get_bits_per_sample(m_pAContext->codec->id)/8;

		/* output a pcm frame */
		/* determine the size of the coded buffer */
		out_size /= osize;
		if (coded_bps)
			out_size *= coded_bps;

		//FIXME pass ost->sync_opts as AVFrame.pts in avcodec_encode_audio()
		ret = avcodec_encode_audio(m_pAContext, m_pAudioEncBuffer, out_size, (short *)out_buffer);
		if (ret < 0)
		{
			return -1;
		}

		if (m_pAudioParam && m_fpAudioEncodeCallback)
		{
			//m_fpAudioEncodeCallback(m_pAudioParam, m_pAudioEncBuffer, ret, (int)m_pAContext->codec_id, m_pAContext->channels, m_pAContext->bits_per_sample, m_pAContext->sample_rate);
#if FFMPEG_0_5
			m_fpAudioEncodeCallback(nStreamType, m_pAudioParam, m_pAudioEncBuffer, out_size, nDstCodecID, m_pAContext->channels, m_pAContext->bits_per_sample, m_pAContext->sample_rate,"", llTimestamp);
#else
			m_fpAudioEncodeCallback(nStreamType, m_pAudioParam, m_pAudioEncBuffer, out_size, nDstCodecID, m_pAContext->channels, 0, m_pAContext->sample_rate,"", llTimestamp);
#endif
		}
	}

	return 0;
}

BOOL_ CEzEncoder::AudioEncoderInitialize(int nCodecID, int nChannel, int nBitsPerSample, int nSampleRate, int nBitRate)
{
	AudioEncoderDestroy();

	m_pACodec = avcodec_find_encoder((CodecID)nCodecID);

	if(!m_pACodec) return FALSE_;

	if(!m_pAContext)
	{
		m_pAContext						= avcodec_alloc_context();
		m_pAContext->channels			= nChannel;
#if FFMPEG_0_5
		m_pAContext->bits_per_sample	= nBitsPerSample;
#endif
		//m_pAContext->sample_rate		= 128000;
		m_pAContext->sample_rate		= nSampleRate;
		m_pAContext->codec_id			= (CodecID)nCodecID;
		m_pAContext->codec_type			= AVMEDIA_TYPE_AUDIO;//CODEC_TYPE_AUDIO;
		m_pAContext->bit_rate			= nSampleRate*nChannel*nBitsPerSample;
		m_pAContext->bit_rate			= nBitRate;
		m_pAContext->sample_fmt			= SAMPLE_FMT_U8;//SAMPLE_FMT_S16;
		m_pAContext->sample_fmt			= SAMPLE_FMT_NB;//WHY???  Number of sample formats. DO NOT USE if dynamically linking to libavcodec
		m_pAContext->sample_fmt			= SAMPLE_FMT_S16;
	}

	pthread_mutex_lock(&g_vFFMPEGLock);
	if (avcodec_open(m_pAContext, m_pACodec) < 0)
	{
		pthread_mutex_unlock(&g_vFFMPEGLock);
		AudioEncoderDestroy();
		return FALSE_;
	}
	pthread_mutex_unlock(&g_vFFMPEGLock);

	if (!m_pAudioEncBuffer)
		m_pAudioEncBuffer = new uint8_t[AUDIO_ENCODE_SIZE];
	if (!m_pAudioEncBuffer)
		return FALSE_;

	return TRUE_;
}

BOOL_ CEzEncoder::AudioEncoderDestroy()
{
	if (m_pAContext)
	{
		pthread_mutex_lock(&g_vFFMPEGLock);
		avcodec_close(m_pAContext);
		av_free(m_pAContext);
		m_pAContext = NULL;
		m_pACodec = NULL;
		pthread_mutex_unlock(&g_vFFMPEGLock);
	}
	if (m_pReSampleBuffer)
	{
		free(m_pReSampleBuffer);
		m_pReSampleBuffer = NULL;
	}

	// 	if (m_pFifoBuffer)
	// 	{
	// 		av_fifo_free(m_pFifoBuffer);
	// 		delete m_pFifoBuffer;
	// 		m_pFifoBuffer = NULL;
	// 	}

	if (m_pReSampleCtx)
	{
		audio_resample_close(m_pReSampleCtx);
		m_pReSampleCtx = NULL;
	}

	if (m_pAudioEncBuffer)
	{
		delete[] m_pAudioEncBuffer;
		m_pAudioEncBuffer = NULL;
	}

	if (m_pAudioBuffer)
	{
		delete[] m_pAudioBuffer;
		m_pAudioBuffer = NULL;
	}

	if (m_pFIFOBuffer)
	{
		delete[] m_pFIFOBuffer;
		m_pFIFOBuffer = NULL;
	}

	return TRUE_;
}

AVStream* CEzEncoder::Add_Audio_Stream(AVFormatContext* pFmtCtx)
{
	AVStream *st;

	if (NULL == pFmtCtx)
		return NULL;

	st = av_new_stream(pFmtCtx, 0);
	if (!st) {
		return NULL;
	}

	return st;
}

AVStream* CEzEncoder::Add_Video_Stream(AVFormatContext* pFmtCtx)
{
	AVStream *st;

	if (NULL == pFmtCtx)
		return NULL;

	st = av_new_stream(pFmtCtx, 1);
	if (!st) {
		return NULL;
	}

	return st;
}

void CEzEncoder::Write_Video_Frame(uint8_t *pData, int nSize, int isKeyFrame)
{
	if (NULL == m_pFormatCtx)
		return;

	av_init_packet(&m_Packet);

	if(1 == isKeyFrame)
	{
		m_Packet.flags |= AV_PKT_FLAG_KEY;//PKT_FLAG_KEY;
	}
	m_Packet.pts= av_rescale_q(m_pVideoStream->codec->coded_frame->pts, m_pVideoStream->codec->time_base, m_pVideoStream->time_base);

	m_Packet.stream_index= m_pVideoStream->index;
	m_Packet.data = pData;
	m_Packet.size = nSize;

	int nRet = /*av_write_frame*/av_interleaved_write_frame(m_pFormatCtx, &m_Packet);

	//TRACE("## write video frame, size = %d, isKey = %d, ret = %d ##\n", nSize, isKeyFrame, nRet);

	//	double audio_st_pts = (double)m_pAudioStream->pts.val * m_pAudioStream->time_base.num / m_pAudioStream->time_base.den;
	//	double video_st_pts = (double)m_pVideoStream->pts.val * m_pVideoStream->time_base.num / m_pVideoStream->time_base.den;
	//	TRACE("222.write video: audio_st_pts=%f, video_st_pts=%f, size=%d\n", audio_st_pts, video_st_pts, nSize);
}

void CEzEncoder::Write_Audio_Frame(uint8_t *pData, int nSize)
{
	if (NULL == m_pFormatCtx)
		return;

	av_init_packet(&m_Packet);
	m_Packet.pts= av_rescale_q(m_pAudioStream->codec->coded_frame->pts, m_pAudioStream->codec->time_base, m_pAudioStream->time_base);

	m_Packet.stream_index= m_pAudioStream->index;
	m_Packet.flags |= AV_PKT_FLAG_KEY;//PKT_FLAG_KEY;
	m_Packet.data = pData;
	m_Packet.size = nSize;

	int nRet = /*av_write_frame*/av_interleaved_write_frame(m_pFormatCtx, &m_Packet);

	//TRACE("## write audio frame, size = %d, ret = %d ##\n", nSize, nRet);

	//	double audio_st_pts = (double)m_pAudioStream->pts.val * m_pAudioStream->time_base.num / m_pAudioStream->time_base.den;
	//	double video_st_pts = (double)m_pVideoStream->pts.val * m_pVideoStream->time_base.num / m_pVideoStream->time_base.den;
	//	TRACE("111.write audio: audio_st_pts=%f, video_st_pts=%f, size=%d\n", audio_st_pts, video_st_pts, nSize);
}

BOOL_ CEzEncoder::CreateAviFile(char *pFileName, int nACodecID, int nSampleRate, int nBitsPerSample, int nChannels, int nVCodecID, int nVBitRate, int nFrameRate, int nImageWidth, int nImageHeight, int nGopSize)
{
	AVOutputFormat* pOutputFmt = av_guess_format("avi", NULL, NULL);
	if (!pOutputFmt) {
		return FALSE_;
	}
	if (nACodecID != CODEC_ID_NONE)
		pOutputFmt->audio_codec = (CodecID)nACodecID;
	else
		pOutputFmt->audio_codec = (CodecID)CODEC_ID_NONE;

	if (nVCodecID != CODEC_ID_NONE)
		pOutputFmt->video_codec = (CodecID)nVCodecID;
	else
		pOutputFmt->video_codec = (CodecID)CODEC_ID_NONE;

	// allocate the output media context
	m_pFormatCtx = avformat_alloc_context();
	if (!m_pFormatCtx) {
		return FALSE_;
	}
	m_pFormatCtx->oformat = pOutputFmt;
	strcpy_s(m_pFormatCtx->filename, 1024, pFileName);

	if (pOutputFmt->audio_codec != CODEC_ID_NONE)
	{
		m_pAudioStream = Add_Audio_Stream(m_pFormatCtx);
		if (m_pAudioStream)
		{
			//if (!AudioEncoderInitialize(nACodecID, nChannels, nBitsPerSample, nSampleRate))
			//	return FALSE_;
		}
		else
			return FALSE_;
	}

	if (pOutputFmt->video_codec != CODEC_ID_NONE)
	{
		m_pVideoStream = Add_Video_Stream(m_pFormatCtx);
		if (m_pVideoStream)
		{
			//if (!VideoEncoderInitialize(nVCodecID, nImageWidth, nImageHeight, nVBitRate, nFrameRate, nGopSize))
			//	return FALSE_;
		}
		else
			return FALSE_;
	}

	if (av_set_parameters(m_pFormatCtx, NULL) < 0) 
	{
		return FALSE_;
	}
	/* open the output file, if needed */
	if (!(m_pFormatCtx->flags & AVFMT_NOFILE))
	{
		if (url_fopen(&m_pFormatCtx->pb, m_pFormatCtx->filename, URL_WRONLY) < 0) 
		{
			return FALSE_;
		}
	}

	/*m_vAVStatus.nFileOpened = 1;

	m_vAVParams.nACodecID = nACodecID;
	m_vAVParams.nChannels = nChannels;
	m_vAVParams.nBitsPerSample = nBitsPerSample;
	m_vAVParams.nSampleRate = nSampleRate;

	m_vAVParams.nVCodecID = nVCodecID;
	m_vAVParams.nFrameRate = nFrameRate;
	m_vAVParams.nVBitRate = nVBitRate;
	m_vAVParams.nGopSize = nGopSize;
	m_vAVParams.nImageWidth = nImageWidth;
	m_vAVParams.nImageHeight = nImageHeight;*/

	/* write the stream header, if any */
	av_write_header(m_pFormatCtx);
	//m_vAVStatus.nHeaderWrited = 1;

	return TRUE_;
}
