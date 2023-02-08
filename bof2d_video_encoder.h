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
#include "bof2d_av_codec.h"
#include <C:\Program Files (x86)\Visual Leak Detector\include\vld.h>

#include <bofstd/bofcommandlineparser.h>

extern "C"
{
#include <libavformat/avformat.h>
}

BEGIN_BOF2D_NAMESPACE()

struct BOF2D_EXPORT BOF2D_VID_ENC_OPTION
{
  BOF::BofPath BasePath;
  BOF2D_AV_VIDEO_FORMAT Format_E;

  BOF2D_VID_ENC_OPTION()
  {
    Reset();
  }

  void Reset()
  {
    BasePath = "";
    Format_E = BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_MAX;
  }
};

struct BOF2D_EXPORT BOF2D_VID_ENC_OUT
{
  intptr_t Io;
  uint64_t Size_U64;

  BOF2D_VID_ENC_OUT()
  {
    Reset();
  }
  void Reset()
  {
    Io = BOF::BOF_FS_INVALID_HANDLE;
    Size_U64 = 0;
  }
};

class BOF2D_EXPORT Bof2dVideoEncoder
{
public:
  Bof2dVideoEncoder();
  virtual ~Bof2dVideoEncoder();

  BOFERR Open(const std::string &_rOption_S);
  BOFERR Close();
  BOFERR BeginWrite(BOF2D_VID_DEC_OUT &_rVidDecOut_X);
  BOFERR EndWrite();

  bool IsVideoStreamPresent();

private:
  BOFERR CreateFileOut();
  BOFERR WriteHeader();
  BOFERR WriteChunkOut();
  BOFERR CloseFileOut();

  std::atomic<BOF2D_AV_CODEC_STATE> mVidEncState_E = BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_IDLE;
  std::vector<BOF::BOFPARAMETER> mVidEncOptionParam_X;
  BOF2D_VID_ENC_OPTION mVidEncOption_X;
  std::vector<BOF2D_VID_ENC_OUT> mIoCollection;
  BOF2D_VID_DEC_OUT mVidDecOut_X;

  uint64_t mNbTotalVidEncFrame_U64 = 0;

  const int  mVidEncAllocAlignment_i = 32;
};

END_BOF2D_NAMESPACE()
