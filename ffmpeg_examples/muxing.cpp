#include <stdio.h>

extern "C"
{
#include "libavutil/avassert.h"
#include "libavutil/channel_layout.h"
#include "libavutil/opt.h"
#include "libavutil/mathematics.h"
#include "libavutil/timestamp.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

int muxing(const char* file_name)
{
	AVFormatContext* fmt_ctx;
	avformat_alloc_output_context2(&fmt_ctx, NULL, NULL, file_name);
	if (!fmt_ctx)
	{
		printf("avformat_alloc_output_context2 failed. filename: %s\n", file_name);
		return -1;
	}

	AVOutputFormat* fmt = fmt_ctx->oformat;

	/*********************************************/
	/*                add stream                 */
	/*********************************************/
	AVCodec* codec = avcodec_find_encoder(fmt->video_codec);
	if (!codec)
	{
		printf("avcodec_find_decoder failed: %s\n", avcodec_get_name(fmt->video_codec));
		return -1;
	}

	AVStream* stream = avformat_new_stream(fmt_ctx, NULL);
	if (!stream)
	{
		printf("avformat_new_stream failed.\n");
		return -1;
	}
	stream->id = fmt_ctx->nb_streams - 1;

	AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
	if (!codec_ctx)
	{
		printf("avcodec_alloc_context3 failed\n");
		return -1;
	}

	codec_ctx->codec_id = fmt->video_codec;
	codec_ctx->bit_rate = 400*1000;
	codec_ctx->width = 352;
	codec_ctx->height = 288;
	stream->time_base = AVRational{ 1,25 };
	codec_ctx->time_base = AVRational{ 1,25 };
	codec_ctx->framerate = AVRational{ 25,1 };
	codec_ctx->gop_size = 12;
	codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
	// 不加这个参数无法用windows media播放
	if (fmt->flags & AVFMT_GLOBALHEADER)
		codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

	if (codec->id == AV_CODEC_ID_H264)
		av_opt_set(codec_ctx->priv_data, "preset", "slow", 0);
	/*********************************************/

	/*********************************************/
	/*                open video                 */
	/*********************************************/
	int ret = avcodec_open2(codec_ctx, codec, NULL);
	AVFrame* frame = av_frame_alloc();
	frame->format = codec_ctx->pix_fmt;
	frame->width = codec_ctx->width;
	frame->height = codec_ctx->height;
	av_frame_get_buffer(frame, 32);

	avcodec_parameters_from_context(stream->codecpar, codec_ctx);
	/*********************************************/

	av_dump_format(fmt_ctx, 0, file_name, 1);

	if (!(fmt_ctx->flags & AVFMT_NOFILE)) {
		avio_open(&fmt_ctx->pb, file_name, AVIO_FLAG_WRITE);
	}

	avformat_write_header(fmt_ctx, NULL);

	/*********************************************/
	/*                write video                */
	/*********************************************/
	int pts = 0;

	for (int i = 0; i < 300; i++) {
		
		av_frame_make_writable(frame);

		/* Y */
		for (int y = 0; y < frame->height; y++)
			for (int x = 0; x < frame->width; x++)
				frame->data[0][y * frame->linesize[0] + x] = x + y + pts * 3;

		/* Cb and Cr */
		for (int y = 0; y < frame->height / 2; y++) {
			for (int x = 0; x < frame->width / 2; x++) {
				frame->data[1][y * frame->linesize[1] + x] = 128 + y + pts * 2;
				frame->data[2][y * frame->linesize[2] + x] = 64 + x + pts * 5;
			}
		}
		frame->pts = pts++;

		AVPacket packet = { 0 };
		av_init_packet(&packet);
		int got_packet = 0;
		avcodec_encode_video2(codec_ctx, &packet, frame, &got_packet);
		if(got_packet)
		{
			av_packet_rescale_ts(&packet, codec_ctx->time_base, stream->time_base);
			packet.stream_index = stream->index;

			AVRational* time_base = &fmt_ctx->streams[packet.stream_index]->time_base;
			char buffer[AV_TS_MAX_STRING_SIZE] = { 0 };
			printf("pts:%s pts_time:%s dts:%s dts_time:%s duration:%s duration_time:%s stream_index:%d\n",
				av_ts_make_string(buffer, packet.pts), av_ts_make_time_string(buffer, packet.pts, time_base),
				av_ts_make_string(buffer, packet.pts), av_ts_make_time_string(buffer, packet.dts, time_base),
				av_ts_make_string(buffer, packet.pts), av_ts_make_time_string(buffer, packet.duration, time_base),
				packet.stream_index);

			av_interleaved_write_frame(fmt_ctx, &packet);

			av_packet_unref(&packet);
		}
	}
	/*********************************************/

	av_write_trailer(fmt_ctx);

	avformat_free_context(fmt_ctx);

	return 0;
}