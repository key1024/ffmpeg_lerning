/*
	ffmpeg�ڲ�ͨ��ע��Ļص�����(read_packet)����ȡͼ������
*/
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
	//av_log_set_level(AV_LOG_DEBUG);

	uint8_t* buffer = NULL;
	size_t buffer_size;
	int ret = 0;
	// ���ļ�����ӳ�䵽buffer��
	if ((ret = av_file_map(file_name, &buffer, &buffer_size, 0, NULL)) < 0)
	{
		av_log(NULL, AV_LOG_ERROR, "av_file_map error: %s\n", av_make_error_string(err_buf, sizeof(err_buf), ret));
		return ret;
	}

	struct buffer_data bd = { 0 };
	bd.ptr = buffer;
	bd.size = buffer_size;
	printf("buffer_size: %d\n", buffer_size);

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

	// �����ڴ�IOģʽ����
	// read_packetΪ�����ݵĻص���bd�൱���û����ݣ�����read_packet��ʱ���ͨ������������
	// avio_ctx_bufferΪ�����ȡ���ݵ��ڴ�
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

	fmt_ctx->pb = avio_ctx;

	printf("==========================2\n");

	ret = avformat_open_input(&fmt_ctx, NULL, NULL, NULL);
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

	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = nullptr;
	pkt.size = 0;

	int count = 0;
	// ��ʼ�������᲻��ͨ��read_packet�ص���ȡ���ݲ�����
	while (av_read_frame(fmt_ctx, &pkt) >= 0)
	{
		printf("%04d size: %d\n", ++count, pkt.size);
	}

	printf("====================================\n");

	av_dump_format(fmt_ctx, 0, file_name, 0);

	avformat_close_input(&fmt_ctx);
	avio_context_free(&avio_ctx);
	av_free(avio_ctx_buffer);
	avformat_free_context(fmt_ctx);
	av_file_unmap(buffer, buffer_size);
	return 0;
}