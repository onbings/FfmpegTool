#include "bof2d_av_codec.h"

//Incompatible with Dr. Memory
//#define _CRTDBG_MAP_ALLOC
//#include <stdlib.h>
//#include <crtdbg.h>
BOFSTD_EXPORT BOFSTDPARAM GL_BofStdParam_X;
int ExtractAudio(const char *pFnIn_c, const char *pFnOut_c);
int main(int argc, char *argv[])
{
	AVFrame *pVideoData_X, *pAudioData_X;
//	char *pMemoryLeak_c = new char[128];
	//printf("pMemoryLeak_c %p\n", pMemoryLeak_c);
	//char c = pMemoryLeak_c[128];
	//ExtractAudio("C:\\tmp\\sample-15s.mp3", "C:\\tmp\\audio.wav");

	BOFERR Sts_E;
	int16_t *pData_S16;
	BOF2D::Bof2dAvCodec AvCodec;
	Sts_E = AvCodec.Open("C:/tmp/sample-mp4-file.mp4");
//	Sts_E = AvCodec.Open("C:\\tmp\\sample-15s.mp3");
	
	do
	{
		Sts_E = AvCodec.Read(&pVideoData_X, &pAudioData_X);
		if (Sts_E == BOF_ERR_NO_ERROR)
		{
			pData_S16 = (int16_t *)pAudioData_X->data[0];
			printf("Audio %x:%p nbs %d ch %d layout %zx Fmt %d Rate %d Pos %zd Dur %zd Sz %d\n", pAudioData_X->linesize[0], pAudioData_X->data[0],
				pAudioData_X->nb_samples, pAudioData_X->channels, pAudioData_X->channel_layout, pAudioData_X->format, pAudioData_X->sample_rate,
				pAudioData_X->pkt_pos, pAudioData_X->pkt_duration, pAudioData_X->pkt_size);
			printf("Data %04x %04x %04x %04x %04x %04x %04x %04x\n", pData_S16[0], pData_S16[1], pData_S16[2], pData_S16[3], pData_S16[4], pData_S16[5], pData_S16[6], pData_S16[7]);
		}



		break;




		
	} 
	while (Sts_E == BOF_ERR_NO_ERROR);
	
	printf("Exit with '%s'\n", BOF::Bof_ErrorCode(Sts_E));
	AvCodec.Close();
	//_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	//_CrtDumpMemoryLeaks();
	return 0;
}

