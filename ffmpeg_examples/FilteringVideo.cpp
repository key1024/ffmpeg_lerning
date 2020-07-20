#include "FilteringVideo.h"

extern "C"
{
#include "libavutil/opt.h"
#include "libavfilter/buffersrc.h"
#include "libavfilter/buffersink.h"
}

Filtering::Filtering()
{
	m_fmt_ctx = nullptr;
	m_video_codec_ctx = nullptr;
	m_buffersink_ctx = nullptr;
	m_buffersrc_ctx = nullptr;
	m_video_stream_index = -1;
}

Filtering::~Filtering()
{
}

int Filtering::Run(const char* file_name)
{
	m_file_name = file_name;

	if (openInputFile() < 0)
		return -1;

	if (initFilters() < 0)
		return -1;

	AVFrame* frame = av_frame_alloc();
	AVFrame* filt_frame = av_frame_alloc();
	if (!frame || !filt_frame)
		return -1;

	AVPacket packet;
	while (true)
	{
		if (av_read_frame(m_fmt_ctx, &packet) < 0)
			break;

		if (packet.stream_index != m_video_stream_index)
			continue;

		int ret = avcodec_send_packet(m_video_codec_ctx, &packet);
		if (ret < 0)
		{
			printf("Error while sending a packet to the decoder\n");
			break;
		}

		while (ret >= 0)
		{
			ret = avcodec_receive_frame(m_video_codec_ctx, frame);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			else if (ret < 0)
			{
				printf("Error while receiving a frame from the decoder.\n");
				return -1;
			}

			frame->pts = frame->best_effort_timestamp;

			if (av_buffersrc_add_frame_flags(m_buffersrc_ctx, frame, AV_BUFFERSRC_FLAG_KEEP_REF) < 0)
			{
				printf("Error while feeding the filtergraph.\n");
				break;
			}

			while (true)
			{
				ret = av_buffersink_get_frame(m_buffersink_ctx, filt_frame);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
					break;
				else if (ret < 0)
				{
					printf("get frame from filtergraph failed.\n");
					return -1;
				}

				// 经过滤镜处理过的图像
				printf("width: %d height: %d linesize: %d pix_fmt: %d\n",
						filt_frame->width, filt_frame->height, filt_frame->linesize[0], filt_frame->format);
				FILE* fp = fopen("aaa.y", "ab+");
				if (fp)
				{
					fwrite(filt_frame->data[0], 1, filt_frame->linesize[0] * filt_frame->height, fp);
					fclose(fp);
				}

				av_frame_unref(filt_frame);
			}
			av_frame_unref(frame);
		}
		av_packet_unref(&packet);
	}

	return 0;
}

int Filtering::openInputFile()
{
	int ret = avformat_open_input(&m_fmt_ctx, m_file_name.c_str(), NULL, NULL);
	if (ret < 0)
	{
		printf("avformat_open_input failed\n");
		return -1;
	}

	ret = avformat_find_stream_info(m_fmt_ctx, NULL);
	if (ret < 0)
	{
		printf("can not find stream information\n");
		avformat_close_input(&m_fmt_ctx);
		return -1;
	}

	AVCodec* codec;
	ret = av_find_best_stream(m_fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
	if (ret < 0)
	{
		printf("can not find a video stream in the input file. ");
		avformat_close_input(&m_fmt_ctx);
		return -1;
	}
	m_video_stream_index = ret;

	m_video_codec_ctx = avcodec_alloc_context3(codec);
	if (m_video_codec_ctx == NULL)
	{
		printf("create codec context failed\n");
		avformat_close_input(&m_fmt_ctx);
		return -1;
	}

	avcodec_parameters_to_context(m_video_codec_ctx, m_fmt_ctx->streams[m_video_stream_index]->codecpar);

	ret = avcodec_open2(m_video_codec_ctx, codec, NULL);
	if (ret < 0)
	{
		printf("open codec failed\n");
		avcodec_free_context(&m_video_codec_ctx);
		avformat_close_input(&m_fmt_ctx);
		return -1;
	}

	return 0;
}

// 初始化滤镜
int Filtering::initFilters()
{
	AVFilterGraph* filter_graph = avfilter_graph_alloc();
	if (filter_graph == NULL)
	{
		printf("create filter graph failed\n");
		return -1;
	}

	AVRational time_base = m_fmt_ctx->streams[m_video_stream_index]->time_base;
	char args[512] = { 0 };
	snprintf(args, sizeof(args),
		"video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
		m_video_codec_ctx->width, m_video_codec_ctx->height, m_video_codec_ctx->pix_fmt,
		time_base.num, time_base.den,
		m_video_codec_ctx->sample_aspect_ratio.num, m_video_codec_ctx->sample_aspect_ratio.den);

	const AVFilter* buffersrc = avfilter_get_by_name("buffer");
	int ret = avfilter_graph_create_filter(&m_buffersrc_ctx, buffersrc, "in", args, NULL, filter_graph);
	if (ret < 0)
	{
		printf("create buffer src failed\n");
		return -1;
	}

	const AVFilter* buffersink = avfilter_get_by_name("buffersink");
	ret = avfilter_graph_create_filter(&m_buffersink_ctx, buffersink, "out", NULL, NULL, filter_graph);
	if (ret < 0)
	{
		printf("create buffer sink failed\n");
		return -1;
	}

	AVPixelFormat pix_fmts[] = { AV_PIX_FMT_GRAY8, AV_PIX_FMT_NONE };
	ret = av_opt_set_int_list(m_buffersink_ctx, "pix_fmts", pix_fmts, AV_PIX_FMT_NONE, AV_OPT_SEARCH_CHILDREN);
	if (ret < 0)
	{
		printf("cannot set output pixel format\n");
		return -1;
	}

	AVFilterInOut* outputs = avfilter_inout_alloc();
	outputs->name = av_strdup("in");
	outputs->filter_ctx = m_buffersrc_ctx;
	outputs->pad_idx = 0;
	outputs->next = NULL;
	AVFilterInOut* inputs = avfilter_inout_alloc();
	inputs->name = av_strdup("out");
	inputs->filter_ctx = m_buffersink_ctx;
	inputs->pad_idx = 0;
	inputs->next = NULL;
	char filter_descr[] = "scale=78:24,transpose=cclock";
	ret = avfilter_graph_parse_ptr(filter_graph, filter_descr, &inputs, &outputs, NULL);
	if (ret < 0)
	{
		printf("parse graph describe failed\n");
		return -1;
	}

	ret = avfilter_graph_config(filter_graph, NULL);
	if (ret < 0)
	{
		printf("avfilter_graph_config error\n");
		return -1;
	}

	return 0;
}