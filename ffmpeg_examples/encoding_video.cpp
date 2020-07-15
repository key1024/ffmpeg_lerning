/*
	�����ı�������ֻ������������
*/
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

int encoding_video()
{
	AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (!codec)
	{
		printf("���ұ�����ʧ��\n");
		return -1;
	}

	AVCodecContext* ctx = avcodec_alloc_context3(codec);
	if (!ctx)
	{
		printf("��������������ʧ��: %d\n", codec->id);
		return -1;
	}

	// ����(400*1000����400kbs)
	ctx->bit_rate = 400*1000;
	ctx->width = 352;
	ctx->height = 288;
	ctx->time_base = AVRational{ 1,25 };
	ctx->framerate = AVRational{ 25,1 };
	ctx->gop_size = 10;
	ctx->max_b_frames = 1;
	ctx->pix_fmt = AV_PIX_FMT_YUV420P;

	if (codec->id == AV_CODEC_ID_H264)
		av_opt_set(ctx->priv_data, "preset", "slow", 0);

	// �򿪱�����
	int ret = avcodec_open2(ctx, codec, NULL);
	if (ret < 0)
	{
		char err_buffer[1024] = { 0 };
		printf("�򿪱�����ʧ��: %s\n", av_make_error_string(err_buffer, 1024, ret));
		return -1;
	}

	AVPacket* pkt = av_packet_alloc();
	if (!pkt)
	{
		printf("����avpacketʧ��\n");
		return -1;
	}

	AVFrame* frame = av_frame_alloc();
	if (!frame)
	{
		printf("����avframeʧ��\n");
		return -1;
	}
	frame->format = ctx->pix_fmt;
	frame->width = ctx->width;
	frame->height = ctx->height;

	// ����frame�洢������Ҫ���ڴ�
	// 32��ʾ�ڴ�Ҫ����32�ֽڶ���
	ret = av_frame_get_buffer(frame, 32);
	if (ret < 0)
	{
		char err_buffer[1024] = { 0 };
		printf("����frame data ʧ��: %s\n", av_make_error_string(err_buffer, 1024, ret));
		return -1;
	}

	FILE* f = fopen("test.h264", "wb");
	if (!f) {
		fprintf(stderr, "Could not open %s\n", "test.avi");
		exit(1);
	}

	for (int i = 0; i < 250; i++)
	{
		ret = av_frame_make_writable(frame);
		if (ret < 0)
			break;

		for (int y = 0; y < ctx->height; y++) {
			for (int x = 0; x < ctx->width; x++) {
				frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
			}
		}

		/* Cb and Cr */
		for (int y = 0; y < ctx->height / 2; y++) {
			for (int x = 0; x < ctx->width / 2; x++) {
				frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
				frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
			}
		}

		frame->pts = i;

		ret = avcodec_send_frame(ctx, frame);
		if (ret < 0) {
			fprintf(stderr, "Error sending a frame for encoding\n");
			exit(1);
		}

		while (ret >= 0)
		{
			ret = avcodec_receive_packet(ctx, pkt);
			if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
				break;
			else if(ret < 0)
			{
				fprintf(stderr, "Error during encoding\n");
				exit(1);
			}

			printf("Write packet %3(size=%5d)\n", pkt->pts, pkt->size);
			fwrite(pkt->data, 1, pkt->size, f);
			av_packet_unref(pkt);
		}
	}

	uint8_t endcode[] = { 0, 0, 1, 0xb7 };
	if (codec->id == AV_CODEC_ID_MPEG1VIDEO || codec->id == AV_CODEC_ID_MPEG2VIDEO)
		fwrite(endcode, 1, sizeof(endcode), f);

	fclose(f);
	avcodec_free_context(&ctx);
	av_frame_free(&frame);
	av_packet_free(&pkt);

	return 0;
}