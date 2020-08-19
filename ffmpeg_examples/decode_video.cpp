/*
	只创建了解码器，并通过AVCodecParserContext将数据流分割成一帧帧的数据，传入解码器进行解码。
	如果获取到的数据是一帧帧分开的，则可以不使用AVCodecParserContext，将帧数据直接送到解码器。
*/
#include <stdio.h>
#include <time.h>

extern "C"
{
#include "libavcodec/avcodec.h"
}

static void pgm_save(unsigned char* buf, int wrap, int xsize, int ysize,
						char* filename)
{
	FILE* fp = fopen(filename, "w");
	if (fp)
	{
		fprintf(fp, "P5\n%d %d\n%d\n", xsize, ysize, 255);
		for (int i = 0; i < ysize; i++)
			fwrite(buf + i * wrap, 1, xsize, fp);
		fclose(fp);
	}
}

static void decode(AVCodecContext* dec_ctx, AVFrame* frame, AVPacket* pkt,
					const char* filename)
{
	// 将一帧数据发送到解码器
	int ret = avcodec_send_packet(dec_ctx, pkt);
	if (ret < 0)
	{
		printf("error sending a packet for decoding\n");
		return;
	}

	while (ret >= 0)
	{
		// 从解码器中获取一帧解码后的图像
		ret = avcodec_receive_frame(dec_ctx, frame);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
			return;
		else if (ret < 0)
		{
			printf("error during ecoding\n");
			return;
		}

		printf("saving frame %3d\n", dec_ctx->frame_number);
		fflush(stdout);

		char buf[1024] = { 0 };
		snprintf(buf, sizeof(buf), "%s-%d", filename, dec_ctx->frame_number);
		//pgm_save(frame->data[0], frame->linesize[0], frame->width, frame->height, buf);
	}
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
	AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	if (!codec)
	{
		printf("codec not found: %d\n", AV_CODEC_ID_H264);
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

	// 解析器，用来解析一帧数据的开头和结尾
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

	uint8_t inbuf[4096 + 64] = { 0 };
	size_t	data_size = 0;
	while (!feof(fp))
	{
		data_size = fread(inbuf, 1, 4096, fp);
		if (!data_size)
			break;

		uint8_t* data = inbuf;
		while (data_size > 0)
		{
			// 解析数据，将data数据写入packet
			// 如果到了一帧结尾，pkt->size会赋值为该帧的大小，否则pkt->size为0
			int ret = av_parser_parse2(codec_parser_ctx, codec_ctx, &pkt->data, &pkt->size,
										data, data_size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
			if (ret < 0)
			{
				printf("error while parsing\n");
				return -1;
			}
			data += ret;
			data_size -= ret;

			if (pkt->size)
			{
				clock_t t1 = clock();
				decode(codec_ctx, frame, pkt, file_name);
				clock_t t2 = clock();
				printf("耗时: %ld\n", t2 - t1);
			}
		}
	}

	// 解码器中残留的数据
	decode(codec_ctx, frame, NULL, file_name);

	return 0;
}