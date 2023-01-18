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
#include "bof2d_audio_decoder.h"
#include "bof2d_av_codec.h"

#include <bofstd/bofstring.h>
#include <bofstd/bofstringformatter.h>
 //https://www.jensign.com/multichannel/multichannelformat.html
 /*Sample24 bit96kHz5.1.wav
 ----- Sample 24 bit/ 96 kHz 5.1 Multichannel WAV File -------
 Here is a simple 6 channel 24 bit/ 96 kHz multi-tone multichannel wav file.
    wav5196.wav (8.23 MB)
 The file was synthesized programatically using a C# application (to be posted). The wav amplitude is 83% of peak digital and the duration is exactly 5 seconds. The 6 standard "5.1" channels (FL, FR, C, Sub, RL, RR) are targetted in the wav file. The data for each of the 6 channels corresponds to the note sequence of the B11 musical chord. The list below shows, in the order interleaved in the wav file data section, the speaker channel, note name and note number, and the note frequency in the equally-tempered chromatic scale with A4 = 440.000 Hz:
   Front Left          B2  (root)         123.471  Hz
   Front Right         F3# (5th)          184.997	Hz
   Front Center        A3 (flat7th)       220.000  Hz
   Sub                 B1 (root)           61.735  Hz
   Rear Left           C4# (9th)          277.183  Hz
   Rear Right          E4 (11th)          329.628  Hz
 The note sent to the Sub channel is an octave lower than the next highest note which targets the Front Left channel. The Sub note at 61.735 Hz is the lowest note achievable on the F French Horn.
 */
BEGIN_BOF2D_NAMESPACE()

BOF::BofEnum<BOF2D_AUDIO_FORMAT> S_Bof2dAudioFormatEnumConverter({
  { BOF2D_AUDIO_FORMAT::BOF2D_AUDIO_FORMAT_PCM, "PCM" },
  { BOF2D_AUDIO_FORMAT::BOF2D_AUDIO_FORMAT_WAV, "WAV" },
  { BOF2D_AUDIO_FORMAT::BOF2D_AUDIO_FORMAT_MAX, "MAX" },
  }, BOF2D_AUDIO_FORMAT::BOF2D_AUDIO_FORMAT_MAX);

#pragma pack(1)
//https://docs.fileformat.com/audio/wav/#:~:text=up%20to%20date.-,WAV%20File%20Format,contains%20the%20actual%20sample%20data
struct BOF2D_WAV_HEADER
{
  /*000*/  char pRiffHeader_c[4];                 // RIFF Header: "RIFF" Marks the file as a riff file. Characters are each 1 byte long.*/
  /*004*/  uint32_t WavTotalSizeInByteMinus8_U32; // Size of the overall file - 8 bytes, in bytes (32-bit integer). Typically, you’d fill this in after creation. */
  /*008*/  char pWavHeader_c[4];                  //File Type Header. For our purposes, it always equals “WAVE”.
  /*012*/  char pFmtHeader_c[4];                  //Format chunk marker. Includes trailing space and nullptr
  /*016*/  uint32_t FmtChunkSize_U32;             //Length of format data as listed above (Should be 16 for PCM)
  /*020*/  uint16_t AudioFormat_U16;              // Should be 1 for PCM. 3 for IEEE Float 
  /*022*/  uint16_t NbChannel_U16;                //Number of Channels
  /*024*/  uint32_t SampleRateInHz_U32;           //Sample Rate in Hz. Common values are 44100 (CD), 48000 (DAT). Sample Rate = Number of Samples per second, or Hertz.
  /*028*/  uint32_t ByteRate_U32;                 //Number of bytes per second:	(SampleRateInHz_U32 * BitPerSample_U16 * NbChannel_U16) / 8.
  /*032*/  uint16_t SampleAlignment_U16;          //(NbChannel_U16 * BitPerSample_U16) / 8
  /*034*/  uint16_t NbBitPerSample_U16;             //Bits per sample
  /*036*/  char pDataHeader_X[4];                 //“data” chunk header. Marks the beginning of the data section.
  /*040*/  uint32_t DataSizeInByte_U32;           //size of audio: number of samples * num_channels * bit_depth/8
};
#pragma pack()

Bof2dAudioDecoder::Bof2dAudioDecoder()
{
  mAudioOptionParam_X.push_back({ nullptr, "A_BASEFN", "if defined, audio buffer will be saved in this file","","", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_VARIABLE(mAudioOption_X.BasePath, PATH, 0, 0) });
  mAudioOptionParam_X.push_back({ nullptr, "A_CHUNK", "If specifies, each audio subframe will be recorded in a separate file", "", "", BOF::BOFPARAMETER_ARG_FLAG::NONE, BOF_PARAM_DEF_VARIABLE(mAudioOption_X.SaveChunk_B, BOOL, true, 0) });
  mAudioOptionParam_X.push_back({ nullptr, "A_SPLIT", "If specifies, each audio channel is saved in a different file", "", "", BOF::BOFPARAMETER_ARG_FLAG::NONE, BOF_PARAM_DEF_VARIABLE(mAudioOption_X.SpitInChannel_B, BOOL, true, 0) });

  mAudioOptionParam_X.push_back({ nullptr, "A_NBCHNL", "Specifies the number of audio channel to generate","","", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_VARIABLE(mAudioOption_X.NbChannel_U32, UINT32, 0, 4096) });
  mAudioOptionParam_X.push_back({ nullptr, "A_LAYOUT", "Specifies the channel layout to generate","","", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_VARIABLE(mAudioOption_X.ChannelLayout_U64, UINT64, 0, 0) });
  mAudioOptionParam_X.push_back({ nullptr, "A_RATE", "Specifies the audio sample rate to generate","","", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_VARIABLE(mAudioOption_X.SampleRateInHz_U32, UINT32, 0, 128000) });
  mAudioOptionParam_X.push_back({ nullptr, "A_FMT", "Specifies the audio format", "", "", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_ENUM(mAudioOption_X.Format_E, BOF2D_AUDIO_FORMAT::BOF2D_AUDIO_FORMAT_PCM, BOF2D_AUDIO_FORMAT::BOF2D_AUDIO_FORMAT_MAX, S_Bof2dAudioFormatEnumConverter, BOF2D_AUDIO_FORMAT) });
  mAudioOptionParam_X.push_back({ nullptr, "A_BPS", "Specifies the number of bits per sample","","", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_VARIABLE(mAudioOption_X.NbBitPerSample_U32, UINT32, 8, 64) });
}

Bof2dAudioDecoder::~Bof2dAudioDecoder()
{
  Close();
}

BOFERR Bof2dAudioDecoder::Open(const std::string &_rInputFile_S, const std::string &_rOption_S)
{
  BOFERR    Rts_E;
  int       Sts_i;
  uint32_t  i_U32, Nb_U32;
  BOF::BofCommandLineParser OptionParser;
  BOF2D_AUDIO_OUT_FILE OutFile_X;

  // Just in case, close previously opened media file.
  Close();

  mAudioOption_X.Reset();
  Rts_E = OptionParser.ToByte(_rOption_S, mAudioOptionParam_X, nullptr, nullptr);
  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    if (mAudioOption_X.NbBitPerSample_U32 == 8)
    {
      mSampleFmt_E = AV_SAMPLE_FMT_U8;
    }
    else if (mAudioOption_X.NbBitPerSample_U32 == 16)
    {
      mSampleFmt_E = AV_SAMPLE_FMT_S16;
    }
    else if (mAudioOption_X.NbBitPerSample_U32 == 24)
    {
      mSampleFmt_E = AV_SAMPLE_FMT_S32;
    }
    else
    {
      mSampleFmt_E = AV_SAMPLE_FMT_FLT;
    }
    Nb_U32 = mAudioOption_X.SpitInChannel_B ? mAudioOption_X.NbChannel_U32 : 1;
    for (i_U32 = 0; i_U32 < Nb_U32; i_U32++)
    {
      mAudioOutFileCollection.push_back(OutFile_X);
    }

// Open media file.
    Sts_i = avformat_open_input(&mpAudioFormatCtx_X, _rInputFile_S.c_str(), nullptr, nullptr);
    FFMPEG_CHK_IF_ERR(Sts_i, "Couldn't open file " + _rInputFile_S, Rts_E);
    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      /* Retrieve stream information */
      Sts_i = avformat_find_stream_info(mpAudioFormatCtx_X, nullptr);
      FFMPEG_CHK_IF_ERR(Sts_i, "Couldn't find stream information in " + _rInputFile_S, Rts_E);
      if (Rts_E == BOF_ERR_NO_ERROR)
      {
        for (i_U32 = 0; i_U32 < mpAudioFormatCtx_X->nb_streams; i_U32++)
        {
          if (mpAudioFormatCtx_X->streams[i_U32]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
          {
            mpAudioCodecParam_X = mpAudioFormatCtx_X->streams[i_U32]->codecpar;
            break;
          }
        }
        if (mpAudioCodecParam_X == nullptr)
        {
          FFMPEG_CHK_IF_ERR(AVERROR_STREAM_NOT_FOUND, "No audio stream in " + _rInputFile_S, Rts_E);
        }
        else
        {
          mAudioStreamIndex_i = i_U32;
          //av_dict_set(&opts, "b", "2.5M", 0);
          mpAudioCodec_X = avcodec_find_decoder(mpAudioCodecParam_X->codec_id);
          if (mpAudioCodec_X == nullptr)
          {
            FFMPEG_CHK_IF_ERR(AVERROR_STREAM_NOT_FOUND, "Could not find decoder for codec " + std::to_string(mpAudioCodecParam_X->codec_id), Rts_E);
          }
          else
          {
            mpAudioCodecCtx_X = avcodec_alloc_context3(mpAudioCodec_X);
            if (mpAudioCodecCtx_X == nullptr)
            {
              FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not avcodec_alloc_context3 for codec " + std::to_string(mpAudioCodecParam_X->codec_id), Rts_E);
            }
            else
            {
              Sts_i = avcodec_parameters_to_context(mpAudioCodecCtx_X, mpAudioCodecParam_X);
              FFMPEG_CHK_IF_ERR(Sts_i, "Error in avcodec_parameters_to_context", Rts_E);
              if (Rts_E == BOF_ERR_NO_ERROR)
              {
                mpAudioCodecCtx_X->thread_count = 4;
                Sts_i = avcodec_open2(mpAudioCodecCtx_X, mpAudioCodec_X, nullptr);
                FFMPEG_CHK_IF_ERR(Sts_i, "Could not avcodec_open2", Rts_E);
                if (Rts_E == BOF_ERR_NO_ERROR)
                {
                  mpAudioFrame_X = av_frame_alloc();
                  if (mpAudioFrame_X == nullptr)
                  {
                    FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not av_frame_alloc mpFrame_X", Rts_E);
                  }
                  else
                  {
                    mpAudioFrameConverted_X = av_frame_alloc();
                    if (mpAudioFrameConverted_X == nullptr)
                    {
                      FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not av_frame_alloc mpAudioFrameConverted_X", Rts_E);
                    }
                    else
                    {
                      mpAudioBuffer_U8 = (uint8_t *)av_malloc(MAX_AUDIO_SIZE);
                      if (mpAudioBuffer_U8 == nullptr)
                      {
                        FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not av_frame_alloc mpAudioBuffer_U8", Rts_E);
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      mAudioTimeBase_X = mpAudioFormatCtx_X->streams[mAudioStreamIndex_i]->time_base;

      /* prepare resampler */
      mpAudioSwrCtx_X = swr_alloc();
      if (mpAudioSwrCtx_X == nullptr)
      {
        FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not swr_alloc", Rts_E);
      }
      else
      {
        Sts_i = av_opt_set_int(mpAudioSwrCtx_X, "in_channel_count", mpAudioCodecCtx_X->channels, 0);
        FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int in_channel_count", Rts_E);
        if (Rts_E == BOF_ERR_NO_ERROR)
        {
          Sts_i = av_opt_set_int(mpAudioSwrCtx_X, "out_channel_count", mAudioOption_X.NbChannel_U32, 0);
          FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int out_channel_count", Rts_E);
          if (Rts_E == BOF_ERR_NO_ERROR)
          {
            Sts_i = av_opt_set_int(mpAudioSwrCtx_X, "in_channel_layout", mpAudioCodecCtx_X->channel_layout, 0);
            FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int in_channel_layout", Rts_E);
            if (Rts_E == BOF_ERR_NO_ERROR)
            {
              Sts_i = av_opt_set_int(mpAudioSwrCtx_X, "out_channel_layout", mAudioOption_X.ChannelLayout_U64, 0);
              FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int out_channel_layout", Rts_E);
              if (Rts_E == BOF_ERR_NO_ERROR)
              {
                Sts_i = av_opt_set_int(mpAudioSwrCtx_X, "in_sample_rate", mpAudioCodecCtx_X->sample_rate, 0);
                FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int in_sample_rate", Rts_E);
                if (Rts_E == BOF_ERR_NO_ERROR)
                {
                  Sts_i = av_opt_set_int(mpAudioSwrCtx_X, "out_sample_rate", mAudioOption_X.SampleRateInHz_U32, 0);
                  FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int out_sample_rate", Rts_E);
                  if (Rts_E == BOF_ERR_NO_ERROR)
                  {
                    Sts_i = av_opt_set_sample_fmt(mpAudioSwrCtx_X, "in_sample_fmt", mpAudioCodecCtx_X->sample_fmt, 0);
                    FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int in_sample_fmt", Rts_E);
                    if (Rts_E == BOF_ERR_NO_ERROR)
                    {
                      Sts_i = av_opt_set_sample_fmt(mpAudioSwrCtx_X, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
                      FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int out_sample_fmt", Rts_E);
                      if (Rts_E == BOF_ERR_NO_ERROR)
                      {
                        Sts_i = swr_init(mpAudioSwrCtx_X);
                        FFMPEG_CHK_IF_ERR(Sts_i, "Could not swr_init", Rts_E);
                        if (Rts_E == BOF_ERR_NO_ERROR)
                        {
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      if (mAudioOption_X.BasePath.IsValid())
      {
        Rts_E = CreateFileOut();
      }
    }

    if (Rts_E != BOF_ERR_NO_ERROR)
    {
      Close();
    }
  }
  return Rts_E;
}


BOFERR Bof2dAudioDecoder::BeginRead(BOF2D_AUDIO_DATA &_rAudioData_X)
{
  BOFERR Rts_E = BOF_ERR_EOF;
  int Sts_i;
  uint32_t TotalSizeOfAudioConverted_U32;

  _rAudioData_X.Reset();
  if (IsAudioStreamPresent())
  {
    if (mReadBusy_B)
    {
      Rts_E = BOF_ERR_EBUSY;
    }
    else
    {
      mReadBusy_B = true;
      Rts_E = BOF_ERR_NO_ERROR;
      while (Rts_E == BOF_ERR_NO_ERROR)
      {
        Sts_i = av_read_frame(mpAudioFormatCtx_X, &mPacket_X);
        FFMPEG_CHK_IF_ERR(Sts_i, "Cannot avcodec_send_packet Audio", Rts_E);
        if (Rts_E == BOF_ERR_NO_ERROR)
        {
          if (mPacket_X.stream_index == mAudioStreamIndex_i)
          {
            Sts_i = avcodec_send_packet(mpAudioCodecCtx_X, &mPacket_X);
            FFMPEG_CHK_IF_ERR(Sts_i, "Cannot avcodec_send_packet Audio", Rts_E);

            if (Rts_E == BOF_ERR_NO_ERROR)
            {
              mNbAudioPacketSent_U64++;
              Sts_i = avcodec_receive_frame(mpAudioCodecCtx_X, mpAudioFrame_X);
              FFMPEG_CHK_IF_ERR(Sts_i, "Cannot avcodec_receive_frame Audio", Rts_E);
              if (Rts_E == BOF_ERR_NO_ERROR)
              {
                mNbAudioFrameReceived_U64++;
                printf("AudioFrame Snt/Rcv %zd/%zd Pts %zd Fmt %s NbSmp %d Rate %d Buf %x:%p Layout %zx\n", mNbAudioPacketSent_U64, mNbAudioFrameReceived_U64, mpAudioFrame_X->pts, av_get_sample_fmt_name((AVSampleFormat)mpAudioFrame_X->format),
                  mpAudioFrame_X->nb_samples, mpAudioFrame_X->sample_rate, mpAudioFrame_X->linesize[0], mpAudioFrame_X->data[0],
                  mpAudioFrame_X->channel_layout);

                //Rts_E = ConvertAudio(false, mpAudioFrame_X, mpAudioFrameConverted_X, mAudioOption_X.NbChannel_U32, static_cast<uint32_t>(mAudioOption_X.ChannelLayout_U64), mAudioOption_X.SampleRateInHz_U32, AV_SAMPLE_FMT_S16);
                Rts_E = ConvertAudio(TotalSizeOfAudioConverted_U32);
                if (Rts_E == BOF_ERR_NO_ERROR)
                {
                  _rAudioData_X.Data_X.SetStorage(mpAudioFrameConverted_X->linesize[0], TotalSizeOfAudioConverted_U32, mpAudioFrameConverted_X->data[0]);
                  _rAudioData_X.NbSample_U32 = mpAudioFrameConverted_X->nb_samples;
                  _rAudioData_X.Param_X.NbChannel_U32 = mpAudioFrameConverted_X->channels;
                  _rAudioData_X.Param_X.ChannelLayout_U64 = mpAudioFrameConverted_X->channel_layout;
                  _rAudioData_X.Param_X.SampleRateInHz_U32 = mpAudioFrameConverted_X->sample_rate;
                }
                av_packet_unref(&mPacket_X);
                break;  //leave while so av_packet_unref(&mPacket_X);
              }
            }
          }
          av_packet_unref(&mPacket_X);  //Made in while
        }
      }
    }
  }
  return Rts_E;
}

BOFERR Bof2dAudioDecoder::EndRead()
{
  BOFERR Rts_E = BOF_ERR_EOF;

  if (IsAudioStreamPresent())
  {
    if (mReadBusy_B)
    {
      av_frame_unref(mpAudioFrame_X);

      mReadBusy_B = false;
      Rts_E = BOF_ERR_NO_ERROR;
    }
    else
    {
      Rts_E = BOF_ERR_PENDING;
    }
  }
  return Rts_E;
}

BOFERR Bof2dAudioDecoder::WriteHeader(const BOF2D_AUDIO_OUT_FILE &_rOutFile_X)
{
  BOFERR Rts_E = BOF_ERR_NOT_OPENED;
  BOF2D_WAV_HEADER WavHeader_X;
  uint32_t Nb_U32;
  int64_t Pos_S64;

  Pos_S64 = BOF::Bof_GetFileIoPosition(_rOutFile_X.Io);
  if (Pos_S64 != -1)
  {
    BOF::Bof_SetFileIoPosition(_rOutFile_X.Io, 0, BOF::BOF_SEEK_METHOD::BOF_SEEK_BEGIN);
    memcpy(&WavHeader_X.pRiffHeader_c, "RIFF", 4);
    WavHeader_X.WavTotalSizeInByteMinus8_U32 = static_cast<uint32_t>(_rOutFile_X.Size_U64 + sizeof(struct BOF2D_WAV_HEADER) - 8);
    memcpy(&WavHeader_X.pWavHeader_c, "WAVE", 4);
    memcpy(&WavHeader_X.pFmtHeader_c, "fmt ", 4);
    WavHeader_X.FmtChunkSize_U32 = 16;  //PCM
    WavHeader_X.AudioFormat_U16 = 1;    //PCM
    WavHeader_X.NbChannel_U16 = static_cast<uint16_t>(mAudioOption_X.NbChannel_U32);
    WavHeader_X.SampleRateInHz_U32 = mAudioOption_X.SampleRateInHz_U32;
    WavHeader_X.SampleAlignment_U16 = static_cast<uint16_t>((mAudioOption_X.NbChannel_U32 * mAudioOption_X.NbBitPerSample_U32) / 8);
    WavHeader_X.NbBitPerSample_U16 = static_cast<uint16_t>(mAudioOption_X.NbBitPerSample_U32);
    WavHeader_X.ByteRate_U32 = mAudioOption_X.SampleRateInHz_U32 * WavHeader_X.SampleAlignment_U16;
    memcpy(&WavHeader_X.pDataHeader_X, "data", 4);
    WavHeader_X.DataSizeInByte_U32 = static_cast<uint32_t>(_rOutFile_X.Size_U64);
    Nb_U32 = sizeof(struct BOF2D_WAV_HEADER);
    Rts_E = BOF::Bof_WriteFile(_rOutFile_X.Io, Nb_U32, reinterpret_cast<uint8_t *>(&WavHeader_X));
    //NO    BOF::Bof_SetFileIoPosition(mOutAudioFile, Pos_S64, BOF::BOF_SEEK_METHOD::BOF_SEEK_BEGIN);
  }
  return Rts_E;
}

BOFERR Bof2dAudioDecoder::CreateFileOut()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR, Sts_E;
  uint32_t i_U32;
  std::string Extension_S = (mAudioOption_X.Format_E == BOF2D_AUDIO_FORMAT::BOF2D_AUDIO_FORMAT_WAV) ? ".wav" : ".pcm";

  for (i_U32 = 0; i_U32 < mAudioOutFileCollection.size(); i_U32++)
  {
    mAudioOutFileCollection[i_U32].Reset();
    Sts_E = BOF::Bof_CreateFile(BOF::BOF_FILE_PERMISSION_ALL_FOR_OWNER | BOF::BOF_FILE_PERMISSION_READ_FOR_ALL, mAudioOption_X.BasePath.FullPathNameWithoutExtension(false) + Extension_S, false, mAudioOutFileCollection[i_U32].Io);
    if (Sts_E == BOF_ERR_NO_ERROR)
    {
      Sts_E = WriteHeader(mAudioOutFileCollection[i_U32]);
    }
    if (Sts_E != BOF_ERR_NO_ERROR)
    {
      Rts_E = Sts_E;
    }
  }
  return Rts_E;
}

BOFERR Bof2dAudioDecoder::WriteChunkOut(uint32_t _ChunkSizeInByte_U32)
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR, Sts_E;
  uint32_t Nb_U32, i_U32, j_U32;
  intptr_t OutAudioChunkFile;
  std::string ChunkPath_S;

  for (i_U32 = 0; i_U32 < mAudioOutFileCollection.size(); i_U32++)
  {
    Nb_U32 = _ChunkSizeInByte_U32;
    Sts_E = BOF::Bof_WriteFile(mAudioOutFileCollection[i_U32].Io, Nb_U32, mpAudioFrameConverted_X->data[0]);
    if (Sts_E == BOF_ERR_NO_ERROR)
    {
      mAudioOutFileCollection[i_U32].Size_U64 += Nb_U32;
      if (mAudioOption_X.SaveChunk_B)
      {

        continuer avec le split en channel

        //for (j_U32 = 0; j_U32 < mAudioOption_X.NbChannel_U32; j_U32++)
        j_U32 = 0;
        {
          ChunkPath_S = BOF::Bof_Sprintf("%s_%08zd_%03d.pcm", mAudioOption_X.BasePath.FullPathNameWithoutExtension(false).c_str(), mNbTotalAudioFrame_U64, j_U32);
          Sts_E = BOF::Bof_CreateFile(BOF::BOF_FILE_PERMISSION_ALL_FOR_OWNER | BOF::BOF_FILE_PERMISSION_READ_FOR_ALL, ChunkPath_S, false, OutAudioChunkFile);
          if (Sts_E == BOF_ERR_NO_ERROR)
          {
            Nb_U32 = _ChunkSizeInByte_U32;
            Sts_E = BOF::Bof_WriteFile(OutAudioChunkFile, Nb_U32, mpAudioFrameConverted_X->data[i_U32]);
            BOF::Bof_CloseFile(OutAudioChunkFile);
          }
        }
      }
    }
    if (Sts_E != BOF_ERR_NO_ERROR)
    {
      Rts_E = Sts_E;
    }
  }
  return Rts_E;
}
BOFERR Bof2dAudioDecoder::CloseFileOut()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR, Sts_E;
  uint32_t i_U32;

  for (i_U32 = 0; i_U32 < mAudioOutFileCollection.size(); i_U32++)
  {
    Sts_E = WriteHeader(mAudioOutFileCollection[i_U32]);
    if (Sts_E != BOF_ERR_NO_ERROR)
    {
      Rts_E = Sts_E;
    }
    BOF::Bof_CloseFile(mAudioOutFileCollection[i_U32].Io);
    mAudioOutFileCollection[i_U32].Reset();
  }
  return Rts_E;
}

BOFERR Bof2dAudioDecoder::ConvertAudio(uint32_t &_rTotalSizeOfAudioConverted_U32)
{
  BOFERR Rts_E;
  uint32_t NbAudioSample_U32, AudioBufferSize_U32, NbAudioConvertedSamplePerChannel_U32;
  int64_t AudioDelay_S64;

  _rTotalSizeOfAudioConverted_U32 = 0;
  AudioDelay_S64 = swr_get_delay(mpAudioSwrCtx_X, mpAudioCodecCtx_X->sample_rate);
  NbAudioSample_U32 = (int)av_rescale_rnd(AudioDelay_S64 + mpAudioFrame_X->nb_samples, mAudioOption_X.SampleRateInHz_U32, mpAudioCodecCtx_X->sample_rate, AV_ROUND_UP);
  AudioBufferSize_U32 = av_samples_get_buffer_size(nullptr, mAudioOption_X.NbChannel_U32, NbAudioSample_U32, mSampleFmt_E, 0);

  Rts_E = BOF_ERR_TOO_BIG;
  BOF_ASSERT(AudioBufferSize_U32 <= MAX_AUDIO_SIZE);
  if (AudioBufferSize_U32 <= MAX_AUDIO_SIZE)
  {
    Rts_E = BOF_ERR_NO_ERROR;
    av_samples_fill_arrays(mpAudioFrameConverted_X->data, mpAudioFrameConverted_X->linesize, mpAudioBuffer_U8, mAudioOption_X.NbChannel_U32, NbAudioSample_U32, mSampleFmt_E, 0);
    NbAudioConvertedSamplePerChannel_U32 = swr_convert(mpAudioSwrCtx_X, &mpAudioBuffer_U8, NbAudioSample_U32, (const uint8_t **)mpAudioFrame_X->data, mpAudioFrame_X->nb_samples);
    _rTotalSizeOfAudioConverted_U32 = (NbAudioConvertedSamplePerChannel_U32 * mAudioOption_X.NbChannel_U32 * mAudioOption_X.NbBitPerSample_U32) / 8;
    //printf("---->NbAudioSample %d memset %d NbAudioConvertSamplePerChannel %d->%d linesize %d\n", NbAudioSample_U32, AudioBufferSize_U32, NbAudioConvertedSamplePerChannel_U32, _rTotalSizeOfAudioConverted_U32, mpAudioFrameConverted_X->linesize[0]);
    BOF_ASSERT(_rTotalSizeOfAudioConverted_U32 <= static_cast<uint32_t>(mpAudioFrameConverted_X->linesize[0]));
    if (_rTotalSizeOfAudioConverted_U32 <= static_cast<uint32_t>(mpAudioFrameConverted_X->linesize[0]))
    {
      mpAudioFrameConverted_X->nb_samples = NbAudioConvertedSamplePerChannel_U32;
      mpAudioFrameConverted_X->channels = mAudioOption_X.NbChannel_U32;
      mpAudioFrameConverted_X->channel_layout = static_cast<uint32_t>(mAudioOption_X.ChannelLayout_U64);
      mpAudioFrameConverted_X->format = mSampleFmt_E;
      mpAudioFrameConverted_X->sample_rate = mAudioOption_X.SampleRateInHz_U32;
      mpAudioFrameConverted_X->pkt_pos = mpAudioFrame_X->pkt_pos;
      mpAudioFrameConverted_X->pkt_duration = mpAudioFrame_X->pkt_duration;
      mpAudioFrameConverted_X->pkt_size = mpAudioFrame_X->pkt_size;
      mNbTotalAudioFrame_U64++;
      mNbTotaAudioSample_U64 += _rTotalSizeOfAudioConverted_U32;
      if (mAudioOption_X.BasePath.IsValid())
      {
        Rts_E = WriteChunkOut(_rTotalSizeOfAudioConverted_U32);
      }
      int16_t *pData_S16 = (int16_t *)mpAudioFrameConverted_X->data[0];
      printf("Cnv Audio %x->%x:%p nbs %d ch %d layout %zx Fmt %d Rate %d Pos %zd Dur %zd Sz %d\n", mpAudioFrameConverted_X->linesize[0], _rTotalSizeOfAudioConverted_U32, mpAudioFrameConverted_X->data[0],
        mpAudioFrameConverted_X->nb_samples, mpAudioFrameConverted_X->channels, mpAudioFrameConverted_X->channel_layout, mpAudioFrameConverted_X->format, mpAudioFrameConverted_X->sample_rate,
        mpAudioFrameConverted_X->pkt_pos, mpAudioFrameConverted_X->pkt_duration, mpAudioFrameConverted_X->pkt_size);
      printf("Cnv Data %04x %04x %04x %04x %04x %04x %04x %04x\n", pData_S16[0], pData_S16[1], pData_S16[2], pData_S16[3], pData_S16[4], pData_S16[5], pData_S16[6], pData_S16[7]);
    }
    else
    {
      Rts_E = BOF_ERR_EOVERFLOW;
    }
  }

  return Rts_E;
}

BOFERR Bof2dAudioDecoder::Close()
{
  BOFERR Rts_E;
  bool AudioBufferHasBeenFreeed_B = false;
  //  avio_context_free(&mpAudioAvioCtx_X);

  swr_free(&mpAudioSwrCtx_X);
  if (mpAudioFrame_X)
  {
    //If we have used mpAudioBuffer_U8 via the Begin/EndRead method we check that here that buffer in mpAudioFrameConverted_X
    //is the same than mpAudioBuffer_U8. In this case we do not call av_freep(&mpAudioBuffer_U8); and set mpAudioBuffer_U8 to nullptr
    if (mpAudioFrameConverted_X->data[0] == mpAudioBuffer_U8)
    {
      mpAudioBuffer_U8 = nullptr;
      AudioBufferHasBeenFreeed_B = true;  //Done by av_freep(&mpAudioFrame_X->data[0]); just below
    }
    av_freep(&mpAudioFrame_X->data[0]);
    av_frame_free(&mpAudioFrame_X);
  }
  if (!AudioBufferHasBeenFreeed_B)
  {
    //If we just open/close decoder without calling read we need to free mpAudioBuffer_U8 here
    av_freep(&mpAudioBuffer_U8);
  }
  if (mpAudioFrameConverted_X)
  {
    av_freep(&mpAudioFrameConverted_X->data[0]);
    av_frame_free(&mpAudioFrameConverted_X);
  }
  //avcodec_close: Do not use this function. Use avcodec_free_context() to destroy a codec context(either open or closed)
  //avcodec_close(mpAudioCodecCtx_X);
  avcodec_free_context(&mpAudioCodecCtx_X);

  avformat_close_input(&mpAudioFormatCtx_X);

  if (mAudioOption_X.BasePath.IsValid())
  {
    CloseFileOut();
  }
  mSampleFmt_E = AV_SAMPLE_FMT_NONE;
  mAudioStreamIndex_i = -1;
  mpAudioCodecParam_X = nullptr;
  mpAudioCodec_X = nullptr;

  mAudioTimeBase_X = { 0, 0 }; //or av_make_q

  mNbAudioPacketSent_U64 = 0;
  mNbAudioFrameReceived_U64 = 0;
  mNbTotalAudioFrame_U64 = 0;
  mNbTotaAudioSample_U64 = 0;
  mAudioOption_X.Reset();

  Rts_E = BOF_ERR_NO_ERROR;
  return Rts_E;
}

bool Bof2dAudioDecoder::IsAudioStreamPresent()
{
  return(mAudioStreamIndex_i != -1);
}

END_BOF2D_NAMESPACE()