// ffmpeg_player_sdl2.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "SDL2/SDL.h"
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

int open_input_file(const char* fileName)
{


	return 0;
}

int main(int argc, char** argv)
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

	// 到这里其实是完成了解封装
	printf("-------------------File Information------------------------\n");
	av_dump_format(pFormatCtx, 0, filePath, 0);
	printf("-----------------------------------------------------------\n");

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

	// SDL初始化
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		printf("could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	// 创建窗口
	int nScreenW = pCodecCtx->width;
	int nScreenH = pCodecCtx->height;
	SDL_Window* screen = SDL_CreateWindow("Simplest FFmpeg player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, nScreenW, nScreenH, SDL_WINDOW_OPENGL);
	if (!screen)
	{
		printf("could not create window -exiting: %s\n", SDL_GetError());
		return -1;
	}
	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
	SDL_Texture* sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
		pCodecCtx->width, pCodecCtx->height);

	AVFrame* pFrame = av_frame_alloc();
	AVFrame* pFrameYUV = av_frame_alloc();
	unsigned char* out_buffer = (unsigned char*)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
		AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
	AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));
	SwsContext* imgConvertCtx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
		pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	SDL_CreateThread(sfp_refresh_thread, NULL, NULL);
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
					sws_scale(imgConvertCtx, pFrame->data, pFrame->linesize, 0,
						pCodecCtx->height, pFrameYUV->data, pFrameYUV->linesize);
					SDL_Rect rect;
					rect.x = 0; rect.y = 0; rect.w = pFrame->width; rect.h = pFrame->height;
					SDL_UpdateTexture(sdlTexture, NULL, pFrameYUV->data[0], pFrameYUV->linesize[0]);
					SDL_RenderClear(sdlRenderer);
					SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
					SDL_RenderPresent(sdlRenderer);
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

