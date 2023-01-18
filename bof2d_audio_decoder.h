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
#include <bofstd/bofenum.h>
#include <bofstd/bofpath.h>
#include <bofstd/bofcommandlineparser.h>
#include <bofstd/boffs.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
}

BEGIN_BOF2D_NAMESPACE()
constexpr uint32_t MAX_AUDIO_SIZE = (2 * 16 * 4 * 48000);   //2 sec of 16 audio channel containing sample in 32 bit at 480000 (6MB)

enum class BOF2D_AUDIO_FORMAT :int32_t
{
  BOF2D_AUDIO_FORMAT_PCM = 0,
  BOF2D_AUDIO_FORMAT_WAV,
  BOF2D_AUDIO_FORMAT_MAX
};
extern BOF::BofEnum<BOF2D_AUDIO_FORMAT> S_Bof2dAudioFormatEnumConverter;
//"A_BASEFN=AudioOut;A_NBCHNL=2;A_RATE=48000;A_FMT=WAV"
struct BOF2D_EXPORT BOF2D_AUDIO_OPTION
{
  BOF::BofPath BasePath;
  bool SaveChunk_B;
  uint32_t NbChannel_U32;
  uint64_t ChannelLayout_U64;
  uint32_t SampleRateInHz_U32;
  uint32_t NbBitPerSample_U32;
  BOF2D_AUDIO_FORMAT Format_E;

  BOF2D_AUDIO_OPTION()
  {
    Reset();
  }

  void Reset()
  {
    BasePath = "";
    SaveChunk_B = false;
    NbChannel_U32 = 0;
    ChannelLayout_U64 = 0;
    SampleRateInHz_U32 = 0;
    NbBitPerSample_U32 = 0;
    Format_E = BOF2D_AUDIO_FORMAT::BOF2D_AUDIO_FORMAT_MAX;
  }
};

struct BOF2D_EXPORT BOF2D_AUDIO_DATA
{
  BOF::BOF_BUFFER Data_X;
  uint32_t NbSample_U32;

  BOF2D_AUDIO_OPTION Param_X;

  BOF2D_AUDIO_DATA()
  {
    Reset();
  }

  void Reset()
  {
    Data_X.Reset();
    NbSample_U32 = 0;
    Param_X.Reset();
  }
};
class BOF2D_EXPORT Bof2dAudioDecoder
{
public:
  Bof2dAudioDecoder();
  virtual ~Bof2dAudioDecoder();

  BOFERR Open(const std::string &_rInputFile_S, const std::string &_rOption_S);
  BOFERR BeginRead(BOF2D_AUDIO_DATA &_rAudioData_X);
  BOFERR EndRead();
  BOFERR Close();
  bool IsAudioStreamPresent();

private:
  BOFERR WriteHeader(uint64_t _Size_U64);
  BOFERR CreateHeader();
  BOFERR WriteChunk(uint32_t _ChunkSizeInByte_U32);
  BOFERR CloseHeader();
  BOFERR ConvertAudio(uint32_t &_rTotalSizeOfAudioConverted_U32);

  std::atomic<bool> mReadBusy_B = false;
  std::vector<BOF::BOFPARAMETER> mAudioOptionParam_X;
  BOF2D_AUDIO_OPTION mAudioOption_X;
  intptr_t mOutAudioFile = BOF::BOF_FS_INVALID_HANDLE;

  AVPacket mPacket_X;
  AVFormatContext *mpAudioFormatCtx_X = nullptr;
  int mAudioStreamIndex_i = -1;
  const AVCodecParameters *mpAudioCodecParam_X = nullptr;
  const AVCodec *mpAudioCodec_X = nullptr;
  AVCodecContext *mpAudioCodecCtx_X = nullptr;
  AVFrame *mpAudioFrame_X = nullptr;
  AVFrame *mpAudioFrameConverted_X = nullptr;
  uint8_t *mpAudioBuffer_U8 = nullptr;
  enum AVSampleFormat mSampleFmt_E = AV_SAMPLE_FMT_NONE;
  SwrContext *mpAudioSwrCtx_X = nullptr;
  //uint8_t *mpAudioBuffer_U8 = nullptr;
  //BOF::BOF_BUFFER mAudioBuffer_X;

  AVRational mAudioTimeBase_X = { 0, 0 };

  uint64_t mNbAudioPacketSent_U64 = 0;
  uint64_t mNbAudioFrameReceived_U64 = 0;
  uint64_t mNbTotalAudioFrame_U64 = 0;
  uint64_t mNbTotaAudioSample_U64 = 0;

  const int  mAudioAllocAlignment_i = 32;
};

END_BOF2D_NAMESPACE()
