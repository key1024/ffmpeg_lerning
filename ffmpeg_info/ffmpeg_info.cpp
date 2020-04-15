// ffmpeg_test.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
extern "C"
{
#include "./libavformat/avformat.h"
#include "./libavcodec/avcodec.h"
#include "./libavfilter/avfilter.h"
}

void configurationInfo()
{
    av_register_all();
    printf("%s\n", avcodec_configuration());
}

void urlProtocolInfo()
{
    av_register_all();

    struct URLProtocol* pup = NULL;

    while (1)
    {
        const char* tmp = avio_enum_protocols((void**)&pup, 0);
        if (tmp)
            printf("[in ][%10s]\n", tmp);
        else
            break;
    }

    if (pup)
    {
        free(pup);
        pup = NULL;
    }

    while (1)
    {
        const char* tmp = avio_enum_protocols((void**)&pup, 1);
        if (tmp)
            printf("[out][%10s]\n", tmp);
        else
            break;
    }

    if (pup)
    {
        free(pup);
        pup = NULL;
    }

}

void avFormatInfo()
{
    av_register_all();

    AVInputFormat* if_temp = av_iformat_next(NULL);
    while (if_temp)
    {
        printf("[in ][%10s]\n", if_temp->name);
        if_temp = if_temp->next;
    }
    AVOutputFormat* of_temp = av_oformat_next(NULL);
    while (if_temp)
    {
        printf("[out][%10s]\n", of_temp->name);
        of_temp = of_temp->next;
    }
}

void avCodecInfo()
{
    av_register_all();

    AVCodec* c_temp = av_codec_next(NULL);
    while (c_temp)
    {
        char info[64] = { 0 };
        if (c_temp->decode)
            sprintf(info, "[Dec]");
        else
            sprintf(info, "[Enc]");

        switch (c_temp->type)
        {
        case AVMEDIA_TYPE_VIDEO:
            sprintf(info, "%s[Video]", info);
            break;
        case AVMEDIA_TYPE_AUDIO:
            sprintf(info, "%s[Audio]", info);
            break;
        default:
            sprintf(info, "%s[Other]", info);
            break;
        }

        printf("%s[%10s]\n", info, c_temp->name);
        c_temp = c_temp->next;
    }
}

void avFliterInfo()
{
    avfilter_register_all();

    AVFilter* f_temp = (AVFilter*)avfilter_next(NULL);
    while (f_temp)
    {
        printf("%s\n", f_temp->name);
        f_temp = f_temp->next;
    }
}

int main()
{
    printf("configuration info:\n");
    configurationInfo();

    printf("url protocol info:\n");
    urlProtocolInfo();

    printf("av format info:\n");
    avFormatInfo();

    printf("av codec info:\n");
    avCodecInfo();

    printf("av fliter info:\n");
    avFliterInfo();

    return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
