#include <iostream>
#include "DemuxingDecoding.h"

// 查看文件夹内容
// windows下不支持此方法
extern int avio_list_dir(const char* dir_name);
extern int avio_reading(const char* file_name);
extern int decode_video(const char* file_name);
extern int encoding_video();

int main(void)
{
	// avio_list_dir(".");
	// avio_reading("bigbuckbunny_480x272.h265");
	// decode_video("bigbuckbunny_480x272.h265");

	DemuxingDecoding dd;
	dd.DemuxingAndDecoding("bigbuckbunny_480x272.h265");
	//encoding_video();

	return 0;
}