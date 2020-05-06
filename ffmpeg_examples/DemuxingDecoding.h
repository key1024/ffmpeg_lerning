#ifndef _DEMUXING_DECODING_H_
#define _DEMUXING_DECODING_H_

extern "C"
{
#include "libavformat/avformat.h"
}

#include <string>

class DemuxingDecoding
{
public:
	DemuxingDecoding();

	int DemuxingAndDecoding(const char *file_name);

private:

	int openCodecContext(AVMediaType type);
	int decodePacket();
	int decodeVideoPacket(const AVPacket *packet);

	std::string src_file_name;
	AVFormatContext* fmt_ctx;
	int video_stream_idx;
	AVCodecContext* video_codec_ctx;
	AVStream* video_stream;

	char err_buffer[AV_ERROR_MAX_STRING_SIZE];
	uint8_t* video_dst_data[4];
	int video_dst_linesize[4];
};

#endif // !_DEMUXING_DECODING_H_