#include <iostream>

// �鿴�ļ�������
// windows�²�֧�ִ˷���
extern int avio_list_dir(const char* dir_name);
extern int avio_reading(const char* file_name);
extern int decode_video(const char* file_name);

int main(void)
{
	// avio_list_dir(".");
	// avio_reading("bigbuckbunny_480x272.h265");
	decode_video("bigbuckbunny_480x272.h265");

	return 0;
}