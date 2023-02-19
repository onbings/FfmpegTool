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
#include "bof2d_av_codec.h"
#include <C:\Program Files (x86)\Visual Leak Detector\include\vld.h>

#include <bofstd/bofcommandlineparser.h>

BEGIN_BOF2D_NAMESPACE()

struct BOF2D_EXPORT BOF2D_AUD_ENC_OPTION
{
  BOF::BofPath BasePath;
  uint32_t NbChannel_U32; //If 0 set to 2
  BOF2D_AV_AUDIO_FORMAT Format_E; //If BOF2D_AV_AUDIO_FORMAT_MAX set to BOF2D_AV_AUDIO_FORMAT_PCM
// AAC audio is stored in frames which decode to 1024 samples. So, for a 48000 Hz feed, each frame has a duration of 0.02133 seconds.
// but with this param we extract and slice audio data in packet of 960 sample for example (48000 / 50 give 960 sample in PAL 800/801 in ntsc)
  std::vector< uint32_t>  SaveChunkSizeInSampleCollection;  //If empty do not save chunk, if entry 0 is 0, save each chunk with its native size, if entry 0 is different from zero save each chunk with this size and if you have a list use these value in a circular way to save audio data chunk (seq 801 801 801 801 800 for example)

  BOF2D_AUD_ENC_OPTION()
  {
    Reset();
  }

  void Reset()
  {
    BasePath = "";
    NbChannel_U32 = 0;
    Format_E = BOF2D_AV_AUDIO_FORMAT::BOF2D_AV_AUDIO_FORMAT_MAX;
    SaveChunkSizeInSampleCollection.clear();
  }
};

struct BOF2D_EXPORT BOF2D_AUD_ENC_OUT
{
  intptr_t Io;
  uint64_t Size_U64;

  BOF2D_AUD_ENC_OUT()
  {
    Reset();
  }
  void Reset()
  {
    Io = BOF::BOF_FS_INVALID_HANDLE;
    Size_U64 = 0;
  }
};

struct BOF2D_EXPORT BOF2D_AUD_ENC_CHUNK_STATE
{
  uint32_t SaveChunkSizeCollectionIndex_U32;
  std::string LastChunkPath_S;
  uint32_t SubChunkId_U32;
  uint32_t RemainingChunkSize_U32;

  BOF2D_AUD_ENC_CHUNK_STATE()
  {
    Reset();
  }
  void Reset()
  {
    SaveChunkSizeCollectionIndex_U32 = 0;
    LastChunkPath_S = "";
    SubChunkId_U32 = 0;
    RemainingChunkSize_U32 = 0;
  }
};

class BOF2D_EXPORT Bof2dAudioEncoder
{
public:
  Bof2dAudioEncoder();
  virtual ~Bof2dAudioEncoder();

  BOFERR Open(const std::string &_rOption_S, AVRational &_rVideoFrameRate_X);
  BOFERR Close();
  BOFERR BeginWrite(BOF2D_AUD_DEC_OUT &_rAudDecOut_X);
  BOFERR EndWrite();

  bool IsAudioStreamPresent();

private:
  BOFERR CreateFileOut();
  BOFERR WriteHeader();
  BOFERR WriteChunkOut();
  BOFERR CloseFileOut();

  std::atomic<BOF2D_AV_CODEC_STATE> mAudEncState_E = BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_IDLE;
  std::vector<BOF::BOFPARAMETER> mAudEncOptionParam_X;
  BOF2D_AUD_ENC_OPTION mAudEncOption_X;
  std::vector<BOF2D_AUD_ENC_OUT> mIoCollection;
  BOF2D_AUD_DEC_OUT mAudDecOut_X;

  uint64_t mNbTotalAudEncFrame_U64 = 0;

  const int  mAudEncAllocAlignment_i = 32;

  AVRational mVideoFrameRate_X = { 0, 0 };

  std::vector<BOF2D_AUD_ENC_CHUNK_STATE> mAudEncChunkStateCollection;
};

END_BOF2D_NAMESPACE()
