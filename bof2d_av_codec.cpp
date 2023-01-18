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
#include "bof2d_av_codec.h"
#include <map>

BEGIN_BOF2D_NAMESPACE()

Bof2dAvCodec::Bof2dAvCodec(int _LogLevel_i)
{
  //AV_LOG_TRACE
  av_log_set_callback(Bof2d_FfmpegLogCallback);
  av_log_set_level(_LogLevel_i);  // AV_LOG_VERBOSE); // AV_LOG_TRACE); // AV_LOG_ERROR);
  printf("FFmpeg avformat version: %08X\nInfo: %s\nConfig: %s\nLicense: %s\n", avformat_version(), av_version_info(), avformat_configuration(), avformat_license());
  printf("FFmpeg avcodec version:  %08X\nInfo: %s\nConfig: %s\nLicense: %s\n", avcodec_version(), av_version_info(), avcodec_configuration(), avcodec_license());
  printf("FFmpeg avutil version:   %08X\nInfo: %s\nConfig: %s\nLicense: %s\n", avutil_version(), av_version_info(), avutil_configuration(), avutil_license());
  printf("FFmpeg swscale version:  %08X\nInfo: %s\nConfig: %s\nLicense: %s\n", swscale_version(), av_version_info(), swscale_configuration(), swscale_license());

  printf("LogLevel is %d\n", av_log_get_level());

  mpuVideoDecoder = std::make_unique<Bof2dVideoDecoder>();
  BOF_ASSERT(mpuVideoDecoder != nullptr);
  mpuAudioDecoder = std::make_unique<Bof2dAudioDecoder>();
  BOF_ASSERT(mpuAudioDecoder != nullptr);
}
Bof2dAvCodec::~Bof2dAvCodec()
{
  mpuAudioDecoder->Close();
  mpuVideoDecoder->Close();
}
BOFERR Bof2dAvCodec::Open(const std::string &_rInputFile_S, const std::string &_rVideoOption_S, const std::string &_rAudioOption_S)
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR, StsVideo_E, StsAudio_E;

  StsVideo_E = mpuVideoDecoder->Open(_rInputFile_S, _rVideoOption_S);

  StsAudio_E = mpuAudioDecoder->Open(_rInputFile_S, _rAudioOption_S);
  
  if ((StsVideo_E != BOF_ERR_NO_ERROR) && (mpuVideoDecoder->IsVideoStreamPresent()))
  {
    Rts_E = BOF_ERR_NOT_OPENED;
  }
  else if ((StsAudio_E != BOF_ERR_NO_ERROR) && (mpuAudioDecoder->IsAudioStreamPresent()))
  {
    Rts_E = BOF_ERR_NOT_OPENED;
  }
  return Rts_E;
}
BOFERR Bof2dAvCodec::BeginRead(BOF2D_VIDEO_DATA &_rVideoData_X, BOF2D_AUDIO_DATA &_rAudioData_X)
{
  BOFERR StsVideo_E = BOF_ERR_NO_ERROR, StsAudio_E = BOF_ERR_NO_ERROR;

  _rVideoData_X.Reset();
  StsVideo_E = (mpuVideoDecoder->IsVideoStreamPresent()) ? mpuVideoDecoder->BeginRead(_rVideoData_X): BOF_ERR_NO_ERROR;
  
  _rAudioData_X.Reset();
  StsAudio_E = (mpuAudioDecoder->IsAudioStreamPresent()) ? mpuAudioDecoder->BeginRead(_rAudioData_X) : BOF_ERR_NO_ERROR;
  
  return (StsVideo_E == BOF_ERR_NO_ERROR) ? StsAudio_E: StsVideo_E;
}
BOFERR Bof2dAvCodec::EndRead()
{
  BOFERR StsVideo_E = BOF_ERR_NO_ERROR, StsAudio_E = BOF_ERR_NO_ERROR;

  StsVideo_E = (mpuVideoDecoder->IsVideoStreamPresent()) ? mpuVideoDecoder->EndRead() : BOF_ERR_NO_ERROR;

  StsAudio_E = (mpuAudioDecoder->IsAudioStreamPresent()) ? mpuAudioDecoder->EndRead() : BOF_ERR_NO_ERROR;

  return (StsVideo_E == BOF_ERR_NO_ERROR) ? StsAudio_E : StsVideo_E;
}
BOFERR Bof2dAvCodec::Close()
{
  BOFERR Rts_E;

  Rts_E = mpuVideoDecoder->Close();
  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    Rts_E = mpuAudioDecoder->Close();
  }
  return Rts_E;
}
/*
* @param avcl A pointer to an arbitrary struct of which the first field is a
 *        pointer to an AVClass struct or nullptr if general log.
 * @param level The importance level of the message expressed using a @ref
 *        lavu_log_constants "Logging Constant".
 * @param fmt The format string (printf-compatible) that specifies how
 *        subsequent arguments are converted to output.
*/
void Bof2d_FfmpegLogCallback(void *_pAvcl, int _Level_i, const char *_pFormat_c, va_list _VaList)
{
  char pLog_c[0x1000];
  //va_list VaList;
  if (_Level_i <= av_log_get_level())
  {
    //av_printf_format(3, 4);
        //va_start(VaList, _pFormat_c);
    vsprintf(pLog_c, _pFormat_c, _VaList);
    //va_end(VaList);
    printf("Ffmpeg L%03d->%s", _Level_i, pLog_c);
  }
}

BOFERR Bof2d_FfmpegCheckIfError(int _FfmpegErrorCode_i, const std::string &_rErrorContext_S, const std::string &_rFile_S, const std::string &_rFunction_S, uint32_t _Line_U32)
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
  std::string Err_S;
  size_t ErrorSize = AV_ERROR_MAX_STRING_SIZE;
  static const std::map<int, BOFERR> S_Ffmpeg2BoferrorCodeCollection
  {
    {AVERROR_BSF_NOT_FOUND     , BOF_ERR_NOT_FOUND },
    {AVERROR_BUG               , BOF_ERR_INTERNAL},
    {AVERROR_BUFFER_TOO_SMALL  , BOF_ERR_TOO_SMALL},
    {AVERROR_DECODER_NOT_FOUND , BOF_ERR_CODEC},
    {AVERROR_DEMUXER_NOT_FOUND , BOF_ERR_INPUT},
    {AVERROR_ENCODER_NOT_FOUND , BOF_ERR_OUTPUT},
    {AVERROR_EOF               , BOF_ERR_EOF},
    {AVERROR_EXIT              , BOF_ERR_CANNOT_STOP},
    {AVERROR_EXTERNAL          , BOF_ERR_ELIBACC},
    {AVERROR_FILTER_NOT_FOUND  , BOF_ERR_NOT_FOUND},
    {AVERROR_INVALIDDATA       , BOF_ERR_ENODATA},
    {AVERROR_MUXER_NOT_FOUND   , BOF_ERR_NOT_FOUND},
    {AVERROR_OPTION_NOT_FOUND  , BOF_ERR_NOT_FOUND},
    {AVERROR_PATCHWELCOME      , BOF_ERR_EMLINK},
    {AVERROR_PROTOCOL_NOT_FOUND, BOF_ERR_NOT_FOUND},
    {AVERROR_STREAM_NOT_FOUND  , BOF_ERR_NOT_FOUND},
    {AVERROR_BUG2              , BOF_ERR_INTERNAL},
    {AVERROR_UNKNOWN           , BOF_ERR_INTERNAL},
    {AVERROR_EXPERIMENTAL      , BOF_ERR_INTERNAL},
    {AVERROR_INPUT_CHANGED     , BOF_ERR_INPUT},
    {AVERROR_OUTPUT_CHANGED    , BOF_ERR_OUTPUT},
    {AVERROR_HTTP_BAD_REQUEST  , BOF_ERR_EBADRQC},
    {AVERROR_HTTP_UNAUTHORIZED , BOF_ERR_EACCES},
    {AVERROR_HTTP_FORBIDDEN    , BOF_ERR_LOCK},
    {AVERROR_HTTP_NOT_FOUND    , BOF_ERR_NOT_FOUND},
    {AVERROR_HTTP_OTHER_4XX    , BOF_ERR_NOT_FOUND},
    {AVERROR_HTTP_SERVER_ERROR , BOF_ERR_ENODEV}
  };
  if (_FfmpegErrorCode_i < 0)
  {
    Err_S.resize(ErrorSize);
    if (av_strerror(_FfmpegErrorCode_i, &Err_S.at(0), ErrorSize))
    {
      Err_S = "Unable to translate Ffmpeg error code " + std::to_string(_FfmpegErrorCode_i);  //Less than AV_ERROR_MAX_STRING_SIZE 
    }

    auto It = S_Ffmpeg2BoferrorCodeCollection.find(_FfmpegErrorCode_i);
    if (It != S_Ffmpeg2BoferrorCodeCollection.end())
    {
      Rts_E = It->second;
    }
    else
    {
      //Rts_E = BOF_ERR_INTERFACE;
      Rts_E = (BOFERR)(AVUNERROR(_FfmpegErrorCode_i));
    }
    printf("%s:%s:%04d Error %d/%X\nBof:    %s\nCtx:    %s\nFfmpeg: %s\n", _rFile_S.c_str(), _rFunction_S.c_str(), _Line_U32, _FfmpegErrorCode_i, _FfmpegErrorCode_i, BOF::Bof_ErrorCode(Rts_E), _rErrorContext_S.c_str(), Err_S.c_str());
  }
  return Rts_E;
}
END_BOF2D_NAMESPACE()