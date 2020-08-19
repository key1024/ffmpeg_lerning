#include <iostream>
#include "DemuxingDecoding.h"
#include "FilteringVideo.h"

// 查看文件夹内容
// windows下不支持此方法
extern int avio_list_dir(const char* dir_name);
extern int avio_reading(const char* file_name);
extern int decode_video(const char* file_name);
extern int encoding_video();
extern int hw_decode(const char* file_name);
extern int muxing(const char* file_name);

int main(void)
{
	// avio_list_dir(".");
	 //avio_reading("bigbuckbunny_480x272.h265");
	//decode_video("aaaa.h264");

	//DemuxingDecoding dd;
	//dd.DemuxingAndDecoding("test.mp4");
	//encoding_video();

	//Filtering filtering;
	//filtering.Run("test.mp4");

	// 显示支持的硬件编码器类型
	//AVHWDeviceType type = AV_HWDEVICE_TYPE_NONE;
	//while ((type = av_hwdevice_iterate_types(type)) != AV_HWDEVICE_TYPE_NONE)
	//	fprintf(stderr, " %s", av_hwdevice_get_type_name(type));
	//fprintf(stderr, "\n");

	//hw_decode("aaaa.h264");

	//AVFormatContext* fmt_ctx = NULL;
	//avformat_open_input(&fmt_ctx, "test.mp4", NULL, NULL);
	//avformat_find_stream_info(fmt_ctx, NULL);
	//AVDictionaryEntry* tag = NULL;
	//while ((tag = av_dict_get(fmt_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
	//{
	//	printf("%s=%s\n", tag->key, tag->value);
	//}
	//avformat_close_input(&fmt_ctx);

	muxing("muxing.mp4");

	return 0;
}