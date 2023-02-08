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
#pragma once
#pragma message("!!!!!!!!!!!!! CPP => TODO REMOVE FILE LATER !!!!!!!!!!!!!")
#include "bof2d.h"
#include <C:\Program Files (x86)\Visual Leak Detector\include\vld.h>
#include <bofstd/bofcommandlineparser.h>
#include <bofstd/boffs.h>

#include "bof2d_av_codec.h"
#include "bof2d_audio_encoder.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

BEGIN_BOF2D_NAMESPACE()

struct BOF2D_EXPORT BOF2D_AUD_DEC_OPTION
{
  bool DemuxChannel_B;
  uint32_t NbChannel_U32;
  uint64_t ChannelLayout_U64;
  uint32_t SampleRateInHz_U32;
  uint32_t NbBitPerSample_U32;

  BOF2D_AUD_DEC_OPTION()
  {
    Reset();
  }

  void Reset()
  {
    DemuxChannel_B = false;
    NbChannel_U32 = 0;
    ChannelLayout_U64 = 0;
    SampleRateInHz_U32 = 0;
    NbBitPerSample_U32 = 0;
  }
};
class BOF2D_EXPORT Bof2dAudioDecoder
{
public:
  Bof2dAudioDecoder();
  virtual ~Bof2dAudioDecoder();

  BOFERR Open(AVFormatContext *_pDecFormatCtx_X, const std::string &_rAudDecOption_S, int &_rAudDecStreamIndex_i);
  BOFERR Close();
  BOFERR BeginRead(AVPacket *_pDecPacket_X, BOF2D_AUD_DEC_OUT &_rAudDecOut_X);
  BOFERR EndRead();

  bool IsAudioStreamPresent();
  
private:
  BOFERR ConvertAudio(uint32_t &_rNbAudioSampleConvertedPerChannel_U32);

  std::atomic<BOF2D_AV_CODEC_STATE> mAudDecState_E = BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_IDLE;
  std::vector<BOF::BOFPARAMETER> mAudDecOptionParam_X;
  BOF2D_AUD_DEC_OPTION mAudDecOption_X;
  int mAudDecStreamIndex_i = -1;

  const AVCodecParameters *mpAudDecCodecParam_X = nullptr;
  const AVCodec *mpAudDecCodec_X = nullptr;
  AVCodecContext *mpAudDecCodecCtx_X = nullptr;
  AVFrame *mpAudDecFrame_X = nullptr;
  AVFrame *mpAudDecFrameConverted_X = nullptr;
  BOF2D_AUD_DEC_OUT mAudDecOut_X;
  enum AVSampleFormat mSampleFmt_E = AV_SAMPLE_FMT_NONE;
  SwrContext *mpAudDecSwrCtx_X = nullptr;

  AVRational mAudDecTimeBase_X = { 0, 0 };

  uint64_t mNbAudDecPacketSent_U64 = 0;
  uint64_t mNbAudDecFrameReceived_U64 = 0;
  uint64_t mNbTotalAudDecFrame_U64 = 0;
  uint64_t mNbTotaAudDecSample_U64 = 0;

  const int  mAudDecAllocAlignment_i = 32;
};

END_BOF2D_NAMESPACE()
