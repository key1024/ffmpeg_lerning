#pragma once

extern "C"
{
#include "libavformat/avformat.h"
#include "libavfilter/avfilter.h"
}

#include <string>

class Filtering
{
public:
	Filtering();
	~Filtering();

	int Run(const char* file_name);

private:
	int openInputFile();
	// ³õÊ¼»¯ÂË¾µ
	int initFilters();

	std::string m_file_name;
	AVFormatContext* m_fmt_ctx;
	AVCodecContext* m_video_codec_ctx;
	int m_video_stream_index;
	AVFilterContext* m_buffersrc_ctx;
	AVFilterContext* m_buffersink_ctx;
};