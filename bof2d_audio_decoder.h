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
#include <bofstd/bofsystem.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/opt.h>
}

BEGIN_BOF2D_NAMESPACE()
constexpr uint32_t MAX_AUDIO_SIZE = (2 * 16 * 4 * 48000);   //2 sec of 16 audio channel containing sample in 32 bit at 480000 (6MB)

struct BOF2D_EXPORT BOF2D_AUDIO_DATA
{
  BOF::BOF_BUFFER Data_X;
  uint32_t NbSample_U32;
  uint32_t NbChannel_U32;
  uint64_t ChannelLayout_U64;
  uint32_t SampleRateInHz_U32;
  // _pOutAudioFrame_X->format  _pOutAudioFrame_X->pkt_pos, _pOutAudioFrame_X->pkt_duration, _pOutAudioFrame_X->pkt_size);

  BOF2D_AUDIO_DATA()
  {
    Reset();
  }

  void Reset()
  {
    Data_X.Reset();
    NbSample_U32 = 0;
    NbChannel_U32 = 0;
    ChannelLayout_U64 = 0;
    SampleRateInHz_U32 = 0;
  }
};
class BOF2D_EXPORT Bof2dAudioDecoder
{
public:
  Bof2dAudioDecoder();
  virtual ~Bof2dAudioDecoder();

  BOFERR Open(const std::string &_rInputFile_S);
  BOFERR BeginRead(BOF2D_AUDIO_DATA &_rAudioData_X);
  BOFERR EndRead();
  BOFERR Close();
  bool IsAudioStreamPresent();

private:
  BOFERR ConvertAudio(bool _Flush_B, AVFrame *_pInAudioFrame_X, AVFrame *_pOutAudioFrame_X, uint32_t _OutNbAudioChannel_U32, uint32_t _OutAudioChannelLayout_U32, uint32_t _OutAudioSampleRateInHz_U32, enum AVSampleFormat _OutAudioSampleFmt_E);

  std::atomic<bool> mReadBusy_B = false;
  AVPacket mPacket_X;
  AVFormatContext *mpAudioFormatCtx_X = nullptr;
  int mAudioStreamIndex_i = -1;
  const AVCodecParameters *mpAudioCodecParam_X = nullptr;
  const AVCodec *mpAudioCodec_X = nullptr;
  AVCodecContext *mpAudioCodecCtx_X = nullptr;
  AVFrame *mpAudioFrame_X = nullptr;
  AVFrame *mpAudioFrameConverted_X = nullptr;
  uint8_t *mpAudioBuffer_U8 = nullptr;
  //AVFormatContext *fmt_ctx;
  //AVIOContext *mpAudioAvioCtx_X = nullptr;
  SwrContext *mpAudioSwrCtx_X = nullptr;
  //uint8_t *mpAudioBuffer_U8 = nullptr;
  //BOF::BOF_BUFFER mAudioBuffer_X;

  AVRational mAudioTimeBase_X = { 0, 0 };

  uint64_t mNbAudioPacketSent_U64 = 0;
  uint64_t mNbAudioFrameReceived_U64 = 0;
  uint64_t mNbTotalAudioFrame_U64 = 0;

  const int  mAudioAllocAlignment_i = 32;
};

END_BOF2D_NAMESPACE()
