#include <stdio.h>
#include "UdpServer.h"
#include <Windows.h>
#include <time.h>
#include <list>

extern "C"
{
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "SDL2/SDL.h"
}

//Refresh
#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
#define BUFFER_SIZE 10

static int thread_exit = 0;

static UdpServer* server;
static AVCodecParserContext* codec_parser_ctx;
static AVFrame* frame;
static AVCodec* codec;
static AVCodecContext* codec_ctx;
static CRITICAL_SECTION cs;
static AVPacket* pkts[BUFFER_SIZE];
static int pkt_index_parse = 0;
static int pkt_index_codec = -1;

char head[4] = { 0x00, 0x00, 0x00, 0x01 };

int parse_thread(void* opaque)
{
	char* inbuf = (char*)malloc(1024 * 1024);
	size_t	buf_size = 1024*1024;

	int data_len = server->Read(inbuf, 10240);

	while (1)
	{
		char read_buf[10240];
		int read_len = server->Read(read_buf, 10240);
		if (memcmp(read_buf, head, 4) != 0)
		{
			memcpy(inbuf + data_len, read_buf, read_len);
			data_len += read_len;
			continue;
		}

		AVPacket* pkt = pkts[pkt_index_parse];
		memcpy(pkt->data, inbuf, data_len);
		pkt->size = data_len;
		pkt_index_codec = pkt_index_parse;
		pkt_index_parse = (pkt_index_parse + 1) % BUFFER_SIZE;

		memcpy(inbuf, read_buf, read_len);
		data_len = read_len;
	}

	return 0;
}

int decode_thread(void* opaque)
{
	while (1)
	{
		if (pkt_index_codec != -1 && pkts[pkt_index_codec]->size > 0)
		{
			//printf("����һ֡����: %d\n", pkt->size);
			// ��һ֡���ݷ��͵�������
			//clock_t t1 = clock();
			//printf("t1: %ld\n", t1);
			int ret = avcodec_send_packet(codec_ctx, pkts[pkt_index_codec]);
			if (ret < 0)
			{
				printf("error sending a packet for decoding\n");
				Sleep(1);
				continue;
			}

			pkts[pkt_index_codec]->size = 0;

			// �ӽ������л�ȡһ֡������ͼ��
			ret = avcodec_receive_frame(codec_ctx, frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			{
				Sleep(1);
				continue;
			}
			else if (ret < 0)
			{
				printf("error during ecoding\n");
				return -1;
			}

			SDL_Event event;
			event.type = SFM_REFRESH_EVENT;
			SDL_PushEvent(&event);
			Sleep(2);

			{
				//printf("�ӽ�������ȡͼ��: %d %d\n", frame->width, frame->height);
				//printf("0: %p %d\n", frame->data[0], frame->linesize[0]);
				//printf("1: %p %d\n", frame->data[1], frame->linesize[1]);
				//printf("2: %p %d\n", frame->data[2], frame->linesize[2]);
				//FILE* fp = fopen("aaaa.yuv", "ab+");
				//if (fp)
				//{
				//	fwrite(frame->data[0], 1, frame->linesize[0] * frame->height, fp);
				//	fwrite(frame->data[1], 1, frame->linesize[1] * frame->height / 2, fp);
				//	fwrite(frame->data[2], 1, frame->linesize[2] * frame->height / 2, fp);
				//	fclose(fp);
				//	exit(0);
				//}
			}
		}
	}
	return 0;
}

int main(int argc, char** argv)
{
	InitializeCriticalSection(&cs);

	server = new UdpServer(23003);
	server->Open();

	avcodec_register_all();

	// ����ǰ��һ֡����
	for (int i = 0; i < BUFFER_SIZE; i++)
	{
		pkts[i] = av_packet_alloc();
		if (!pkts[i])
		{
			printf("av_packet_alloc error.\n");
			return -1;
		}
		pkts[i]->data = (uint8_t*)malloc(10 * 1024 * 1024);
	}

	// ������һ֡ͼ��
	frame = av_frame_alloc();
	if (!frame)
	{
		printf("av_frame_alloc error.\n");
		return -1;
	}

	// �Ȳ��ҽ�����
	// �������Ѿ�֪��Ҫ�������Ƶ��h265��ʽ�ģ�����ֱ��ָ����AV_CODEC_ID_H265���������
	// ��������ʹ�õ���ʲô�������Ļ�������ʹ��avformat_find_stream_info�����������ȡ��Ƶ��Ϣ
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!codec)
	{
		printf("codec not found: %d\n", AV_CODEC_ID_H264);
		return -1;
	}

	// ��������������
	codec_ctx = avcodec_alloc_context3(codec);
	if (!codec_ctx)
	{
		printf("avcodec_alloc_context3 error.\n");
		return -1;
	}

	// �򿪽�����
	int ret = avcodec_open2(codec_ctx, codec, NULL);
	if (ret < 0)
	{
		printf("avcodec_open2 error.\n");
		return -1;
	}

	// ����������������һ֡���ݵĿ�ͷ�ͽ�β
	codec_parser_ctx = av_parser_init(codec->id);
	if (!codec_parser_ctx)
	{
		printf("av_parser_init error\n");
		return -1;
	}

	// SDL��ʼ��
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
	{
		printf("could not initialize SDL - %s\n", SDL_GetError());
		return -1;
	}
	// ��������
	int nScreenW = 960;
	int nScreenH = 540;
	SDL_Window* screen = SDL_CreateWindow("Simplest FFmpeg player", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, nScreenW, nScreenH, SDL_WINDOW_OPENGL);
	if (!screen)
	{
		printf("could not create window -exiting: %s\n", SDL_GetError());
		return -1;
	}
	SDL_Renderer* sdlRenderer = SDL_CreateRenderer(screen, -1, 0);
	SDL_Texture* sdlTexture = SDL_CreateTexture(sdlRenderer, SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING,
		1920, 1080);

	SDL_CreateThread(parse_thread, NULL, NULL);
	SDL_CreateThread(decode_thread, NULL, NULL);

	clock_t old = 0;
	SDL_Event event;
	while (1)
	{
		SDL_WaitEvent(&event);
		if(event.type == SFM_REFRESH_EVENT)
		{
			SDL_Rect rect;
			rect.x = 0; rect.y = 0; rect.w = frame->width; rect.h = frame->height;
			SDL_UpdateYUVTexture(sdlTexture, NULL, frame->data[0], frame->linesize[0],
				frame->data[1], frame->linesize[1],
				frame->data[2], frame->linesize[2]);
			SDL_RenderClear(sdlRenderer);
			SDL_RenderCopy(sdlRenderer, sdlTexture, NULL, NULL);
			SDL_RenderPresent(sdlRenderer);
			clock_t t2 = clock();
			//printf("֡���: %ld\n", t2 - old);
			old = t2;
		}
	}

	return 0;
}