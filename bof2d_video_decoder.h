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
#pragma once
#pragma message("!!!!!!!!!!!!! CPP => TODO REMOVE FILE LATER !!!!!!!!!!!!!")
#include "bof2d.h"
#include <C:\Program Files (x86)\Visual Leak Detector\include\vld.h>

#include <bofstd/bofsystem.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include "libavutil/imgutils.h"
}

BEGIN_BOF2D_NAMESPACE()
struct BOF2D_EXPORT BOF2D_VIDEO_DATA
{
  BOF::BOF_BUFFER Data_X;
  BOF2D::BOF_SIZE Size_X;

  BOF2D_VIDEO_DATA()
  {
    Reset();
  }

  void Reset()
  {
    Data_X.Reset();
    Size_X.Reset();
  }
};
class BOF2D_EXPORT Bof2dVideoDecoder
{
public:
  Bof2dVideoDecoder();
  virtual ~Bof2dVideoDecoder();

  BOFERR Open(const std::string &_rInputFile_S);
  BOFERR BeginRead(BOF2D_VIDEO_DATA &_rVideoData_X);
  BOFERR EndRead();
  BOFERR Close();
  bool IsVideoStreamPresent();

private:
  std::atomic<bool> mReadBusy_B = false;
  std::string mOutputCodec_S;
  AVFormatContext *mpVideoFormatCtx_X = nullptr;
  int mVideoStreamIndex_i = -1;
  const AVCodecParameters *mpVideoCodecParam_X = nullptr;
  const AVCodec *mpVideoCodec_X = nullptr;
  AVCodecContext *mpVideoCodecCtx_X = nullptr;
  AVFrame *mpVideoFrame_X = nullptr;
  AVFrame *mpVideoDestFrame_X = nullptr;
  AVFrame *mpVideoFrameFiltered_X = nullptr;

  AVColorPrimaries mColorPrimaries_E = AVColorPrimaries::AVCOL_PRI_RESERVED;
  AVColorRange mColorRange_E  = AVColorRange::AVCOL_RANGE_UNSPECIFIED;
  AVColorTransferCharacteristic mColorTrf_E = AVColorTransferCharacteristic::AVCOL_TRC_RESERVED;
  AVColorSpace mColorSpace_E = AVColorSpace::AVCOL_SPC_RESERVED;

  AVPixelFormat mInputVideoPixFmt_E = AVPixelFormat::AV_PIX_FMT_NONE;
  AVPixelFormat mOutputVideoPixFmt_E = AVPixelFormat::AV_PIX_FMT_NONE;
  AVPixelFormat mRgbVideoPixFmt_E = AVPixelFormat::AV_PIX_FMT_NONE;

  bool mIsVideoInterlaced_B = false;
  AVRational mVideoFrameRate_X = { 0, 0 };  //or av_make_q
  AVRational mVideoTimeBase_X = { 0, 0 };
  double mVideoFramePerSecond_lf = 0.0;

  uint64_t mNbVideoPacketSent_U64 = 0;
  uint64_t mNbVideoFrameReceived_U64 = 0;
  uint64_t mNbTotalVideoFrame_U64 = 0;

  const int  mVideoAllocAlignment_i = 32;

  SwsContext *mpVideoSwsImageCtx_X = nullptr;
  SwsContext *mpVideoSwsCtx_X = nullptr;

  AVFilterInOut *mpVideoFilterIn_X = nullptr;
  AVFilterInOut *mpVideoFilterOut_X = nullptr;
  AVFilterGraph *mpVideoFltGraph_X = nullptr;
  AVFilterContext *mpVideoFilterSinkCtx_X = nullptr;
  AVFilterContext *mpVideoFilterSrcCtx_X = nullptr;
};

END_BOF2D_NAMESPACE()
