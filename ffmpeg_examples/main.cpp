#include <iostream>

// 查看文件夹内容
// windows下不支持此方法
extern int avio_list_dir(const char* dir_name);
extern int avio_reading(const char* file_name);

int main(void)
{
	// avio_list_dir(".");
	avio_reading("bigbuckbunny_480x272.h265");

	return 0;
}