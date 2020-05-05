#include <stdio.h>

extern "C"
{
#include "libavcodec/avcodec.h"
}

int decode_video(const char* file_name)
{
	// 解码前的一帧数据
	AVPacket* pkt = av_packet_alloc();
	if (!pkt)
	{
		printf("av_packet_alloc error.\n");
		return -1;
	}
	// 解码后的一帧图像
	AVFrame* frame = av_frame_alloc();
	if (!frame)
	{
		printf("av_frame_alloc error.\n");
		return -1;
	}

	// 先查找解码器
	// 由于我已经知道要解码的视频是h265格式的，所以直接指定了AV_CODEC_ID_H265这个解码器
	// 如果不清楚使用的是什么解码器的话，可以使用avformat_find_stream_info这个函数来获取视频信息
	AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H265);
	if (!codec)
	{
		printf("codec not found: %d\n", AV_CODEC_ID_H265);
		return -1;
	}

	// 创建解码上下文
	AVCodecContext *codec_ctx = avcodec_alloc_context3(codec);
	if (!codec_ctx)
	{
		printf("avcodec_alloc_context3 error.\n");
		return -1;
	}

	// 打开解码器
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