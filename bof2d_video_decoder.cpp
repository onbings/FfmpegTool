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
#include "bof2d_av_codec.h"

BEGIN_BOF2D_NAMESPACE()

Bof2dVideoDecoder::Bof2dVideoDecoder()
{

}
Bof2dVideoDecoder::~Bof2dVideoDecoder()
{
  Close();
}

BOFERR Bof2dVideoDecoder::Open(const std::string &_rInputFile_S)
{
  BOFERR    Rts_E;
  int       Sts_i;
  uint32_t  i_U32;
  int       SrcRange_i, DstRange_i, Brightness_i, Contrast_i, Saturation_i;
  const int *pSwsCoef_i;
  int *pInvCoef_i, *pCoef_i;
  char pFilterParam_c[0x1000];
  const AVFilter *pBufferSrcFlt_X, *pBufferSinkFlt_X;

  // Just in case, close previously opened media file.
  Close();
  // Open media file.
  Sts_i = avformat_open_input(&mpVideoFormatCtx_X, _rInputFile_S.c_str(), nullptr, nullptr);
  FFMPEG_CHK_IF_ERR(Sts_i, "Couldn't open file " + _rInputFile_S, Rts_E);
  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    /* Retrieve stream information */
    Sts_i = avformat_find_stream_info(mpVideoFormatCtx_X, nullptr);
    FFMPEG_CHK_IF_ERR(Sts_i, "Couldn't find stream information in " + _rInputFile_S, Rts_E);
    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      for (i_U32 = 0; i_U32 < mpVideoFormatCtx_X->nb_streams; i_U32++)
      {
        if (mpVideoFormatCtx_X->streams[i_U32]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
          mpVideoCodecParam_X = mpVideoFormatCtx_X->streams[i_U32]->codecpar;
          break;
        }
      }
      if (mpVideoCodecParam_X == nullptr)
      {
        FFMPEG_CHK_IF_ERR(AVERROR_STREAM_NOT_FOUND, "No video stream in " + _rInputFile_S, Rts_E);
      }
      else
      {
        mVideoStreamIndex_i = i_U32;
        //av_dict_set(&opts, "b", "2.5M", 0);
        mpVideoCodec_X = avcodec_find_decoder(mpVideoCodecParam_X->codec_id);
        if (mpVideoCodec_X == nullptr)
        {
          FFMPEG_CHK_IF_ERR(AVERROR_STREAM_NOT_FOUND, "Could not find decoder for codec " + std::to_string(mpVideoCodecParam_X->codec_id), Rts_E);
        }
        else
        {
          mpVideoCodecCtx_X = avcodec_alloc_context3(mpVideoCodec_X);
          if (mpVideoCodecCtx_X == nullptr)
          {
            FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not avcodec_alloc_context3 for codec " + std::to_string(mpVideoCodecParam_X->codec_id), Rts_E);
          }
          else
          {
            Sts_i = avcodec_parameters_to_context(mpVideoCodecCtx_X, mpVideoCodecParam_X);
            FFMPEG_CHK_IF_ERR(Sts_i, "Error in avcodec_parameters_to_context", Rts_E);
            if (Rts_E == BOF_ERR_NO_ERROR)
            {
              mpVideoCodecCtx_X->thread_count = 4;
              Sts_i = avcodec_open2(mpVideoCodecCtx_X, mpVideoCodec_X, nullptr);
              FFMPEG_CHK_IF_ERR(Sts_i, "Could not avcodec_open2", Rts_E);
              if (Rts_E == BOF_ERR_NO_ERROR)
              {
                mpVideoFrame_X = av_frame_alloc();
                if (mpVideoFrame_X == nullptr)
                {
                  FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not av_frame_alloc mpFrame_X", Rts_E);
                }
                else
                {
                  mpVideoDestFrame_X = av_frame_alloc();
                  if (mpVideoDestFrame_X == nullptr)
                  {
                    FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not av_frame_alloc mpDestFrame_X", Rts_E);
                  }
                  else
                  {
                    mpVideoFrameFiltered_X = av_frame_alloc();
                    if (mpVideoFrameFiltered_X == nullptr)
                    {
                      FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not av_frame_alloc mpFrameFiltered_X", Rts_E);
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
    mVideoFrameRate_X = mpVideoFormatCtx_X->streams[mVideoStreamIndex_i]->avg_frame_rate;
    mVideoTimeBase_X = mpVideoFormatCtx_X->streams[mVideoStreamIndex_i]->time_base;
    mVideoFramePerSecond_lf = av_q2d(mpVideoFormatCtx_X->streams[mVideoStreamIndex_i]->r_frame_rate);

    mColorPrimaries_E = mpVideoCodecCtx_X->color_primaries;
    mColorRange_E = mpVideoCodecCtx_X->color_range;
    mColorSpace_E = mpVideoCodecCtx_X->colorspace;
    mColorTrf_E = mpVideoCodecCtx_X->color_trc;
    mInputVideoPixFmt_E = mpVideoCodecCtx_X->pix_fmt;

    if (mOutputCodec_S == "h264")
    {
      mOutputVideoPixFmt_E = mInputVideoPixFmt_E;
    }
    mNbTotalVideoFrame_U64 = static_cast<int64_t>(mpVideoFormatCtx_X->duration / 1.0e6 * mVideoFrameRate_X.num / mVideoFrameRate_X.den);

    // SWScale
    if (mpVideoCodecCtx_X->color_primaries == AVColorPrimaries::AVCOL_PRI_BT2020)
    {
      mRgbVideoPixFmt_E = AVPixelFormat::AV_PIX_FMT_RGB48LE;
    }
    else
    {
      mRgbVideoPixFmt_E = AVPixelFormat::AV_PIX_FMT_RGB24;
    }
    Sts_i = av_image_alloc(mpVideoDestFrame_X->data, mpVideoDestFrame_X->linesize, mpVideoCodecParam_X->width, mpVideoCodecParam_X->height, mRgbVideoPixFmt_E, mVideoAllocAlignment_i);
    FFMPEG_CHK_IF_ERR(Sts_i, "Error in av_image_alloc", Rts_E);
    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      printf("Convert from %s %s to %s %dx%d\n", av_get_pix_fmt_name(mpVideoCodecCtx_X->pix_fmt), av_color_space_name(mpVideoCodecCtx_X->colorspace),
        av_get_pix_fmt_name(mRgbVideoPixFmt_E), mpVideoCodecParam_X->width, mpVideoCodecParam_X->height);

      //TODO a remplacer par sws_getContext !!!
      // sws_getCachedContext(nullptr:  If context is NULL, just calls sws_getContext()
      mpVideoSwsImageCtx_X = sws_getCachedContext(nullptr, mpVideoCodecCtx_X->width, mpVideoCodecCtx_X->height, mpVideoCodecCtx_X->pix_fmt, mpVideoCodecParam_X->width, mpVideoCodecParam_X->height, mRgbVideoPixFmt_E, SWS_BICUBIC, nullptr, nullptr, nullptr);

      if (mpVideoSwsImageCtx_X == nullptr)
      {
        FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not sws_getCachedContext", Rts_E);
      }
      else
      {
        pSwsCoef_i = sws_getCoefficients(mpVideoCodecCtx_X->colorspace);
        Sts_i = sws_getColorspaceDetails(mpVideoSwsImageCtx_X, &pInvCoef_i, &SrcRange_i, &pCoef_i, &DstRange_i, &Brightness_i, &Contrast_i, &Saturation_i);
        FFMPEG_CHK_IF_ERR(Sts_i, "Swscale conversion not supported", Rts_E);
        if (Rts_E == BOF_ERR_NO_ERROR)
        {
          // color range: (1=jpeg / 0=mpeg)
          //  SrcRange_i = 0;
          //  DstRange_i = 1;

          Sts_i = sws_setColorspaceDetails(mpVideoSwsImageCtx_X, pSwsCoef_i, SrcRange_i, pSwsCoef_i, DstRange_i, Brightness_i, Contrast_i, Saturation_i);
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
        mpVideoFilterOut_X = avfilter_inout_alloc();
        if (mpVideoFilterOut_X == nullptr)
        {
          FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not avfilter_inout_alloc out", Rts_E);
        }
        else
        {
          mpVideoFilterIn_X = avfilter_inout_alloc();

          mpVideoFltGraph_X = avfilter_graph_alloc();
          if (mpVideoFilterOut_X == nullptr)
          {
            FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not avfilter_inout_alloc out", Rts_E);
          }
          else
          {
            /* buffer video source: the decoded frames from the decoder will be inserted here. */
            snprintf(pFilterParam_c, sizeof(pFilterParam_c),
              "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
              mpVideoCodecCtx_X->width, mpVideoCodecCtx_X->height,
              mpVideoCodecCtx_X->pix_fmt,
              mVideoTimeBase_X.num, mVideoTimeBase_X.den,
              mpVideoCodecCtx_X->sample_aspect_ratio.num,
              mpVideoCodecCtx_X->sample_aspect_ratio.den);


            Sts_i = avfilter_graph_create_filter(&mpVideoFilterSrcCtx_X, pBufferSrcFlt_X, "in", pFilterParam_c, nullptr, mpVideoFltGraph_X);
            FFMPEG_CHK_IF_ERR(Sts_i, "Could not avfilter_graph_create_filter In", Rts_E);
            if (Rts_E == BOF_ERR_NO_ERROR)
            {
              /* buffer video sink: to terminate the filter chain. */
              Sts_i = avfilter_graph_create_filter(&mpVideoFilterSinkCtx_X, pBufferSinkFlt_X, "out", nullptr, nullptr, mpVideoFltGraph_X);
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
                mpVideoFilterOut_X->name = av_strdup("in");
                mpVideoFilterOut_X->filter_ctx = mpVideoFilterSrcCtx_X;
                mpVideoFilterOut_X->pad_idx = 0;
                mpVideoFilterOut_X->next = nullptr;

                /*
                 * The buffer sink input must be connected to the output pad of
                 * the last filter described by filters_descr; since the last
                 * filter output label is not specified, it is set to "out" by
                 * default.
                 */
                mpVideoFilterIn_X->name = av_strdup("out");
                mpVideoFilterIn_X->filter_ctx = mpVideoFilterSinkCtx_X;
                mpVideoFilterIn_X->pad_idx = 0;
                mpVideoFilterIn_X->next = nullptr;

                Sts_i = avfilter_graph_parse_ptr(mpVideoFltGraph_X, "yadif=mode=1", &mpVideoFilterIn_X, &mpVideoFilterOut_X, nullptr);
                FFMPEG_CHK_IF_ERR(Sts_i, "Could not avfilter_graph_parse_ptr", Rts_E);
                if (Rts_E == BOF_ERR_NO_ERROR)
                {
                  Sts_i = avfilter_graph_config(mpVideoFltGraph_X, nullptr);
                  FFMPEG_CHK_IF_ERR(Sts_i, "Could not avfilter_graph_config", Rts_E);
                  if (Rts_E == BOF_ERR_NO_ERROR)
                  {
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

  return Rts_E;
}
BOFERR Bof2dVideoDecoder::BeginRead(BOF2D_VIDEO_DATA &_rVideoData_X)
{
  BOFERR Rts_E = BOF_ERR_EOF;

  _rVideoData_X.Reset();
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

BOFERR Bof2dVideoDecoder::Close()
{
  BOFERR Rts_E;

  avfilter_graph_free(&mpVideoFltGraph_X);
  avfilter_inout_free(&mpVideoFilterIn_X);
  avfilter_inout_free(&mpVideoFilterOut_X);

  sws_freeContext(mpVideoSwsImageCtx_X);
  mpVideoSwsImageCtx_X = nullptr;
  if (mpVideoFrame_X)
  {
    av_freep(&mpVideoFrame_X->data[0]);
    av_frame_free(&mpVideoFrame_X);
  }
  if (mpVideoDestFrame_X)
  {
    av_freep(&mpVideoDestFrame_X->data[0]);
    av_frame_free(&mpVideoDestFrame_X);
  }
  if (mpVideoFrameFiltered_X)
  {
    av_freep(&mpVideoFrameFiltered_X->data[0]);
    av_frame_free(&mpVideoFrameFiltered_X);
  }
  //avcodec_close: Do not use this function. Use avcodec_free_context() to destroy a codec context(either open or closed)
  avcodec_free_context(&mpVideoCodecCtx_X);
  avformat_close_input(&mpVideoFormatCtx_X);

  mVideoStreamIndex_i = -1;
  mpVideoCodecParam_X = nullptr;
  mpVideoCodec_X = nullptr;

  mColorPrimaries_E = AVColorPrimaries::AVCOL_PRI_RESERVED;
  mColorRange_E = AVColorRange::AVCOL_RANGE_UNSPECIFIED;
  mColorTrf_E = AVColorTransferCharacteristic::AVCOL_TRC_RESERVED;
  mColorSpace_E = AVColorSpace::AVCOL_SPC_RESERVED;

  mInputVideoPixFmt_E = AVPixelFormat::AV_PIX_FMT_NONE;
  mOutputVideoPixFmt_E = AVPixelFormat::AV_PIX_FMT_NONE;
  mRgbVideoPixFmt_E = AVPixelFormat::AV_PIX_FMT_NONE;

  mIsVideoInterlaced_B = false;
  mVideoFrameRate_X = { 0, 0 };  //or av_make_q
  mVideoTimeBase_X = { 0, 0 };
  mVideoFramePerSecond_lf = 0.0;

  mNbVideoPacketSent_U64 = 0;
  mNbVideoFrameReceived_U64 = 0;
  mNbTotalVideoFrame_U64 = 0;

  Rts_E = BOF_ERR_NO_ERROR;
  return Rts_E;
}

bool Bof2dVideoDecoder::IsVideoStreamPresent()
{
  return(mVideoStreamIndex_i != -1);
}

END_BOF2D_NAMESPACE()