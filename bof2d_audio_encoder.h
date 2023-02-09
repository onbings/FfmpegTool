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
  bool SaveChunk_B; 
  uint32_t NbChannel_U32; //If 0 set to 2
  BOF2D_AV_AUDIO_FORMAT Format_E; //If BOF2D_AV_AUDIO_FORMAT_MAX set to BOF2D_AV_AUDIO_FORMAT_PCM

  BOF2D_AUD_ENC_OPTION()
  {
    Reset();
  }

  void Reset()
  {
    BasePath = "";
    SaveChunk_B = false;
    NbChannel_U32 = 0;
    Format_E = BOF2D_AV_AUDIO_FORMAT::BOF2D_AV_AUDIO_FORMAT_MAX;
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

class BOF2D_EXPORT Bof2dAudioEncoder
{
public:
  Bof2dAudioEncoder();
  virtual ~Bof2dAudioEncoder();

  BOFERR Open(const std::string &_rOption_S);
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
};

END_BOF2D_NAMESPACE()
