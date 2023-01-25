#include "bof2d_av_codec.h"

//Incompatible with Dr. Memory
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>
BOFSTD_EXPORT BOFSTDPARAM GL_BofStdParam_X;
int ExtractAudio(const char *pFnIn_c, const char *pFnOut_c);
int main(int argc, char *argv[])
{
	BOF2D::BOF2D_VID_DEC_OUT VidDecOut_X;
	BOF2D::BOF2D_AUD_DEC_OUT AudDecOut_X;
	uint64_t FrameCounter_U64;

//	char *pMemoryLeak_c = new char[128];
	//printf("pMemoryLeak_c %p\n", pMemoryLeak_c);
	//char c = pMemoryLeak_c[128];
	//ExtractAudio("C:\\tmp\\sample-15s.mp3", "C:\\tmp\\audio.wav");

	BOFERR Sts_E;
	int16_t *pData_S16;
	BOF2D::Bof2dAvCodec AvCodec;
	//Sts_E = AvCodec.Open("C:/tmp/sample-mp4-file.mp4", "", "--A_BASEFN=C:/tmp/AudioOut;--A_NBCHNL=2;--A_LAYOUT=3;--A_RATE=48000;--A_BPS=16;--A_FMT=WAV;--A_DEMUX");
//	Sts_E = AvCodec.Open("C:/tmp/sample-15s.mp3", "", "--A_BASEFN=C:/tmp/AudioOut;--A_NBCHNL=2;--A_LAYOUT=3;--A_RATE=48000;--A_BPS=16;--A_FMT=WAV");
//  Sts_E = AvCodec.OpenDecoder("C:/tmp/Sample24bit96kHz5.1.wav", "", "--A_BASEFN=C:/tmp/AudioOut;--A_NBCHNL=6;--A_LAYOUT=0x3F;--A_RATE=48000;--A_BPS=16;--A_FMT=WAV;--A_DEMUX;--A_SPLIT");
//	Sts_E = AvCodec.OpenDecoder("C:/tmp/Sample24bit96kHz5.1.wav", "", "--A_BASEFN=C:/tmp/AudioOut;--A_NBCHNL=6;--A_LAYOUT=0x3F;--A_RATE=48000;--A_BPS=16;--A_FMT=WAV;--A_DEMUX");
	Sts_E = AvCodec.OpenDecoder("C:/tmp/sample-mp4-file.mp4", "--V_WIDTH=160;V_HEIGHT=120;V_BPS=24", "--A_NBCHNL=2;--A_LAYOUT=3;--A_RATE=48000;--A_BPS=16;--A_DEMUX");
	//Sts_E = AvCodec.OpenDecoder("C:/tmp/Sample24bit96kHz5.1.wav", "", "--A_NBCHNL=6;--A_LAYOUT=0x3F;--A_RATE=48000;--A_BPS=16;--A_DEMUX");
	if (Sts_E == BOF_ERR_NO_ERROR)
	{
		Sts_E = AvCodec.OpenEncoder(BOF2D::BOF2D_AV_CONTAINER_FORMAT::BOF2D_AV_CONTAINER_FORMAT_NONE, BOF2D::BOF2D_AV_VIDEO_FORMAT::BOF2D_AV_VIDEO_FORMAT_BMP, BOF2D::BOF2D_AV_AUDIO_FORMAT::BOF2D_AV_AUDIO_FORMAT_WAV, "--V_BASEFN=C:/tmp/VideoOut;--V_FMT=BMP", "--A_BASEFN=C:/tmp/AudioOut;--A_FMT=WAV");
		if (Sts_E == BOF_ERR_NO_ERROR)
		{
			//	Sts_E = AvCodec.Open("C:\\tmp\\sample-15s.mp3");
			//goto l;
			FrameCounter_U64 = 0;
			do
			{
				Sts_E = AvCodec.BeginRead(VidDecOut_X, AudDecOut_X);
				if (Sts_E == BOF_ERR_NO_ERROR)
				{
					FrameCounter_U64++;
					pData_S16 = (int16_t *)AudDecOut_X.InterleavedData_X.pData_U8;
					BOF_ASSERT(pData_S16);
					if (pData_S16)
					{
						printf("%06zd: Got Audio %zx/%zx:%p nbs %d ch %d layout %zx Rate %d\n", FrameCounter_U64, AudDecOut_X.InterleavedData_X.Size_U64, AudDecOut_X.InterleavedData_X.Capacity_U64, AudDecOut_X.InterleavedData_X.pData_U8,
							AudDecOut_X.NbSample_U32, AudDecOut_X.NbChannel_U32, AudDecOut_X.ChannelLayout_U64, AudDecOut_X.SampleRateInHz_U32);
						printf("        Got Data %04x %04x %04x %04x %04x %04x %04x %04x\n", pData_S16[0], pData_S16[1], pData_S16[2], pData_S16[3], pData_S16[4], pData_S16[5], pData_S16[6], pData_S16[7]);

						Sts_E = AvCodec.EndRead();
						Sts_E = AvCodec.BeginWrite(VidDecOut_X, AudDecOut_X);
						Sts_E = AvCodec.EndWrite();
						if (FrameCounter_U64 > 512)
						{
							break;
						}
					}
				}
			} while (Sts_E == BOF_ERR_NO_ERROR);
			//l:
			printf("Exit with '%s'\n", BOF::Bof_ErrorCode(Sts_E));
			AvCodec.CloseEncoder();
		}
		AvCodec.CloseDecoder();
	}
	//_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	//_CrtDumpMemoryLeaks();
	return 0;
}

