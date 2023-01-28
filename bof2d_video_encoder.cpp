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

BEGIN_BOF2D_NAMESPACE()

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

Bof2dVideoEncoder::Bof2dVideoEncoder()
{
  mVidEncOptionParam_X.push_back({ nullptr, "V_BASEFN", "if defined, video buffer will be saved in this file","","", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_VARIABLE(mVidEncOption_X.BasePath, PATH, 0, 0) });
  mVidEncOptionParam_X.push_back({ nullptr, "V_FMT", "Specifies the video format", "", "", BOF::BOFPARAMETER_ARG_FLAG::CMDLINE_LONGOPT_NEED_ARG, BOF_PARAM_DEF_ENUM(mVidEncOption_X.Format_E, BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_PNG, BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_MAX, S_Bof2dAvVideoFormatEnumConverter, BOF2D_AV_VIDEO_FORMAT) });
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

  if (mEncoderReady_B == false)
  {
    Close();

    mVidEncOption_X.Reset();
    Rts_E = OptionParser.ToByte(_rOption_S, mVidEncOptionParam_X, nullptr, nullptr);
    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      Rts_E = BOF_ERR_INVALID_DST;
      if (mVidEncOption_X.BasePath.IsValid())
      {
        mIoCollection.clear();
        mIoCollection.push_back(VidEncOut_X);  //Entry 0 is for interleaved sample global file

        Rts_E = CreateFileOut();
        if (Rts_E == BOF_ERR_NO_ERROR)
        {
          mEncoderReady_B = true;
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

  CloseFileOut();

  mEncoderReady_B = false;
  mWriteBusy_B = false;
  mWritePending_B = false;
  mVidEncOption_X.Reset();
  mIoCollection.clear();
  mVidDecOut_X.Reset();

  mNbVidEncPacketSent_U64 = 0;
  mNbVidEncFrameReceived_U64 = 0;
  mNbTotalVidEncFrame_U64 = 0;
  mNbTotalVidEncSample_U64 = 0;

  Rts_E = BOF_ERR_NO_ERROR;
  return Rts_E;
}


BOFERR Bof2dVideoEncoder::BeginWrite(BOF2D_VID_DEC_OUT &_rVidDecOut_X)
{
  BOFERR Rts_E = BOF_ERR_ECANCELED;

  if (mEncoderReady_B)
  {
    Rts_E = BOF_ERR_EOF;
    if (mWriteBusy_B)
    {
      Rts_E = BOF_ERR_EBUSY;
    }
    else
    {
      mWriteBusy_B = true;
      mVidDecOut_X = _rVidDecOut_X;
      Rts_E = WriteChunkOut();
      if (Rts_E == BOF_ERR_EAGAIN)
      {
        mWriteBusy_B = false;
        mWritePending_B = true;
        Rts_E = BOF_ERR_NO_ERROR;
      }
    }
  }

  return Rts_E;
}

BOFERR Bof2dVideoEncoder::EndWrite()
{
  BOFERR Rts_E = BOF_ERR_ECANCELED;

  if (mEncoderReady_B)
  {
    Rts_E = BOF_ERR_EOF;
    if (mWriteBusy_B)
    {
      mWriteBusy_B = false;
      mWritePending_B = false;
      Rts_E = BOF_ERR_NO_ERROR;
    }
    else
    {
      Rts_E = BOF_ERR_PENDING;
    }
  }
  return Rts_E;
}

BOFERR Bof2dVideoEncoder::CreateFileOut()
{
  BOFERR Rts_E;
  std::string Extension_S = S_Bof2dAvVideoFormatEnumConverter.ToString(mVidEncOption_X.Format_E);

  Rts_E = BOF::Bof_CreateFile(BOF::BOF_FILE_PERMISSION_ALL_FOR_OWNER | BOF::BOF_FILE_PERMISSION_READ_FOR_ALL, mVidEncOption_X.BasePath.FullPathNameWithoutExtension(false) + '.' + Extension_S, false, mIoCollection[0].Io);
  if (Rts_E == BOF_ERR_NO_ERROR)
  {
    Rts_E = WriteHeader();
  }
  return Rts_E;
}

BOFERR Bof2dVideoEncoder::WriteHeader()
{
  BOFERR Rts_E = BOF_ERR_NO_ERROR;
  BOF2D_BMP_HEADER BmpHeader_X;
  uint32_t i_U32, Nb_U32;
  int64_t Pos_S64;

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
        BmpHeader_X.BmpSize_U32 = sizeof(BOF2D_BMP_HEADER) + (3 * mVidDecOut_X.Size_X.Width_U32 * mVidDecOut_X.Size_X.Height_U32);
        BmpHeader_X.DataOffset_U32 = sizeof(BOF2D_BMP_HEADER);
        BmpHeader_X.Reserved1_U16 = 0;
        BmpHeader_X.Reserved2_U16 = 0;

        //BOF2D_BMP_INFO_HEADER &bmiHeader = *((BOF2D_BMP_INFO_HEADER *)(pData + headersSize - sizeof(BOF2D_BMP_INFO_HEADER)));

        BmpHeader_X.Bpp_U16 = 3 * 8;
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
  return Rts_E;
}

BOFERR Bof2dVideoEncoder::WriteChunkOut()
{
  BOFERR Rts_E;
  uint32_t Nb_U32, i_U32;  
  int32_t Span_S32;
  uint8_t *pData_U8;
  
  pData_U8 = mVidDecOut_X.Data_X.pData_U8 + ((mVidDecOut_X.LineSize_S32 * mVidDecOut_X.Size_X.Height_U32) - mVidDecOut_X.LineSize_S32);
  Span_S32 = -mVidDecOut_X.LineSize_S32;

  int numberOfBytesToWrite = 3 * mVidDecOut_X.Size_X.Width_U32;

  for (i_U32 = 0; i_U32 < mVidDecOut_X.Size_X.Height_U32; i_U32++, pData_U8 += Span_S32)
  {
    Nb_U32 = 3 * mVidDecOut_X.Size_X.Width_U32;
    Rts_E = BOF::Bof_WriteFile(mIoCollection[0].Io, Nb_U32, pData_U8);
    if (Rts_E == BOF_ERR_NO_ERROR)
    {
      mIoCollection[0].Size_U64 += Nb_U32;
    }
  }
  return Rts_E;
}

BOFERR Bof2dVideoEncoder::CloseFileOut()
{
  BOFERR Rts_E;
  uint32_t i_U32;

  Rts_E = WriteHeader();
  for (i_U32 = 0; i_U32 < mIoCollection.size(); i_U32++)  //Entry 0 is for interleaved sample global file
  {
    if (mIoCollection[i_U32].Io != BOF::BOF_FS_INVALID_HANDLE)
    {
      BOF::Bof_CloseFile(mIoCollection[i_U32].Io);
      mIoCollection[i_U32].Reset();
    }
  }
  return Rts_E;
}

bool Bof2dVideoEncoder::IsVideoStreamPresent()
{
  return mEncoderReady_B;
}

void Bof2dVideoEncoder::GetVideoWriteFlag(bool &_rBusy_B, bool &_rPending_B)
{
  _rBusy_B = mWriteBusy_B;
  _rPending_B = mWritePending_B;
}
END_BOF2D_NAMESPACE()