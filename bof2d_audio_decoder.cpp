/*
   Copyright (c) 2000-2026, OnBings. All rights reserved.

   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
   KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
   PURPOSE.

   This module defines the interface to the Ffmpeg audio lib

   Author:      Bernard HARMEL: onbings@gmail.com
   Web:			    onbings.dscloud.me
   Revision:    1.0

   History:

   V 1.00  Sep 30 2000  BHA : Initial release
 */
#include "bof2d_audio_decoder.h"
#include "bof2d_av_codec.h"

#define WAVE_OUT_NB_CHANNEL  2
#define WAVE_SAMPLE_RATE  48000	//      16000	//desired sample rate of the output WAVE audio.
#define AVIO_CTX_BUF_SZ          4096	//definition of the WAVE header, there is plenty of documentation on this, so nothing more will be said

BEGIN_BOF2D_NAMESPACE()

int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
  BOF::BOF_BUFFER *audio_buf = (BOF::BOF_BUFFER *)opaque;

  buf_size = FFMIN(buf_size, (int)audio_buf->Size_U64);

  /* copy internal buffer data to buf */
  memcpy(buf, audio_buf->pData_U8, buf_size);
  //audio_buf->ptr += buf_size;
  //audio_buf->size -= buf_size;

  return buf_size;
}


Bof2dAudioDecoder::Bof2dAudioDecoder()
{

}
Bof2dAudioDecoder::~Bof2dAudioDecoder()
{
  Close();
}

BOFERR Bof2dAudioDecoder::Open(const std::string &_rInputFile_S)
{
  BOFERR    Rts_E;
  int       Sts_i;
  uint32_t  i_U32;

  // Just in case, close previously opened media file.
  Close();

//  mpAudioFormatCtx_X = avformat_alloc_context();
//  mpAudioBuffer_U8 = (uint8_t *)av_malloc(AVIO_CTX_BUF_SZ);
//  mpAudioAvioCtx_X = avio_alloc_context(mpAudioBuffer_U8, AVIO_CTX_BUF_SZ, 0, &mAudioBuffer_X, &read_packet, nullptr, nullptr);
//  mpAudioFormatCtx_X->pb = mpAudioAvioCtx_X;


  // Open media file.
  Sts_i = avformat_open_input(&mpAudioFormatCtx_X, _rInputFile_S.c_str(), nullptr, nullptr);
  FFMPEG_CHK_IF_ERR(Sts_i, "Couldn't open file " + _rInputFile_S, Rts_E);
  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    /* Retrieve stream information */
    Sts_i = avformat_find_stream_info(mpAudioFormatCtx_X, nullptr);
    FFMPEG_CHK_IF_ERR(Sts_i, "Couldn't find stream information in " + _rInputFile_S, Rts_E);
    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      for (i_U32 = 0; i_U32 < mpAudioFormatCtx_X->nb_streams; i_U32++)
      {
        if (mpAudioFormatCtx_X->streams[i_U32]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
          mpAudioCodecParam_X = mpAudioFormatCtx_X->streams[i_U32]->codecpar;
          break;
        }
      }
      if (mpAudioCodecParam_X == nullptr)
      {
        FFMPEG_CHK_IF_ERR(AVERROR_STREAM_NOT_FOUND, "No audio stream in " + _rInputFile_S, Rts_E);
      }
      else
      {
        mAudioStreamIndex_i = i_U32;
        //av_dict_set(&opts, "b", "2.5M", 0);
        mpAudioCodec_X = avcodec_find_decoder(mpAudioCodecParam_X->codec_id);
        if (mpAudioCodec_X == nullptr)
        {
          FFMPEG_CHK_IF_ERR(AVERROR_STREAM_NOT_FOUND, "Could not find decoder for codec " + std::to_string(mpAudioCodecParam_X->codec_id), Rts_E);
        }
        else
        {
          mpAudioCodecCtx_X = avcodec_alloc_context3(mpAudioCodec_X);
          if (mpAudioCodecCtx_X == nullptr)
          {
            FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not avcodec_alloc_context3 for codec " + std::to_string(mpAudioCodecParam_X->codec_id), Rts_E);
          }
          else
          {
            Sts_i = avcodec_parameters_to_context(mpAudioCodecCtx_X, mpAudioCodecParam_X);
            FFMPEG_CHK_IF_ERR(Sts_i, "Error in avcodec_parameters_to_context", Rts_E);
            if (Rts_E == BOF_ERR_NO_ERROR)
            {
              mpAudioCodecCtx_X->thread_count = 4;
              Sts_i = avcodec_open2(mpAudioCodecCtx_X, mpAudioCodec_X, nullptr);
              FFMPEG_CHK_IF_ERR(Sts_i, "Could not avcodec_open2", Rts_E);
              if (Rts_E == BOF_ERR_NO_ERROR)
              {
                mpAudioFrame_X = av_frame_alloc();
                if (mpAudioFrame_X == nullptr)
                {
                  FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not av_frame_alloc mpFrame_X", Rts_E);
                }
                else
                {
                  mpAudioFrameConverted_X = av_frame_alloc();
                  if (mpAudioFrameConverted_X == nullptr)
                  {
                    FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not av_frame_alloc mpAudioFrameConverted_X", Rts_E);
                  }
                  else
                  {
                    mpAudioBuffer_U8 = (uint8_t *)av_malloc(MAX_AUDIO_SIZE);
                    if (mpAudioBuffer_U8 == nullptr)
                    {
                      FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not av_frame_alloc mpAudioBuffer_U8", Rts_E);
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  
  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    mAudioTimeBase_X = mpAudioFormatCtx_X->streams[mAudioStreamIndex_i]->time_base;
/*
    mpAudioBuffer_U8 = (uint8_t *)av_malloc(AVIO_CTX_BUF_SZ);
    if (mpAudioBuffer_U8 == nullptr)
    {
      FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not av_malloc", Rts_E);
    }
    else
    {
      mpAudioAvioCtx_X = avio_alloc_context(mpAudioBuffer_U8, AVIO_CTX_BUF_SZ, 0, &mAudioBuffer_X, &read_packet, nullptr, nullptr);
      if (mpAudioAvioCtx_X == nullptr)
      {
        FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not avio_alloc_context", Rts_E);
      }
      else
      {
        //fmt_ctx->pb = mpAudioAvioCtx_X;
        //mpAudioFormatCtx_X->pb = mpAudioAvioCtx_X;
*/
        /* prepare resampler */
        mpAudioSwrCtx_X = swr_alloc();
        if (mpAudioSwrCtx_X == nullptr)
        {
          FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not swr_alloc", Rts_E);
        }
        else
        {
          Sts_i = av_opt_set_int(mpAudioSwrCtx_X, "in_channel_count", mpAudioCodecCtx_X->channels, 0);
          FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int in_channel_count", Rts_E);
          if (Rts_E == BOF_ERR_NO_ERROR)
          {
            Sts_i = av_opt_set_int(mpAudioSwrCtx_X, "out_channel_count", WAVE_OUT_NB_CHANNEL, 0);
            FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int out_channel_count", Rts_E);
            if (Rts_E == BOF_ERR_NO_ERROR)
            {
              Sts_i = av_opt_set_int(mpAudioSwrCtx_X, "in_channel_layout", mpAudioCodecCtx_X->channel_layout, 0);
              FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int in_channel_layout", Rts_E);
              if (Rts_E == BOF_ERR_NO_ERROR)
              {
                Sts_i = av_opt_set_int(mpAudioSwrCtx_X, "out_channel_layout", (WAVE_OUT_NB_CHANNEL == 2) ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO, 0);
                FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int out_channel_layout", Rts_E);
                if (Rts_E == BOF_ERR_NO_ERROR)
                {
                  Sts_i = av_opt_set_int(mpAudioSwrCtx_X, "in_sample_rate", mpAudioCodecCtx_X->sample_rate, 0);
                  FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int in_sample_rate", Rts_E);
                  if (Rts_E == BOF_ERR_NO_ERROR)
                  {
                    Sts_i = av_opt_set_int(mpAudioSwrCtx_X, "out_sample_rate", WAVE_SAMPLE_RATE, 0);
                    FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int out_sample_rate", Rts_E);
                    if (Rts_E == BOF_ERR_NO_ERROR)
                    {
                      Sts_i = av_opt_set_sample_fmt(mpAudioSwrCtx_X, "in_sample_fmt", mpAudioCodecCtx_X->sample_fmt, 0);
                      FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int in_sample_fmt", Rts_E);
                      if (Rts_E == BOF_ERR_NO_ERROR)
                      {
                        Sts_i = av_opt_set_sample_fmt(mpAudioSwrCtx_X, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
                        FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int out_sample_fmt", Rts_E);
                        if (Rts_E == BOF_ERR_NO_ERROR)
                        {
                          Sts_i = swr_init(mpAudioSwrCtx_X);
                          FFMPEG_CHK_IF_ERR(Sts_i, "Could not swr_init", Rts_E);
                          if (Rts_E == BOF_ERR_NO_ERROR)
                          {
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
/*
    }
  }
  */
  if (Rts_E != BOF_ERR_NO_ERROR)
  {
    Close();
  }

  return Rts_E;
}
#if 0
int avio_close2(AVIOContext *s)
{
 // URLContext *h;

  if (!s)
    return 0;

  avio_flush(s);
  //h = s->opaque;
  s->opaque = nullptr;

  av_freep(&s->buffer);
//  if (s->write_flag)
//    av_log(s, AV_LOG_VERBOSE, "Statistics: %d seeks, %d writeouts\n", s->seek_count, s->writeout_count);
//  else
//    av_log(s, AV_LOG_VERBOSE, "Statistics: %"PRId64" bytes read, %d seeks\n", s->bytes_read, s->seek_count);
  av_opt_free(s);

  avio_context_free(&s);

  return 0; // ffurl_close(h);
}

void avformat_close_input2(AVFormatContext **ps)
{
  AVFormatContext *s;
  AVIOContext *pb;

  if (!ps || !*ps)
    return;

  s = *ps;
  pb = s->pb;

  if ((s->iformat && strcmp(s->iformat->name, "image2") && s->iformat->flags & AVFMT_NOFILE) ||
    (s->flags & AVFMT_FLAG_CUSTOM_IO))
    pb = nullptr;

  //flush_packet_queue(s);

  if (s->iformat)
    if (s->iformat->read_close)
      s->iformat->read_close(s);

  avformat_free_context(s);

  *ps = nullptr;

  avio_close2(pb);
}
#endif
BOFERR Bof2dAudioDecoder::Read(AVFrame **_ppAudioData_X)
{
  BOFERR Rts_E = BOF_ERR_EINVAL;
  int Sts_i;

  if (_ppAudioData_X)
  {
    Rts_E = BOF_ERR_NO_ERROR;
    while (Rts_E == BOF_ERR_NO_ERROR)
    {
      Sts_i = av_read_frame(mpAudioFormatCtx_X, &mPacket_X);
      FFMPEG_CHK_IF_ERR(Sts_i, "Cannot avcodec_send_packet Audio", Rts_E);
      if (Rts_E == BOF_ERR_NO_ERROR)
      {
        if (mPacket_X.stream_index == mAudioStreamIndex_i)
        {
          Sts_i = avcodec_send_packet(mpAudioCodecCtx_X, &mPacket_X);
          FFMPEG_CHK_IF_ERR(Sts_i, "Cannot avcodec_send_packet Audio", Rts_E);

          if (Rts_E == BOF_ERR_NO_ERROR)
          {
            mNbAudioPacketSent_U64++;
            Sts_i = avcodec_receive_frame(mpAudioCodecCtx_X, mpAudioFrame_X);
            FFMPEG_CHK_IF_ERR(Sts_i, "Cannot avcodec_receive_frame Audio", Rts_E);
            if (Rts_E == BOF_ERR_NO_ERROR)
            {
              mNbAudioFrameReceived_U64++;
              printf("AudioFrame Snt/Rcv %zd/%zd Pts %zd Fmt %s NbSmp %d Rate %d Buf %x:%p Layout %zx\n", mNbAudioPacketSent_U64, mNbAudioFrameReceived_U64, mpAudioFrame_X->pts, av_get_sample_fmt_name((AVSampleFormat)mpAudioFrame_X->format),
                mpAudioFrame_X->nb_samples, mpAudioFrame_X->sample_rate, mpAudioFrame_X->linesize[0], mpAudioFrame_X->data[0],
                mpAudioFrame_X->channel_layout);

              //continuer leak ici 
              //!! break dans main fct
              //! 
              //Rts_E = ConvertAudio(false, mpAudioFrame_X, mpAudioFrameConverted_X,  WAVE_OUT_NB_CHANNEL, (WAVE_OUT_NB_CHANNEL == 2) ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO, WAVE_SAMPLE_RATE, AV_SAMPLE_FMT_S16);
             Rts_E = BOF_ERR_ADDRESS;

              if (Rts_E == BOF_ERR_NO_ERROR)
              {
                *_ppAudioData_X = mpAudioFrameConverted_X;
              }
              av_frame_unref(mpAudioFrame_X);
              av_packet_unref(&mPacket_X);
              break;  //leave while so av_packet_unref(&mPacket_X);
            }
          }
        }
        av_packet_unref(&mPacket_X);  //Made in while
      }
    }
  }
  return Rts_E;
}

BOFERR Bof2dAudioDecoder::ConvertAudio(bool _Flush_B, AVFrame *_pInAudioFrame_X, AVFrame *_pOutAudioFrame_X, uint32_t _OutNbAudioChannel_U32, uint32_t _OutAudioChannelLayout_U32, uint32_t _OutAudioSampleRateInHz_U32,  enum AVSampleFormat _OutAudioSampleFmt_E)
{
  BOFERR Rts_E = BOF_ERR_EINVAL;
  uint32_t NbAudioSample_U32, AudioBufferSize_U32, AudioConvertSizePerChannelInSample_U32;
  int64_t AudioDelay_S64;

  if ((_pInAudioFrame_X) && (_pOutAudioFrame_X) && (_pInAudioFrame_X->nb_samples))
  {
    AudioDelay_S64 = swr_get_delay(mpAudioSwrCtx_X, mpAudioCodecCtx_X->sample_rate);
    NbAudioSample_U32 = (int)av_rescale_rnd(AudioDelay_S64 + _pInAudioFrame_X->nb_samples, _OutAudioSampleRateInHz_U32, mpAudioCodecCtx_X->sample_rate, AV_ROUND_UP);
    AudioBufferSize_U32 = av_samples_get_buffer_size(NULL, _OutNbAudioChannel_U32, NbAudioSample_U32, _OutAudioSampleFmt_E, 0);

    //pAudioBuffer_U8 = (uint8_t *)av_mallocz(AudioBufferSize_U32);
    BOF_ASSERT(AudioBufferSize_U32 <= MAX_AUDIO_SIZE);
    memset(mpAudioBuffer_U8, 0, AudioBufferSize_U32);
    Rts_E = BOF_ERR_NO_ERROR;
    av_samples_fill_arrays(_pOutAudioFrame_X->data, _pOutAudioFrame_X->linesize, mpAudioBuffer_U8, _OutNbAudioChannel_U32, NbAudioSample_U32, _OutAudioSampleFmt_E, 0);
    //int sz = NbAudioSample_U32 * sizeof(int16_t) * _OutNbAudioChannel_U32;
    /*
      * !flush is used to check if we are flushing any remaining
      * conversion buffers...
      */
    AudioConvertSizePerChannelInSample_U32 = swr_convert(mpAudioSwrCtx_X, &mpAudioBuffer_U8, NbAudioSample_U32, _Flush_B ? nullptr : (const uint8_t **)_pInAudioFrame_X->data, _Flush_B ? 0 : _pInAudioFrame_X->nb_samples);

    _pOutAudioFrame_X->nb_samples = AudioConvertSizePerChannelInSample_U32;
    _pOutAudioFrame_X->channels = _OutNbAudioChannel_U32;
    _pOutAudioFrame_X->channel_layout = _OutAudioChannelLayout_U32; // (WAVE_OUT_NB_CHANNEL == 2) ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO
    _pOutAudioFrame_X->format = _OutAudioSampleFmt_E;
    _pOutAudioFrame_X->sample_rate = _OutAudioSampleRateInHz_U32;
    _pOutAudioFrame_X->pkt_pos = _pInAudioFrame_X->pkt_pos;
    _pOutAudioFrame_X->pkt_duration = _pInAudioFrame_X->pkt_duration;
    _pOutAudioFrame_X->pkt_size = _pInAudioFrame_X->pkt_size;
     
    int16_t *pData_S16 = (int16_t *)_pOutAudioFrame_X->data[0];
    printf("Audio %x:%p nbs %d ch %d layout %zx Fmt %d Rate %d Pos %zd Dur %zd Sz %d\n", _pOutAudioFrame_X->linesize[0], _pOutAudioFrame_X->data[0],
      _pOutAudioFrame_X->nb_samples, _pOutAudioFrame_X->channels, _pOutAudioFrame_X->channel_layout, _pOutAudioFrame_X->format, _pOutAudioFrame_X->sample_rate,  
      _pOutAudioFrame_X->pkt_pos, _pOutAudioFrame_X->pkt_duration, _pOutAudioFrame_X->pkt_size);
    printf("Data %04x %04x %04x %04x %04x %04x %04x %04x\n", pData_S16[0], pData_S16[1], pData_S16[2], pData_S16[3], pData_S16[4], pData_S16[5], pData_S16[6], pData_S16[7]);
  }
  return Rts_E;
}
/*
<< frame_->width << "x" << frame_->height << " in "
//        << av_color_space_name(frame_->colorspace) << ", "
//        << av_color_range_name(frame_->color_range) << ", ";
//    frame_->interlaced_frame ? cout << "I" : cout << "P";


if (frame_->interlaced_frame)
{
  videoParams_.interlaced_ = true;

  handleInterlaceFrame(frame_);
}
else
{
  // progressive
  convert(*frame_, *frameDest_);

  afterConvert = std::chrono::steady_clock::now();

  sendFrame(*frameDest_);
}
*/

BOFERR Bof2dAudioDecoder::Close()
{
  BOFERR Rts_E;

//  avio_context_free(&mpAudioAvioCtx_X);
  av_freep(&mpAudioBuffer_U8);

  swr_free(&mpAudioSwrCtx_X);
  if (mpAudioFrame_X)
  {
    av_freep(&mpAudioFrame_X->data[0]);
    av_frame_free(&mpAudioFrame_X);
  }
  if (mpAudioFrameConverted_X)
  {
    av_freep(&mpAudioFrameConverted_X->data[0]);
    av_frame_free(&mpAudioFrameConverted_X);
  }
  //avcodec_close: Do not use this function. Use avcodec_free_context() to destroy a codec context(either open or closed)
  avcodec_close(mpAudioCodecCtx_X);
  avcodec_free_context(&mpAudioCodecCtx_X);
  
  avformat_close_input(&mpAudioFormatCtx_X);

  mAudioStreamIndex_i = -1;
  mpAudioCodecParam_X = nullptr;
  mpAudioCodec_X = nullptr;

  mAudioTimeBase_X = { 0, 0 }; //or av_make_q

  mNbAudioPacketSent_U64 = 0;
  mNbAudioFrameReceived_U64 = 0;
  mNbTotalAudioFrame_U64 = 0;

  Rts_E = BOF_ERR_NO_ERROR;
  return Rts_E;
}

bool Bof2dAudioDecoder::IsAudioStreamPresent()
{
  return(mAudioStreamIndex_i != -1);
}

END_BOF2D_NAMESPACE()