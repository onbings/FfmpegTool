/*
	It is FFmpeg decoder class. Sample for article from unick-soft.ru
*/
//
#include "FfmpegDecoder.h"

//#define min(a,b) (a > b ? b : a)


/*
 * WAVE file header based on definition from
 * https://gist.github.com/Jon-Schneider/8b7c53d27a7a13346a643dac9c19d34f
 *
 * We must ensure this structure doesn't have any holes or
 * padding so we can just map it straight to the WAVE data.
 */
#pragma pack(1)
struct wave_hdr {
	/* RIFF Header: "RIFF" */
	char riff_header[4];
	/* size of audio data + sizeof(struct wave_hdr) - 8 */
	int wav_size;
	/* "WAVE" */
	char wav_header[4];

	/* Format Header */
	/* "fmt " (includes trailing space) */
	char fmt_header[4];
	/* Should be 16 for PCM */
	int fmt_chunk_size;
	/* Should be 1 for PCM. 3 for IEEE Float */
	int16_t audio_format;
	int16_t num_channels;
	int sample_rate;
	/*
	 * Number of bytes per second
	 * sample_rate * num_channels * bit_depth/8
	 */
	int byte_rate;
	/* num_channels * bytes per sample */
	int16_t sample_alignment;
	/* bits per sample */
	int16_t bit_depth;

	/* Data Header */
	/* "data" */
	char data_header[4];
	/*
	 * size of audio
	 * number of samples * num_channels * bit_depth/8
	 */
	int data_bytes;
};
#pragma pack()


struct audio_buffer {
	uint8_t *ptr;
	int size; /* size left in the buffer */
};

//writes out the wave header

void write_wave_hdr(int fd, size_t size)
{
	struct wave_hdr wh;

	memcpy(&wh.riff_header, "RIFF", 4);
	wh.wav_size = (int)(size + sizeof(struct wave_hdr) - 8);
	memcpy(&wh.wav_header, "WAVE", 4);
	memcpy(&wh.fmt_header, "fmt ", 4);
	wh.fmt_chunk_size = 16;
	wh.audio_format = 1;
	wh.num_channels = WAVE_OUT_NB_CHANNEL;
	wh.sample_rate = WAVE_SAMPLE_RATE;
	wh.sample_alignment = 2;
	wh.bit_depth = 16;
	wh.byte_rate = wh.sample_rate * wh.sample_alignment;
	memcpy(&wh.data_header, "data", 4);
	wh.data_bytes = (int)size;

	_write(fd, &wh, sizeof(struct wave_hdr));
}


int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
	struct audio_buffer *audio_buf = (struct audio_buffer *)opaque;

	buf_size = FFMIN(buf_size, audio_buf->size);

	/* copy internal buffer data to buf */
	memcpy(buf, audio_buf->ptr, buf_size);
	audio_buf->ptr += buf_size;
	audio_buf->size -= buf_size;

	return buf_size;
}

// core one responsible for doing the actual audio conversion, it’s called in a loop(from decode_audio()) operating on a frame of data at a time

void convert_frame(struct SwrContext *swr, AVCodecContext *codec, AVFrame *frame, int16_t **data, int *size, bool flush)
{
	int nr_samples;
	int64_t delay;
	uint8_t *buffer;

	if (frame->nb_samples)
	{
		delay = swr_get_delay(swr, codec->sample_rate);
		nr_samples = (int)av_rescale_rnd(delay + frame->nb_samples, WAVE_SAMPLE_RATE, codec->sample_rate, AV_ROUND_UP);
		av_samples_alloc(&buffer, NULL, WAVE_OUT_NB_CHANNEL, nr_samples, AV_SAMPLE_FMT_S16, 0);

		/*
		 * !flush is used to check if we are flushing any remaining
		 * conversion buffers...
		 */
#if 1
		nr_samples = swr_convert(swr, &buffer, nr_samples,
			!flush ? (const uint8_t **)frame->data : NULL,
			!flush ? frame->nb_samples : 0);
#else
		float *p = (float *)frame->data[0];
		for (int i = 0; i < nr_samples; i++)
		{
			buffer[i] = (unsigned short)(p[i] * 32767.0f);
		}
#endif

		*data = (int16_t *)realloc(*data, (*size + nr_samples) * sizeof(int16_t) * WAVE_OUT_NB_CHANNEL);
		memcpy(*data + *size, buffer, nr_samples * sizeof(int16_t) * WAVE_OUT_NB_CHANNEL);
		printf("nr_samples %d memcpy(%p,%p,%zx) %04x %04x %04x %04x %04x %04x %04x %04x\n", nr_samples, *data + *size, buffer, nr_samples * sizeof(int16_t),
			buffer[0], buffer[1], buffer[2], buffer[3], buffer[4], buffer[5], buffer[6], buffer[7]);
		*size += (nr_samples * WAVE_OUT_NB_CHANNEL);
		av_freep(&buffer);
	}
}

//Next up a simple helper function, this was done really just to help with the readability of the code…

bool is_audio_stream(const AVStream *stream)
{
	if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		return true;

	return false;
}

//Now for the main driver of the whole thing, we initialise various ffmpeg data structures, configure the WAVE output parameters anddo the audio conversion

int decode_audio(struct audio_buffer *audio_buf, int16_t **data, int *size)
{
	AVFormatContext *fmt_ctx;
	AVIOContext *avio_ctx;
	AVStream *stream;
	AVCodecContext *codec;
	AVPacket packet;
	AVFrame *frame;
	struct SwrContext *swr;
	uint8_t *avio_ctx_buffer;
	unsigned int i;
	int stream_index = -1;
	int err;

	fmt_ctx = avformat_alloc_context();
	avio_ctx_buffer = (uint8_t *)av_malloc(AVIO_CTX_BUF_SZ);
	avio_ctx = avio_alloc_context(avio_ctx_buffer, AVIO_CTX_BUF_SZ, 0, audio_buf, &read_packet, NULL, NULL);
	fmt_ctx->pb = avio_ctx;

	avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
	avformat_find_stream_info(fmt_ctx, NULL);

	for (i = 0; i < fmt_ctx->nb_streams; i++) {
		if (is_audio_stream(fmt_ctx->streams[i])) {
			stream_index = i;
			break;
		}
	}

	stream = fmt_ctx->streams[stream_index];
	codec = avcodec_alloc_context3(avcodec_find_decoder(stream->codecpar->codec_id));
	avcodec_parameters_to_context(codec, stream->codecpar);
	avcodec_open2(codec, avcodec_find_decoder(codec->codec_id), NULL);

	/* prepare resampler */
	swr = swr_alloc();

	av_opt_set_int(swr, "in_channel_count", codec->channels, 0);
	av_opt_set_int(swr, "out_channel_count", WAVE_OUT_NB_CHANNEL, 0);
	av_opt_set_int(swr, "in_channel_layout", codec->channel_layout, 0);
	av_opt_set_int(swr, "out_channel_layout", (WAVE_OUT_NB_CHANNEL == 2) ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO, 0);
	av_opt_set_int(swr, "in_sample_rate", codec->sample_rate, 0);
	av_opt_set_int(swr, "out_sample_rate", WAVE_SAMPLE_RATE, 0);
	av_opt_set_sample_fmt(swr, "in_sample_fmt", codec->sample_fmt, 0);
	av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

	swr_init(swr);
	//av_init_packet(&packet);
	frame = av_frame_alloc();

	/* iterate through frames */
	*data = NULL;
	*size = 0;
	while (av_read_frame(fmt_ctx, &packet) >= 0) {
		avcodec_send_packet(codec, &packet);

		err = avcodec_receive_frame(codec, frame);
		if (err == AVERROR(EAGAIN))
			continue;

		convert_frame(swr, codec, frame, data, size, false);
		av_packet_unref(&packet);
	}
	/* Flush any remaining conversion buffers... */
	convert_frame(swr, codec, frame, data, size, true);

	av_frame_free(&frame);
	swr_free(&swr);
	avcodec_close(codec);
	avformat_close_input(&fmt_ctx);
	avcodec_free_context(&codec);
	avformat_free_context(fmt_ctx);

	if (avio_ctx) {
		av_freep(&avio_ctx->buffer);
		av_freep(&avio_ctx);
	}

	return 0;
}

//Now for a simple main() function to drive the above


//int f(int argc, char *argv[])
int ExtractAudio(const char *pFnIn_c, const char *pFnOut_c)
{
	struct audio_buffer audio_buf;
	int16_t *data;
	int size;
	int ifd;
	int ofd;
	int NbRead_i;
	void *pBuffer;

	ifd = _open(pFnIn_c, O_BINARY | O_RDONLY);
	ofd = _open(pFnOut_c, O_BINARY | O_WRONLY | O_CREAT | O_TRUNC, 0666);

	_lseek(ifd, 0L, SEEK_END);
	audio_buf.size = _tell(ifd);
	//audio_buf.size = 307453;
	pBuffer = malloc(audio_buf.size);
	audio_buf.ptr = (uint8_t *)pBuffer;	//This var will be modified so we use pBuffer to free the buffer

	_lseek(ifd, 0L, SEEK_SET);
	NbRead_i = _read(ifd, audio_buf.ptr, audio_buf.size);

	decode_audio(&audio_buf, &data, &size);

	write_wave_hdr(ofd, size * sizeof(int16_t));
	_write(ofd, data, size * sizeof(int16_t));

	_close(ifd);
	_close(ofd);

	free(data);
	free(pBuffer);


	exit(EXIT_SUCCESS);
}

bool FFmpegDecoder::OpenFile(const std::string& inputFile)
{
	CloseFile();

	// Register all components
	//BHA 'av_register_all': was declared deprecated 	av_register_all();

	// Open media file.
	if (avformat_open_input(&pFormatCtx, inputFile.c_str(), NULL, NULL) != 0)
	{
		CloseFile();
		return false;
	}

	// Get format info.
	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		CloseFile();
		return false;
	}

	// open video and audio stream.
	bool hasVideo = OpenVideo();
	bool hasAudio = OpenAudio(); 

	if (!hasVideo)
	{
		CloseFile();
		return false;
	}

	isOpen = true;

	// Get file information.
	if (videoStreamIndex != -1)
	{
		videoFramePerSecond = av_q2d(pFormatCtx->streams[videoStreamIndex]->r_frame_rate);
		// Need for convert time to ffmpeg time.
		videoBaseTime       = av_q2d(pFormatCtx->streams[videoStreamIndex]->time_base); 
	}

	if (audioStreamIndex != -1)
	{
		audioBaseTime = av_q2d(pFormatCtx->streams[audioStreamIndex]->time_base);
	}

	return true;
}


bool FFmpegDecoder::CloseFile()
{
	isOpen = false;

	// Close video and audio.
	CloseVideo();
	CloseAudio();

	if (pFormatCtx)
	{
		//BHA av_close_input_file(pFormatCtx);
		avformat_close_input(&pFormatCtx),
		pFormatCtx = NULL;
	}

	return true;
}

#if 0
AVFrame * FFmpegDecoder::GetNextFrame()
{
	AVFrame * res = NULL;

	if (videoStreamIndex != -1)
	{
		AVFrame *pVideoYuv = av_frame_alloc();	//BHA  avcodec_alloc_frame();
		AVPacket packet;

		if (isOpen)
		{
			// Read packet.
			while (av_read_frame(pFormatCtx, &packet) >= 0)
			{
				int64_t pts = 0;
				pts = (packet.dts != AV_NOPTS_VALUE) ? packet.dts : 0;

				if(packet.stream_index == videoStreamIndex)
				{
					// Convert ffmpeg frame timestamp to real frame number.
					int64_t numberFrame = (int64_t)((double)(pts - pFormatCtx->streams[videoStreamIndex]->start_time) *
						videoBaseTime * videoFramePerSecond); 

					// Decode frame
					bool isDecodeComplite = DecodeVideo(&packet, pVideoYuv);
					if (isDecodeComplite)
					{
						res = GetRGBAFrame(pVideoYuv);
					}
					break;
				} 
				else if (packet.stream_index == audioStreamIndex)
				{
					if (packet.dts != AV_NOPTS_VALUE)
					{
						int audioFrameSize = MAX_AUDIO_PACKET;
						uint8_t * pFrameAudio = new uint8_t[audioFrameSize];
						if (pFrameAudio)
						{
							double fCurrentTime = (double)(pts - pFormatCtx->streams[videoStreamIndex]->start_time)
								* audioBaseTime;
							double fCurrentDuration = (double)packet.duration * audioBaseTime;

							// Decode audio
							//BHA int nDecodedSize = DecodeAudio(audioStreamIndex, &packet, pFrameAudio, audioFrameSize);
							size_t nDecodedSize = DecodeAudio(audioStreamIndex, &packet, pFrameAudio, audioFrameSize);

							if (nDecodedSize > 0 && pAudioCodecCtx->sample_fmt == AV_SAMPLE_FMT_FLTP)
							{
								// Process audio here.
								/* Uncommend sample if you want write raw data to file.*/
								{
									size_t size = nDecodedSize / sizeof(float);
									signed short * ar = new signed short[nDecodedSize / sizeof(float)];
									float* pointer = (float*)pFrameAudio;
									// Convert float to S16.
									for (int i = 0; i < size / 2; i ++)
									{
										ar[i] = (unsigned short)(pointer[i] * 32767.0f);
									}

									FILE* file = fopen("c:\\tmp\\AudioRaw.raw", "ab");
									fwrite(ar, 1, size * sizeof (signed short) / 2, file);
									fclose(file);
								}
								
							}
							
							// Delete buffer.
							delete[] pFrameAudio;
							pFrameAudio = NULL;
						}
					}
				}

				av_packet_unref(&packet); //BHA av_free_packet(&packet);
				
				packet = AVPacket();
			}

			av_free(pVideoYuv);
		}    
	}

	return res;
}
#else
int FFmpegDecoder::GetNextFrame(AVFrame **ppVideo, AVFrame **ppAudio)
{
//	AVFrame *res = NULL;
	int Rts_i = 0;
	*ppVideo = nullptr;
	*ppAudio = nullptr;
	if (videoStreamIndex != -1)
	{
		AVFrame *pVideoFrame = av_frame_alloc();	//BHA  avcodec_alloc_frame();
		AVFrame *pAudioFrame = av_frame_alloc();	//BHA  avcodec_alloc_frame();
		AVPacket packet;

		if (isOpen)
		{
			// Read packet.
			while (av_read_frame(pFormatCtx, &packet) >= 0)
			{
				int64_t pts = 0;
				pts = (packet.dts != AV_NOPTS_VALUE) ? packet.dts : 0;

				if (packet.stream_index == videoStreamIndex)
				{
					// Convert ffmpeg frame timestamp to real frame number.
					int64_t numberFrame = (int64_t)((double)(pts - pFormatCtx->streams[videoStreamIndex]->start_time) *
						videoBaseTime * videoFramePerSecond);

					// Decode frame
					bool isDecodeComplite = DecodeVideo(&packet, pVideoFrame);
					if (isDecodeComplite)
					{
						*ppVideo = GetVideoFrame(pVideoFrame, AV_PIX_FMT_BGR24);
					}
					break;
				}
				else if (packet.stream_index == audioStreamIndex)
				{
#if 0
					if (packet.dts != AV_NOPTS_VALUE)
					{
						int audioFrameSize = MAX_AUDIO_PACKET;
						uint8_t *pFrameAudio = new uint8_t[audioFrameSize];
						if (pFrameAudio)
						{
							double fCurrentTime = (double)(pts - pFormatCtx->streams[videoStreamIndex]->start_time)
								* audioBaseTime;
							double fCurrentDuration = (double)packet.duration * audioBaseTime;

							// Decode audio
							//BHA int nDecodedSize = DecodeAudio(audioStreamIndex, &packet, pFrameAudio, audioFrameSize);
							size_t nDecodedSize = DecodeAudio(audioStreamIndex, &packet, pFrameAudio, audioFrameSize);

							if (nDecodedSize > 0 && pAudioCodecCtx->sample_fmt == AV_SAMPLE_FMT_FLTP)
							{
								// Process audio here.
								/* Uncommend sample if you want write raw data to file.*/
								/*
								{
									size_t size = nDecodedSize / sizeof(float);
									signed short *ar = new signed short[nDecodedSize / sizeof(float)];
									float *pointer = (float *)pFrameAudio;
									// Convert float to S16.
									for (int i = 0; i < size / 2; i++)
									{
										ar[i] = (unsigned short)(pointer[i] * 32767.0f);
									}

									FILE *file = fopen("c:\\tmp\\AudioRaw.raw", "ab");
									fwrite(ar, 1, size * sizeof(signed short) / 2, file);
									fclose(file);
								}
								*/
							}

							// Delete buffer.
							delete[] pFrameAudio;
							pFrameAudio = NULL;
						}
					}
#else
					if (packet.dts != AV_NOPTS_VALUE)
					{
						// Decode audio
						//BHA int nDecodedSize = DecodeAudio(audioStreamIndex, &packet, pFrameAudio, audioFrameSize);
						bool isDecodeComplite = DecodeAudio(&packet, pAudioFrame);	//, pFrameAudio, audioFrameSize);
						if (isDecodeComplite)
						{
							*ppAudio = GetAudioFrame(pAudioFrame, WAVE_OUT_NB_CHANNEL, (WAVE_OUT_NB_CHANNEL == 2) ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO, WAVE_SAMPLE_RATE, false, AV_SAMPLE_FMT_S16);
//							*ppAudio = GetAudioFrame(pAudioFrame, pAudioFrame->channels, pAudioFrame->channel_layout, WAVE_SAMPLE_RATE, false, AV_SAMPLE_FMT_S16);
						}
					}
					else
					{
						printf("jj");
					}

					break;
				}
#endif
				

//				packet = AVPacket();
			}
			/* Flush any remaining conversion buffers... */
			if (audioStreamIndex != -1)
			{
				//convert_frame(swr, pAudioCodecCtx, pAudioFrame, &pAudioData, &AudioSize, true);
			}
			av_packet_unref(&packet); //BHA av_free_packet(&packet);
			av_free(pVideoFrame);
			av_free(pAudioFrame);
		}
	}

	return Rts_i;
}
#endif
AVFrame *FFmpegDecoder::GetVideoFrame(AVFrame *pFrameYuv, enum AVPixelFormat pix_fmt)	//AV_PIX_FMT_BGR24
{
	AVFrame *frame = NULL;
	int width = pVideoCodecCtx->width;
	int height = pVideoCodecCtx->height;
	int align = 16, OutputHeight_i;
	//BHA int bufferImgSize = avpicture_get_size(PIX_FMT_BGR24, width, height);
	int bufferImgSize = av_image_get_buffer_size(pix_fmt, width, height, align);
	frame = av_frame_alloc();	//BHA avcodec_alloc_frame();
	uint8_t *buffer = (uint8_t *)av_mallocz(bufferImgSize);
	if (frame)
	{
		//BHA		avpicture_fill((AVPicture *)frame, buffer, PIX_FMT_BGR24, width, height);
		av_image_fill_arrays(frame->data, frame->linesize, buffer, pix_fmt, width, height, align);
		frame->width = width;
		frame->height = height;
		//frame->data[0] = buffer;

		OutputHeight_i = sws_scale(pImgConvertCtx, pFrameYuv->data, pFrameYuv->linesize, 0, height, frame->data, frame->linesize);
	}

	return frame;
}
AVFrame * FFmpegDecoder::GetAudioFrame(AVFrame *pFramePcm, int nb_channels, int channellayout, int samplerateinhz, 
	bool flush, enum AVSampleFormat sample_fmt) //AV_SAMPLE_FMT_S16
{
	AVFrame * frame = NULL;
	int nr_samples, bufferAudSize;
	int64_t delay;
	uint8_t *buffer;

	if (pFramePcm->nb_samples)
	{
		delay = swr_get_delay(swr, pAudioCodecCtx->sample_rate);
		nr_samples = (int)av_rescale_rnd(delay + pFramePcm->nb_samples, samplerateinhz, pAudioCodecCtx->sample_rate, AV_ROUND_UP);

		bufferAudSize = av_samples_get_buffer_size(NULL, nb_channels, nr_samples, sample_fmt, 0);

		frame = av_frame_alloc();	//BHA avcodec_alloc_frame();
		buffer = (uint8_t *)av_mallocz(bufferAudSize);
		if (frame) 
		{ 
			//BHA		avpicture_fill((AVPicture *)frame, buffer, PIX_FMT_BGR24, width, height);
			av_samples_fill_arrays(frame->data, frame->linesize, buffer, nb_channels, nr_samples, sample_fmt, 0);
			int sz = nr_samples * sizeof(int16_t) * nb_channels;


			//av_samples_alloc(&buffer, NULL, nb_channels, nr_samples, sample_fmt, 0);

			/*
			 * !flush is used to check if we are flushing any remaining
			 * conversion buffers...
			 */
			sz = swr_convert(swr, &buffer, nr_samples,
				!flush ? (const uint8_t **)pFramePcm->data : NULL,
				!flush ? pFramePcm->nb_samples : 0);

			frame->nb_samples = sz;
			frame->format = sample_fmt;
			frame->sample_rate = samplerateinhz;
			frame->channel_layout = channellayout; // (WAVE_OUT_NB_CHANNEL == 2) ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO
			frame->pkt_pos = pFramePcm->pkt_pos;
			frame->pkt_duration = pFramePcm->pkt_duration;
			frame->channels = nb_channels;
			frame->pkt_size = pFramePcm->pkt_size;

			


			/*
			*data = (int16_t *)realloc(*data, (*size + nr_samples) * sizeof(int16_t) * nb_channels);
			memcpy(*data + *size, buffer, nr_samples * sizeof(int16_t) * nb_channels);
			*/
			int16_t *pData_S16 = (int16_t *)buffer;
			printf("fr %x:%p nbs %d buffer %x:%p %04x %04x %04x %04x %04x %04x %04x %04x\n", frame->linesize[0], frame->data[0], nr_samples, sz, buffer, 
				pData_S16[0], pData_S16[1], pData_S16[2], pData_S16[3], pData_S16[4], pData_S16[5], pData_S16[6], pData_S16[7]);
			/*
			*size += (nr_samples * nb_channels);
			av_freep(&buffer);
			*/
		}
	}
	return frame;
}


bool FFmpegDecoder::OpenVideo()
{
	bool res = false;

	if (pFormatCtx)
	{
		videoStreamIndex = -1;

		for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++)
		{
//BHA			if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
			if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
			{
				videoStreamIndex = i;
				//BHA 2 lines below after the if pVideoCodecCtx = pFormatCtx->streams[i]->codec;	//AVCodecContext
				pVideoCodec = avcodec_find_decoder(pFormatCtx->streams[i]->codecpar->codec_id);
				if (pVideoCodec)
				{
					pVideoCodecCtx = avcodec_alloc_context3(pVideoCodec);
					if (avcodec_parameters_to_context(pVideoCodecCtx, pFormatCtx->streams[i]->codecpar) < 0)
					{
						return false;
					}
					res     = !(avcodec_open2(pVideoCodecCtx, pVideoCodec, NULL) < 0);
					width   = pVideoCodecCtx->coded_width;
					height  = pVideoCodecCtx->coded_height;
				}

				break;
			}
		}

		if (!res)
		{
			CloseVideo();
		}
		else
		{
			pImgConvertCtx = sws_getContext(pVideoCodecCtx->width, pVideoCodecCtx->height,
				pVideoCodecCtx->pix_fmt,
				pVideoCodecCtx->width, pVideoCodecCtx->height,
				//BHA PIX_FMT_BGR24,
				AV_PIX_FMT_BGR24,
				SWS_BICUBIC, NULL, NULL, NULL);
		}
	}

	return res;
}




bool FFmpegDecoder::OpenAudio()
{
	bool res = false;

	if (pFormatCtx)
	{   
		audioStreamIndex = -1;

		for (unsigned int i = 0; i < pFormatCtx->nb_streams; i++)
		{
			//BHA if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
			if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
			{
				audioStreamIndex = i;
				//BHA 2 lines below after the if pAudioCodecCtx = pFormatCtx->streams[i]->codec;
				pAudioCodec = avcodec_find_decoder(pFormatCtx->streams[i]->codecpar->codec_id);
				if (pAudioCodec)
				{
					pAudioCodecCtx = avcodec_alloc_context3(pAudioCodec);
					if (avcodec_parameters_to_context(pAudioCodecCtx, pFormatCtx->streams[i]->codecpar) < 0)
					{
						return false;
					}
					res = !(avcodec_open2(pAudioCodecCtx, pAudioCodec, NULL) < 0);       

					if (res)
					{
						//AVFormatContext *fmt_ctx;
						//AVIOContext *avio_ctx;
						//AVStream *stream;
						//AVCodecContext *codec;
						//AVPacket packet;
						//AVFrame *frame;
						//struct SwrContext *swr;
						//uint8_t *avio_ctx_buffer;
						//unsigned int i;
						//int stream_index = -1;
						//int err;
						//struct audio_buffer *audio_buf;

						fmt_ctx = avformat_alloc_context();
						avio_ctx_buffer = (uint8_t *)av_malloc(AVIO_CTX_BUF_SZ);
						avio_ctx = avio_alloc_context(avio_ctx_buffer, AVIO_CTX_BUF_SZ, 0, audio_buf, &read_packet, NULL, NULL);
						fmt_ctx->pb = avio_ctx;



						/* prepare resampler */
						swr = swr_alloc();

						av_opt_set_int(swr, "in_channel_count", pAudioCodecCtx->channels, 0);
						av_opt_set_int(swr, "out_channel_count", WAVE_OUT_NB_CHANNEL, 0);
						av_opt_set_int(swr, "in_channel_layout", pAudioCodecCtx->channel_layout, 0);
						av_opt_set_int(swr, "out_channel_layout", (WAVE_OUT_NB_CHANNEL == 2) ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO, 0);
						av_opt_set_int(swr, "in_sample_rate", pAudioCodecCtx->sample_rate, 0);
						av_opt_set_int(swr, "out_sample_rate", WAVE_SAMPLE_RATE, 0);
						av_opt_set_sample_fmt(swr, "in_sample_fmt", pAudioCodecCtx->sample_fmt, 0);
						av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);

						swr_init(swr);
						//av_init_packet(&packet);
						//pAudioFrame = av_frame_alloc();

						/* iterate through frames */
						///pAudioData = NULL;
						///AudioSize = 0;
#if 0

						while (av_read_frame(fmt_ctx, &packet) >= 0) {
							avcodec_send_packet(codec, &packet);

							err = avcodec_receive_frame(codec, frame);
							if (err == AVERROR(EAGAIN))
								continue;

							convert_frame(swr, codec, frame, data, size, false);
						}
						/* Flush any remaining conversion buffers... */
						convert_frame(swr, codec, frame, data, size, true);

						av_frame_free(&frame);
						swr_free(&swr);
						avcodec_close(codec);
						avformat_close_input(&fmt_ctx);
						avformat_free_context(fmt_ctx);

						if (avio_ctx) {
							av_freep(&avio_ctx->buffer);
							av_freep(&avio_ctx);
						}

						return 0;
#endif
					}



				}
				break;
			}
		}

		if (! res)
		{
			CloseAudio();
		}

	}

	return res;
}


void FFmpegDecoder::CloseVideo()
{
	if (pVideoCodecCtx)
	{
//BHA avcodec_close(pVideoCodecCtx);
		avcodec_free_context(&pVideoCodecCtx);
		
		pVideoCodecCtx = NULL;
		pVideoCodec = NULL;
		videoStreamIndex = -1;
	}
}


void FFmpegDecoder::CloseAudio()
{    
	if (pAudioCodecCtx)
	{
		//av_frame_free(&pAudioFrame);
		swr_free(&swr);
		//avcodec_close(codec);
		//BHA		avcodec_close(pAudioCodecCtx);
		avcodec_free_context(&pAudioCodecCtx);

		avformat_close_input(&fmt_ctx);
		avformat_free_context(fmt_ctx);

/* done in avformat_close_input

		if (avio_ctx) {
			av_freep(&avio_ctx->buffer);
			av_freep(&avio_ctx);
		}
		*/
		pAudioCodecCtx   = NULL;
		pAudioCodec      = NULL;
		audioStreamIndex = -1;
	}  
}
bool FFmpegDecoder::DecodeVideo(AVPacket *avpkt, AVFrame *pOutFrame)
{
	bool res = false;

	if (pVideoCodecCtx)
	{
		if (avpkt && pOutFrame)
		{
			int got_picture_ptr = 0;
			//BHA			int videoFrameBytes = avcodec_decode_video2(pVideoCodecCtx, pOutFrame, &got_picture_ptr, avpkt);
			int error;

			if ((error = avcodec_send_packet(pVideoCodecCtx, avpkt)) < 0)
			{
				size_t errorSize = 256;
				std::string errorStr;
				errorStr.resize(errorSize);
				av_strerror(error, &errorStr.at(0), errorSize);
				//				errorManager_->internalError(logger_, "decodeFile: avcodec_send_packet failed: " + std::to_string(error) + ": "					+ errorStr, filename_);
				av_packet_unref(avpkt);
				return false;
			}

			//nbPacketsSent_++;

			//cout << "packet sent: " << nbPacketsSent << endl;

			if ((error = avcodec_receive_frame(pVideoCodecCtx, pOutFrame)) < 0)
			{
				if (error == AVERROR(EAGAIN))
				{
					/*
					 * For some files, you need to avcodec_send_packet() multiple times before
					 * you can receive a frame.
					 */
					av_packet_unref(avpkt);
					return false;
				}

				size_t errorSize = 256;
				std::string errorStr;
				errorStr.resize(errorSize);
				av_strerror(error, &errorStr.at(0), errorSize);
				//errorManager_->internalError(logger_, "avcodec_receive_frame failed " + std::to_string(error) + ": " + errorStr,					filename_);
				av_packet_unref(avpkt);
				return false;
			}

			//nbFramesReceived_++;
			res = true;
		}
	}

	return res;
}
bool FFmpegDecoder::DecodeAudio(AVPacket *avpkt, AVFrame *pOutFrame)	//, uint8_t* pOutBuffer, size_t nOutBufferSize)
{
	bool Rts_B = false;
	//BHA int decodedSize = 0;

	int packetSize = avpkt->size;
	uint8_t* pPacketData = (uint8_t*) avpkt->data;

//	while (packetSize > 0)
	{
		//BHA int sizeToDecode = nOutBufferSize;
		int got_picture_ptr = 0;

		//BHA int packetDecodedSize = avcodec_decode_audio4(pAudioCodecCtx, audioFrame, &got_picture_ptr, avpkt);
		int packetDecodedSize = 0;
		int error, bytesConsumed;

		//BHA packetDecodedSize = (size_t)avcodec_decode_audio4(pAudioCodecCtx, audioFrame, &got_picture_ptr, avpkt);
		bytesConsumed = avcodec_send_packet(pAudioCodecCtx, avpkt);
		if (bytesConsumed < 0)
		{
			size_t errorSize = 256;
			std::string errorStr;
			errorStr.resize(errorSize);
			av_strerror(bytesConsumed, &errorStr.at(0), errorSize);
			//				errorManager_->internalError(logger_, "decodeFile: avcodec_send_packet failed: " + std::to_string(error) + ": "					+ errorStr, filename_);
			av_packet_unref(avpkt);
			return false;
		}

		//nbPacketsSent_++;

		//cout << "packet sent: " << nbPacketsSent << endl;

		if ((error = avcodec_receive_frame(pAudioCodecCtx, pOutFrame)) < 0)
		{
			if (error == AVERROR(EAGAIN))
			{
				/*
				 * For some files, you need to avcodec_send_packet() multiple times before
				 * you can receive a frame.
				 */
				av_packet_unref(avpkt);
				return false;
			}

			size_t errorSize = 256;
			std::string errorStr;
			errorStr.resize(errorSize);
			av_strerror(error, &errorStr.at(0), errorSize);
			//errorManager_->internalError(logger_, "avcodec_receive_frame failed " + std::to_string(error) + ": " + errorStr,					filename_);
			av_packet_unref(avpkt);
			return false;
		}
//		convert_frame(swr, pAudioCodecCtx, pOutFrame, &pAudioData, &AudioSize, false);
//		printf("ptr %p sz %x\n", pAudioData, AudioSize);
		//nbFramesReceived_++;
		Rts_B = true;
	}	

	return Rts_B;
}


int ExtractAudio2(const char *pFnIn_c, const char *pFnOut_c)
{
	// Initialize the FFmpeg library
	//av_register_all();

	// Open the input file
	AVFormatContext *format_context = NULL;
	if (avformat_open_input(&format_context, pFnIn_c, NULL, NULL) != 0) {
		fprintf(stderr, "Error: Could not open input file\n");
		return 1;
	}

	// Retrieve the stream information
	if (avformat_find_stream_info(format_context, NULL) < 0) {
		fprintf(stderr, "Error: Could not find stream information\n");
		return 1;
	}

	// Find the audio stream and its decoder
	int audio_stream_index = -1;
	AVCodec *codec = NULL;
	for (unsigned int i = 0; i < format_context->nb_streams; i++) {
		if (format_context->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
			audio_stream_index = i;
			codec = avcodec_find_decoder(format_context->streams[i]->codecpar->codec_id);
			if (codec == NULL) {
				fprintf(stderr, "Error: Could not find audio decoder\n");
				return 1;
			}
			break;
		}
	}
	if (audio_stream_index == -1) {
		fprintf(stderr, "Error: Could not find audio stream\n");
		return 1;
	}

	// Open the codec
	AVCodecContext *codec_context = NULL;
	codec_context = avcodec_alloc_context3(codec);
	if (avcodec_parameters_to_context(codec_context, format_context->streams[audio_stream_index]->codecpar) < 0) {
		fprintf(stderr, "Error: Could not copy codec context\n");
		return 1;
	}
	if (avcodec_open2(codec_context, codec, NULL) < 0) {
		fprintf(stderr, "Error: Could not open codec\n");
		return 1;
	}

	// Initialize the packet and frame
	AVPacket *packet = av_packet_alloc();
	//av_init_packet(packet);
	packet->data = NULL;
	packet->size = 0;
	AVFrame *frame = av_frame_alloc();
	if (frame == NULL) {
		fprintf(stderr, "Error: Could not allocate frame\n");
		return 1;
	}

	// Open the output file
	FILE *outfile = fopen(pFnOut_c, "wb");
	if (outfile == NULL) {
		fprintf(stderr, "Error: Could not open output file\n");
		return 1;
	}

	// Read packets and decode the audio

	while (av_read_frame(format_context, packet) == 0) {
		if (packet->stream_index == audio_stream_index) {
			// Decode the audio
			int result = avcodec_send_packet(codec_context, packet);
			if (result < 0) {
				fprintf(stderr, "Error: Could not send packet for decoding\n");
				return 1;
			}
			result = avcodec_receive_frame(codec_context, frame);
			if (result < 0) {
				fprintf(stderr, "Error: Could not decode frame\n");
				return 1;
			}

			// Get the size of the decoded audio data
			int data_size = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16) * frame->nb_samples;	// *frame->channels;
			data_size = frame->linesize[0];
			// Write the PCM data to the output file
			//for (int i = 0; i < frame->channels; i++)
			int i = 0;
			{
				fwrite(frame->data[i], 1, data_size, outfile);
			}
		}

		// Free the packet and frame
		av_packet_unref(packet);
		av_frame_unref(frame);
	}

	// Close the output file
	fclose(outfile);

	// Free the allocated memory
	av_frame_free(&frame);
	av_packet_free(&packet);

	// Close the codec
	avcodec_close(codec_context);
	avcodec_free_context(&codec_context);

	// Close the input file
	avformat_close_input(&format_context);

	return 0;
}




/*
C:\tmp>ffmpeg -i sample-15s.mp3 -f s16le -acodec pcm_s16le out.pcm
ffmpeg version 5.1.2-full_build-www.gyan.dev Copyright (c) 2000-2022 the FFmpeg developers
	built with gcc 12.1.0 (Rev2, Built by MSYS2 project)
	configuration: --enable-gpl --enable-version3 --enable-static --disable-w32threads --disable-autodetect --enable-fontconfig --enable-iconv --enable-gnutls --enable-libxml2 --enable-gmp --enable-bzlib --enable-lzma --enable-libsnappy --enable-zlib --enable-librist --enable-libsrt --enable-libssh --enable-libzmq --enable-avisynth --enable-libbluray --enable-libcaca --enable-sdl2 --enable-libaribb24 --enable-libdav1d --enable-libdavs2 --enable-libuavs3d --enable-libzvbi --enable-librav1e --enable-libsvtav1 --enable-libwebp --enable-libx264 --enable-libx265 --enable-libxavs2 --enable-libxvid --enable-libaom --enable-libjxl --enable-libopenjpeg --enable-libvpx --enable-mediafoundation --enable-libass --enable-frei0r --enable-libfreetype --enable-libfribidi --enable-liblensfun --enable-libvidstab --enable-libvmaf --enable-libzimg --enable-amf --enable-cuda-llvm --enable-cuvid --enable-ffnvcodec --enable-nvdec --enable-nvenc --enable-d3d11va --enable-dxva2 --enable-libmfx --enable-libshaderc --enable-vulkan --enable-libplacebo --enable-opencl --enable-libcdio --enable-libgme --enable-libmodplug --enable-libopenmpt --enable-libopencore-amrwb --enable-libmp3lame --enable-libshine --enable-libtheora --enable-libtwolame --enable-libvo-amrwbenc --enable-libilbc --enable-libgsm --enable-libopencore-amrnb --enable-libopus --enable-libspeex --enable-libvorbis --enable-ladspa --enable-libbs2b --enable-libflite --enable-libmysofa --enable-librubberband --enable-libsoxr --enable-chromaprint
	libavutil      57. 28.100 / 57. 28.100
	libavcodec     59. 37.100 / 59. 37.100
	libavformat    59. 27.100 / 59. 27.100
	libavdevice    59.  7.100 / 59.  7.100
	libavfilter     8. 44.100 /  8. 44.100
	libswscale      6.  7.100 /  6.  7.100
	libswresample   4.  7.100 /  4.  7.100
	libpostproc    56.  6.100 / 56.  6.100
Input #0, mp3, from 'sample-15s.mp3':
	Metadata:
		encoder         : Lavf57.83.100
	Duration: 00:00:19.20, start: 0.025057, bitrate: 128 kb/s
	Stream #0:0: Audio: mp3, 44100 Hz, stereo, fltp, 128 kb/s
		Metadata:
			encoder         : Lavc57.10
Stream mapping:
	Stream #0:0 -> #0:0 (mp3 (mp3float) -> pcm_s16le (native))
Press [q] to stop, [?] for help
Output #0, s16le, to 'out.pcm':
	Metadata:
		encoder         : Lavf59.27.100
	Stream #0:0: Audio: pcm_s16le, 44100 Hz, stereo, int16_t, 1411 kb/s
		Metadata:
			encoder         : Lavc59.37.100 pcm_s16le
size=    3303kB time=00:00:19.17 bitrate=1411.2kbits/s speed= 721x
video:0kB audio:3303kB subtitle:0kB other streams:0kB global headers:0kB muxing overhead: 0.000000%

*/