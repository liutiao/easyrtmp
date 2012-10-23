
#include "FFmpegCodec.h"
//#include "MainService.h"
//#include "ColorSpace.h"

pthread_mutex_t g_vFFMPEGLock;
BOOL_ s_bFFMPGELock = FALSE_;

#ifdef WIN32
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "avformat.lib")
#pragma comment(lib, "swscale.lib")
#endif

CFFmpegCodec::CFFmpegCodec()
{
	strcpy(TAG, "FFmpegCodec");
	if (!s_bFFMPGELock)
	{
		pthread_mutex_init(&g_vFFMPEGLock, NULL);
		s_bFFMPGELock = TRUE_;
	}
//	m_pColorSpace		= NULL;

	m_szCodecName		= "FFMPEG_CODEC";
	m_nCodecType		= AV_CODEC_NONE;

	m_pYUVFrame1		= NULL;
	m_pYUVFrame2		= NULL;
	m_pYUVMergeFrame	= NULL;
	m_pDeinterlace		= NULL;
	
	m_pCodecAD			= NULL;
	m_pContextAD		= NULL;
	m_pCodecVD			= NULL;
	m_pContextVD		= NULL;

	m_pCodecAE			= NULL;
	m_pContextAE		= NULL;
	m_pCodecVE			= NULL;
	m_pContextVE		= NULL;

	m_pPicture			= NULL;	
	m_pVideoBuffer		= NULL;
	m_pAudioBuffer		= NULL;
	m_pSample			= NULL;
	m_pSwsContext		= NULL;

	m_pAudioDecBuffer	= NULL;
	m_pAudioEncBuffer	= NULL;
	m_pVideoDecBuffer	= NULL;
	m_pVideoEncBuffer	= NULL;

    avcodec_init();
	avcodec_register_all();
}

CFFmpegCodec::~CFFmpegCodec()
{
	//if (m_pColorSpace)
	//{
	//	delete m_pColorSpace;
	//	m_pColorSpace = NULL;
	//}

	ResetCodec();

	if (m_nCodecType == AUDIO_DECODE)
		AudioDecoderDestroy();
	if (m_nCodecType == VIDEO_DECODE)
		VideoDecoderDestroy();
	if (m_nCodecType == AUDIO_ENCODE)
		AudioEncoderDestroy();
	if (m_nCodecType == VIDEO_ENCODE)
		VideoEncoderDestroy();
}

BOOL_ CFFmpegCodec::AudioDecoderInitialize(int nCodecID, int nChannel, int nBitRate, int nBitsPerSample, int nSampleRate)
{
	AudioDecoderDestroy();

	m_nCodecType = AUDIO_DECODE;
	/*
	switch(nCodecID)
	{
	case ATYPE_ADPCM_IMA_WAV:
		m_pCodecAD = avcodec_find_decoder(CODEC_ID_ADPCM_IMA_WAV);
		break;
	case ATYPE_PCM_MULAW:
		m_pCodecAD = avcodec_find_decoder(CODEC_ID_PCM_MULAW);
		break;
	case ATYPE_FFMPEG_MP2:
		m_pCodecAD = avcodec_find_decoder(CODEC_ID_MP2);
		break;
	case ATYPE_FFMPEG_MP3:
		m_pCodecAD = avcodec_find_decoder(CODEC_ID_MP3);
		break;
	case ATYPE_ADPCM_G726_24:
	case ATYPE_ADPCM_G726_32:
	case ATYPE_ADPCM_G726_40:
		m_pCodecAD = avcodec_find_decoder(CODEC_ID_ADPCM_G726);
		break;
	default:
		m_pCodecAD = avcodec_find_decoder(CODEC_ID_MP3);
		break;
	}
	*/
	m_pCodecAD = avcodec_find_decoder((CodecID)nCodecID);
	
	if(!m_pCodecAD)
		return FALSE_;
	
	if(!m_pContextAD)
	{
		m_pContextAD = avcodec_alloc_context();
		m_pContextAD->bit_rate = nBitRate;
		m_pContextAD->channels = nChannel;
//		m_pContextAD->bits_per_sample = nBitsPerSample;
		m_pContextAD->sample_rate = nSampleRate;
	}
	
	pthread_mutex_lock(&g_vFFMPEGLock);
	if (avcodec_open(m_pContextAD, m_pCodecAD) < 0) 
	{
		pthread_mutex_unlock(&g_vFFMPEGLock);
		av_free(m_pContextAD);
		m_pContextAD = NULL;
		return FALSE_;
	}
	pthread_mutex_unlock(&g_vFFMPEGLock);

	if (!m_pAudioDecBuffer)
	{
		m_pAudioDecBuffer = new unsigned char[AUDIO_DECODE_SIZE];
	}

	m_nCodecID = nCodecID;
	return TRUE_;
}

BOOL_ CFFmpegCodec::VideoDecoderInitialize(int nCodecID, int nImageWidth, int nImageHeight)
{
	VideoDecoderDestroy();

	m_nCodecType = VIDEO_DECODE;

	/*
	switch(nCodecID)
	{
	case VTYPE_FFMPEG_H264:
		m_pCodecVD = avcodec_find_decoder(CODEC_ID_H264);
		break;
	case VTYPE_JPEG:
		m_pCodecVD = avcodec_find_decoder(CODEC_ID_MJPEG);
		break;
	case VTYPE_FFMPEG_MPEG4:
		m_pCodecVD = avcodec_find_decoder(CODEC_ID_MPEG4);
		break;
	case VTYPE_FFMPEG_MPEG2:
		m_pCodecVD = avcodec_find_decoder(CODEC_ID_MPEG2VIDEO);
		break;
	default:
		m_pCodecVD = avcodec_find_decoder(CODEC_ID_MPEG4);
		break;
	}
	*/
	m_pCodecVD = avcodec_find_decoder((CodecID)nCodecID);

	if (!m_pCodecVD)
	{
		return FALSE_;
	}

	if (!m_pContextVD)
	{
		m_pContextVD				= avcodec_alloc_context();
		m_pContextVD->width		= nImageWidth;
		m_pContextVD->height		= nImageHeight;
		m_pContextVD->pix_fmt	= PIX_FMT_YUV420P;
	}

	//for some codecs, such as msmpeg4 and mpeg4, width and height
	//MUST be initialized there because these info are not available
	//in the bitstream
	//we dont send complete frames
	if (m_pCodecVD->capabilities & CODEC_CAP_TRUNCATED)
		m_pContextVD->flags |= CODEC_FLAG_TRUNCATED;

	// open it
	pthread_mutex_lock(&g_vFFMPEGLock);
	if (avcodec_open(m_pContextVD, m_pCodecVD) < 0)
	{
		av_free(m_pContextVD);
		m_pContextVD = NULL;
		pthread_mutex_unlock(&g_vFFMPEGLock);
		return FALSE_;
	}
	pthread_mutex_unlock(&g_vFFMPEGLock);

	if (!m_pPicture)
		m_pPicture			= avcodec_alloc_frame();

	m_pVideoBuffer			= new unsigned char[VIDEO_QUEUE_SIZE];
	m_dwVideoWriteIndex		= 0;
	m_dwVideoReadIndex		= 0;

	if (!m_pDeinterlace)
	{
		m_pDeinterlace		= alloc_picture(PIX_FMT_YUV420P, nImageWidth, nImageHeight);
	}

	if (!m_pVideoDecBuffer)
	{
		m_pVideoDecBuffer = new unsigned char[VIDEO_DECODE_SIZE];
	}

	m_nCodecID = nCodecID;

	m_nInterleaveCount = 0;
	return TRUE_;
}

BOOL_ CFFmpegCodec::AudioEncoderInitialize(int nCodecID, int nChannel, int nBitRate, int nBitsPerSample, int nSampleRate)
{
	AudioEncoderDestroy();

	m_nCodecType = AUDIO_ENCODE;

	//Find the audio encoder 
	m_pCodecAE = avcodec_find_encoder((CodecID)nCodecID);
	if (!m_pCodecAE) {
		return FALSE_;
	}

	if(!m_pContextAE)
	{
		m_pContextAE = avcodec_alloc_context();
		m_pContextAE->bit_rate		= nChannel*nBitsPerSample*nSampleRate;
		m_pContextAE->channels		= nChannel;
#if FFMPEG_0_5
		m_pContextAE->bits_per_sample = nBitsPerSample;
#endif
		m_pContextAE->sample_rate	= nSampleRate;
		//////////////////////////////////////////////////////////////////////////
		//m_pContextAE->sample_fmt = SAMPLE_FMT_NB;//WHY???  Number of sample formats. DO NOT USE if dynamically linking to libavcodec
		m_pContextAE->sample_fmt		= SAMPLE_FMT_U8;
		m_pContextAE->time_base.den	= 1;
		m_pContextAE->time_base.num	= 10;
	}
	// Open it
	pthread_mutex_lock(&g_vFFMPEGLock);
	if (avcodec_open(m_pContextAE, m_pCodecAE) < 0) {
		pthread_mutex_unlock(&g_vFFMPEGLock);
		return FALSE_;
	}
	pthread_mutex_unlock(&g_vFFMPEGLock);

	if (!m_pSample)
	{
		m_nAudioFrameSize = m_pContextAE->frame_size * 2 * m_pContextAE->channels;
		m_nAudioFrameSize = 600;
		m_pSample = (short *)new char(m_nAudioFrameSize);
	}

	if (!m_pAudioBuffer)
	{
		m_pAudioBuffer = new unsigned char[AUDIO_QUEUE_SIZE];
		m_dwAudioWriteIndex = 0;
		m_dwAudioReadIndex = 0;
	}

	if (!m_pAudioEncBuffer)
	{
		m_pAudioEncBuffer = new unsigned char[AUDIO_ENCODE_SIZE];
	}

	m_nCodecID = nCodecID;

	return TRUE_;
}

BOOL_ CFFmpegCodec::AudioEncoderInitialize_new(int nCodecID, int nChannel, int nBitRate, int nBitsPerSample, int nSampleRate)
{
	AudioEncoderDestroy();

	m_nCodecType = AUDIO_ENCODE;

    //Find the audio encoder 
    m_pCodecAE = avcodec_find_encoder((CodecID)nCodecID);
    if (!m_pCodecAE) {
        return FALSE_;
    }
	
	if(!m_pContextAE)
	{
		m_pContextAE = avcodec_alloc_context();
		m_pContextAE->bit_rate = nBitRate;
		m_pContextAE->channels = nChannel;
//		m_pContextAE->bits_per_sample = nBitsPerSample;
		m_pContextAE->sample_rate = nSampleRate;		
	}
    // Open it
	pthread_mutex_lock(&g_vFFMPEGLock);
    if (avcodec_open(m_pContextAE, m_pCodecAE) < 0) {
		pthread_mutex_unlock(&g_vFFMPEGLock);
		return FALSE_;
    }
	pthread_mutex_unlock(&g_vFFMPEGLock);
	
	if (!m_pSample)
	{
		m_nAudioFrameSize = m_pContextAE->frame_size * 2 * m_pContextAE->channels;
		//	m_nAudioFrameSize = 4096;
		m_pSample = (short *)new char(m_nAudioFrameSize);
	}
	
	if (!m_pAudioBuffer)
	{
		m_pAudioBuffer = new unsigned char[AUDIO_QUEUE_SIZE];
		m_dwAudioWriteIndex = 0;
		m_dwAudioReadIndex = 0;
	}
	
	if (!m_pAudioEncBuffer)
	{
		m_pAudioEncBuffer = new unsigned char[AUDIO_ENCODE_SIZE];
	}

	m_nCodecID = nCodecID;
	
	return TRUE_;
}

BOOL_ CFFmpegCodec::VideoEncoderInitialize(int nCodecID, int nImageWidth, int nImageHeight, int nLevel, int nBitRate, 
										  int nFrameRate, int nGopSize, int nStreamType)
{
	VideoEncoderDestroy();

	m_nCodecType = VIDEO_ENCODE;

    //Find the video encoder
//	nCodecID = CODEC_ID_VC1;
//	nCodecID = CODEC_ID_WMV1;
//	nCodecID = CODEC_ID_MPEG4;
	m_pCodecVE = avcodec_find_encoder((CodecID)nCodecID);
    if (!m_pCodecVE)
	{
		printf("avcodec_find_encoder failed\n");
		return FALSE_;
    }
	
	if (!m_pContextVE)
	{
		m_pContextVE = avcodec_alloc_context();
		m_pContextVE->width		= nImageWidth;
		m_pContextVE->height		= nImageHeight;
		//m_pContextV->time_base.den = nFrameRate;             //frame rate

		//据说在ffmpeg中，time_base只是起到一个时间戳增量的作用，并不能控制帧率，对吧！
		AVRational time_base;
		time_base.num				= 1;
		time_base.den				= nFrameRate;
		m_pContextVE->time_base		= time_base;
		m_pContextVE->time_base.den	= int(nFrameRate*100+0.5);
		m_pContextVE->time_base.num	= 100;

//		m_pContextVE->gop_size	= nGopSize;            /* emit one intra frame every 30 frames at most */ 
		m_pContextVE->max_b_frames	= 0;
		m_pContextVE->pix_fmt		= PIX_FMT_YUV420P;
		m_pContextVE->profile		= 66;
		if (nLevel == 1)
			m_pContextVE->level		= 11;
		else if (nLevel == 2)
			m_pContextVE->level		= 12;
		else if (nLevel == 3)
			m_pContextVE->level		= 13;

		//if (nBitRate == 0)
		//	nBitRate = nImageWidth*nImageHeight*24*10;
		//	nBitRate = nImageWidth*nImageHeight*3;
		//CBR
		if (1)
		{
			m_pContextVE->bit_rate	= nBitRate;
			m_pContextVE->rc_min_rate= nBitRate;
			m_pContextVE->rc_max_rate= nBitRate;
			m_pContextVE->bit_rate_tolerance	= nBitRate;
			m_pContextVE->rc_buffer_size		=nBitRate;
			m_pContextVE->rc_initial_buffer_occupancy = m_pContextVE->rc_buffer_size*3/4;
			m_pContextVE->rc_buffer_aggressivity= (float)1.0;
			m_pContextVE->rc_initial_cplx		= 0.5;
		}
		//VBR
		else
		{
			int minbr							= (int)(nBitRate/5);
			int maxbr							= (int)(nBitRate*1.5);
			m_pContextVE->flags					|= CODEC_FLAG_QSCALE;
			m_pContextVE->rc_min_rate			= minbr;
			m_pContextVE->rc_max_rate			= maxbr;
			m_pContextVE->bit_rate				= nBitRate;
			m_pContextVE->bit_rate_tolerance		= 2;
			//m_pContextVE->rc_max_available_vbv_use =  1.5;
			//m_pContextVE->rc_min_vbv_overflow_use	= 0.5;
		}

//hard code
#ifndef WIN32
		//m_pContextVE->bit_rate = 128000;
		//m_pContextVE->time_base = (AVRational){1,5};
		//m_pContextVE->pix_fmt = PIX_FMT_YUV420P;
		//m_pContextVE->width = 320;
		//m_pContextVE->height = 240;
		//m_pContextVE->gop_size = 10;
		//m_pContextVE->max_b_frames = 1;
#endif
	}
	
	if (nCodecID == CODEC_ID_H264)
	{
	        m_pContextVE->me_range = 16;
	        m_pContextVE->max_qdiff = 4;
	        m_pContextVE->qmin = 10;
	        m_pContextVE->qmax = 51;
	        m_pContextVE->qcompress = 0.6f;
	}
	else if (nCodecID == CODEC_ID_WMV1
		//|| nCodecID == CODEC_ID_WMV2
		|| nCodecID == CODEC_ID_MPEG4
		)
	{
		m_pContextVE->me_range = 16;
		m_pContextVE->max_qdiff = 4;
		m_pContextVE->qmin = 10;
		m_pContextVE->qmax = 51;
		m_pContextVE->qcompress = 0.6f;
	}
	
	printf("avcodec_open :Codec=%d,Width=%d,Height=%d,BitRate=%d,FrameRate=%d,GOP=%d,StreamType=%d\n", 
		nCodecID, nImageWidth, nImageHeight, nBitRate, nFrameRate, nGopSize, nStreamType);

    // Open the codec
	pthread_mutex_lock(&g_vFFMPEGLock);
    if (avcodec_open(m_pContextVE, m_pCodecVE) < 0)
	{
		av_free(m_pContextVE);
		m_pContextVE = NULL;
		pthread_mutex_unlock(&g_vFFMPEGLock);
		printf("avcodec_open failed\n");
		return FALSE_;
    }
	pthread_mutex_unlock(&g_vFFMPEGLock);
	
	/*
	if (m_pContextVE->codec_id == CODEC_ID_MPEG2VIDEO)
        m_pContextVE->max_b_frames = 2;
	
    if (m_pContextVE->codec_id == CODEC_ID_MPEG1VIDEO)
        m_pContextVE->mb_decision=2;
	*/

	/*
	if (!m_pDeinterlace)
	{
		m_pDeinterlace= alloc_picture(PIX_FMT_YUV420P, nImageWidth, nImageHeight);
	}
	*/

	if (!m_pVideoEncBuffer)
	{
		m_pVideoEncBuffer = new unsigned char[VIDEO_ENCODE_SIZE];
	}

	m_nCodecID = nCodecID;
	m_nStreamType = nStreamType;
	
	return TRUE_;
}

int CFFmpegCodec::AudioDecodeFrame(int nCodecID, unsigned char* pEncBuffer, int nEncSize, int nChannel, int nBitRate, int nBitsPerSample, int nSampleRate)
{
	if (!m_pContextAD || m_nCodecID != nCodecID)
	{
		if(!AudioDecoderInitialize(nCodecID, nChannel, nBitRate, nBitsPerSample, nSampleRate))
			return CODEC_INIT_ERROR;
	}
	
	if (m_pContextAD == NULL) return CODEC_ERROR;
	
	int length = 0;
	
	try
	{
		int nDecSize = FFMAX(nEncSize, AVCODEC_MAX_AUDIO_FRAME_SIZE);
		
		AVPacket pkt;
		av_init_packet(&pkt);

		pkt.flags |= AV_PKT_FLAG_KEY;
		//pkt.stream_index = audio_st->index;
		pkt.data = (uint8_t *)pEncBuffer;
		pkt.size = nEncSize;//sizeof(AVPicture);

		length = avcodec_decode_audio3(m_pContextAD, (int16_t*)m_pAudioDecBuffer, &nDecSize, &pkt);
		
		if (length < 0)
		{
			AudioDecoderDestroy();
			return CODEC_ERROR;
		}
		if (nDecSize > 0)
		{
			if (m_fpAudioDecodeCallback)
			{
				m_fpAudioDecodeCallback(m_pAudioParam, m_pAudioDecBuffer, nDecSize, nChannel, nBitRate, nBitsPerSample, nSampleRate);
			}
		}
	}
	catch (...)
	{
		
	}
	return CODEC_SUCCEED;
}

int CFFmpegCodec::VideoDecodeFrame(int nCodecID, unsigned char* pEncBuffer, int nEncSize, int nImageWidth, int nImageHeight)
{
	if (nImageHeight == 0 || nImageWidth == 0)
	{
		return CODEC_INVALID_DATA;
	}
	if(!m_pContextVD || m_nCodecID != nCodecID || nImageWidth != m_pContextVD->width)
	{
		if(!VideoDecoderInitialize(nCodecID, nImageWidth, nImageHeight))
		{
			return CODEC_INIT_ERROR;
		}
	}

	if (m_pContextVD == NULL) 
	{
		return CODEC_INIT_ERROR;
	}

	int length, got_picture;

	if(m_dwVideoWriteIndex+nEncSize >= VIDEO_QUEUE_SIZE)
	{
		unsigned long dwBufferSize = m_dwVideoWriteIndex-m_dwVideoReadIndex;
		if(dwBufferSize + nEncSize >= VIDEO_QUEUE_SIZE)
		{
			m_dwVideoReadIndex = 0;
			m_dwVideoWriteIndex = 0;
		}
		else
		{
			memmove(m_pVideoBuffer, m_pVideoBuffer+m_dwVideoReadIndex, dwBufferSize);
			m_dwVideoReadIndex	= 0;
			m_dwVideoWriteIndex	= dwBufferSize;
		}
	}

	memcpy(m_pVideoBuffer+m_dwVideoWriteIndex, pEncBuffer, nEncSize);
	m_dwVideoWriteIndex += nEncSize;
	
	try
	{
VIDEODECODELOOP:
		got_picture = 0;
		AVPacket pkt;
		av_init_packet(&pkt);

		pkt.flags |= AV_PKT_FLAG_KEY;
		//pkt.stream_index = video_st->index;
		pkt.data = (uint8_t *)m_pVideoBuffer+m_dwVideoReadIndex;
		pkt.size = m_dwVideoWriteIndex-m_dwVideoReadIndex;//sizeof(AVPicture);

		length = avcodec_decode_video2(
			m_pContextVD, 
			m_pPicture, 
			&got_picture, 
			&pkt
			);

		if(length < 0)
		{
			VideoDecoderDestroy();
			return CODEC_ERROR;
		}
		else if (length == 0)
			return CODEC_NOTENOUGH_DATA;
		else if (length > 0)
			m_dwVideoReadIndex += length;

		if (got_picture)
		{
			if (nImageHeight == m_pContextVD->height*2)
			{
				m_bNeedInterleave = TRUE_;
				if (!m_pYUVFrame1)
				{
					m_pYUVFrame1 = alloc_picture(PIX_FMT_YUV420P, m_pContextVD->width, m_pContextVD->height);
				}
				if (!m_pYUVFrame2)
				{
					m_pYUVFrame2 = alloc_picture(PIX_FMT_YUV420P, m_pContextVD->width, m_pContextVD->height);
				}
				if (!m_pYUVMergeFrame)
				{
					m_pYUVMergeFrame = alloc_picture(PIX_FMT_YUV420P, m_pContextVD->width, m_pContextVD->height * 2);
				}
				
				if (m_nInterleaveCount == 0)
				{
					av_picture_copy((AVPicture*)m_pYUVFrame1, (const AVPicture*)m_pPicture, PIX_FMT_YUV420P, m_pContextVD->width, m_pContextVD->height);
					m_nInterleaveCount = 1;
				}
				else if (m_nInterleaveCount == 1)
				{
					av_picture_copy((AVPicture*)m_pYUVFrame2, (const AVPicture*)m_pPicture, PIX_FMT_YUV420P, m_pContextVD->width, m_pContextVD->height);
					m_nInterleaveCount = 0;
				}
			}
			else
			{
				m_bNeedInterleave = FALSE_;
			}
			if (!m_bNeedInterleave)
			{
				int i,j,shift;
				uint8_t *yuv_factor;
				int nDecSize = 0;

				int iRet = avpicture_deinterlace((AVPicture*)m_pDeinterlace, (const AVPicture*)m_pPicture, PIX_FMT_YUV420P, m_pContextVD->width, m_pContextVD->height);
				if (iRet == -1)
					printf("avpicture_deinterlace failed\n");

				int padSize = m_pDeinterlace->linesize[0] - m_pContextVD->width;//32bytes

				for(i = 0; i < 3; i++) 
				{
					shift = (i == 0 ? 0:1);
					yuv_factor = m_pDeinterlace->data[i];
					for(j = 0; j < (m_pContextVD->height>>shift); j++) 
					{
						memcpy(m_pVideoDecBuffer+nDecSize, yuv_factor+(padSize>>(1+shift)), m_pContextVD->width>>shift);
						yuv_factor += m_pDeinterlace->linesize[i];
						nDecSize += m_pContextVD->width>>shift;
					}
				}

				nImageHeight= m_pContextVD->height;
				nImageWidth	= m_pContextVD->width;

				//callback
				if (m_fpVideoDecodeCallback)
				{
					m_fpVideoDecodeCallback(m_pVideoParam, m_pVideoDecBuffer, nDecSize, nImageWidth, nImageHeight, PIX_FMT_YUV420P);//RAW_VIDEO_I420
				}
			}
			else if (m_bNeedInterleave && m_nInterleaveCount == 0)
			{
				int i,j,shift;
				
				m_pYUVMergeFrame->linesize[0] = m_pYUVFrame1->linesize[0];
				m_pYUVMergeFrame->linesize[1] = m_pYUVMergeFrame->linesize[0]/2;
				m_pYUVMergeFrame->linesize[2] = m_pYUVMergeFrame->linesize[0]/2;
				
				uint8_t *yuv_factor1;
				uint8_t *yuv_factor2;
				int yuv_length;
				for(i = 0; i < 3; i++) 
				{
					shift = (i == 0 ? 0:1);
					yuv_factor1 = m_pYUVFrame1->data[i];
					yuv_factor2 = m_pYUVFrame2->data[i];
					yuv_length = 0;
					for(j = 0; j < (m_pContextVD->height>>shift); j++) 
					{
						memcpy(m_pYUVMergeFrame->data[i]+yuv_length, yuv_factor1, m_pYUVFrame1->linesize[i]);
						yuv_factor1 += m_pYUVFrame1->linesize[i];
						yuv_length += m_pYUVMergeFrame->linesize[i];
						
						memcpy(m_pYUVMergeFrame->data[i]+yuv_length, yuv_factor2, m_pYUVFrame2->linesize[i]);
						yuv_factor2 += m_pYUVFrame2->linesize[i];
						yuv_length += m_pYUVMergeFrame->linesize[i];
					}
				}

				int iRet = avpicture_deinterlace((AVPicture*)m_pDeinterlace, (const AVPicture*)m_pYUVMergeFrame, PIX_FMT_YUV420P, m_pContextVD->width, m_pContextVD->height * 2);
				if (iRet == -1)
					printf("avpicture_deinterlace failed\n");

				int padSize = m_pDeinterlace->linesize[0] - m_pContextVD->width;//32bytes
				uint8_t *yuv_factor;
				int nDecSize = 0;
				for(i = 0; i < 3; i++) 
				{
					shift = (i == 0 ? 0:1);
					yuv_factor = m_pDeinterlace->data[i];
					for(j = 0; j < (m_pContextVD->height>>shift)*2; j++) 
					{
						memcpy(m_pVideoDecBuffer+nDecSize, yuv_factor+(padSize>>(1+shift)), m_pContextVD->width>>shift);
						yuv_factor += m_pDeinterlace->linesize[i];
						nDecSize += m_pContextVD->width>>shift;
					}
				}

				nImageHeight= m_pContextVD->height * 2;
				nImageWidth	= m_pContextVD->width;

				if (m_fpVideoDecodeCallback)
				{
					m_fpVideoDecodeCallback(m_pVideoParam, m_pVideoDecBuffer, nDecSize, nImageWidth, nImageHeight, PIX_FMT_YUV420P);
				}
			}
		}
		if(got_picture && m_dwVideoWriteIndex > m_dwVideoReadIndex)
			goto VIDEODECODELOOP;
	}
	catch (...)
	{
		printf("Exception @ Decode Frame\n");
		return CODEC_ERROR;
	}

	return CODEC_SUCCEED;
}

int	CFFmpegCodec::VideoEncodeFrame(unsigned char* pSrcData, int nSrcBytes, int nSrcPixFmt, int nSrcImageWidth, int nSrcImageHeight, int nDstCodecID, int nDstImageWidth, int nDstImageHeight, int nLevel, int nBitRate, int nFrameRate, int nGopSize, int nStreamType, unsigned long long llTimestamp)
{
	try
	{
		if(!m_pContextVE || m_nCodecID != nDstCodecID)
		{
			if(!VideoEncoderInitialize(nDstCodecID, nDstImageWidth, nDstImageHeight, nLevel, nBitRate, nFrameRate, nGopSize, nStreamType))
			{
				if (m_fpLogReport)
					m_fpLogReport(m_pLogHandler, "", 3, "I", TAG, "CREATE ERROR, stream-type=%d", nStreamType);
				return CODEC_INIT_ERROR;
			}
			else
			{
				if (m_fpLogReport)
					m_fpLogReport(m_pLogHandler, "", 3, "I", TAG, "CREATE SUCCEED, stream-type=%d", nStreamType);
			}
		}
		
		if (m_pContextVE == NULL) 
			return CODEC_INIT_ERROR;

		PixelFormat npixFmt = (PixelFormat)nSrcPixFmt;//PIX_FMT_BGR24;
		/*
		if (nSrcPixFmt == RAW_VIDEO_YV12 || nSrcPixFmt == RAW_VIDEO_I420)
		{
			npixFmt = PIX_FMT_YUV420P;
		}
		else if (nSrcPixFmt == RAW_VIDEO_RGB24)
		{
			npixFmt = PIX_FMT_RGB24;
			npixFmt = PIX_FMT_BGR24;
		}
		else if (nSrcPixFmt == RAW_VIDEO_RGB32)
			npixFmt = PIX_FMT_RGB32;
		*/

		if (!m_pSwsContext)
		{
			m_pSwsContext = sws_getContext(
                nSrcImageWidth,
                nSrcImageHeight,
                npixFmt,
                nDstImageWidth,
                nDstImageHeight,
                PIX_FMT_YUV420P,
				SWS_BICUBIC, NULL, NULL, NULL);

			if (m_pSwsContext == NULL)
			{
				return CODEC_INIT_ERROR;
			}
		}
		
		if (!m_pPicture)
		{
			m_pPicture = alloc_picture(PIX_FMT_YUV420P, nDstImageWidth, nDstImageHeight);
		}
		
		uint8_t* src[4];
		int srcStride[4];
		if (nSrcPixFmt == PIX_FMT_YUV420P)//RAW_VIDEO_YV12
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
		else if (nSrcPixFmt == PIX_FMT_RGB24)//RAW_VIDEO_RGB24
		{
			//if (!m_pColorSpace)
			//	m_pColorSpace = new CColorSpace();
			//m_pColorSpace->RGB24_RotateY(pSrcData, nSrcImageWidth, nSrcImageHeight);
			src[0] = pSrcData;
			src[1] = NULL;
			src[2] = NULL;
			src[3] = NULL;
			srcStride[0] = nSrcImageWidth*3;
			srcStride[1] = 0;
			srcStride[2] = 0;
			srcStride[3] = 0;
		}
		else if (nSrcPixFmt == PIX_FMT_RGB32)//RAW_VIDEO_RGB32
		{
			src[0] = pSrcData;
			src[1] = NULL;
			src[2] = NULL;
			src[3] = NULL;
			srcStride[0] = nSrcImageWidth*4;
			srcStride[2] = 0;
			srcStride[1] = 0;
			srcStride[3] = 0;
		}

		int nRet = sws_scale(m_pSwsContext, src, srcStride, 0, 
			nSrcImageHeight, 
			m_pPicture->data, 
			m_pPicture->linesize);

		int out_size;
		out_size = avcodec_encode_video(
			m_pContextVE, 
			m_pVideoEncBuffer, 
			nDstImageWidth*nDstImageHeight*3/2, 
			m_pPicture 
			);

		if (m_pContextVE->extradata_size > 0)
		{
//			printf("VOL=%s\r\n", m_pContextV->extradata);
		}

		if(out_size < 0)
		{
			VideoEncoderDestroy();
			return CODEC_ERROR;
		}
		else if (out_size == 0)
		{
			return CODEC_NOTENOUGH_DATA;
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
							//config = extradata2psets_h264(m_pContextVE);  //generate SDP
							if (config)
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
								bFoundVOL = TRUE_;
							}
							if (!bFoundVOL) continue;
							if (m_pVideoEncBuffer[i] == 0x0 && m_pVideoEncBuffer[i+1] ==0x0 && m_pVideoEncBuffer[i+2] == 0x01 && m_pVideoEncBuffer[i+3] ==0xB6)
							{
								nEnd = i;
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

			if (m_fpVideoEncodeCallback)
			{
			//	printf("--------encode : FrameType=%d, FrameNumber=%d\n", m_pContextV->coded_frame->key_frame, m_pContextV->frame_number);
				m_fpVideoEncodeCallback(nStreamType, m_pVideoParam, m_pVideoEncBuffer, out_size, nDstCodecID,nDstImageWidth, nDstImageHeight, m_pContextVE->coded_frame->key_frame, m_pContextVE->frame_number,szExtraData.c_str(), llTimestamp);
			}
		}
	}
	catch (...)
	{
		printf("video encode error\n");
		return CODEC_ERROR;
	}

	return CODEC_SUCCEED;
}

//int CFFmpegCodec::AudioEncodeFrame(unsigned char* pSrcData, int nSrcBytes, int nDstCodecID, int nChannel, int nBitRate, int nBitsPerSample, int nSampleRate)
int	CFFmpegCodec::AudioEncodeFrame(unsigned char* pSrcData, int nSrcBytes, int nSrcRawFmt, int nSrcChannel, int nSrcBitsPerSample, int nSrcSamplePerSec, 
								   int nDstCodecID, int nDstChannels, int nDstBitsPerSample, int nDstSamplePerSec, int nDstBitRate, int nStreamType, unsigned long long llTimestamp)
{
	if (!m_pContextAE || m_nCodecID != nDstCodecID)
	{
		if(!AudioEncoderInitialize(nDstCodecID, nSrcChannel, 0, nSrcBitsPerSample, nSrcSamplePerSec))
			return CODEC_INIT_ERROR;
	}

	if (m_pContextAE == NULL) return CODEC_INIT_ERROR;
	int out_size = 0;

	if(m_dwAudioWriteIndex+nSrcBytes >= AUDIO_QUEUE_SIZE)
	{
		unsigned long dwBufferSize = m_dwAudioWriteIndex-m_dwAudioReadIndex;
		if(dwBufferSize + nSrcBytes >= AUDIO_QUEUE_SIZE)
		{
			m_dwAudioReadIndex = 0;
			m_dwAudioWriteIndex = 0;
		}
		else
		{
			memmove(m_pAudioBuffer, m_pAudioBuffer+m_dwAudioReadIndex, dwBufferSize);
			m_dwAudioReadIndex	= 0;
			m_dwAudioWriteIndex	= dwBufferSize;
		}
	}

	memcpy(m_pAudioBuffer+m_dwAudioWriteIndex, pSrcData, nSrcBytes);
	m_dwAudioWriteIndex += nSrcBytes;

	try
	{	
		while ((int)(m_dwAudioWriteIndex-m_dwAudioReadIndex) >= m_nAudioFrameSize)
		{
			memcpy(m_pSample, m_pAudioBuffer + m_dwAudioReadIndex, m_nAudioFrameSize);
			m_dwAudioReadIndex += m_nAudioFrameSize;

			out_size = 0;
			out_size = avcodec_encode_audio(m_pContextAE, (uint8_t*)m_pAudioEncBuffer, m_nAudioFrameSize/4, (const short *)m_pSample);
			if (out_size < 0)
			{
				AudioEncoderDestroy();
				return CODEC_ERROR;
			}
			if (out_size > 0)
			{
				if (m_fpAudioEncodeCallback)
#if FFMPEG_0_5
					m_fpAudioEncodeCallback(nStreamType, m_pAudioParam, m_pAudioEncBuffer, out_size, nDstCodecID, m_pContextAE->channels, m_pContextAE->bits_per_sample, m_pContextAE->sample_rate,"", llTimestamp);
#else
					m_fpAudioEncodeCallback(nStreamType, m_pAudioParam, m_pAudioEncBuffer, out_size, nDstCodecID, m_pContextAE->channels, 0, m_pContextAE->sample_rate,"", llTimestamp);
#endif
			}
		}		
	}
	catch (...)
	{
		printf("Audio Encode failed!!\n");
		return CODEC_ERROR;
	}

	return CODEC_SUCCEED;
}

//int CFFmpegCodec::AudioEncodeFrame(unsigned char* pSrcData, int nSrcBytes, int nDstCodecID, int nChannel, int nBitRate, int nBitsPerSample, int nSampleRate)
//int CFFmpegCodec::AudioEncodeFrame(unsigned char* pSrcData, int nSrcBytes, int nSrcRawType, int nSrcChannel, int nSrcBitRate, int nBitsPerSample,
//								   int iDstCodecID, int iDstChannels,int iDstBitsPerSample, int iDstSamplePerSec, int nStreamType);
int	CFFmpegCodec::AudioEncodeFrame_OLD(unsigned char* pSrcData, int nSrcBytes, int nSrcRawFmt, int nSrcChannel, int nSrcBitsPerSample, int nSrcSamplePerSec, 
								   int nDstCodecID, int nDstChannels, int nDstBitsPerSample, int nDstSamplePerSec, int nDstBitRate, int nStreamType, unsigned long long llTimestamp)
{
	if (!m_pContextAE || m_nCodecID != nDstCodecID)
	{
		if(!AudioEncoderInitialize(nDstCodecID, nDstChannels, nDstBitRate, nDstBitsPerSample, nDstSamplePerSec))
			return CODEC_INIT_ERROR;
	}
	
	if (m_pContextAE == NULL) return CODEC_INIT_ERROR;
	int out_size = 0;
	
	if(m_dwAudioWriteIndex+nSrcBytes >= AUDIO_QUEUE_SIZE)
	{
		unsigned long dwBufferSize = m_dwAudioWriteIndex-m_dwAudioReadIndex;
		if(dwBufferSize + nSrcBytes >= AUDIO_QUEUE_SIZE)
		{
			m_dwAudioReadIndex = 0;
			m_dwAudioWriteIndex = 0;
		}
		else
		{
			memmove(m_pAudioBuffer, m_pAudioBuffer+m_dwAudioReadIndex, dwBufferSize);
			m_dwAudioReadIndex	= 0;
			m_dwAudioWriteIndex	= dwBufferSize;
		}
	}
	
	memcpy(m_pAudioBuffer+m_dwAudioWriteIndex, pSrcData, nSrcBytes);
	m_dwAudioWriteIndex += nSrcBytes;
	
	try
	{	
		while ((int)(m_dwAudioWriteIndex-m_dwAudioReadIndex) >= m_nAudioFrameSize)
		{
			memcpy(m_pSample, m_pAudioBuffer+m_dwAudioReadIndex, m_nAudioFrameSize);
			m_dwAudioReadIndex += m_nAudioFrameSize;
			
			out_size = 0;
			out_size = avcodec_encode_audio(m_pContextAE, (uint8_t*)m_pAudioEncBuffer, m_nAudioFrameSize, (const short *)m_pSample);
			if (out_size < 0)
			{
				AudioEncoderDestroy();
				return CODEC_ERROR;
			}
			if (out_size > 0)
			{
//	typedef void (*AUDIOENCODECALLBACK)(int nStreamType, void* pParam, unsigned char* pEncBuffer, int nEncSize, int nCodecID,int nChannel, int nBytesPerSample, int nSamplePerSec, char* szExtraData);

				if (m_fpAudioEncodeCallback)
//#ifdef WIN32
//					m_fpAudioEncodeCallback(nStreamType, m_pAudioParam, m_pAudioEncBuffer, out_size, nDstCodecID, m_pContextAE->channels, m_pContextAE->bits_per_sample, m_pContextAE->sample_rate,"", llTimestamp);
//#else
					m_fpAudioEncodeCallback(nStreamType, m_pAudioParam, m_pAudioEncBuffer, out_size, nDstCodecID, m_pContextAE->channels, 16, m_pContextAE->sample_rate,"", llTimestamp);
//#endif
			}
		}
	}
	catch (...)
	{
		printf("Audio Encode failed!!\n");
		return CODEC_ERROR;
	}

	return CODEC_SUCCEED;
}

BOOL_ CFFmpegCodec::AudioDecoderDestroy()
{
	if (m_pContextAD)
	{
		pthread_mutex_lock(&g_vFFMPEGLock);
		avcodec_close(m_pContextAD);
		av_free(m_pContextAD);
		m_pContextAD = NULL;
		pthread_mutex_unlock(&g_vFFMPEGLock);
	}
	if (m_pAudioDecBuffer)
	{
		delete[] m_pAudioDecBuffer;
		m_pAudioDecBuffer = NULL;
	}
	
	return TRUE_;
}

BOOL_ CFFmpegCodec::VideoDecoderDestroy()
{
	if (m_pContextVD)
	{
		pthread_mutex_lock(&g_vFFMPEGLock);
		avcodec_close(m_pContextVD);
		av_free(m_pContextVD);
		m_pContextVD = NULL;
		pthread_mutex_unlock(&g_vFFMPEGLock);
	}
	
	if (m_pPicture)
	{
		av_free(m_pPicture);
		m_pPicture = NULL;
	}
	
	if (m_pYUVFrame1)
	{
		if (m_pYUVFrame1->data[0])
			free(m_pYUVFrame1->data[0]);
		av_free(m_pYUVFrame1);
		m_pYUVFrame1 = NULL;
	}
	
	if (m_pYUVFrame2)
	{
		if (m_pYUVFrame2->data[0])
			free(m_pYUVFrame2->data[0]);
		av_free(m_pYUVFrame2);
		m_pYUVFrame2 = NULL;
	}
	
	if (m_pYUVMergeFrame)
	{
		if (m_pYUVMergeFrame->data[0])
			free(m_pYUVMergeFrame->data[0]);
		av_free(m_pYUVMergeFrame);
		m_pYUVMergeFrame = NULL;
	}
	
	if (m_pDeinterlace)
	{
		if (m_pDeinterlace->data[0])
			free(m_pDeinterlace->data[0]);
		av_free(m_pDeinterlace);
		m_pDeinterlace = NULL;
	}
	
	if(m_pVideoBuffer)
	{
		delete[] m_pVideoBuffer;
		m_pVideoBuffer = NULL;
	}

	if (m_pVideoDecBuffer)
	{
		delete[] m_pVideoDecBuffer;
		m_pVideoDecBuffer = NULL;
	}
		
	return TRUE_;
}

BOOL_ CFFmpegCodec::AudioEncoderDestroy()
{
	if (m_pContextAE)
	{
		pthread_mutex_lock(&g_vFFMPEGLock);
		avcodec_close(m_pContextAE);
		av_free(m_pContextAE);
		m_pContextAE = NULL;
		pthread_mutex_unlock(&g_vFFMPEGLock);
	}
	if (m_pSample)
	{
		free(m_pSample);
		m_pSample = NULL;
	}
	if (m_pAudioBuffer)
	{
		delete[] m_pAudioBuffer;
		m_pAudioBuffer = NULL;
	}

	if (m_pAudioEncBuffer)
	{
		delete[] m_pAudioEncBuffer;
		m_pAudioEncBuffer = NULL;
	}

	return TRUE_;
}

BOOL_ CFFmpegCodec::VideoEncoderDestroy()
{
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
		if (m_pPicture->data[0])
			free(m_pPicture->data[0]);
		av_free(m_pPicture);
		m_pPicture = NULL;
	}
	
	if (m_pSwsContext)
	{
		sws_freeContext(m_pSwsContext);
		m_pSwsContext = NULL;
	}

	/*
	if (m_pDeinterlace)
	{
		if (m_pDeinterlace->data[0])
			free(m_pDeinterlace->data[0]);
		av_free(m_pDeinterlace);
		m_pDeinterlace = NULL;
	}
	*/
	if (m_pVideoEncBuffer)
	{
		delete[] m_pVideoEncBuffer;
		m_pVideoEncBuffer = NULL;
	}

	return TRUE_;
}

void CFFmpegCodec::ResetCodec()
{
//	if (m_pColorSpace)
//	{
//		delete m_pColorSpace;
//		m_pColorSpace = NULL;
//	}

	m_dwVideoReadIndex = 0;
	m_dwVideoWriteIndex = 0;

	m_dwAudioReadIndex = 0;
	m_dwAudioWriteIndex = 0;

	m_fpAudioDecodeCallback = NULL;
	m_fpAudioEncodeCallback = NULL;
	m_fpVideoDecodeCallback = NULL;
	m_fpVideoEncodeCallback = NULL;
}

#define MAX_PSET_SIZE 1024
char *CFFmpegCodec::extradata2psets_h264(AVCodecContext *c)
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


const uint8_t *CFFmpegCodec::ff_avc_find_startcode(const uint8_t *p, const uint8_t *end)
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


char *CFFmpegCodec::av_base64_encode(char * buf, int buf_len, const uint8_t * src, int len)
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

char *CFFmpegCodec::extradata2psets_mpeg4(AVCodecContext *c)
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

//新建一帧
AVFrame * CFFmpegCodec::alloc_picture(int pix_fmt, int width, int height)
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

	avpicture_fill((AVPicture *)picture, picture_buf, (PixelFormat)pix_fmt, width, height);
	return picture;
}
