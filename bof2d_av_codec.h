/*
   Copyright (c) 2000-2026, OnBings. All rights reserved.

   THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
   KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
   PURPOSE.

   This module defines the interface to the Ffmpeg lib

   Author:      Bernard HARMEL: onbings@gmail.com
   Web:			    onbings.dscloud.me
   Revision:    1.0

   History:

   V 1.00  Sep 30 2000  BHA : Initial release
 */
 //https://digital-domain.net/programming/ffmpeg-libs-audio-transcode.html
 //https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/avio_reading.c
 //https://github.com/UnickSoft/FFMpeg-decode-example/blob/master/Bof2dAudioDecoder/ffmpegDecode.cpp

#pragma once
#pragma message("!!!!!!!!!!!!! CPP => TODO REMOVE FILE LATER !!!!!!!!!!!!!")
#include "bof2d.h"
#include <C:\Program Files (x86)\Visual Leak Detector\include\vld.h>

#include "bof2d_video_decoder.h"
#include "bof2d_audio_decoder.h"

extern "C"
{
#include <libavutil/common.h>
#include <libavutil/error.h>
}

#define FFMPEG_CHK_IF_ERR(Sts, Ctx, Rts)  {const char *pFile_c; BOF_GET_FILE_FROM__FILE__(pFile_c); Rts = Bof2d_FfmpegCheckIfError(Sts, Ctx, pFile_c, __func__, __LINE__);}

BEGIN_BOF2D_NAMESPACE()

void Bof2d_FfmpegLogCallback(void *_pAvcl, int _Level_i, const char *_pFormat_c, va_list _VaList);
BOFERR Bof2d_FfmpegCheckIfError(int _FfmpegErrorCode_i, const std::string &_rErrorContext_S, const std::string &_rFile_S, const std::string &_rFunction_S, uint32_t _Line_U32);



class BOF2D_EXPORT Bof2dAvCodec
{
public:
  Bof2dAvCodec(int _LogLevel_i = AV_LOG_ERROR);
  virtual ~Bof2dAvCodec();

  BOFERR Open(const std::string &_rInputFile_S, const std::string &_rOption_S);
  BOFERR BeginRead(BOF2D_VIDEO_DATA &_rVideoData_X, BOF2D_AUDIO_DATA &_rAudioData_X);
  BOFERR EndRead();
  BOFERR Close();

private:
  std::unique_ptr<Bof2dVideoDecoder> mpuVideoDecoder;
  std::unique_ptr<Bof2dAudioDecoder> mpuAudioDecoder;
};

END_BOF2D_NAMESPACE()
