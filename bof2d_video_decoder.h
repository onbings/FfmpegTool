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

#include "bof2d_av_codec.h"
#include <bofstd/bofcommandlineparser.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

BEGIN_BOF2D_NAMESPACE()

struct BOF2D_EXPORT BOF2D_VID_DEC_OPTION
{
  uint32_t Width_U32; //If 0 get value from source
  uint32_t Height_U32;  //If 0 get value from source
  uint32_t NbBitPerPixel_U32; //If 0 set to AV_PIX_FMT_BGRA
  uint32_t NbThread_U32;  //If 0 set to 1

  BOF2D_VID_DEC_OPTION()
  {
    Reset();
  }

  void Reset()
  {
    Width_U32 = 0;
    Height_U32 = 0;
    NbBitPerPixel_U32 = 0;
    NbThread_U32 = 0;
  }
};
class BOF2D_EXPORT Bof2dVideoDecoder
{
public:
  Bof2dVideoDecoder();
  virtual ~Bof2dVideoDecoder();

  BOFERR Open(AVFormatContext *_pDecFormatCtx_X, const std::string &_rVidDecOption_S, int &_rAudDecStreamIndex_i);
  BOFERR Close();
  BOFERR BeginRead(AVPacket *_pDecPacket_X, BOF2D_VID_DEC_OUT &_rVidDecOut_X);
  BOFERR EndRead();

  bool IsVideoStreamPresent();
  AVRational GetVideoFrameRate();

private:
  BOFERR ConvertVideo(uint32_t &_rTotalSizeOfVideoConverted_U32);

  std::atomic<BOF2D_AV_CODEC_STATE> mVidDecState_E = BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_IDLE;
  std::vector<BOF::BOFPARAMETER> mVidDecOptionParam_X;
  BOF2D_VID_DEC_OPTION mVidDecOption_X;
  int mVidDecStreamIndex_i = -1;

  BOF2D_VID_DEC_OUT mVidDecOut_X;
  AVPixelFormat mPixelFmt_E = AV_PIX_FMT_NONE;
  uint32_t  mImgSize_U32 = 0;
  //std::string mOutputCodec_S;
  const AVCodecParameters *mpVidDecCodecParam_X = nullptr;
  const AVCodec *mpVidDecCodec_X = nullptr;
  AVCodecContext *mpVidDecCodecCtx_X = nullptr;
  AVFrame *mpVidDecFrame_X = nullptr;
  AVFrame *mpVidDecFrameConverted_X = nullptr;
  AVFrame *mpVidDecFrameFiltered_X = nullptr;

  AVColorPrimaries mVidDecColorPrimaries_E = AVColorPrimaries::AVCOL_PRI_RESERVED;
  AVColorRange mVidDecColorRange_E  = AVColorRange::AVCOL_RANGE_UNSPECIFIED;
  AVColorTransferCharacteristic mVidDecColorTrf_E = AVColorTransferCharacteristic::AVCOL_TRC_RESERVED;
  AVColorSpace mVidDecColorSpace_E = AVColorSpace::AVCOL_SPC_RESERVED;

  AVPixelFormat mVidDecInPixFmt_E = AVPixelFormat::AV_PIX_FMT_NONE;
  AVPixelFormat mVidDecVideoOutPixFmt_E = AVPixelFormat::AV_PIX_FMT_NONE;
  // replaced by mPixelFmt_E AVPixelFormat mVidDecRgbPixFmt_E = AVPixelFormat::AV_PIX_FMT_NONE;

  bool mIsVideoInterlaced_B = false;
//  AVRational mVidDecFrameRate_X = { 0, 0 };  //or av_make_q
  AVRational mVidDecTimeBase_X = { 0, 0 };
//  double mVidDecFramePerSecond_lf = 0.0;

  uint64_t mNbVidDecPacketSent_U64 = 0;
  uint64_t mNbVidDecFrameReceived_U64 = 0;
  uint64_t mNbTotalVidDecFrame_U64 = 0;

  SwsContext *mpVidDecSwsCtx_X = nullptr;

  AVFilterInOut *mpVidDecFilterIn_X = nullptr;
  AVFilterInOut *mpVidDecFilterOut_X = nullptr;
  AVFilterGraph *mpVidDecFltGraph_X = nullptr;
  AVFilterContext *mpVidDecFilterSinkCtx_X = nullptr;
  AVFilterContext *mpVidDecFilterSrcCtx_X = nullptr;

  const int  mVidDecAllocAlignment_i = 32;
  AVStream   *mpVidStream_X = nullptr;
  double     mVidDurationInSec_lf = 0;
  std::map<std::string, std::string> mVidMetadataCollection;
};

END_BOF2D_NAMESPACE()
