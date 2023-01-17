/*
	It is FFmpeg decoder class. Sample for article from unick-soft.ru
*/

#ifndef __FFMPEG_DECODER__
#define __FFMPEG_DECODER__

extern "C"
{
#include <libavcodec/avcodec.h>
#include <avformat.h>
#include <swscale.h>
#include "avcodec.h"
#include "avformat.h"
#include "avcodec.h"
#include "libavutil/imgutils.h"
#include "libswresample/swresample.h"
#include <libavutil/opt.h>
}

#include <string>
#include <io.h>
#include <fcntl.h>
#define MP3_FILE_NAME          "C:\\tmp\\sample-15s.mp3"
#define MP4_FILE_NAME          "C:\\tmp\\sample-mp4-file.mp4"
#define AUDIO_OUT          "C:\\tmp\\audio.wav"
#define VIDEO_OUTPUT_FILE_PREFIX "c:\\tmp\\image%d.bmp"
#define AUDIO_OUTPUT_FILE_PREFIX "c:\\tmp\\audio%d.pcm"
#define AUDIO_OUTPUT_FILE "c:\\tmp\\audio.pcm"
#define FRAME_COUNT        500
#define MAX_AUDIO_PACKET (2 * 1024 * 1024)

#define WAVE_OUT_NB_CHANNEL  2
#define WAVE_SAMPLE_RATE  48000	//      16000	//desired sample rate of the output WAVE audio.
#define AVIO_CTX_BUF_SZ          4096	//definition of the WAVE header, there is plenty of documentation on this, so nothing more will be said

int ExtractAudio(const char *pFnIn_c, const char *pFnOut_c);
int decode_audio_file(const char *path, const int sample_rate, double **data, int *size);

class FFmpegDecoder
{ 
  // constructor.
  public: FFmpegDecoder() : pImgConvertCtx(NULL), audioBaseTime(0.0), videoBaseTime(0.0),
          videoFramePerSecond(0.0), isOpen(false), audioStreamIndex(-1), videoStreamIndex(-1),
          pAudioCodec(NULL), pAudioCodecCtx(NULL), pVideoCodec(NULL), pVideoCodecCtx(NULL),
          pFormatCtx(NULL) {;}

  // destructor.
  public: virtual ~FFmpegDecoder() 
  {
    CloseFile();
  }

  // Open file
  public: virtual bool OpenFile(const std::string& inputFile);

  // Close file and free resourses.
  public: virtual bool CloseFile();

  // Return next frame FFmpeg.
//public: virtual AVFrame *GetNextFrame();
public: int GetNextFrame(AVFrame **ppVideo, AVFrame **ppAudio);

  public: int GetWidth()
  {
    return width;
  }
  public: int GetHeight()
  {
    return height;
  }
public:
 // int16_t *GetAudioBuffer(int &_rAudioSize) { _rAudioSize = AudioSize; return pAudioData; }
  // open video stream.
  private: bool OpenVideo();

  // open audio stream.
  private: bool OpenAudio();

  // close video stream.
  private: void CloseVideo();

  // close audio stream.
  private: void CloseAudio();

  // return rgb image 
  private: 
    AVFrame *GetVideoFrame(AVFrame *pFrameYuv, enum AVPixelFormat pix_fmt);
    AVFrame *GetAudioFrame(AVFrame *pFramePcm, int nb_channels, int channellayout, int samplerateinhz, bool flush, enum AVSampleFormat sample_fmt);

  // Decode audio from packet.
//BHA private: int DecodeAudio(int nStreamIndex, const AVPacket *avpkt, uint8_t *pOutBuffer, size_t nOutBufferSize);
private: bool DecodeAudio(AVPacket *avpkt, AVFrame *pOutFrame); // , uint8_t *pOutBuffer, size_t nOutBufferSize);

  // Decode video buffer.
  private: bool DecodeVideo(AVPacket *avpkt, AVFrame *pOutFrame);

  // FFmpeg file format.
  private: AVFormatContext* pFormatCtx;  

  // FFmpeg codec context.
  private: AVCodecContext* pVideoCodecCtx;

  // FFmpeg codec for video.
  private: AVCodec* pVideoCodec;

  // FFmpeg codec context for audio.
  private: AVCodecContext* pAudioCodecCtx;

  // FFmpeg codec for audio.
  private: AVCodec* pAudioCodec;

  // Video stream number in file.
  private: int videoStreamIndex;

  // Audio stream number in file.
  private: int audioStreamIndex;

  // File is open or not.
  private: bool isOpen;

  // Video frame per seconds.
  private: double videoFramePerSecond;

  // FFmpeg timebase for video.
  private: double videoBaseTime;
 
  // FFmpeg timebase for audio.
  private: double audioBaseTime;

  // FFmpeg context convert image.
  private: struct SwsContext *pImgConvertCtx;

  // Width of image
  private: int width;
  
  // Height of image
  private: int height;
private:
  AVFormatContext *fmt_ctx;
  AVIOContext *avio_ctx;
  struct SwrContext *swr;
  uint8_t *avio_ctx_buffer;
  struct audio_buffer *audio_buf;
  //AVFrame *pAudioFrame;
//  int16_t *pAudioData;
//  int AudioSize;
};

#endif