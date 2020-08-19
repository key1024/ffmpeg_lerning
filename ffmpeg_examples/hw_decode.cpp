extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/pixdesc.h"
#include "libavutil/hwcontext.h"
#include "libavutil/opt.h"
#include "libavutil/avassert.h"
#include "libavutil/imgutils.h"
}

#include <time.h>

static AVPixelFormat hw_pix_fmt;
static AVBufferRef* hw_device_ctx = NULL;

static enum AVPixelFormat get_hw_format(AVCodecContext* ctx,
	const enum AVPixelFormat* pix_fmts)
{
	const enum AVPixelFormat* p;

	for (p = pix_fmts; *p != -1; p++) {
		if (*p == hw_pix_fmt)
			return *p;
	}

	fprintf(stderr, "Failed to get HW surface format.\n");
	return AV_PIX_FMT_NONE;
}

int hw_decode(const char* file_name)
{
	AVFormatContext* fmt_ctx = NULL;

	int ret = avformat_open_input(&fmt_ctx, file_name, NULL, NULL);
	if (ret != 0) {
		fprintf(stderr, "Cannot open input file '%s'\n",file_name);
		return -1;
	}

	avformat_find_stream_info(fmt_ctx, NULL);

	AVCodec* decoder = NULL;
	ret = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
	if (ret < 0) {
		fprintf(stderr, "Cannot find a video stream in the input file\n");
		return -1;
	}

	int video_stream = ret;

	for (int i = 0;; i++)
	{
		const AVCodecHWConfig* config = avcodec_get_hw_config(decoder, i);
		if (!config) {
			fprintf(stderr, "Decoder %s does not support device type.\n",
				decoder->name);
			return -1;
		}
		if (config->methods & AV_CODEC_HW_CONFIG_METHOD_HW_DEVICE_CTX
			&& config->device_type == AV_HWDEVICE_TYPE_CUDA) {
			hw_pix_fmt = config->pix_fmt;
			break;
		}
	}

	AVCodecContext* decoder_ctx = avcodec_alloc_context3(decoder);
	if (!decoder) {
		printf("avcodec_alloc_context3 failed.\n");
		return -1;
	}

	AVStream* video = fmt_ctx->streams[video_stream];

	avcodec_parameters_to_context(decoder_ctx, video->codecpar);

	decoder_ctx->get_format = get_hw_format;

	av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_CUDA, NULL, NULL, 0);
	decoder_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);

	avcodec_open2(decoder_ctx, decoder, NULL);

	AVPacket packet;
	AVFrame* frame = av_frame_alloc();
	AVFrame* hw_frame = av_frame_alloc();
	while (true) {
		if ((ret = av_read_frame(fmt_ctx, &packet)) < 0)
			break;

		clock_t t1 = clock();
		if (packet.stream_index == video_stream) {
			avcodec_send_packet(decoder_ctx, &packet);
			while (true)
			{
				ret = avcodec_receive_frame(decoder_ctx, frame);
				if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
					break;

				AVFrame* tmp_frame = NULL;
				if (frame->format == hw_pix_fmt)
				{
					av_hwframe_transfer_data(hw_frame, frame, 0);
					tmp_frame = hw_frame;
				}
				else
					tmp_frame = frame;

				int size = av_image_get_buffer_size((AVPixelFormat)tmp_frame->format, tmp_frame->width,
														tmp_frame->height, 1);

				uint8_t* buffer = (uint8_t*)av_malloc(size);
				ret = av_image_copy_to_buffer(buffer, size, (const uint8_t* const*)tmp_frame->data,
					tmp_frame->linesize, (AVPixelFormat)tmp_frame->format, tmp_frame->width, tmp_frame->height, 1);

				av_free(buffer);
			}
		}

		av_packet_unref(&packet);

		clock_t t2 = clock();
		printf("ºÄÊ±: %ld\n", t2 - t1);
	}

	av_frame_free(&frame);
	av_frame_free(&hw_frame);

	avcodec_free_context(&decoder_ctx);
	avformat_close_input(&fmt_ctx);
	av_buffer_unref(&hw_device_ctx);

	return 0;
}