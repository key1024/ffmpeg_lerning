#include <stdio.h>
#include <time.h>

extern "C"
{
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "SDL2/SDL.h"
}

#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)

static int thread_exit = 0;

int sfp_refresh_thread(void* opaque)
{
	SDL_Event event;
	while (thread_exit == 0)
	{
		event.type = SFM_REFRESH_EVENT;
		SDL_PushEvent(&event);
		SDL_Delay(33);
	}

	return 0;
}

void show_device()
{
	AVFormatContext* pFormatCtx = avformat_alloc_context();
	AVDictionary* options = NULL;
	av_dict_set(&options, "list_devices", "true", 0);
	AVInputFormat* iformat = av_find_input_format("dshow");
	printf("========Device Info=============\n");
	avformat_open_input(&pFormatCtx, "video=dummy", iformat, &options);
	printf("================================\n");
	avformat_close_input(&pFormatCtx);
	avformat_free_context(pFormatCtx);
}

void show_device_option()
{
	AVFormatContext* pFormatCtx = avformat_alloc_context();
	AVDictionary* options = NULL;
	av_dict_set(&options, "list_options", "true", 0);
	AVInputFormat* iformat = av_find_input_format("dshow");
	printf("========Device Option Info======\n");
	avformat_open_input(&pFormatCtx, "video=SIT USB2.0 Camera", iformat, &options);
	printf("================================\n");
	avformat_close_input(&pFormatCtx);
	avformat_free_context(pFormatCtx);
}

int main(int argc, char** argv)
{
	avdevice_register_all();
	av_register_all();

	//show_device();
	//show_device_option();

	//return 0;
	
	AVInputFormat* fmt = av_find_input_format("dshow");
	printf("format: %s\n", fmt->name);

	AVFormatContext* fmt_ctx = avformat_alloc_context();
	AVDictionary* option = nullptr;
	av_dict_set(&option, "video_size", "1920x1080", 0);
	av_dict_set(&option, "framerate", "30", 0);
	if (avformat_open_input(&fmt_ctx, "video=SIT USB2.0 Camera", fmt, &option) != 0)
	{
		printf("open input stream failed\n");
		return -1;
	}

	if (avformat_find_stream_info(fmt_ctx, nullptr) != 0)
	{
		printf("couldn't find stream info\n");
		return -1;
	}

	int video_index = -1;
	for (int i = 0; i < fmt_ctx->nb_streams; i++)
	{
		if (fmt_ctx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video_index = i;
			break;
		}
	}

	if (video_index == -1)
	{
		printf("couldn't find video stream\n");
		return -1;
	}

	AVCodecContext* codec_ctx = fmt_ctx->streams[video_index]->codec;
	AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);
	if (codec == nullptr)
	{
		printf("codec not found\n");
		return -1;
	}

	if (avcodec_open2(codec_ctx, codec, nullptr) != 0)
	{
		printf("open codec failed\n");
		return -1;
	}


	int width = codec_ctx->width;
	int height = codec_ctx->height;
	SwsContext* sws_ctx = sws_getContext(codec_ctx->width, codec_ctx->height, codec_ctx->pix_fmt,
		codec_ctx->width, codec_ctx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr);

	AVPacket* packet = av_packet_alloc();
	AVFrame* frame = av_frame_alloc();

	AVFrame* yuv_frame = av_frame_alloc();
	yuv_frame->data[0] = (uint8_t*)malloc(width * height);
	yuv_frame->data[1] = (uint8_t*)malloc(width * height / 2);
	yuv_frame->data[2] = (uint8_t*)malloc(width * height / 2);
	yuv_frame->linesize[0] = width;
	yuv_frame->linesize[1] = width / 2;
	yuv_frame->linesize[2] = width / 2;
	yuv_frame->height = codec_ctx->height;
	yuv_frame->width = codec_ctx->width;

	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
	int w_width = width;
	int w_height = height;
	SDL_Window* window = SDL_CreateWindow("play camera", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w_width, w_height, SDL_WINDOW_OPENGL);
	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
	SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, codec_ctx->width, codec_ctx->height);

	SDL_CreateThread(sfp_refresh_thread, NULL, NULL);
	SDL_Event event;

	clock_t old = 0;
	while (true)
	{
		SDL_WaitEvent(&event);
		if (event.type == SFM_REFRESH_EVENT)
		{
			if (av_read_frame(fmt_ctx, packet) >= 0)
			{
				if (packet->stream_index == video_index)
				{
					int got_picture = -1;
					int ret = avcodec_decode_video2(codec_ctx, frame, &got_picture, packet);
					if (ret < 0)
					{
						printf("decodec error: %d\n", ret);
						return -1;
					}

					if (got_picture)
					{
						sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, yuv_frame->data, yuv_frame->linesize);
						SDL_UpdateYUVTexture(texture, NULL, yuv_frame->data[0], yuv_frame->linesize[0],
							yuv_frame->data[1], yuv_frame->linesize[1],
							yuv_frame->data[2], yuv_frame->linesize[2]);
						SDL_RenderClear(renderer);
						SDL_RenderCopy(renderer, texture, NULL, NULL);
						SDL_RenderPresent(renderer);
						clock_t t2 = clock();
						printf("Ö¡¼ä¸ô: %ld\n", t2 - old);
						old = t2;
					}
				}

				av_free_packet(packet);
			}
		}

	}

	return 0;
}