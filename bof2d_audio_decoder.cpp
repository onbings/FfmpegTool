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
#include "bof2d_audio_encoder.h"

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
  mAudDecOptionParam_X.push_back({ nullptr, "A_NBCHNL", "Specifies the number of audio channel to generate","","", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_VARIABLE(mAudDecOption_X.NbChannel_U32, UINT32, 0, 4096) });
  mAudDecOptionParam_X.push_back({ nullptr, "A_LAYOUT", "Specifies the channel layout to generate","","", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_VARIABLE(mAudDecOption_X.ChannelLayout_U64, UINT64, 0, 0) });
  mAudDecOptionParam_X.push_back({ nullptr, "A_RATE", "Specifies the audio sample rate to generate","","", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_VARIABLE(mAudDecOption_X.SampleRateInHz_U32, UINT32, 0, 128000) });
  mAudDecOptionParam_X.push_back({ nullptr, "A_BPS", "Specifies the number of bits per sample","","", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_VARIABLE(mAudDecOption_X.NbBitPerSample_U32, UINT32, 8, 64) });
  mAudDecOptionParam_X.push_back({ nullptr, "A_DEMUX", "If specifies, each audio channel is extracted and stored in a different memory buffer", "", "", BOF::BOFPARAMETER_ARG_FLAG::NONE, BOF_PARAM_DEF_VARIABLE(mAudDecOption_X.DemuxChannel_B, BOOL, true, 0) });
  mAudDecOptionParam_X.push_back({ nullptr, "V_THREAD", "Specifies number of thread used by decodeer (affect delay)","","", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_VARIABLE(mAudDecOption_X.NbThread_U32, UINT32, 1, 64) });
}

Bof2dAudioDecoder::~Bof2dAudioDecoder()
{
  Close();
}

BOFERR Bof2dAudioDecoder::Open(AVFormatContext *_pDecFormatCtx_X, const std::string &_rVidDecOption_S, int &_rAudDecStreamIndex_i)
{
  BOFERR    Rts_E = BOF_ERR_EINVAL;
  int       Sts_i;
  uint32_t  i_U32, NbMaxAudioSamplePerChannel_U32, MaxAudioBufferSize_U32;
  uint8_t *pAudDecBuffer_U8;
  BOF::BofCommandLineParser OptionParser;
  BOF::BOF_BUFFER AudDecBuffer_X;
  AVDictionary *pMetadata_X;
  AVDictionaryEntry *pEntry_X;

  _rAudDecStreamIndex_i = -1;
  if (_pDecFormatCtx_X)
  {
    Rts_E = BOF_ERR_ALREADY_OPENED;
    if (mAudDecState_E == BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_IDLE)
    {
      Close();

      mAudDecOption_X.Reset();
      Rts_E = OptionParser.ToByte(_rVidDecOption_S, mAudDecOptionParam_X, nullptr, nullptr);
      if (Rts_E == BOF_ERR_NO_ERROR)
      {
        if (mAudDecOption_X.NbChannel_U32 == 0)
        {
          mAudDecOption_X.NbChannel_U32 = 2;
        }
        if (mAudDecOption_X.ChannelLayout_U64 == 0)
        {
          mAudDecOption_X.ChannelLayout_U64 = 3;
        }
        if (mAudDecOption_X.SampleRateInHz_U32 == 0)
        {
          mAudDecOption_X.SampleRateInHz_U32 = 48000;
        }
        if (mAudDecOption_X.NbBitPerSample_U32 == 0)
        {
          mAudDecOption_X.NbBitPerSample_U32 = 24;
        }
        mAudDecNbBitPerSample_U32 = mAudDecOption_X.NbBitPerSample_U32;
        if (mAudDecOption_X.NbBitPerSample_U32 == 8)
        {
          mSampleFmt_E = AV_SAMPLE_FMT_U8;
        }
        else if (mAudDecOption_X.NbBitPerSample_U32 == 16)
        {
          mSampleFmt_E = AV_SAMPLE_FMT_S16;
        }
        else if (mAudDecOption_X.NbBitPerSample_U32 == 24)
        {
          mSampleFmt_E = AV_SAMPLE_FMT_S32;
          mAudDecNbBitPerSample_U32 = 32; //Like this in ffmpeg
        }
        else
        {
          mSampleFmt_E = AV_SAMPLE_FMT_FLT;
        }

        for (i_U32 = 0; i_U32 < _pDecFormatCtx_X->nb_streams; i_U32++)
        {
          if (_pDecFormatCtx_X->streams[i_U32]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
          {
            mpAudStream_X = _pDecFormatCtx_X->streams[i_U32];
            mpAudDecCodecParam_X = mpAudStream_X->codecpar;
            break;
          }
        }
        if (mpAudDecCodecParam_X == nullptr)
        {
          FFMPEG_CHK_IF_ERR(AVERROR_STREAM_NOT_FOUND, "No audio stream present", Rts_E);
        }
        else
        {
          mAudDecStreamIndex_i = i_U32;
          _rAudDecStreamIndex_i = mAudDecStreamIndex_i;
          //av_dict_set(&opts, "b", "2.5M", 0);
          pMetadata_X = mpAudStream_X->metadata;
          if (pMetadata_X != nullptr)
          {
            pEntry_X = nullptr;
            while (pEntry_X = av_dict_get(pMetadata_X, "", pEntry_X, AV_DICT_IGNORE_SUFFIX))
            {
              printf("Audio Metadata %s: %s\n", pEntry_X->key, pEntry_X->value);
              mAudMetadataCollection[pEntry_X->key] = pEntry_X->value;
            }
          }
          mpAudDecCodec_X = avcodec_find_decoder(mpAudDecCodecParam_X->codec_id);
          if (mpAudDecCodec_X == nullptr)
          {
            FFMPEG_CHK_IF_ERR(AVERROR_STREAM_NOT_FOUND, "Could not find decoder for codec " + std::to_string(mpAudDecCodecParam_X->codec_id), Rts_E);
          }
          else
          {
            mpAudDecCodecCtx_X = avcodec_alloc_context3(mpAudDecCodec_X);
            if (mpAudDecCodecCtx_X == nullptr)
            {
              FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not avcodec_alloc_context3 for codec " + std::to_string(mpAudDecCodecParam_X->codec_id), Rts_E);
            }
            else
            {
              Sts_i = avcodec_parameters_to_context(mpAudDecCodecCtx_X, mpAudDecCodecParam_X);
              FFMPEG_CHK_IF_ERR(Sts_i, "Error in avcodec_parameters_to_context", Rts_E);
              if (Rts_E == BOF_ERR_NO_ERROR)
              {
//                mpAudDecCodecCtx_X->bits_per_raw_sample = 24;

                if (mAudDecOption_X.NbThread_U32 == 0)
                {
                  mAudDecOption_X.NbThread_U32 = 1;
                }
                mpAudDecCodecCtx_X->thread_count = mAudDecOption_X.NbThread_U32;
                Sts_i = avcodec_open2(mpAudDecCodecCtx_X, mpAudDecCodec_X, nullptr);
                FFMPEG_CHK_IF_ERR(Sts_i, "Could not avcodec_open2", Rts_E);
                if (Rts_E == BOF_ERR_NO_ERROR)
                {
                  mpAudDecFrame_X = av_frame_alloc();
                  if (mpAudDecFrame_X == nullptr)
                  {
                    FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not av_frame_alloc mpAudDecFrame_X", Rts_E);
                  }
                  else
                  {
                    mpAudDecFrameConverted_X = av_frame_alloc();
                    if (mpAudDecFrameConverted_X == nullptr)
                    {
                      FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not av_frame_alloc mpAudDecFrameConverted_X", Rts_E);
                    }
                    else
                    {
                      //Allocate global audio decoder buffer
                      NbMaxAudioSamplePerChannel_U32 = 0x4000;  //Just as a starting point
                      MaxAudioBufferSize_U32 = 0x1000 + av_samples_get_buffer_size(nullptr, mAudDecOption_X.NbChannel_U32, NbMaxAudioSamplePerChannel_U32, mSampleFmt_E, 0); //0x1000 Security and protection for the division later in the code
                      pAudDecBuffer_U8 = (uint8_t *)av_malloc(MaxAudioBufferSize_U32);
                      if (pAudDecBuffer_U8 == nullptr)
                      {
                        FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not av_frame_alloc global audio", Rts_E);
                      }
                      else
                      {
                        //Store global audio decoder buffer characteristics
                        mAudDecOut_X.InterleavedData_X.SetStorage(MaxAudioBufferSize_U32, 0, pAudDecBuffer_U8);
                        if (mAudDecOption_X.DemuxChannel_B)
                        {
                          //Allocate and store per channel audio decoder buffer characteristics
                          MaxAudioBufferSize_U32 /= (mAudDecOption_X.NbChannel_U32 ? mAudDecOption_X.NbChannel_U32 : 1);
                          for (i_U32 = 0; i_U32 < mAudDecOption_X.NbChannel_U32; i_U32++)
                          {
                            AudDecBuffer_X.Reset();
                            pAudDecBuffer_U8 = new uint8_t[MaxAudioBufferSize_U32];
                            if (pAudDecBuffer_U8 == nullptr)
                            {
                              FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not av_frame_alloc channel audio", Rts_E);
                            }
                            else
                            {
                              AudDecBuffer_X.SetStorage(MaxAudioBufferSize_U32, 0, pAudDecBuffer_U8);
                              mAudDecOut_X.ChannelBufferCollection.push_back(AudDecBuffer_X);
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
          mAudDecTimeBase_X = mpAudStream_X->time_base;
          mAudDurationInSec_lf = av_q2d(mAudDecTimeBase_X) * mpAudStream_X->duration;

          /* prepare resampler */
          mpAudDecSwrCtx_X = swr_alloc();
          if (mpAudDecSwrCtx_X == nullptr)
          {
            FFMPEG_CHK_IF_ERR(-BOF_ERR_ENOMEM, "Could not swr_alloc", Rts_E);
          }
          else
          {
            Sts_i = av_opt_set_int(mpAudDecSwrCtx_X, "in_channel_count", mpAudDecCodecCtx_X->channels, 0);
            FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int in_channel_count", Rts_E);
            if (Rts_E == BOF_ERR_NO_ERROR)
            {
              Sts_i = av_opt_set_int(mpAudDecSwrCtx_X, "out_channel_count", mAudDecOption_X.NbChannel_U32, 0);
              FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int out_channel_count", Rts_E);
              if (Rts_E == BOF_ERR_NO_ERROR)
              {
                Sts_i = av_opt_set_int(mpAudDecSwrCtx_X, "in_channel_layout", mpAudDecCodecCtx_X->channel_layout, 0);
                FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int in_channel_layout", Rts_E);
                if (Rts_E == BOF_ERR_NO_ERROR)
                {
                  Sts_i = av_opt_set_int(mpAudDecSwrCtx_X, "out_channel_layout", mAudDecOption_X.ChannelLayout_U64, 0);
                  FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int out_channel_layout", Rts_E);
                  if (Rts_E == BOF_ERR_NO_ERROR)
                  {
                    Sts_i = av_opt_set_int(mpAudDecSwrCtx_X, "in_sample_rate", mpAudDecCodecCtx_X->sample_rate, 0);
                    FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int in_sample_rate", Rts_E);
                    if (Rts_E == BOF_ERR_NO_ERROR)
                    {
                      Sts_i = av_opt_set_int(mpAudDecSwrCtx_X, "out_sample_rate", mAudDecOption_X.SampleRateInHz_U32, 0);
                      FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_int out_sample_rate", Rts_E);
                      if (Rts_E == BOF_ERR_NO_ERROR)
                      {
                        Sts_i = av_opt_set_sample_fmt(mpAudDecSwrCtx_X, "in_sample_fmt", mpAudDecCodecCtx_X->sample_fmt, 0);
                        FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_sample_fmt in_sample_fmt", Rts_E);
                        if (Rts_E == BOF_ERR_NO_ERROR)
                        {
                          Sts_i = av_opt_set_sample_fmt(mpAudDecSwrCtx_X, "out_sample_fmt", AV_SAMPLE_FMT_S16, 0);
                          FFMPEG_CHK_IF_ERR(Sts_i, "Could not av_opt_set_sample_fmt out_sample_fmt", Rts_E);
                          if (Rts_E == BOF_ERR_NO_ERROR)
                          {
                            Sts_i = swr_init(mpAudDecSwrCtx_X);
                            FFMPEG_CHK_IF_ERR(Sts_i, "Could not swr_init", Rts_E);
                            if (Rts_E == BOF_ERR_NO_ERROR)
                            {
                              mAudDecState_E = BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_READY;
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

        if (Rts_E != BOF_ERR_NO_ERROR)
        {
          Close();
        }
      }
    }
  }
  return Rts_E;
}

BOFERR Bof2dAudioDecoder::Close()
{
  BOFERR Rts_E;
  bool AudioBufferHasBeenFreeed_B = false;

  swr_free(&mpAudDecSwrCtx_X);
  if (mpAudDecFrame_X)
  {
    //If we have used Begin/EndRead method we check that here that the buffer in mpVidDecFrameConverted_X is the same than mpVidDecFrameConverted_X->data[0]. 
    if (mpAudDecFrameConverted_X->data[0] == mAudDecOut_X.InterleavedData_X.pData_U8)
    {
      mAudDecOut_X.InterleavedData_X.Reset();
      AudioBufferHasBeenFreeed_B = true;  //Done by av_freep(&mpAudDecFrame_X->data[0]); just below
    }
    av_freep(&mpAudDecFrame_X->data[0]);
    av_frame_free(&mpAudDecFrame_X);
  }
  if (!AudioBufferHasBeenFreeed_B)
  {
    //If we just open/close decoder without calling read we need to free mAudDecOut_X.InterleavedData_X.pData_U8 here (see remark above)
    av_freep(&mAudDecOut_X.InterleavedData_X.pData_U8);
    mAudDecOut_X.InterleavedData_X.Reset();
  }
  if (mpAudDecFrameConverted_X)
  {
    av_freep(&mpAudDecFrameConverted_X->data[0]);
    av_frame_free(&mpAudDecFrameConverted_X);
  }
  //avcodec_close: Do not use this function. Use avcodec_free_context() to destroy a codec context(either open or closed)
  //avcodec_close(mpAudDecCodecCtx_X);
  avcodec_free_context(&mpAudDecCodecCtx_X);


  for (auto &rIt : mAudDecOut_X.ChannelBufferCollection)
  {
    BOF_SAFE_DELETE_ARRAY(rIt.pData_U8);
  }

  mAudDecState_E = BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_IDLE;
  mAudDecOption_X.Reset();

  mAudDecStreamIndex_i = -1;
  mpAudDecCodecParam_X = nullptr;
  mpAudDecCodec_X = nullptr;
  mpAudDecCodecCtx_X = nullptr;
  mpAudDecFrame_X = nullptr;
  mpAudDecFrameConverted_X = nullptr;
  mAudDecOut_X.Reset();
  mSampleFmt_E = AV_SAMPLE_FMT_NONE;
  mAudDecNbBitPerSample_U32 = 0;
  mpAudDecSwrCtx_X = nullptr;
  mAudDecTimeBase_X = { 0, 0 }; //or av_make_q

  mNbAudDecPacketSent_U64 = 0;
  mNbAudDecFrameReceived_U64 = 0;
  mNbTotalAudDecFrame_U64 = 0;
  mNbTotaAudDecSample_U64 = 0;

  mAudMetadataCollection.clear();
  mpAudStream_X = nullptr;
  mAudDurationInSec_lf = 0.0;

  Rts_E = BOF_ERR_NO_ERROR;
  return Rts_E;
}

BOFERR Bof2dAudioDecoder::BeginRead(AVPacket *_pDecPacket_X, BOF2D_AUD_DEC_OUT &_rAudDecOut_X)
{
  BOFERR Rts_E = BOF_ERR_EOF;
  int Sts_i;
  uint32_t NbAudioSampleConvertedPerChannel_U32, TotalSizeOfAudioConvertedPerChannel_U32, TotalSizeOfAudioConverted_U32, i_U32, j_U32, Index_U32;

  _rAudDecOut_X.Reset();
  if (mAudDecState_E != BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_IDLE)
  {
    if (mAudDecState_E == BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_BUSY)
    {
      Rts_E = BOF_ERR_EBUSY;
    }
    else
    {
      mAudDecState_E = BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_BUSY;
      mAudDecOut_X.InterleavedData_X.Size_U64 = 0;
      for (auto &rIt : mAudDecOut_X.ChannelBufferCollection)
      {
        rIt.Size_U64 = 0;
      }

      Rts_E = BOF_ERR_INDEX;
      if ((_pDecPacket_X) && (_pDecPacket_X->stream_index == mAudDecStreamIndex_i))
      {
        Sts_i = avcodec_send_packet(mpAudDecCodecCtx_X, _pDecPacket_X);
        FFMPEG_CHK_IF_ERR(Sts_i, "Cannot avcodec_send_packet Audio", Rts_E);
        if (Rts_E == BOF_ERR_NO_ERROR)
        {
          mNbAudDecPacketSent_U64++;
          Sts_i = avcodec_receive_frame(mpAudDecCodecCtx_X, mpAudDecFrame_X);
          FFMPEG_CHK_IF_ERR(Sts_i, "Cannot avcodec_receive_frame Audio", Rts_E);
          if (Rts_E == BOF_ERR_NO_ERROR)
          {
            mNbAudDecFrameReceived_U64++;
            printf("AudioFrame Snt/Rcv %zd/%zd Pts %zd Fmt %s NbSmp %d Rate %d Buf %x:%p Layout %zx\n", mNbAudDecPacketSent_U64, mNbAudDecFrameReceived_U64, mpAudDecFrame_X->pts, av_get_sample_fmt_name((AVSampleFormat)mpAudDecFrame_X->format),
              mpAudDecFrame_X->nb_samples, mpAudDecFrame_X->sample_rate, mpAudDecFrame_X->linesize[0], mpAudDecFrame_X->data[0],
              mpAudDecFrame_X->channel_layout);

            //Rts_E = ConvertAudio(false, mpAudDecFrame_X, mpAudDecFrameConverted_X, mAudDecOption_X.NbChannel_U32, static_cast<uint32_t>(mAudDecOption_X.ChannelLayout_U64), mAudDecOption_X.SampleRateInHz_U32, AV_SAMPLE_FMT_S16);
            Rts_E = ConvertAudio(NbAudioSampleConvertedPerChannel_U32);
            if (Rts_E == BOF_ERR_NO_ERROR)
            {
              TotalSizeOfAudioConvertedPerChannel_U32 = (NbAudioSampleConvertedPerChannel_U32 * (mAudDecOption_X.DemuxChannel_B ? mAudDecOption_X.NbBitPerSample_U32 : mAudDecNbBitPerSample_U32) / 8);
              TotalSizeOfAudioConverted_U32 = TotalSizeOfAudioConvertedPerChannel_U32 * mAudDecOption_X.NbChannel_U32;
              mAudDecOut_X.InterleavedData_X.Size_U64 = TotalSizeOfAudioConverted_U32;
              for (auto &rIt : mAudDecOut_X.ChannelBufferCollection)
              {
                rIt.Size_U64 = TotalSizeOfAudioConvertedPerChannel_U32;
              }
              //_rAudDecOut_X.InterleavedData_X.SetStorage(mpAudDecFrameConverted_X->linesize[0], TotalSizeOfAudioConverted_U32, mpAudDecFrameConverted_X->data[0]);
              _rAudDecOut_X.InterleavedData_X = mAudDecOut_X.InterleavedData_X;
              _rAudDecOut_X.ChannelBufferCollection = mAudDecOut_X.ChannelBufferCollection;
              _rAudDecOut_X.NbSample_U32 = mpAudDecFrameConverted_X->nb_samples;
              _rAudDecOut_X.NbChannel_U32 = mpAudDecFrameConverted_X->channels;
              _rAudDecOut_X.ChannelLayout_U64 = mpAudDecFrameConverted_X->channel_layout;
              _rAudDecOut_X.SampleRateInHz_U32 = mpAudDecFrameConverted_X->sample_rate;
              _rAudDecOut_X.NbBitPerSample_U32 = mAudDecOption_X.DemuxChannel_B ? mAudDecOption_X.NbBitPerSample_U32:mAudDecNbBitPerSample_U32;

              if (mAudDecOption_X.DemuxChannel_B)
              {
                if (mAudDecOption_X.NbChannel_U32 == 1)
                {
                  mAudDecOut_X.ChannelBufferCollection[0].MemCpy(TotalSizeOfAudioConverted_U32, mAudDecOut_X.InterleavedData_X.pData_U8);
                }
                else
                {
                  switch (mAudDecOption_X.NbBitPerSample_U32)
                  {
                    case 8:
                      for (i_U32 = 0; i_U32 < mAudDecOption_X.NbChannel_U32; i_U32++)
                      {
                        for (Index_U32 = i_U32, j_U32 = 0; j_U32 < NbAudioSampleConvertedPerChannel_U32; j_U32++, Index_U32 += mAudDecOption_X.NbChannel_U32)
                        {
                          ((uint8_t *)mAudDecOut_X.ChannelBufferCollection[i_U32].pData_U8)[j_U32] = ((uint8_t *)mAudDecOut_X.InterleavedData_X.pData_U8)[Index_U32];
                        }
                      }
                      break;

                    case 16:
                      for (i_U32 = 0; i_U32 < mAudDecOption_X.NbChannel_U32; i_U32++)
                      {
                        for (Index_U32 = i_U32, j_U32 = 0; j_U32 < NbAudioSampleConvertedPerChannel_U32; j_U32++, Index_U32 += mAudDecOption_X.NbChannel_U32)
                        {
                          ((uint16_t *)mAudDecOut_X.ChannelBufferCollection[i_U32].pData_U8)[j_U32] = ((uint16_t *)mAudDecOut_X.InterleavedData_X.pData_U8)[Index_U32];
                        }
                      }
                      break;

                    case 24:
#if 0 //FFMPEG put 24 bits audio sample in S32 var
                      for (i_U32 = 0; i_U32 < mAudDecOption_X.NbChannel_U32; i_U32++)
                      {
                        for (Index_U32 = (i_U32 * 3), j_U32 = 0; j_U32 < (NbAudioSampleConvertedPerChannel_U32 * 3); j_U32++, Index_U32 += (mAudDecOption_X.NbChannel_U32 * 3))
                        {
                          ((uint8_t *)mAudDecOut_X.ChannelBufferCollection[i_U32].pData_U8)[j_U32++] = ((uint8_t *)mAudDecOut_X.InterleavedData_X.pData_U8)[Index_U32 + 0];
                          ((uint8_t *)mAudDecOut_X.ChannelBufferCollection[i_U32].pData_U8)[j_U32++] = ((uint8_t *)mAudDecOut_X.InterleavedData_X.pData_U8)[Index_U32 + 1];
                          ((uint8_t *)mAudDecOut_X.ChannelBufferCollection[i_U32].pData_U8)[j_U32++] = ((uint8_t *)mAudDecOut_X.InterleavedData_X.pData_U8)[Index_U32 + 2];
                        }
                      }
#else
                      for (i_U32 = 0; i_U32 < mAudDecOption_X.NbChannel_U32; i_U32++)
                      {
                        for (Index_U32 = (i_U32 * 3), j_U32 = 0; j_U32 < (NbAudioSampleConvertedPerChannel_U32 * 3); Index_U32 += (mAudDecOption_X.NbChannel_U32 * 4))
                        {
                          ((uint8_t *)mAudDecOut_X.ChannelBufferCollection[i_U32].pData_U8)[j_U32++] = ((uint8_t *)mAudDecOut_X.InterleavedData_X.pData_U8)[Index_U32 + 0];
                          ((uint8_t *)mAudDecOut_X.ChannelBufferCollection[i_U32].pData_U8)[j_U32++] = ((uint8_t *)mAudDecOut_X.InterleavedData_X.pData_U8)[Index_U32 + 1];
                          ((uint8_t *)mAudDecOut_X.ChannelBufferCollection[i_U32].pData_U8)[j_U32++] = ((uint8_t *)mAudDecOut_X.InterleavedData_X.pData_U8)[Index_U32 + 2];
                        }
                      }

#endif
                      break;

                    case 32:
                      for (i_U32 = 0; i_U32 < mAudDecOption_X.NbChannel_U32; i_U32++)
                      {
                        for (Index_U32 = i_U32, j_U32 = 0; j_U32 < NbAudioSampleConvertedPerChannel_U32; j_U32++, Index_U32 += mAudDecOption_X.NbChannel_U32)
                        {
                          ((uint32_t *)mAudDecOut_X.ChannelBufferCollection[i_U32].pData_U8)[j_U32] = ((uint32_t *)mAudDecOut_X.InterleavedData_X.pData_U8)[Index_U32];
                        }
                      }
                      break;

                    default:
                      Rts_E = BOF_ERR_NOT_SUPPORTED;
                      break;
                  }
                }
              }
            }
            av_packet_unref(_pDecPacket_X);
            //break;  //leave while so av_packet_unref(&mAudDecPacket_X);
          }
        }
      }
          

      if (Rts_E == BOF_ERR_EAGAIN)
      {
        mAudDecState_E = BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_PENDING;
        Rts_E = BOF_ERR_NO_ERROR;
      }
    }
  }
  return Rts_E;
}

BOFERR Bof2dAudioDecoder::EndRead()
{
  BOFERR Rts_E = BOF_ERR_EOF;

  if (mAudDecState_E != BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_IDLE)
  {
    if (mAudDecState_E == BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_BUSY)
    {
      av_frame_unref(mpAudDecFrame_X);
      mAudDecState_E = BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_READY;
      Rts_E = BOF_ERR_NO_ERROR;
    }
    else
    {
//      Rts_E = (mAudDecState_E == BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_PENDING) ? BOF_ERR_NO_ERROR: BOF_ERR_INVALID_STATE;
      Rts_E = BOF_ERR_NO_ERROR;
    }
  }
  return Rts_E;
}

BOFERR Bof2dAudioDecoder::ConvertAudio(uint32_t &_rNbAudioSampleConvertedPerChannel_U32)
{
  BOFERR Rts_E;
  uint32_t NbAudioSample_U32, AudioBufferSize_U32, ChannelSizeOfAudioConverted_U32, TotalSizeOfAudioConverted_U32;
  int64_t AudioDelay_S64;

  _rNbAudioSampleConvertedPerChannel_U32 = 0;
  AudioDelay_S64 = swr_get_delay(mpAudDecSwrCtx_X, mpAudDecCodecCtx_X->sample_rate);
  NbAudioSample_U32 = (int)av_rescale_rnd(AudioDelay_S64 + mpAudDecFrame_X->nb_samples, mAudDecOption_X.SampleRateInHz_U32, mpAudDecCodecCtx_X->sample_rate, AV_ROUND_UP);
  AudioBufferSize_U32 = av_samples_get_buffer_size(nullptr, mAudDecOption_X.NbChannel_U32, NbAudioSample_U32, mSampleFmt_E, 0);

  Rts_E = BOF_ERR_TOO_BIG;
  BOF_ASSERT(AudioBufferSize_U32 <= mAudDecOut_X.InterleavedData_X.Capacity_U64);
  if (AudioBufferSize_U32 <= mAudDecOut_X.InterleavedData_X.Capacity_U64)
  {
    Rts_E = BOF_ERR_NO_ERROR;
    av_samples_fill_arrays(mpAudDecFrameConverted_X->data, mpAudDecFrameConverted_X->linesize, mAudDecOut_X.InterleavedData_X.pData_U8, mAudDecOption_X.NbChannel_U32, NbAudioSample_U32, mSampleFmt_E, mAudDecAllocAlignment_i);
    _rNbAudioSampleConvertedPerChannel_U32 = swr_convert(mpAudDecSwrCtx_X, &mAudDecOut_X.InterleavedData_X.pData_U8, NbAudioSample_U32, (const uint8_t **)mpAudDecFrame_X->data, mpAudDecFrame_X->nb_samples);
    ChannelSizeOfAudioConverted_U32 = (_rNbAudioSampleConvertedPerChannel_U32 * mAudDecNbBitPerSample_U32) / 8;
    TotalSizeOfAudioConverted_U32 = (ChannelSizeOfAudioConverted_U32 * mAudDecOption_X.NbChannel_U32);
    //printf("---->NbAudioSample %d memset %d NbAudioConvertSamplePerChannel %d->%d linesize %d\n", NbAudioSample_U32, AudioBufferSize_U32, NbAudioSampleConvertedPerChannel_U32, _rTotalSizeOfAudioConverted_U32, mpAudDecFrameConverted_X->linesize[0]);
    BOF_ASSERT(TotalSizeOfAudioConverted_U32 <= static_cast<uint32_t>(mpAudDecFrameConverted_X->linesize[0]));
    if (TotalSizeOfAudioConverted_U32 <= static_cast<uint32_t>(mpAudDecFrameConverted_X->linesize[0]))
    {
      mpAudDecFrameConverted_X->nb_samples = _rNbAudioSampleConvertedPerChannel_U32;
      mpAudDecFrameConverted_X->channels = mAudDecOption_X.NbChannel_U32;
      mpAudDecFrameConverted_X->channel_layout = static_cast<uint32_t>(mAudDecOption_X.ChannelLayout_U64);
      mpAudDecFrameConverted_X->format = mSampleFmt_E;
      mpAudDecFrameConverted_X->sample_rate = mAudDecOption_X.SampleRateInHz_U32;
      mpAudDecFrameConverted_X->pkt_pos = mpAudDecFrame_X->pkt_pos;
      mpAudDecFrameConverted_X->pkt_duration = mpAudDecFrame_X->pkt_duration;
      mpAudDecFrameConverted_X->pkt_size = mpAudDecFrame_X->pkt_size;
      mNbTotalAudDecFrame_U64++;
      mNbTotaAudDecSample_U64 += TotalSizeOfAudioConverted_U32;
      int16_t *pData_S16 = (int16_t *)mAudDecOut_X.InterleavedData_X.pData_U8;
      printf("Cnv Audio %x->%x:%p nbs %d ch %d layout %zx Fmt %d Rate %d Pos %zd Dur %zd Sz %d\n", mpAudDecFrameConverted_X->linesize[0], TotalSizeOfAudioConverted_U32, mAudDecOut_X.InterleavedData_X.pData_U8,
        mpAudDecFrameConverted_X->nb_samples, mpAudDecFrameConverted_X->channels, mpAudDecFrameConverted_X->channel_layout, mpAudDecFrameConverted_X->format, mpAudDecFrameConverted_X->sample_rate,
        mpAudDecFrameConverted_X->pkt_pos, mpAudDecFrameConverted_X->pkt_duration, mpAudDecFrameConverted_X->pkt_size);
      printf("Cnv Data %04x %04x %04x %04x %04x %04x %04x %04x\n", pData_S16[0], pData_S16[1], pData_S16[2], pData_S16[3], pData_S16[4], pData_S16[5], pData_S16[6], pData_S16[7]);
    }
    else
    {
      Rts_E = BOF_ERR_EOVERFLOW;
    }
  }

  return Rts_E;
}

bool Bof2dAudioDecoder::IsAudioStreamPresent()
{
  return(mAudDecStreamIndex_i != -1);
}
END_BOF2D_NAMESPACE()