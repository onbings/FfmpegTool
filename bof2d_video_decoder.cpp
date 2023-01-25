/*
   Copyright (c) 2000-2026, OnBings. All rights reserved.

   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
   KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
   PURPOSE.

   This module defines the interface to the Ffmpeg video lib

   Author:      Bernard HARMEL: onbings@gmail.com
   Web:			    onbings.dscloud.me
   Revision:    1.0

   History:

   V 1.00  Sep 30 2000  BHA : Initial release
 */
#include "bof2d_video_decoder.h"

BEGIN_BOF2D_NAMESPACE()

Bof2dVideoDecoder::Bof2dVideoDecoder()
{
  mVidDecOptionParam_X.push_back({ nullptr, "V_WIDTH", "Specifies converted picture width","","", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_VARIABLE(mVidDecOption_X.Width_U32, UINT32, 2, 32768) });
  mVidDecOptionParam_X.push_back({ nullptr, "V_HEIGHT", "Specifies converted picture height","","", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_VARIABLE(mVidDecOption_X.Height_U32, UINT32, 2, 32768) });
  mVidDecOptionParam_X.push_back({ nullptr, "V_BPS", "Specifies converted picture bit per pixel","","", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_VARIABLE(mVidDecOption_X.Bps_U32, UINT32, 1, 64) });
}
Bof2dVideoDecoder::~Bof2dVideoDecoder()
{
  Close();
}

BOFERR Bof2dVideoDecoder::Open(const std::string &_rInputFile_S, const std::string &_rOption_S)
{
  BOFERR    Rts_E;
  int       Sts_i;
  uint32_t  i_U32, BufferImgSize_U32;
  uint8_t   *pVidDecBuffer_U8;
  int       SrcRange_i, DstRange_i, Brightness_i, Contrast_i, Saturation_i;
  const int *pSwsCoef_i;
  int *pInvCoef_i, *pCoef_i;
  char pFilterParam_c[0x1000];
  const AVFilter *pBufferSrcFlt_X, *pBufferSinkFlt_X;

  if (mDecoderReady_B == false)
  {
    Close();
    // Open media file.
    Sts_i = avformat_open_input(&mpVidDecFormatCtx_X, _rInputFile_S.c_str(), nullptr, nullptr);
    FFMPEG_CHK_IF_ERR(Sts_i, "Couldn't open file " + _rInputFile_S, Rts_E);
    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      /* Retrieve stream information */
      Sts_i = avformat_find_stream_info(mpVidDecFormatCtx_X, nullptr);
      FFMPEG_CHK_IF_ERR(Sts_i, "Couldn't find stream information in " + _rInputFile_S, Rts_E);
      if (Rts_E == BOF_ERR_NO_ERROR)
      {
        for (i_U32 = 0; i_U32 < mpVidDecFormatCtx_X->nb_streams; i_U32++)
        {
          if (mpVidDecFormatCtx_X->streams[i_U32]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
          {
            mpVidDecCodecParam_X = mpVidDecFormatCtx_X->streams[i_U32]->codecpar;
            break;
          }
        }
        if (mpVidDecCodecParam_X == nullptr)
        {
          FFMPEG_CHK_IF_ERR(AVERROR_STREAM_NOT_FOUND, "No video stream in " + _rInputFile_S, Rts_E);
        }
        else
        {
          mVideoStreamIndex_i = i_U32;
          //av_dict_set(&opts, "b", "2.5M", 0);
          mpVidDecCodec_X = avcodec_find_decoder(mpVidDecCodecParam_X->codec_id);
          if (mpVidDecCodec_X == nullptr)
          {
            FFMPEG_CHK_IF_ERR(AVERROR_STREAM_NOT_FOUND, "Could not find decoder for codec " + std::to_string(mpVidDecCodecParam_X->codec_id), Rts_E);
          }
          else
          {
            mpVidDecCodecCtx_X = avcodec_alloc_context3(mpVidDecCodec_X);
            if (mpVidDecCodecCtx_X == nullptr)
            {
              FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not avcodec_alloc_context3 for codec " + std::to_string(mpVidDecCodecParam_X->codec_id), Rts_E);
            }
            else
            {
              Sts_i = avcodec_parameters_to_context(mpVidDecCodecCtx_X, mpVidDecCodecParam_X);
              FFMPEG_CHK_IF_ERR(Sts_i, "Error in avcodec_parameters_to_context", Rts_E);
              if (Rts_E == BOF_ERR_NO_ERROR)
              {
                mpVidDecCodecCtx_X->thread_count = 4;
                Sts_i = avcodec_open2(mpVidDecCodecCtx_X, mpVidDecCodec_X, nullptr);
                FFMPEG_CHK_IF_ERR(Sts_i, "Could not avcodec_open2", Rts_E);
                if (Rts_E == BOF_ERR_NO_ERROR)
                {
                  mpVidDecFrame_X = av_frame_alloc();
                  if (mpVidDecFrame_X == nullptr)
                  {
                    FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not av_frame_alloc mpFrame_X", Rts_E);
                  }
                  else
                  {
                    mpVidDecDestFrame_X = av_frame_alloc();
                    if (mpVidDecDestFrame_X == nullptr)
                    {
                      FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not av_frame_alloc mpDestFrame_X", Rts_E);
                    }
                    else
                    {
                      mpVidDecFrameFiltered_X = av_frame_alloc();
                      if (mpVidDecFrameFiltered_X == nullptr)
                      {
                        FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not av_frame_alloc mpFrameFiltered_X", Rts_E);
                      }
                      else
                      {
                        //Allocate global video decoder buffer
                        mPixelFmt_E = AV_PIX_FMT_BGR24;
                        BufferImgSize_U32 = av_image_get_buffer_size(mPixelFmt_E, mpVidDecCodecCtx_X->width, mpVidDecCodecCtx_X->height, mVidDecAllocAlignment_i);
                        pVidDecBuffer_U8 = (uint8_t *)av_malloc(BufferImgSize_U32);
                        if (pVidDecBuffer_U8 == nullptr)
                        {
                          FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not av_mallocz global video", Rts_E);
                        }
                        else
                        {
                          //Store global audio decoder buffer characteristics
                          mVidDecOut_X.Data_X.SetStorage(BufferImgSize_U32, 0, pVidDecBuffer_U8);
                          avpicture_fill((AVPicture *)mpVidDecFrame_X, mVidDecOut_X.Data_X.pData_U8, mPixelFmt_E, mpVidDecCodecCtx_X->width, mpVidDecCodecCtx_X->height);
                          mpVidDecFrame_X->width = mpVidDecCodecCtx_X->width;
                          mpVidDecFrame_X->height = mpVidDecCodecCtx_X->height;
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
    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      mVidDecFrameRate_X = mpVidDecFormatCtx_X->streams[mVideoStreamIndex_i]->avg_frame_rate;
      mVidDecTimeBase_X = mpVidDecFormatCtx_X->streams[mVideoStreamIndex_i]->time_base;
      mVidDecFramePerSecond_lf = av_q2d(mpVidDecFormatCtx_X->streams[mVideoStreamIndex_i]->r_frame_rate);

      mVidDecColorPrimaries_E = mpVidDecCodecCtx_X->color_primaries;
      mVidDecColorRange_E = mpVidDecCodecCtx_X->color_range;
      mVidDecColorSpace_E = mpVidDecCodecCtx_X->colorspace;
      mVidDecColorTrf_E = mpVidDecCodecCtx_X->color_trc;
      mVidDecInPixFmt_E = mpVidDecCodecCtx_X->pix_fmt;
      /*
      if (mOutputCodec_S == "h264")
      {
        mVidDecVideoOutPixFmt_E = mVidDecInPixFmt_E;
      }
      */
      mNbTotalVidDecFrame_U64 = static_cast<int64_t>(mpVidDecFormatCtx_X->duration / 1.0e6 * mVidDecFrameRate_X.num / mVidDecFrameRate_X.den);

      // SWScale
      if (mpVidDecCodecCtx_X->color_primaries == AVColorPrimaries::AVCOL_PRI_BT2020)
      {
        mVidDecRgbPixFmt_E = AVPixelFormat::AV_PIX_FMT_RGB48LE;
      }
      else
      {
        mVidDecRgbPixFmt_E = AVPixelFormat::AV_PIX_FMT_RGB24;
      }
      Sts_i = av_image_alloc(mpVidDecDestFrame_X->data, mpVidDecDestFrame_X->linesize, mpVidDecCodecParam_X->width, mpVidDecCodecParam_X->height, mVidDecRgbPixFmt_E, mVidDecAllocAlignment_i);
      FFMPEG_CHK_IF_ERR(Sts_i, "Error in av_image_alloc", Rts_E);
      if (Rts_E == BOF_ERR_NO_ERROR)
      {
        printf("Convert from %s %s to %s %dx%d\n", av_get_pix_fmt_name(mpVidDecCodecCtx_X->pix_fmt), av_color_space_name(mpVidDecCodecCtx_X->colorspace),
          av_get_pix_fmt_name(mVidDecRgbPixFmt_E), mpVidDecCodecParam_X->width, mpVidDecCodecParam_X->height);

        //TODO a remplacer par sws_getContext !!!
        // sws_getCachedContext(nullptr:  If context is NULL, just calls sws_getContext()
        mpVideoSwsCtx_X = sws_getCachedContext(nullptr, mpVidDecCodecCtx_X->width, mpVidDecCodecCtx_X->height, mpVidDecCodecCtx_X->pix_fmt, mpVidDecCodecParam_X->width, mpVidDecCodecParam_X->height, mVidDecRgbPixFmt_E, SWS_BICUBIC, nullptr, nullptr, nullptr);

        if (mpVideoSwsCtx_X == nullptr)
        {
          FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not sws_getCachedContext", Rts_E);
        }
        else
        {
          pSwsCoef_i = sws_getCoefficients(mpVidDecCodecCtx_X->colorspace);
          Sts_i = sws_getColorspaceDetails(mpVideoSwsCtx_X, &pInvCoef_i, &SrcRange_i, &pCoef_i, &DstRange_i, &Brightness_i, &Contrast_i, &Saturation_i);
          FFMPEG_CHK_IF_ERR(Sts_i, "Swscale conversion not supported", Rts_E);
          if (Rts_E == BOF_ERR_NO_ERROR)
          {
            // color range: (1=jpeg / 0=mpeg)
            //  SrcRange_i = 0;
            //  DstRange_i = 1;

            Sts_i = sws_setColorspaceDetails(mpVideoSwsCtx_X, pSwsCoef_i, SrcRange_i, pSwsCoef_i, DstRange_i, Brightness_i, Contrast_i, Saturation_i);
            FFMPEG_CHK_IF_ERR(Sts_i, "Swscale conversion not supported", Rts_E);
            if (Rts_E == BOF_ERR_NO_ERROR)
            {
            }
          }
        }
      }
    }
    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      pBufferSrcFlt_X = avfilter_get_by_name("buffer");
      if (pBufferSrcFlt_X == nullptr)
      {
        FFMPEG_CHK_IF_ERR(-BOF_ERR_NOT_FOUND, "Could not avfilter_get_by_name('buffer')", Rts_E);
      }
      else
      {
        pBufferSinkFlt_X = avfilter_get_by_name("buffersink");
        if (pBufferSinkFlt_X == nullptr)
        {
          FFMPEG_CHK_IF_ERR(-BOF_ERR_NOT_FOUND, "Could not avfilter_get_by_name('buffersink')", Rts_E);
        }
        else
        {
          mpVidDecFilterOut_X = avfilter_inout_alloc();
          if (mpVidDecFilterOut_X == nullptr)
          {
            FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not avfilter_inout_alloc out", Rts_E);
          }
          else
          {
            mpVidDecFilterIn_X = avfilter_inout_alloc();

            mpVidDecFltGraph_X = avfilter_graph_alloc();
            if (mpVidDecFilterOut_X == nullptr)
            {
              FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not avfilter_inout_alloc out", Rts_E);
            }
            else
            {
              /* buffer video source: the decoded frames from the decoder will be inserted here. */
              snprintf(pFilterParam_c, sizeof(pFilterParam_c),
                "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
                mpVidDecCodecCtx_X->width, mpVidDecCodecCtx_X->height,
                mpVidDecCodecCtx_X->pix_fmt,
                mVidDecTimeBase_X.num, mVidDecTimeBase_X.den,
                mpVidDecCodecCtx_X->sample_aspect_ratio.num,
                mpVidDecCodecCtx_X->sample_aspect_ratio.den);


              Sts_i = avfilter_graph_create_filter(&mpVidDecFilterSrcCtx_X, pBufferSrcFlt_X, "in", pFilterParam_c, nullptr, mpVidDecFltGraph_X);
              FFMPEG_CHK_IF_ERR(Sts_i, "Could not avfilter_graph_create_filter In", Rts_E);
              if (Rts_E == BOF_ERR_NO_ERROR)
              {
                /* buffer video sink: to terminate the filter chain. */
                Sts_i = avfilter_graph_create_filter(&mpVidDecFilterSinkCtx_X, pBufferSinkFlt_X, "out", nullptr, nullptr, mpVidDecFltGraph_X);
                FFMPEG_CHK_IF_ERR(Sts_i, "Could not avfilter_graph_create_filter Out", Rts_E);
                if (Rts_E == BOF_ERR_NO_ERROR)
                {

                  /*
                   * Set the endpoints for the filter graph. The filter_graph will
                   * be linked to the graph described by filters_descr.
                   */

                   /*
                    * The buffer source output must be connected to the input pad of
                    * the first filter described by filters_descr; since the first
                    * filter input label is not specified, it is set to "in" by
                    * default.
                    */
                  mpVidDecFilterOut_X->name = av_strdup("in");
                  mpVidDecFilterOut_X->filter_ctx = mpVidDecFilterSrcCtx_X;
                  mpVidDecFilterOut_X->pad_idx = 0;
                  mpVidDecFilterOut_X->next = nullptr;

                  /*
                   * The buffer sink input must be connected to the output pad of
                   * the last filter described by filters_descr; since the last
                   * filter output label is not specified, it is set to "out" by
                   * default.
                   */
                  mpVidDecFilterIn_X->name = av_strdup("out");
                  mpVidDecFilterIn_X->filter_ctx = mpVidDecFilterSinkCtx_X;
                  mpVidDecFilterIn_X->pad_idx = 0;
                  mpVidDecFilterIn_X->next = nullptr;

                  Sts_i = avfilter_graph_parse_ptr(mpVidDecFltGraph_X, "yadif=mode=1", &mpVidDecFilterIn_X, &mpVidDecFilterOut_X, nullptr);
                  FFMPEG_CHK_IF_ERR(Sts_i, "Could not avfilter_graph_parse_ptr", Rts_E);
                  if (Rts_E == BOF_ERR_NO_ERROR)
                  {
                    Sts_i = avfilter_graph_config(mpVidDecFltGraph_X, nullptr);
                    FFMPEG_CHK_IF_ERR(Sts_i, "Could not avfilter_graph_config", Rts_E);
                    if (Rts_E == BOF_ERR_NO_ERROR)
                    {
                      mDecoderReady_B = true;
                      //mIsInterlaced_B = false;
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    if (Rts_E != BOF_ERR_NO_ERROR)
    {
      Close();
    }
  }
  return Rts_E;
}

BOFERR Bof2dVideoDecoder::Close()
{
  BOFERR Rts_E;

  avfilter_graph_free(&mpVidDecFltGraph_X);
  avfilter_inout_free(&mpVidDecFilterIn_X);
  avfilter_inout_free(&mpVidDecFilterOut_X);

  sws_freeContext(mpVideoSwsCtx_X);
  mpVideoSwsCtx_X = nullptr;
  if (mpVidDecFrame_X)
  {
    av_freep(&mpVidDecFrame_X->data[0]);
    av_frame_free(&mpVidDecFrame_X);
  }
  if (mpVidDecDestFrame_X)
  {
    av_freep(&mpVidDecDestFrame_X->data[0]);
    av_frame_free(&mpVidDecDestFrame_X);
  }
  if (mpVidDecFrameFiltered_X)
  {
    av_freep(&mpVidDecFrameFiltered_X->data[0]);
    av_frame_free(&mpVidDecFrameFiltered_X);
  }
  //avcodec_close: Do not use this function. Use avcodec_free_context() to destroy a codec context(either open or closed)
  avcodec_free_context(&mpVidDecCodecCtx_X);
  avformat_close_input(&mpVidDecFormatCtx_X);

  av_freep(mVidDecOut_X.Data_X.pData_U8);

  mDecoderReady_B = false;
  mReadBusy_B = false;
  mVidDecOption_X.Reset();
  mVidDecOut_X.Reset();
  mPixelFmt_E = AV_PIX_FMT_NONE;

  //mVidDecPacket_X
  mVideoStreamIndex_i = -1;
  mpVidDecCodecParam_X = nullptr;
  mpVidDecCodec_X = nullptr;
  mpVidDecFrame_X = nullptr;
  mpVidDecDestFrame_X = nullptr;
  mpVidDecFrameFiltered_X = nullptr;

  mVidDecColorPrimaries_E = AVColorPrimaries::AVCOL_PRI_RESERVED;
  mVidDecColorRange_E = AVColorRange::AVCOL_RANGE_UNSPECIFIED;
  mVidDecColorTrf_E = AVColorTransferCharacteristic::AVCOL_TRC_RESERVED;
  mVidDecColorSpace_E = AVColorSpace::AVCOL_SPC_RESERVED;

  mVidDecInPixFmt_E = AVPixelFormat::AV_PIX_FMT_NONE;
  mVidDecVideoOutPixFmt_E = AVPixelFormat::AV_PIX_FMT_NONE;
  mVidDecRgbPixFmt_E = AVPixelFormat::AV_PIX_FMT_NONE;

  mIsVideoInterlaced_B = false;
  mVidDecFrameRate_X = { 0, 0 };  //or av_make_q
  mVidDecTimeBase_X = { 0, 0 };
  mVidDecFramePerSecond_lf = 0.0;

  mNbVidDecPacketSent_U64 = 0;
  mNbVidDecFrameReceived_U64 = 0;
  mNbTotalVidDecFrame_U64 = 0;

  mpVideoSwsCtx_X = nullptr;

  mpVidDecFilterIn_X = nullptr;
  mpVidDecFilterOut_X = nullptr;
  mpVidDecFltGraph_X = nullptr;
  mpVidDecFilterSinkCtx_X = nullptr;
  mpVidDecFilterSrcCtx_X = nullptr;

  Rts_E = BOF_ERR_NO_ERROR;
  return Rts_E;
}

BOFERR Bof2dVideoDecoder::BeginRead(BOF2D_VID_DEC_OUT &_rVidDecOut_X)
{
  BOFERR Rts_E = BOF_ERR_EOF;
  int Sts_i;

  _rVidDecOut_X.Reset();
  if (IsVideoStreamPresent())
  {
    if (mReadBusy_B)
    {
      Rts_E = BOF_ERR_EBUSY;
    }
    else
    {
      mReadBusy_B = true;
      Rts_E = BOF_ERR_NO_ERROR;
      mVidDecOut_X.Data_X.Size_U64 = 0;
      while (Rts_E == BOF_ERR_NO_ERROR)
      {
        Sts_i = av_read_frame(mpVidDecFormatCtx_X, &mVidDecPacket_X);
        FFMPEG_CHK_IF_ERR(Sts_i, "Cannot av_read_frame Video", Rts_E);
        if (Rts_E == BOF_ERR_NO_ERROR)
        {
          if (mVidDecPacket_X.stream_index == mVideoStreamIndex_i)
          {
            // Convert ffmpeg frame timestamp to real frame number.
            //int64_t numberFrame = (double)((int64_t)pts -  pFormatCtx->streams[videoStreamIndex]->start_time) *  videoBaseTime * videoFramePerSecond;

            Sts_i = avcodec_send_packet(mpVidDecCodecCtx_X, &mVidDecPacket_X);
            FFMPEG_CHK_IF_ERR(Sts_i, "Cannot avcodec_send_packet Video", Rts_E);

            if (Rts_E == BOF_ERR_NO_ERROR)
            {
              mNbVidDecPacketSent_U64++;
              Sts_i = avcodec_receive_frame(mpVidDecCodecCtx_X, mpVidDecFrame_X);
              FFMPEG_CHK_IF_ERR(Sts_i, "Cannot avcodec_receive_frame Video", Rts_E);
              if (Rts_E == BOF_ERR_NO_ERROR)
              {
                mNbVidDecFrameReceived_U64++;
                printf("VideoFrame Snt/Rcv %zd/%zd Pts %zd Fmt %s Width %d Height %d Buf %x:%p\n", mNbVidDecPacketSent_U64, mNbVidDecFrameReceived_U64, mpVidDecFrame_X->pts, av_get_pix_fmt_name((AVPixelFormat)mpVidDecFrame_X->format),
                  mpVidDecFrame_X->width, mpVidDecFrame_X->height, mpVidDecFrame_X->linesize[0], mpVidDecFrame_X->data[0]);

                Rts_E = ConvertVideo(); 
                if (Rts_E == BOF_ERR_NO_ERROR)
                {
                  _rVidDecOut_X.Data_X = mVidDecOut_X.Data_X;
                  _rVidDecOut_X.Size_X = mVidDecOut_X.Size_X;
                  _rVidDecOut_X.LineSize_S32 = mVidDecOut_X.LineSize_S32;
                }
                av_packet_unref(&mVidDecPacket_X);
                break;  //leave while so av_packet_unref(&mVidDecPacket_X);
              }
            }
          }
          av_packet_unref(&mVidDecPacket_X);  //Made in while
        }
      }
    }
  }
  return Rts_E;
}

BOFERR Bof2dVideoDecoder::EndRead()
{
  BOFERR Rts_E = BOF_ERR_EOF;

  if (IsVideoStreamPresent())
  {
    if (mReadBusy_B)
    {
      mReadBusy_B = false;
      Rts_E = BOF_ERR_NO_ERROR;
    }
    else
    {
      Rts_E = BOF_ERR_PENDING;
    }
  }
  return Rts_E;
}

BOFERR Bof2dVideoDecoder::ConvertVideo()  
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
  int Sts_i;

  Sts_i = sws_scale(mpVideoSwsCtx_X, mpVidDecFrame_X->data, mpVidDecFrame_X->linesize, 0, height, mpVidDecFrame_X->data, frame->linesize);
  
  return Rts_E;


  uint32_t NbAudioSample_U32, AudioBufferSize_U32, ChannelSizeOfAudioConverted_U32, TotalSizeOfAudioConverted_U32;
  int64_t AudioDelay_S64;

  _rNbAudioSampleConvertedPerChannel_U32 = 0;
  AudioDelay_S64 = swr_get_delay(mpAudDecSwrCtx_X, mpAudDecCodecCtx_X->sample_rate);
  NbAudioSample_U32 = (int)av_rescale_rnd(AudioDelay_S64 + mpAudDecFrame_X->nb_samples, mAudDecOption_X.SampleRateInHz_U32, mpAudDecCodecCtx_X->sample_rate, AV_ROUND_UP);
  AudioBufferSize_U32 = av_samples_get_buffer_size(nullptr, mAudDecOption_X.NbChannel_U32, NbAudioSample_U32, mSampleFmt_E, 0);

  Rts_E = BOF_ERR_TOO_BIG;
  BOF_ASSERT(AudioBufferSize_U32 <= mAudDecOut_X.InterleavedData_X.Capacity_U64);
  if (AudioBufferSize_U32 <= mAudDecOut_X.InterleavedData_X.Capacity_U64)
  {
    Rts_E = BOF_ERR_NO_ERROR;
    av_samples_fill_arrays(mpAudDecFrameConverted_X->data, mpAudDecFrameConverted_X->linesize, mAudDecOut_X.InterleavedData_X.pData_U8, mAudDecOption_X.NbChannel_U32, NbAudioSample_U32, mSampleFmt_E, 0);
    _rNbAudioSampleConvertedPerChannel_U32 = swr_convert(mpAudDecSwrCtx_X, &mAudDecOut_X.InterleavedData_X.pData_U8, NbAudioSample_U32, (const uint8_t **)mpAudDecFrame_X->data, mpAudDecFrame_X->nb_samples);
    ChannelSizeOfAudioConverted_U32 = (_rNbAudioSampleConvertedPerChannel_U32 * mAudDecOption_X.NbBitPerSample_U32) / 8;
    TotalSizeOfAudioConverted_U32 = (ChannelSizeOfAudioConverted_U32 * mAudDecOption_X.NbChannel_U32);
    //printf("---->NbAudioSample %d memset %d NbAudioConvertSamplePerChannel %d->%d linesize %d\n", NbAudioSample_U32, AudioBufferSize_U32, NbAudioSampleConvertedPerChannel_U32, _rTotalSizeOfAudioConverted_U32, mpAudDecFrameConverted_X->linesize[0]);
    BOF_ASSERT(TotalSizeOfAudioConverted_U32 <= static_cast<uint32_t>(mpAudDecFrameConverted_X->linesize[0]));
    if (TotalSizeOfAudioConverted_U32 <= static_cast<uint32_t>(mpAudDecFrameConverted_X->linesize[0]))
    {
      mpAudDecFrameConverted_X->nb_samples = _rNbAudioSampleConvertedPerChannel_U32;
      mpAudDecFrameConverted_X->channels = mAudDecOption_X.NbChannel_U32;
      mpAudDecFrameConverted_X->channel_layout = static_cast<uint32_t>(mAudDecOption_X.ChannelLayout_U64);
      mpAudDecFrameConverted_X->format = mSampleFmt_E;
      mpAudDecFrameConverted_X->sample_rate = mAudDecOption_X.SampleRateInHz_U32;
      mpAudDecFrameConverted_X->pkt_pos = mpAudDecFrame_X->pkt_pos;
      mpAudDecFrameConverted_X->pkt_duration = mpAudDecFrame_X->pkt_duration;
      mpAudDecFrameConverted_X->pkt_size = mpAudDecFrame_X->pkt_size;
      mNbTotalAudDecFrame_U64++;
      mNbTotaAudDecSample_U64 += TotalSizeOfAudioConverted_U32;
      int16_t *pData_S16 = (int16_t *)mAudDecOut_X.InterleavedData_X.pData_U8;
      printf("Cnv Audio %x->%x:%p nbs %d ch %d layout %zx Fmt %d Rate %d Pos %zd Dur %zd Sz %d\n", mpAudDecFrameConverted_X->linesize[0], TotalSizeOfAudioConverted_U32, mAudDecOut_X.InterleavedData_X.pData_U8,
        mpAudDecFrameConverted_X->nb_samples, mpAudDecFrameConverted_X->channels, mpAudDecFrameConverted_X->channel_layout, mpAudDecFrameConverted_X->format, mpAudDecFrameConverted_X->sample_rate,
        mpAudDecFrameConverted_X->pkt_pos, mpAudDecFrameConverted_X->pkt_duration, mpAudDecFrameConverted_X->pkt_size);
      printf("Cnv Data %04x %04x %04x %04x %04x %04x %04x %04x\n", pData_S16[0], pData_S16[1], pData_S16[2], pData_S16[3], pData_S16[4], pData_S16[5], pData_S16[6], pData_S16[7]);
    }
    else
    {
      Rts_E = BOF_ERR_EOVERFLOW;
    }
  }

  return Rts_E;
}


bool Bof2dVideoDecoder::IsVideoStreamPresent()
{
  return(mVideoStreamIndex_i != -1);
}

END_BOF2D_NAMESPACE()