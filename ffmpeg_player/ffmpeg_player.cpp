// ffmpeg_player.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "SDL/SDL.h"
};

//Refresh
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)

static int thread_exit = 0;

int sfp_refresh_thread(void* opaque)
{
	SDL_Event event;
	while (thread_exit == 0)
	{
		event.type = SFM_REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(40);
	}

	return 0;
}

int main(int argc, char **argv)
{
	av_register_all();

	AVFormatContext* pFormatCtx = avformat_alloc_context();

	//char filePath[] = "a.h264";
	char filePath[] = "bigbuckbunny_480x272.h265";
	if (avformat_open_input(&pFormatCtx, filePath, NULL, NULL) != 0)
	{
		printf("Couldn't open input stream.\n");
		return -1;
	}

	if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
	{
		printf("Couldn't find steam information.\n");
		return -1;
	}

	int nVideoIndex = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			nVideoIndex = i;
			break;
		}
	}
	if (nVideoIndex == -1)
	{
		printf("Didn't find a video stream.\n");
		return -1;
	}

	AVCodecContext* pCodecCtx = pFormatCtx->streams[nVideoIndex]->codec;
	AVCodec* pCodec = avcodec_find_decoder(pCodecCtx->codec_id);
	if (pCodec == NULL)
	{
		printf("codec not found.\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0)
	{
		printf("Could not open codec.\n");
		return -1;
	}

	printf("-------------------File Information------------------------\n");
	av_dump_format(pFormatCtx, 0, filePath, 0);
	printf("-----------------------------------------------------------\n");

	// SDL初始化
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		printf("could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	// 设置窗口标题
	SDL_WM_SetCaption("Simplest FFmpeg player", NULL);

	int nScreenW = pCodecCtx->width;
	int nScreenH = pCodecCtx->height;
	SDL_Surface* screen = SDL_SetVideoMode(nScreenW, nScreenH, 0, 0);
	if (!screen)
	{
		printf("could not set video mode -exiting: %s\n", SDL_GetError());
		return -1;
	}

	SDL_Overlay* bmp = SDL_CreateYUVOverlay(nScreenW, nScreenH, SDL_YV12_OVERLAY, screen);

	AVFrame* pFrame = av_frame_alloc();
	AVFrame* pFrameYUV = av_frame_alloc();
	AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));
	SwsContext* imgConvertCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	SDL_CreateThread(sfp_refresh_thread, NULL);
	SDL_Event event;
	while (1)
	{
		SDL_WaitEvent(&event);
		if (event.type == SFM_REFRESH_EVENT)
		{
			if (av_read_frame(pFormatCtx, packet) < 0)
			{
				thread_exit = -1;
				break;
			}
			if (packet->stream_index == nVideoIndex)
			{
				int nGotPicture = 0;
				if (avcodec_decode_video2(pCodecCtx, pFrame, &nGotPicture, packet) < 0)
				{
					printf("Decode error.\n");
					return -1;
				}
				if (nGotPicture)
				{
					/********************************
					有人会疑惑，为什么解码后的pFrame不直接用于显示，而是调用swscale()转换之后进行显示？

					如果不进行转换，而是直接调用SDL进行显示的话，会发现显示出来的图像是混乱的。
					关键问题在于解码后的pFrame的linesize里存储的不是图像的宽度，而是比宽度大一些的一个值。
					其原因目前还没有仔细调查（大概是出于性能的考虑）。例如分辨率为480x272的图像，解码后的视频的linesize[0]为512，而不是480。
					以第1行亮度像素（pFrame->data[0]）为例，从0-480存储的是亮度数据，而从480-512则存储的是无效的数据。
					因此需要使用swscale()进行转换。转换后去除了无效数据，linesize[0]变为480。就可以正常显示了。

					*********************************/
					SDL_LockYUVOverlay(bmp);
					pFrameYUV->data[0] = bmp->pixels[0];
					pFrameYUV->data[1] = bmp->pixels[2];
					pFrameYUV->data[2] = bmp->pixels[1];
					pFrameYUV->linesize[0] = bmp->pitches[0];
					pFrameYUV->linesize[1] = bmp->pitches[2];
					pFrameYUV->linesize[2] = bmp->pitches[1];
					sws_scale(imgConvertCtx, pFrame->data, pFrame->linesize, 0,
						pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
					SDL_UnlockYUVOverlay(bmp);
					SDL_Rect rect;
					rect.x = 0; rect.y = 0; rect.w = pFrame->width; rect.h = pFrame->height;
					SDL_DisplayYUVOverlay(bmp, &rect);
				}
			}
			av_free_packet(packet);
		}
	}

	sws_freeContext(imgConvertCtx);

	SDL_Quit();
	av_free(pFrameYUV);
	av_free(pFrame);
	av_free(packet);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);

	return 0;
}
