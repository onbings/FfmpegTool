#include "bof2d_av_codec.h"

//Incompatible with Dr. Memory
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>
BOFSTD_EXPORT BOFSTDPARAM GL_BofStdParam_X;
int ExtractAudio(const char *pFnIn_c, const char *pFnOut_c);
int main(int argc, char *argv[])
{
	BOF2D::BOF2D_VIDEO_DATA VideoData_X;
	BOF2D::BOF2D_AUDIO_DATA AudioData_X;
	uint64_t FrameCounter_U64;

//	char *pMemoryLeak_c = new char[128];
	//printf("pMemoryLeak_c %p\n", pMemoryLeak_c);
	//char c = pMemoryLeak_c[128];
	//ExtractAudio("C:\\tmp\\sample-15s.mp3", "C:\\tmp\\audio.wav");

	BOFERR Sts_E;
	int16_t *pData_S16;
	BOF2D::Bof2dAvCodec AvCodec;
	//Sts_E = AvCodec.Open("C:/tmp/sample-mp4-file.mp4", "", "--A_BASEFN=C:/tmp/AudioOut;--A_NBCHNL=2;--A_LAYOUT=3;--A_RATE=48000;--A_BPS=16;--A_FMT=WAV;--A_CHUNK");
//	Sts_E = AvCodec.Open("C:/tmp/sample-15s.mp3", "", "--A_BASEFN=C:/tmp/AudioOut;--A_NBCHNL=2;--A_LAYOUT=3;--A_RATE=48000;--A_BPS=16;--A_FMT=WAV");
	Sts_E = AvCodec.Open("C:/tmp/Sample24bit96kHz5.1.wav", "", "--A_BASEFN=C:/tmp/AudioOut;--A_NBCHNL=6;--A_LAYOUT=0x3F;--A_RATE=48000;--A_BPS=16;--A_FMT=WAV");
	//	Sts_E = AvCodec.Open("C:\\tmp\\sample-15s.mp3");
	//goto l;
	FrameCounter_U64 = 0;
	do
	{ 
		Sts_E = AvCodec.BeginRead(VideoData_X, AudioData_X);
		if (Sts_E == BOF_ERR_NO_ERROR)
		{
			FrameCounter_U64++;
			pData_S16 = (int16_t *)AudioData_X.Data_X.pData_U8;
			printf("%06zd: Got Audio %zx/%zx:%p nbs %d ch %d layout %zx Rate %d\n", FrameCounter_U64, AudioData_X.Data_X.Size_U64, AudioData_X.Data_X.Capacity_U64, AudioData_X.Data_X.pData_U8,
				AudioData_X.NbSample_U32, AudioData_X.Param_X.NbChannel_U32, AudioData_X.Param_X.ChannelLayout_U64, AudioData_X.Param_X.SampleRateInHz_U32);
			printf("        Got Data %04x %04x %04x %04x %04x %04x %04x %04x\n", pData_S16[0], pData_S16[1], pData_S16[2], pData_S16[3], pData_S16[4], pData_S16[5], pData_S16[6], pData_S16[7]);

			Sts_E = AvCodec.EndRead();
			if (FrameCounter_U64 > 512)
			{
				//break;
			}
		}
	}  
	while (Sts_E == BOF_ERR_NO_ERROR);
	//l:
	printf("Exit with '%s'\n", BOF::Bof_ErrorCode(Sts_E));
	AvCodec.Close();
	//_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	//_CrtDumpMemoryLeaks();
	return 0;
}

