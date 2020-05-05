#include <stdio.h>

extern "C"
{
#include "libavcodec/avcodec.h"
}

int decode_video(const char* file_name)
{
	// ����ǰ��һ֡����
	AVPacket* pkt = av_packet_alloc();
	if (!pkt)
	{
		printf("av_packet_alloc error.\n");
		return -1;
	}
	// ������һ֡ͼ��
	AVFrame* frame = av_frame_alloc();
	if (!frame)
	{
		printf("av_frame_alloc error.\n");
		return -1;
	}

	// �Ȳ��ҽ�����
	// �������Ѿ�֪��Ҫ�������Ƶ��h265��ʽ�ģ�����ֱ��ָ����AV_CODEC_ID_H265���������
	// ��������ʹ�õ���ʲô�������Ļ�������ʹ��avformat_find_stream_info�����������ȡ��Ƶ��Ϣ
	AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H265);
	if (!codec)
	{
		printf("codec not found: %d\n", AV_CODEC_ID_H265);
		return -1;
	}

	// ��������������
	AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
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

	AVCodecParserContext* codec_parser_ctx = av_parser_init(codec->id);
	if (!codec_parser_ctx)
	{
		printf("av_parser_init error\n");
		return -1;
	}

	FILE* fp = fopen(file_name, "rb");
	if (!fp)
	{
		printf("open %s failed\n", file_name);
		return -1;
	}

	return 0;
}