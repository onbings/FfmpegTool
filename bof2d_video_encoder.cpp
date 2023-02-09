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
#include "bof2d_video_encoder.h"

#include <bofstd/bofstring.h>
#include <bofstd/bofstringformatter.h>
#include <bofstd/boffs.h>

#include <zlib.h>
uint8_t *ZlibCompressor(unsigned char *data, int _DataToCompressSizeInByte_i, int *out_len, int quality);
#define STBIW_ZLIB_COMPRESS ZlibCompressor
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
/*
   You can #define STBIW_ZLIB_COMPRESS to use a custom zlib-style compress function
   for PNG compression (instead of the builtin one), it must have the following signature:
   unsigned char * my_compress(unsigned char *data, int _DataToCompressSizeInByte_i, int *out_len, int quality);
   The returned data will be freed with STBIW_FREE() (free() by default),
   so it must be heap allocated with STBIW_MALLOC() (malloc() by default),

*/
//const long int CHUNK = { 16384 };
uint8_t *ZlibCompressor(uint8_t *_pDataToCompress_U8, int _DataToCompressSizeInByte_i, int *_pDataCompressedSizeInByte_i, int _Quality_i)
{
  uint8_t *pRts_U8 = nullptr;
  unsigned long OutLen_U32 = 0;
  int Sts_i;

  if ((_pDataToCompress_U8) && (_pDataCompressedSizeInByte_i))
  {
    pRts_U8 = (uint8_t *)STBIW_MALLOC(_DataToCompressSizeInByte_i);
    OutLen_U32 = _DataToCompressSizeInByte_i;
    Sts_i = compress2(pRts_U8, &OutLen_U32, _pDataToCompress_U8, _DataToCompressSizeInByte_i, _Quality_i);
    if (Sts_i)
    {
      STBIW_FREE(pRts_U8);
      pRts_U8 = nullptr;
      OutLen_U32 = 0;
    }
  }
  *_pDataCompressedSizeInByte_i = static_cast<int>(OutLen_U32);

  return pRts_U8;
}

BEGIN_BOF2D_NAMESPACE()


#if defined(BHA_BMP)
#pragma pack(1)
//https://en.wikipedia.org/wiki/BMP_file_format
struct BOF2D_BMP_HEADER               //Alias of BOF2D_BMP_HEADER
{
  /*000*/ uint16_t Type_U16;          //The header field used to identify the BMP and DIB file is 0x42 0x4D in hexadecimal, same as BM in ASCII.
  /*002*/ uint32_t BmpSize_U32;       //The size of the BMP file in bytes
  /*006*/ uint16_t Reserved1_U16;     //Reserved; actual value depends on the application that creates the image, if created manually can be 0
  /*008*/ uint16_t Reserved2_U16;     //Reserved; actual value depends on the application that creates the image, if created manually can be 0
  /*010*/ uint32_t DataOffset_U32;    //The offset, i.e. starting address, of the byte where the bitmap image data (pixel array) can be found.

  //Followed by Alias of BOF2D_BMP_INFO_HEADER (40 bytes)

  /*014*/ uint32_t BibSize_U32;       //Number of bytes in the DIB header (from this point)
  /*018*/ int32_t  Width_S32;         //Width of the bitmap in pixels
  /*022*/ int32_t  Height_S32;        //Height of the bitmap in pixels.Positive for bottom to top pixel order.
  /*026*/ uint16_t NbColPlane_U16;    //Number of color planes being used
  /*028*/ uint16_t Bpp_U16;           //Number of bits per pixel
  /*030*/ uint32_t CompType_U32;      //BI_RGB (0), no pixel array compression used
  /*034*/ uint32_t SizeImage_U32;     //Size of the raw bitmap data (including padding)
  /*038*/ int32_t  XPelsPerMeter_S32; //Print resolution of the image,  72 DPI × 39.3701 inches per metre yields 2834.6472
  /*042*/ int32_t  YPelsPerMeter_S32; //Print resolution of the image,  72 DPI × 39.3701 inches per metre yields 2834.6472
  /*046*/ uint32_t NbPalCol_U32;      //Number of colors in the palette
  /*050*/ uint32_t ColImportant_U32;  //0 means all colors are important
};
#pragma pack()
#endif

Bof2dVideoEncoder::Bof2dVideoEncoder()
{
  mVidEncOptionParam_X.push_back({ nullptr, "V_BASEFN", "if defined, video buffer will be saved in this file","","", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_VARIABLE(mVidEncOption_X.BasePath, PATH, 0, 0) });
  mVidEncOptionParam_X.push_back({ nullptr, "V_FMT", "Specifies the video format", "", "", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_ENUM(mVidEncOption_X.Format_E, BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_PNG, BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_MAX, S_Bof2dAvVideoFormatEnumConverter, BOF2D_AV_VIDEO_FORMAT) });
  mVidEncOptionParam_X.push_back({ nullptr, "V_QUALITY", "Specifies the encoding quality (depend on format)","","", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_VARIABLE(mVidEncOption_X.EncQuality_S32, INT32, 0, 0) });

  //  bool a= InitFFmpeg("png");
  //  size_t b=receiveAndWrite();
  //  uint8_t p[0x1000];
  //  encode(p);
}

Bof2dVideoEncoder::~Bof2dVideoEncoder()
{
  Close();
}

BOFERR Bof2dVideoEncoder::Open(const std::string &_rOption_S)
{
  BOFERR    Rts_E = BOF_ERR_ECANCELED;
  BOF::BofCommandLineParser OptionParser;
  BOF2D_VID_ENC_OUT VidEncOut_X;

  if (mVidEncState_E == BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_IDLE)
  {
    Close();

    mVidEncOption_X.Reset();
    Rts_E = OptionParser.ToByte(_rOption_S, mVidEncOptionParam_X, nullptr, nullptr);
    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      Rts_E = BOF_ERR_INVALID_DST;
      BOF::Bof_CreateDirectory(BOF::BOF_FILE_PERMISSION_ALL_FOR_OWNER | BOF::BOF_FILE_PERMISSION_READ_FOR_ALL, mVidEncOption_X.BasePath.DirectoryName(true, false));
      if (mVidEncOption_X.BasePath.IsValid())
      {
        if (mVidEncOption_X.Format_E == BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_MAX)
        {
          mVidEncOption_X.Format_E = BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_PNG;
          mVidEncOption_X.EncQuality_S32 = 9;
        }
#if defined(BHA_BMP)
        mIoCollection.clear();
        mIoCollection.push_back(VidEncOut_X);  //Entry 0 is for interleaved sample global file
#endif
        //Rts_E = CreateFileOut();
        Rts_E = BOF_ERR_NO_ERROR;
        if (Rts_E == BOF_ERR_NO_ERROR)
        {
          mVidEncState_E = BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_READY;
        }
      }

      if (Rts_E != BOF_ERR_NO_ERROR)
      {
        Close();
      }
    }
  }
  return Rts_E;
}


BOFERR Bof2dVideoEncoder::Close()
{
  BOFERR Rts_E;

  //CloseFileOut();

  mVidEncState_E = BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_IDLE;
  mVidEncOption_X.Reset();
#if defined(BHA_BMP)
  mIoCollection.clear();
#endif
  mVidDecOut_X.Reset();

  mNbTotalVidEncFrame_U64 = 0;
  mImagePath_S = "";

  Rts_E = BOF_ERR_NO_ERROR;
  return Rts_E;
}


BOFERR Bof2dVideoEncoder::BeginWrite(BOF2D_VID_DEC_OUT &_rVidDecOut_X)
{
  BOFERR Rts_E = BOF_ERR_ECANCELED;

  if (mVidEncState_E != BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_IDLE)
  {
    if (mVidEncState_E == BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_BUSY)
    {
      Rts_E = BOF_ERR_EBUSY;
    }
    else
    {
      mVidEncState_E = BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_BUSY;
      mVidDecOut_X = _rVidDecOut_X;
      Rts_E = CreateFileOut();
      if (Rts_E == BOF_ERR_NO_ERROR)
      {
        Rts_E = WriteChunkOut();
        if (Rts_E == BOF_ERR_NO_ERROR)
        {
          Rts_E = CloseFileOut();
          if (Rts_E == BOF_ERR_NO_ERROR)
          {
            mNbTotalVidEncFrame_U64++;
          }
        }
      }
    }
  }

  return Rts_E;
}

BOFERR Bof2dVideoEncoder::EndWrite()
{
  BOFERR Rts_E = BOF_ERR_ECANCELED;

  if (mVidEncState_E != BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_IDLE)
  {
    Rts_E = BOF_ERR_EOF;
    if (mVidEncState_E == BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_BUSY)
    {
      mVidEncState_E = BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_READY;
      Rts_E = BOF_ERR_NO_ERROR;
    }
    else
    {
      //      Rts_E = (mVidEncState_E == BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_PENDING) ? BOF_ERR_NO_ERROR : BOF_ERR_INVALID_STATE;
      Rts_E = BOF_ERR_NO_ERROR;
    }
  }
  return Rts_E;
}

BOFERR Bof2dVideoEncoder::CreateFileOut()
{
  BOFERR Rts_E;
  std::string Extension_S = S_Bof2dAvVideoFormatEnumConverter.ToString(mVidEncOption_X.Format_E);

  mImagePath_S = BOF::Bof_Sprintf("%s_%06zd.%s", mVidEncOption_X.BasePath.FullPathNameWithoutExtension(false).c_str(), mNbTotalVidEncFrame_U64, Extension_S.c_str());
  switch (mVidEncOption_X.Format_E)
  {
    case BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_BMP:
#if defined(BHA_BMP)
      Rts_E = BOF::Bof_CreateFile(BOF::BOF_FILE_PERMISSION_ALL_FOR_OWNER | BOF::BOF_FILE_PERMISSION_READ_FOR_ALL, mImagePath_S, false, mIoCollection[0].Io);
#else
      Rts_E = BOF_ERR_NO_ERROR;
#endif
      break;

    case BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_PNG:
      Rts_E = BOF_ERR_NO_ERROR;
      //int stbi_write_tga_with_rle;             // defaults to true; set to 0 to disable RLE
/*
STBIW_ZLIB_COMPRESS: Use Zlib:
   The compression level must be Z_DEFAULT_COMPRESSION, or between 0 and 9:
   1 gives best speed, 9 gives best compression, 0 gives no compression at all
   (the input data is simply copied a block at a time).  Z_DEFAULT_COMPRESSION
   requests a default compromise between speed and compression (currently
   equivalent to level 6).

*/
      stbi_write_png_compression_level = mVidEncOption_X.EncQuality_S32;    //if not zlib: defaults to 8; set to higher for more compression
      //https://optipng.sourceforge.net/pngtech/png_optimization.html#:~:text=The%20PNG%20format%20employs%20five,and%2For%20the%20upper%20left.
      stbi_write_force_png_filter = -1;
      break;

    case BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_TGA:
      Rts_E = BOF_ERR_NO_ERROR;
      stbi_write_tga_with_rle = (mVidEncOption_X.EncQuality_S32 !=0) ? true:false;             // defaults to true; set to 0 to disable RLE
      break;

    case BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_JPG:
      Rts_E = BOF_ERR_NO_ERROR;
      break;

    default:
      Rts_E = BOF_ERR_FORMAT;
      break;
  }
  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    Rts_E = WriteHeader();
  }
  return Rts_E;
}

BOFERR Bof2dVideoEncoder::WriteHeader()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
#if defined(BHA_BMP)
  BOF2D_BMP_HEADER BmpHeader_X;
  uint32_t i_U32, Nb_U32;
  int64_t Pos_S64;
#endif

  switch (mVidEncOption_X.Format_E)
  {
    case BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_BMP:
#if defined(BHA_BMP)
      for (i_U32 = 0; i_U32 < mIoCollection.size(); i_U32++)  //Entry 0 is for interleaved sample global file
      {
        if (mIoCollection[i_U32].Io != BOF::BOF_FS_INVALID_HANDLE)
        {
          Pos_S64 = BOF::Bof_GetFileIoPosition(mIoCollection[i_U32].Io);
          if (Pos_S64 != -1)
          {
            // RGB image
            BOF::Bof_SetFileIoPosition(mIoCollection[i_U32].Io, 0, BOF::BOF_SEEK_METHOD::BOF_SEEK_BEGIN);
            memcpy(&BmpHeader_X.Type_U16, "BM", 2);
            BmpHeader_X.BmpSize_U32 = sizeof(BOF2D_BMP_HEADER) + (mVidDecOut_X.LineSize_S32 * mVidDecOut_X.Size_X.Height_U32);
            BmpHeader_X.DataOffset_U32 = sizeof(BOF2D_BMP_HEADER);
            BmpHeader_X.Reserved1_U16 = 0;
            BmpHeader_X.Reserved2_U16 = 0;

            //BOF2D_BMP_INFO_HEADER &bmiHeader = *((BOF2D_BMP_INFO_HEADER *)(pData + headersSize - sizeof(BOF2D_BMP_INFO_HEADER)));

            BmpHeader_X.Bpp_U16 = (mVidDecOut_X.LineSize_S32 / mVidDecOut_X.Size_X.Width_U32) * 8;
            BmpHeader_X.Width_S32 = mVidDecOut_X.Size_X.Width_U32;
            BmpHeader_X.Height_S32 = mVidDecOut_X.Size_X.Height_U32;
            BmpHeader_X.NbColPlane_U16 = 1;
            BmpHeader_X.BibSize_U32 = 40; // sizeof(bmiHeader);
            BmpHeader_X.CompType_U32 = 0;  // BI_RGB;
            BmpHeader_X.ColImportant_U32 = 0;
            BmpHeader_X.NbPalCol_U32 = 0;
            BmpHeader_X.SizeImage_U32 = 0;
            BmpHeader_X.XPelsPerMeter_S32 = 0;
            BmpHeader_X.YPelsPerMeter_S32 = 0;
            Nb_U32 = sizeof(BOF2D_BMP_HEADER);
            Rts_E = BOF::Bof_WriteFile(mIoCollection[i_U32].Io, Nb_U32, reinterpret_cast<uint8_t *>(&BmpHeader_X));
            //NO    BOF::Bof_SetFileIoPosition(mIoCollection[0].Io, Pos_S64, BOF::BOF_SEEK_METHOD::BOF_SEEK_BEGIN);
          }
          else
          {
            Rts_E = BOF_ERR_INVALID_HANDLE;
          }
        }
      }
#else
      Rts_E = BOF_ERR_NO_ERROR;
#endif
      break;

    case BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_PNG:
    case BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_TGA:
    case BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_JPG:
      Rts_E = BOF_ERR_NO_ERROR;
      break;

    default:
      Rts_E = BOF_ERR_FORMAT;
      break;
  }
  return Rts_E;
}

BOFERR Bof2dVideoEncoder::WriteChunkOut()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
#if defined(BHA_BMP)
  uint32_t Nb_U32, i_U32;
  int32_t Span_S32;
  uint8_t *pData_U8;
#endif

  if ((mVidDecOut_X.Data_X.pData_U8) && (mVidDecOut_X.Data_X.Size_U64))
  {
    switch (mVidEncOption_X.Format_E)
  {
    case BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_BMP:
#if defined(BHA_BMP)
      pData_U8 = mVidDecOut_X.Data_X.pData_U8 + ((mVidDecOut_X.LineSize_S32 * mVidDecOut_X.Size_X.Height_U32) - mVidDecOut_X.LineSize_S32);
      Span_S32 = -mVidDecOut_X.LineSize_S32;

      for (i_U32 = 0; i_U32 < mVidDecOut_X.Size_X.Height_U32; i_U32++, pData_U8 += Span_S32)
      {
        Nb_U32 = mVidDecOut_X.LineSize_S32;
        Rts_E = BOF::Bof_WriteFile(mIoCollection[0].Io, Nb_U32, pData_U8);
        if (Rts_E == BOF_ERR_NO_ERROR)
        {
          mIoCollection[0].Size_U64 += Nb_U32;
        }
      }
#else
      Rts_E = (stbi_write_bmp(mImagePath_S.c_str(), mVidDecOut_X.Size_X.Width_U32, mVidDecOut_X.Size_X.Height_U32, mVidDecOut_X.NbChannel_U32, mVidDecOut_X.Data_X.pData_U8) == 0) ? BOF_ERR_INTERNAL : BOF_ERR_NO_ERROR;
#endif
      break;

    case BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_PNG:
      /*
      Most of the parameters are the same ones that are also passed to stbi_write_png which has a documentation in the header file:

      pixels is the uncompressed image data
      stride_bytes is the number of bytes between two the start of two consecutive rows:
      "stride_in_bytes" is the distance in bytes from the first byte of a row of pixels to the first byte of the next row of pixels.

      x (called w in stbi_write_png) is the width of the image
      y (called h in stbi_write_png) is the height of the image
      n (called comp in stbi_write_png) is the number of channels:
      Each pixel contains 'comp' channels of data stored interleaved with 8-bits per channel, in the following order: 1=Y, 2=YA, 3=RGB, 4=RGBA.

      out_len is a output parameter which returns the number of bytes the compressed data has.
      */
      Rts_E = (stbi_write_png(mImagePath_S.c_str(), mVidDecOut_X.Size_X.Width_U32, mVidDecOut_X.Size_X.Height_U32, mVidDecOut_X.NbChannel_U32, mVidDecOut_X.Data_X.pData_U8, mVidDecOut_X.LineSize_S32) == 0) ? BOF_ERR_INTERNAL : BOF_ERR_NO_ERROR;
      break;

    case BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_TGA:
      Rts_E = (stbi_write_tga(mImagePath_S.c_str(), mVidDecOut_X.Size_X.Width_U32, mVidDecOut_X.Size_X.Height_U32, mVidDecOut_X.NbChannel_U32, mVidDecOut_X.Data_X.pData_U8) == 0) ? BOF_ERR_INTERNAL : BOF_ERR_NO_ERROR;
      break;

    case BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_JPG:
/*
The last parameter of the stbi_write_jpg function is a quality parameter that goes from 1 to 100. 
Since JPG is a lossy image format, you can chose how much data is dropped at save time. Lower quality 
means smaller image size on disk and lower visual image quality. 
Most image editors, like GIMP, will save jpg images with a default quality parameter of 80 or 90, 
but the user can tune the quality parameter if required. I’ve used a quality parameter of 100 in all 
examples from this article.
*/
      Rts_E = (stbi_write_jpg(mImagePath_S.c_str(), mVidDecOut_X.Size_X.Width_U32, mVidDecOut_X.Size_X.Height_U32, mVidDecOut_X.NbChannel_U32, mVidDecOut_X.Data_X.pData_U8, mVidEncOption_X.EncQuality_S32) == 0) ? BOF_ERR_INTERNAL : BOF_ERR_NO_ERROR;
      break;

    default:
      Rts_E = BOF_ERR_FORMAT;
      break;
  }
  }
  return Rts_E;
}

BOFERR Bof2dVideoEncoder::CloseFileOut()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;

#if defined(BHA_BMP)
  uint32_t i_U32;
  //Rts_E = WriteHeader();
  for (i_U32 = 0; i_U32 < mIoCollection.size(); i_U32++)  //Entry 0 is for interleaved sample global file
  {
    if (mIoCollection[i_U32].Io != BOF::BOF_FS_INVALID_HANDLE)
    {
      BOF::Bof_CloseFile(mIoCollection[i_U32].Io);
      mIoCollection[i_U32].Reset();
    }
  }
#endif
  return Rts_E;
}

bool Bof2dVideoEncoder::IsVideoStreamPresent()
{
  return mVidEncState_E != BOF2D_AV_CODEC_STATE::BOF2D_AV_CODEC_STATE_IDLE;
}


#if defined(BHA_FFMPEG_ENC)
bool Bof2dVideoEncoder::InitFFmpeg(std::string codec)
{
  bool Rts_B = false;


  width_ = 200;
  height_ = 100;
  std::string filename("c:\\tmp\\out.png");



  frame_ = av_frame_alloc();
  if (frame_)
  {
    codecName_ = codec;
    //avcodec_register_all();


    AVInputFormat *iformat = NULL;
    iformat = av_find_input_format(filename.c_str());
    AVFormatContext *ps = NULL;
    int e = avformat_open_input(&ps, filename.c_str(), NULL, NULL);
    //codec_ = avcodec_find_encoder_by_name(codecName_.c_str());
    codec_ = avcodec_find_encoder(AV_CODEC_ID_PNG);
    //codec_ = avcodec_find_encoder(AV_CODEC_ID_MJPEG);
    if (codec_)
    {
      codecCtx_ = avcodec_alloc_context3(codec_);
      if (codecCtx_)
      {
        codecCtx_->bit_rate = 20 * 1000 * 1000;
        codecCtx_->gop_size = 10;
        codecCtx_->max_b_frames = 1;

        codecCtx_->width = width_; /* resolution must be a multiple of two */
        codecCtx_->height = height_;
        codecCtx_->time_base = { 90000,1 }; // videoParams_.timeBase_;
        codecCtx_->framerate = { 1,1 }; // videoParams_.frameRate_;
        codecCtx_->pix_fmt = outputVideoPixFmt_; // videoParams_.outputVideoPixFmt_;
        codecCtx_->flags = 0;
        codecCtx_->color_primaries = AVCOL_PRI_BT709; // videoParams_.colorPrimaries_;
        codecCtx_->color_range = AVCOL_RANGE_JPEG;  // videoParams_.colorRange_;
        codecCtx_->color_trc = AVCOL_TRC_BT709; // videoParams_.colorTrc_;
        codecCtx_->colorspace = AVCOL_SPC_RGB; //AVCOL_SPC_BT709  videoParams_.colorSpace_;

        if (0)  //(videoParams_.interlaced_)
        {
          codecCtx_->flags = AV_CODEC_FLAG_INTERLACED_DCT | AV_CODEC_FLAG_INTERLACED_ME;
        }
        /*
        if (codec_->id == AV_CODEC_ID_H264)
        {
          if (av_opt_set(codecCtx_->priv_data, "preset", "ultrafast", 0) != 0)
          {
            LOG_WARNING(logger_, "Could not set preset to ultrafast");
          }
        }
        */
        int ret = avcodec_open2(codecCtx_, codec_, NULL);
        if (ret >= 0)
        {
          // Output file
          avformat_alloc_output_context2(&oc_, NULL, NULL, filename.c_str());
          if (oc_)
          {

            auto fmt = oc_->oformat;

            /* Add the audio and video streams using the default format codecs
             * and initialize the codecs. */
            if (fmt->video_codec != AV_CODEC_ID_NONE)
            {

              stream_ = avformat_new_stream(oc_, NULL);
              if (stream_)
              {

                stream_->id = (int)oc_->nb_streams - 1;
                stream_->time_base = codecCtx_->time_base;
                stream_->avg_frame_rate = codecCtx_->framerate;

                /* Some formats want stream headers to be separate. */
                if (oc_->oformat->flags & AVFMT_GLOBALHEADER)
                  codecCtx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

                /* copy the stream parameters to the muxer */
                ret = avcodec_parameters_from_context(stream_->codecpar, codecCtx_);
                if (ret >= 0)
                {

                  av_dump_format(oc_, 0, filename.c_str(), true);

                  /* open the output file, if needed */
                  if (!(fmt->flags & AVFMT_NOFILE))
                  {
                    ret = avio_open(&oc_->pb, filename.c_str(), AVIO_FLAG_WRITE);
                    if (ret >= 0)
                    {

                      /* Write the stream header, if any. */
                      ret = avformat_write_header(oc_, nullptr /*AVDictionary*/);
                      if (ret >= 0)
                      {

                        memset(&pkt_, 0, sizeof(pkt_));

                        pkt_.pts = AV_NOPTS_VALUE;
                        pkt_.dts = AV_NOPTS_VALUE;
                        pkt_.pos = -1;
                        //in stream just above pkt_.time_base = av_make_q(0, 1);
                        Rts_B = true;
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
  return Rts_B;
}

size_t Bof2dVideoEncoder::receiveAndWrite()
{
  int error = 0;
  size_t nbPacketsWritten = 0;

  while (error >= 0)
  {
    error = avcodec_receive_packet(codecCtx_, &pkt_);
    if (error == AVERROR(EAGAIN) || error == AVERROR_EOF)
    {
      continue;
    }
    else if (error < 0)
    {
      //errorManager_->internalError(logger_, "Error during encoding", inputFile_);
      continue;
    }

    /* rescale output packet timestamp values from codec to stream timebase */
    av_packet_rescale_ts(&pkt_, codecCtx_->time_base, stream_->time_base);
    pkt_.stream_index = stream_->index;

    /* Write the compressed frame to the media file. */
    auto ret = av_interleaved_write_frame(oc_, &pkt_);

    /* pkt is now blank (av_interleaved_write_frame() takes ownership of
     * its contents and resets pkt), so that no unreferencing is necessary.
     * This would be different if one used av_write_frame(). */
    if (ret < 0)
    {
      //errorManager_->internalError(logger_, "Error while writing output packet", inputFile_);
      continue;
    }

    nbPacketsWritten++;
  }

  nbPacketsWritten_ += nbPacketsWritten;
  //cyclePktsWritten_ += nbPacketsWritten;
  //endTp_ = std::chrono::steady_clock::now();

  return nbPacketsWritten;
}

void Bof2dVideoEncoder::encode(const uint8_t *src)  //const RawFrame &frame)
{
  int error = 0;

  if ((error = av_image_fill_arrays(frame_->data, frame_->linesize, src, outputVideoPixFmt_, width_, height_, mVidEncAllocAlignment_i) >= 0))
  {

    frame_->pts = pts_++;
    frame_->width = width_;
    frame_->height = height_;
    frame_->format = outputVideoPixFmt_; // frame.videoParams_.outputVideoPixFmt_;
    frame_->colorspace = AVCOL_SPC_RGB; //AVCOL_SPC_BT709 frame.videoParams_.colorSpace_;
    frame_->color_trc = AVCOL_TRC_BT709; // frame.videoParams_.colorTrc_;
    frame_->color_range = AVCOL_RANGE_JPEG;  //frame.videoParams_.colorRange_;
    frame_->color_primaries = AVCOL_PRI_BT709; // frame.videoParams_.colorPrimaries_;

    if (0)  //frame.videoParams_.interlaced_)
    {
      frame_->interlaced_frame = 1;
      frame_->top_field_first = 1;
    }

    //cout << "Encoding frame " << frame.frameNumber_ << endl;

    error = avcodec_send_frame(codecCtx_, frame_);

    av_frame_unref(frame_);

    if (error < 0)
    {
      size_t errorSize = 256;
      std::string errorStr;
      errorStr.resize(errorSize);

      av_strerror(error, &errorStr.at(0), errorSize);
      //errorManager_->internalError(logger_, "Error sending a frame for encoding: "  + std::to_string(error) + ": " + errorStr, inputFile_);
    }
  }

}




https://andy-hsieh.com/thumbnail-maker-ffmpeglibav-c-qt/


AVOutputFormat *outputFormat = av_guess_format("mjpeg", NULL, NULL);

if (outputFormat == nullptr)
{
  curios::LogErr("Fail to guess format");
  return false;
}

// set up codecParams, which will be bound to codecContext later.
AVCodecParameters *parameters = avcodec_parameters_alloc();
parameters->codec_id = outputFormat->video_codec;
parameters->codec_type = AVMEDIA_TYPE_VIDEO;
parameters->format = AV_PIX_FMT_YUVJ420P; //JPEG TYPE!
parameters->width = pFrame->width;
parameters->height = pFrame->height;

// find AVCodec encoder
AVCodec *codec = avcodec_find_encoder(parameters->codec_id);
if (!codec)
{
  curios::LogErr("Cant find AVCodec to save image");
  return false;
}

AVCodecContext *codecContext = avcodec_alloc_context3(codec);
if (!codecContext)
{
  curios::LogErr("Cant allocate avCodecContext for saving thumbnail");
  return false;
}

int ret = avcodec_parameters_to_context(codecContext, parameters);
if (ret < 0)
{
  curios::LogErr("Failed to copy codec parameters to decoder context");
  return false;
}
codecContext->time_base = AVRational{ 1, 25 };

// Set Quality
// https://coderedirect.com/questions/412396/how-to-set-ffmpeg-qscale-in-c-c-for-image-encoding
//int qscale = 1;
//codecContext->flags |= AV_CODEC_FLAG_QSCALE;
//codecContext->global_quality = FF_QP2LAMBDA * qscale;

ret = avcodec_open2(codecContext, codec, NULL);
if (ret < 0)
{
  curios::LogErr("Fail to init codecContext for a given codec");
  return false;
}

ret = avcodec_send_frame(codecContext, pFrame);
if (ret < 0)
{
  char buffer[AV_ERROR_MAX_STRING_SIZE];
  auto s = av_make_error_string(buffer, AV_ERROR_MAX_STRING_SIZE, ret);
  curios::LogErr("Can't avcodec_send_frame: %s", s);
  return false;
}

AVPacket *pPacket = av_packet_alloc();
if (!pPacket)
{
  curios::LogErr("Cant av_packet_alloc create packet buffer");
  return false;
}
ret = avcodec_receive_packet(codecContext, pPacket);
if (ret < 0)
{
  curios::LogErr("Cant avcodec_receive_packet from codecContext");
  return false;
}
Now, we have an encoded packet, we want to add it to our container.We create the format context this time.

// encoding done, now write to file
// ------------------------------------------------------------------
// create file (AVIOContext)
AVFormatContext *pFormatContext = avformat_alloc_context();
pFormatContext->oformat = outputFormat;


ret = avio_open(&pFormatContext->pb, url.c_str(), AVIO_FLAG_READ_WRITE);
if (ret < 0)
{
  curios::LogErr("Cant create file image container (avio_open): %s", url);
  return false;
}
then we construct an(image / video) stream bound to the container, as stated before, each stream has a corresponding codec parameter, we need to fill it up correctly.

//construct a PNG stream(image) bound to the container for writing the data.
AVStream *pStream = avformat_new_stream(pFormatContext, NULL);
*(pStream->codecpar) = *parameters; // we can't just override the ptr, memeory leak! we want deep-copy.
Then we can write the frame. (the header, trailer).Then release all the corresponding resources.A better way to ensure your resource is safely released is to wrap all the C - struct into the C++ RAII mechanism, but that’s not our concern here for this simple project.

if (pStream == nullptr)
{
  curios::LogErr("Cant avformat_new_stream on the image container.");
  return false;
}

// save the pPacket to file, header
ret = avformat_write_header(pFormatContext, nullptr);
if (ret < 0)
{
  curios::LogErr("Cant write header to file: %s", url);
  return false;
}
// write data
ret = av_write_frame(pFormatContext, pPacket);
if (ret < 0)
{
  curios::LogErr("Cant write packet to file: %s", url);
  return false;
}
// write trailer.
av_write_trailer(pFormatContext);

// release resource
avcodec_close(codecContext);
avio_close(pFormatContext->pb);

av_packet_unref(pPacket);
avcodec_parameters_free(&parameters);
avcodec_free_context(&codecContext);
avformat_free_context(pFormatContext);

#endif


END_BOF2D_NAMESPACE()