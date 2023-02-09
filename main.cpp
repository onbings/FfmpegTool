 //https://digital-domain.net/programming/ffmpeg-libs-audio-transcode.html
 //https://github.com/FFmpeg/FFmpeg/blob/master/doc/examples/avio_reading.c
 //https://github.com/UnickSoft/FFMpeg-decode-example/blob/master/Bof2dAudioDecoder/ffmpegDecode.cpp
//https://github.com/UnickSoft/FFMpeg-decode-example/blob/master/ffmpegDecoder/ffmpegDecode.h
//http://unick-soft.ru/article.php?id=14
//https://andy-hsieh.com/thumbnail-maker-ffmpeglibav-c-qt/

#include "bof2d_av_codec.h"
#include "FfmpegDecoder.h"

//Incompatible with Dr. Memory
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>
BOFSTD_EXPORT BOFSTDPARAM GL_BofStdParam_X;
int ExtractAudio(const char *pFnIn_c, const char *pFnOut_c);
int main(int argc, char *argv[])
{
#if 0
	FFmpegDecoder Dec;
	bool Sts_B;
	int Sts_i, i;
	AVFrame *pVideo_X, *pAudio_X;

	Sts_B = Dec.OpenFile("C:/tmp/sample-mp4-file-compare.mp4");
	for (i = 0; i < 50; i++)
	{
		Sts_i = Dec.GetNextFrame(&pVideo_X, &pAudio_X);
	}
	Sts_B = Dec.CloseFile();
#else

	BOF2D::BOF2D_VID_DEC_OUT VidDecOut_X;
	BOF2D::BOF2D_AUD_DEC_OUT AudDecOut_X;
	uint64_t VideoFrameCounter_U64, AudioFrameCounter_U64;

//	char *pMemoryLeak_c = new char[128];
	//printf("pMemoryLeak_c %p\n", pMemoryLeak_c);
	//char c = pMemoryLeak_c[128];
	//ExtractAudio("C:\\tmp\\sample-15s.mp3", "C:\\tmp\\audio.wav");

	BOFERR Sts_E;
	int16_t *pAudioData_S16;
	uint32_t *pVideoData_U32;
	bool EncodeData_B;
	BOF2D::Bof2dAvCodec AvCodec;
	//Sts_E = AvCodec.OpenDecoder("C:/tmp/Sample24bit96kHz5.1.wav", "", "--A_NBCHNL=6;--A_LAYOUT=0x3F;--A_RATE=48000;--A_BPS=16;--A_DEMUX");
	Sts_E = AvCodec.OpenDecoder("C:/tmp/sample-mp4-file.mp4", 
		//"--V_WIDTH=160;--V_HEIGHT=120;--V_BPS=32;--V_THREAD=1",
		"",
		//"--A_NBCHNL=2;--A_LAYOUT=3;--A_RATE=48000;--A_BPS=16;--A_DEMUX");
		"--A_DEMUX");
	if (Sts_E == BOF_ERR_NO_ERROR)
	{
		Sts_E = AvCodec.OpenEncoder(BOF2D::BOF2D_AV_CONTAINER_FORMAT::BOF2D_AV_CONTAINER_FORMAT_NONE, 
																//"--V_BASEFN=C:/tmp/ffmpeg/VideoOut;--V_FMT=PNG;--V_QUALITY=9",
																"--V_BASEFN=C:/tmp/ffmpeg/VideoOut",
																//"--A_BASEFN=C:/tmp/ffmpeg/AudioOut;--A_NBCHNL=2;--A_FMT=PCM;--A_CHUNK");
																"--A_BASEFN=C:/tmp/ffmpeg/AudioOut;--A_CHUNK");
		if (Sts_E == BOF_ERR_NO_ERROR)
		{
			//	Sts_E = AvCodec.Open("C:\\tmp\\sample-15s.mp3");
			//goto l;
			VideoFrameCounter_U64 = 0;
			AudioFrameCounter_U64 = 0;
			do
			{
				EncodeData_B = false;
				Sts_E = AvCodec.BeginRead(VidDecOut_X, AudDecOut_X);
				if (Sts_E == BOF_ERR_NO_ERROR)
				{
					if (VidDecOut_X.Data_X.Size_U64)
					{
						VideoFrameCounter_U64++;
						EncodeData_B = true;

						pVideoData_U32 = (uint32_t *)VidDecOut_X.Data_X.pData_U8;
						BOF_ASSERT(pVideoData_U32 != nullptr);
						if (pVideoData_U32)
						{
							printf("%06zd: Got Video %zx/%zx:%p Ls %d %dx%d\n", VideoFrameCounter_U64, VidDecOut_X.Data_X.Size_U64, VidDecOut_X.Data_X.Capacity_U64, VidDecOut_X.Data_X.pData_U8,
								VidDecOut_X.LineSize_S32, VidDecOut_X.Size_X.Width_U32, VidDecOut_X.Size_X.Height_U32);
							printf("        Got Data %08x %08x %08x %08x %08x %08x %08x %08x\n", pVideoData_U32[0], pVideoData_U32[1], pVideoData_U32[2], pVideoData_U32[3], pVideoData_U32[4], pVideoData_U32[5], pVideoData_U32[6], pVideoData_U32[7]);
						}
					}
					if (AudDecOut_X.InterleavedData_X.Size_U64)
					{
						AudioFrameCounter_U64++;
						EncodeData_B = true;

						pAudioData_S16 = (int16_t *)AudDecOut_X.InterleavedData_X.pData_U8;
						BOF_ASSERT(pAudioData_S16 != nullptr);
						if (pAudioData_S16)
						{
							printf("%06zd: Got Audio %zx/%zx:%p nbs %d ch %d layout %zx Rate %d\n", AudioFrameCounter_U64, AudDecOut_X.InterleavedData_X.Size_U64, AudDecOut_X.InterleavedData_X.Capacity_U64, AudDecOut_X.InterleavedData_X.pData_U8,
								AudDecOut_X.NbSample_U32, AudDecOut_X.NbChannel_U32, AudDecOut_X.ChannelLayout_U64, AudDecOut_X.SampleRateInHz_U32);
							printf("        Got Data %04x %04x %04x %04x %04x %04x %04x %04x\n", pAudioData_S16[0], pAudioData_S16[1], pAudioData_S16[2], pAudioData_S16[3], pAudioData_S16[4], pAudioData_S16[5], pAudioData_S16[6], pAudioData_S16[7]);
						}
					}
					if (EncodeData_B)
					{
						Sts_E = AvCodec.BeginWrite(VidDecOut_X, AudDecOut_X);
						Sts_E = AvCodec.EndWrite();
					}
					Sts_E = AvCodec.EndRead();
				
					if ((VideoFrameCounter_U64 > 768))	// || (AudioFrameCounter_U64 > 512))
					{
						break;
					}
				}
			}
			while (Sts_E == BOF_ERR_NO_ERROR);

			//l:
			printf("Exit with '%s'\n", BOF::Bof_ErrorCode(Sts_E));
			AvCodec.CloseEncoder();
		}
		AvCodec.CloseDecoder();
	}
#endif
	//_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	//_CrtDumpMemoryLeaks();
	return 0;
}

