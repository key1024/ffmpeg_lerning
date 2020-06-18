extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

int encoding_video()
{
	char codec_name[8] = "avi";
	AVCodec* codec = avcodec_find_encoder_by_name("avi");
	if (!codec)
	{
		printf("查找%s对应的编码器失败\n", codec_name);
		return -1;
	}

	AVCodecContext* ctx = avcodec_alloc_context3(codec);
	if (!ctx)
	{
		printf("创建编码上下文失败: %d\n", codec->id);
		return -1;
	}

	// 码率(400*1000代表400kbs)
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

	// 打开编码器
	int ret = avcodec_open2(ctx, codec, NULL);
	if (ret < 0)
	{
		char err_buffer[1024] = { 0 };
		printf("打开编码器失败: %s\n", av_make_error_string(err_buffer, 1024, ret));
		return -1;
	}

	AVPacket* pkt = av_packet_alloc();
	if (!pkt)
	{
		printf("创建avpacket失败\n");
		return -1;
	}

	AVFrame* frame = av_frame_alloc();
	if (!frame)
	{
		printf("创建avframe失败\n");
		return -1;
	}
	frame->format = ctx->pix_fmt;
	frame->width = ctx->width;
	frame->height = ctx->height;

	// 申请frame存储数据需要的内存
	// 32表示内存要按照32字节对齐
	ret = av_frame_get_buffer(frame, 32);
	if (ret < 0)
	{
		char err_buffer[1024] = { 0 };
		printf("申请frame data 失败: %s\n", av_make_error_string(err_buffer, 1024, ret));
		return -1;
	}



	return 0;
}