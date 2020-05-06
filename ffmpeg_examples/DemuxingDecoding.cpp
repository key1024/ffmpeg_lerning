#include "DemuxingDecoding.h"
#include <iostream>

extern "C"
{
#include "libavutil/imgutils.h"
}

DemuxingDecoding::DemuxingDecoding()
{
	fmt_ctx = nullptr;
	video_stream_idx = -1;
	video_codec_ctx = nullptr;
	video_stream = nullptr;
}

int DemuxingDecoding::DemuxingAndDecoding(const char* file_name)
{
	src_file_name = file_name;

	int ret = avformat_open_input(&fmt_ctx, src_file_name.c_str(), NULL, NULL);
	if (ret < 0)
	{
		std::cout << "avformat_open_input error: " << src_file_name << std::endl;
		return -1;
	}

	ret = avformat_find_stream_info(fmt_ctx, NULL);
	if (ret < 0)
	{
		std::cout << "avformat_find_stream_info error" << std::endl;
		return -1;
	}

	ret = openCodecContext(AVMEDIA_TYPE_VIDEO);
	if (ret < 0)
		return -1;

	ret = av_image_alloc(video_dst_data, video_dst_linesize, 
		video_codec_ctx->width, video_codec_ctx->height, video_codec_ctx->pix_fmt, 1);
	if (ret < 0)
	{
		std::cout << "could not allocate raw video buffer\n";
		return -1;
	}

	av_dump_format(fmt_ctx, 0, src_file_name.c_str(), 0);

	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = nullptr;
	pkt.size = 0;

	while (av_read_frame(fmt_ctx, &pkt) >= 0)
	{
		if (pkt.stream_index == video_stream_idx)
			decodeVideoPacket(&pkt);
	}

	return 0;
}

int DemuxingDecoding::openCodecContext(AVMediaType type)
{
	int ret = av_find_best_stream(fmt_ctx, type, -1, -1, nullptr, 0);
	if (ret < 0)
	{
		std::cout << "could not find " << av_get_media_type_string(type)
			<< "stream in input file '" << src_file_name << "'\n";
		return -1;
	}
	video_stream_idx = ret;
	video_stream = fmt_ctx->streams[video_stream_idx];
	AVCodec* codec = avcodec_find_decoder(video_stream->codecpar->codec_id);// ���id��codec->codec_id��һ����
	if (!codec)
	{
		std::cout << "failed to find " << av_get_media_type_string(type) << " codec\n";
		return -1;
	}

	video_codec_ctx = avcodec_alloc_context3(codec);
	if (!video_codec_ctx)
	{
		std::cout << "failed to allocate the " << av_get_media_type_string(type) << " codec context\n";
		return -1;
	}

	ret = avcodec_parameters_to_context(video_codec_ctx, video_stream->codecpar);
	if (ret < 0)
	{
		std::cout << "failed to copy " << av_get_media_type_string(type) 
			<< " codec parameters to decoder context\n";
		return -1;
	}
	
	ret = avcodec_open2(video_codec_ctx, codec, NULL);
	if (ret < 0)
	{
		std::cout << "failed to open " << av_get_media_type_string(type) << " codec\n";
		return -1;
	}

	return 0;
}

int DemuxingDecoding::decodePacket()
{


	return 0;
}

int DemuxingDecoding::decodeVideoPacket(const AVPacket* packet)
{
	AVFrame* frame = av_frame_alloc();
	if (!frame)
	{
		std::cout << "alloc frame failed\n";
		return -1;
	}

	int got_frame = 0;
	int ret = avcodec_decode_video2(video_codec_ctx, frame, &got_frame, packet);
	if (ret < 0)
	{
		std::cout << "error decoding video frame: "
			<< av_make_error_string(err_buffer, AV_ERROR_MAX_STRING_SIZE, ret) << std::endl;
		return -1;
	}

	int video_frame_count = 0;
	if (got_frame)
	{
		std::cout << "video_frame n:" << video_frame_count++ 
			<< " coded_n:" << frame->coded_picture_number << std::endl;

		av_image_copy(video_dst_data, video_dst_linesize, (const uint8_t**)frame->data, 
			frame->linesize, (AVPixelFormat)frame->format, frame->width, frame->height);
	}

	return 0;
}