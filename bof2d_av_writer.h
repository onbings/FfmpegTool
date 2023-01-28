/*
   Copyright (c) 2000-2026, OnBings. All rights reserved.

   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
   KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
   PURPOSE.

   This module defines the interface to the bof2d writer class

   Author:      Bernard HARMEL: onbings@gmail.com
   Web:			    onbings.dscloud.me
   Revision:    1.0

   History:

   V 1.00  Sep 30 2000  BHA : Initial release
 */
#pragma once
#pragma message("!!!!!!!!!!!!! CPP => TODO REMOVE FILE LATER !!!!!!!!!!!!!")
#include "bof2d.h"
//#include "bof2d_av_codec.h"
#include <C:\Program Files (x86)\Visual Leak Detector\include\vld.h>

BEGIN_BOF2D_NAMESPACE()

class BOF2D_EXPORT Bof2dAvWriter
{
public:
  Bof2dAvWriter();
  virtual ~Bof2dAvWriter();

  BOFERR Open(const std::string &_rOption_S);
  BOFERR Close();
  BOFERR BeginWrite();
  BOFERR EndWrite();

  bool IsAudioStreamPresent();
  void GetAudioWriteFlag(bool &_rBusy_B, bool &_rPending_B);

private:
  BOFERR CreateFileOut();
  BOFERR WriteHeader();
  BOFERR WriteChunkOut();
  BOFERR CloseFileOut();

  std::atomic<bool> mEncoderReady_B = false;
  std::atomic<bool> mWriteBusy_B = false;
  std::atomic<bool> mWritePending_B = false;
  std::vector<BOF::BOFPARAMETER> mAudEncOptionParam_X;
  BOF2D_AUD_ENC_OPTION mAudEncOption_X;
  std::vector<BOF2D_AUD_ENC_OUT> mIoCollection;
  BOF2D_AUD_DEC_OUT mAudDecOut_X;

  uint64_t mNbAudEncPacketSent_U64 = 0;
  uint64_t mNbAudEncFrameReceived_U64 = 0;
  uint64_t mNbTotalAudEncFrame_U64 = 0;
  uint64_t mNbTotalAudEncSample_U64 = 0;

  const int  mAudEncAllocAlignment_i = 32;
};

END_BOF2D_NAMESPACE()
