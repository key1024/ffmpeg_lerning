#include <stdint.h>

extern "C"
{
#include "libavutil/file.h"
#include "libavutil/log.h"
#include "libavformat/avformat.h"
}

struct buffer_data {
	uint8_t* ptr;
	size_t size;
};

static int read_packet(void* opaque, uint8_t* buf, int buf_size)
{
	struct buffer_data* bd = (struct buffer_data*)opaque;
	buf_size = FFMIN(buf_size, bd->size);

	if (!buf_size)
		return AVERROR_EOF;
	printf("ptr:%p size:%zu\n", bd->ptr, bd->size);

	/* copy internal buffer data to buf */
	memcpy(buf, bd->ptr, buf_size);
	bd->ptr += buf_size;
	bd->size -= buf_size;

	return buf_size;
}

int avio_reading(const char* file_name)
{
	printf("======================1\n");

	char err_buf[64] = { 0 };
	av_log_set_level(AV_LOG_DEBUG);

	uint8_t* buffer = NULL;
	size_t buffer_size;
	int ret = 0;
	if ((ret = av_file_map(file_name, &buffer, &buffer_size, 0, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "av_file_map error: %s\n", av_make_error_string(err_buf, sizeof(err_buf), ret));
		return ret;
	}

	struct buffer_data bd = { 0 };
	bd.ptr = buffer;
	bd.size = buffer_size;

	AVFormatContext* fmt_ctx = avformat_alloc_context();
	if (fmt_ctx == NULL)
	{
		printf("avformat_alloc_context error\n");
		av_file_unmap(buffer, buffer_size);
		return -1;
	}

	size_t avio_ctx_buffer_size = 4096;
	uint8_t* avio_ctx_buffer = (uint8_t*)av_malloc(avio_ctx_buffer_size);
	if (avio_ctx_buffer == NULL)
	{
		printf("av_malloc error\n");
		avformat_free_context(fmt_ctx);
		av_file_unmap(buffer, buffer_size);
		return -1;
	}

	AVIOContext* avio_ctx = avio_alloc_context(avio_ctx_buffer, avio_ctx_buffer_size, 
												0, &bd, &read_packet, NULL, NULL);
	if (avio_ctx == NULL)
	{
		printf("avio_alloc_context error\n");
		av_free(avio_ctx_buffer);
		avformat_free_context(fmt_ctx);
		av_file_unmap(buffer, buffer_size);
		return -1;
	}

	//fmt_ctx->pb = avio_ctx;

	printf("==========================2\n");

	ret = avformat_open_input(&fmt_ctx, file_name, NULL, NULL);
	if (ret < 0)
	{
		printf("avformat_open_input error.\n");
		avio_context_free(&avio_ctx);
		av_free(avio_ctx_buffer);
		avformat_free_context(fmt_ctx);
		av_file_unmap(buffer, buffer_size);
		return -1;
	}

	printf("========================3\n");

	ret = avformat_find_stream_info(fmt_ctx, NULL);
	if (ret < 0)
	{
		printf("avformat_find_stream_info error\n");
		avformat_close_input(&fmt_ctx);
		avio_context_free(&avio_ctx);
		av_free(avio_ctx_buffer);
		avformat_free_context(fmt_ctx);
		av_file_unmap(buffer, buffer_size);
		return -1;
	}

	printf("codec id: %d\n", fmt_ctx->streams[0]->codec->codec_id);
	printf("====================================\n");

	av_dump_format(fmt_ctx, 0, file_name, 0);

	avformat_close_input(&fmt_ctx);
	avio_context_free(&avio_ctx);
	av_free(avio_ctx_buffer);
	avformat_free_context(fmt_ctx);
	av_file_unmap(buffer, buffer_size);
	return 0;
}