#include <stdio.h>
#include <unistd.h>
#include <curl/curl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>


struct fileInfo {
	char* fileptr;//文件描述符
	int offset;//偏移量
};

size_t writeFunc(void* ptr, size_t size, size_t memb, void* userdata)
{
	struct fileInfo* info = (struct fileInfo*)userdata;
	printf("writeFunc: %ld\n", size * memb);


	memcpy(info->fileptr + info->offset, ptr, size * memb);
	info->offset += size * memb;

	return size * memb;
}


//获取文件大小
double getDownloadFileLength(const char* url)
{
	double downloadFileLength = 0;
	CURL* curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Safari/537.36");
	curl_easy_setopt(curl, CURLOPT_HEADER, 1);//获得http头部
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1);

	CURLcode res = curl_easy_perform(curl);
	if (res == CURLE_OK) 
	{
		printf("downloadFileLength success\n");
		curl_easy_getinfo(curl, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &downloadFileLength);
	}
	else 
	{
		printf("downloadFileLength error\n");
		downloadFileLength = -1;
	}

	curl_easy_cleanup(curl);

	return downloadFileLength;
}

//下载函数
int download(const char* url, const char* filename)
{
	//获取大小
	double downloadFileLength = getDownloadFileLength(url);

	int fd = open(filename, O_RDWR | O_CREAT);
	if (fd == -1) 
	{
		return -1;
	}

	//移动指针，使得文件被初始化为
	if (-1 == lseek(fd, downloadFileLength - 1, SEEK_SET)) 
	{
		close(fd);
		return -1;
	}

	if (1 != write(fd, "", 1)) {
		perror("write");
		close(fd);
		return -1;
	}

	char* fileptr = (char*)mmap(NULL, downloadFileLength, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (fileptr == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return -1;
	}

	struct fileInfo* info = (struct fileInfo*)malloc(sizeof(struct fileInfo));
	if (info == NULL)
	{
		munmap(fileptr, downloadFileLength);
		close(fd);
		return -1;
	}
	info->fileptr = fileptr;
	info->offset = 0;

	CURL* curl = curl_easy_init();

	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFunc);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fileptr);

	CURLcode res = curl_easy_perform(curl);
	if (res != CURLE_OK) 
	{
		printf("res %d\n", res);
	}

	curl_easy_cleanup(curl);

	free(info);
	munmap(fileptr, downloadFileLength);
	close(fd);
	return 0;
}

// 可以以这两个为例
// https://releases.ubuntu.com/22.04/ubuntu-22.04.2-live-server-amd64.iso.zsync
// ubuntu.zsync
int main(int argc, const char* argv[]) 
{
	if (argc != 3) {
		printf("arg error\n");
		return -1;
	}

	download(argv[1], argv[2]);
}