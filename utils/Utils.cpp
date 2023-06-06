// from https://github.com/vgmoose/libget/blob/master/src/Utils.cpp
// operating system level utilities
// contains directory utils, http utils, and helper methods

#if defined(__WIIU__)
#include <nsysnet/socket.h>
#include <nsysnet/nssl.h>
#include <nn/ac.h>
#endif

#if defined(WII)
#include <wiisocket.h>
#endif

#if defined(SWITCH)
#include <switch.h>
#endif

#if defined(_3DS)
#include <3ds.h>
#include <malloc.h>
#endif

#include <algorithm>
#include <cstdint>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <ctime>

#include "Utils.hpp"


#define BUF_SIZE 0x800000 //8MB.


int (*networking_callback)(void*, double, double, double, double);

// reference to the curl handle so that we can re-use the connection
#ifndef NETWORK_MOCK
CURL* curl = NULL;
#endif

#define SOCU_ALIGN 0x1000
#define SOCU_BUFFERSIZE 0x100000

#if defined(__WIIU__)
NSSLContextHandle nsslctx;
#endif

#if defined(_3DS)
u32* SOCUBuffer;
#endif

bool CreateSubfolder(char* cpath)
{
	std::string path(cpath);
	return mkpath(path);
}

// http://stackoverflow.com/a/11366985
bool mkpath(std::string path)
{
	bool bSuccess = false;
	int nRC = ::mkdir(path.c_str(), 0775);
	if (nRC == -1)
	{
		switch (errno)
		{
		case ENOENT:
			//parent didn't exist, try to create it
			if (mkpath(path.substr(0, path.find_last_of('/'))))
				//Now, try to create again.
				bSuccess = 0 == ::mkdir(path.c_str(), 0775);
			else
				bSuccess = false;
			break;
		case EEXIST:
			//Done!
			bSuccess = true;
			break;
		default:
			bSuccess = false;
			break;
		}
	}
	else
		bSuccess = true;
	return bSuccess;
}

#ifndef NETWORK_MOCK
void setPlatformCurlFlags(CURL* c)
{
#if defined(__WIIU__)
  // enable ssl support (TLSv1 only)
	curl_easy_setopt(c, CURLOPT_NSSL_CONTEXT, nsslctx);
	curl_easy_setopt(c, (CURLoption)211, 0);

	// network optimizations
	curl_easy_setopt(c, (CURLoption)213, 1);
	curl_easy_setopt(c, (CURLoption)212, 0x8000);
#endif

#if defined(SWITCH) || defined(WII)
  // ignore cert verification (TODO: not have to do this in the future)
  curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 0L);
  curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 0L);
#endif
}
#endif

static size_t MemoryWriteCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
 	((std::string*)userp)->append((char*)contents, size * nmemb);
 	return size * nmemb;
}

static size_t DiskWriteCallback(void* contents, size_t size, size_t num_files, void* userp)
{
	ntwrk_struct_t *data_struct = (ntwrk_struct_t *)userp;
    size_t realsize = size * num_files;

	if (realsize + data_struct->offset >= data_struct->data_size)
    {
        fwrite(data_struct->data, data_struct->offset, 1, data_struct->out);
        data_struct->offset = 0;
    }

    memcpy(&data_struct->data[data_struct->offset], contents, realsize);
    data_struct->offset += realsize;
    data_struct->data[data_struct->offset] = 0;
    return realsize;
}

// https://gist.github.com/alghanmi/c5d7b761b2c9ab199157
// if data_struct is specified, file will go straight to disk as it downloads
bool downloadFileCommon(std::string path, std::string* buffer = NULL, ntwrk_struct_t* data_struct = NULL)
{
#ifndef NETWORK_MOCK
	CURLcode res;

	if (!curl)
		return false;

  setPlatformCurlFlags(curl);

	curl_easy_setopt(curl, CURLOPT_URL, path.c_str());
	curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, networking_callback);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);

	// set user agent
	const char* userAgent = "Mozilla/5.0 (Generic; Chesto) litehtml/0.8 (KHTML, like Gecko) Broccolini/0.0";

	curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent);

	bool skipDisk = data_struct == NULL;

	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, skipDisk ? MemoryWriteCallback : DiskWriteCallback);

	if (skipDisk)
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer);
	else
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, data_struct);

	return curl_easy_perform(curl) == CURLE_OK;
#else
  return true;
#endif
}

bool downloadFileToMemory(std::string path, std::string* buffer)
{
	return downloadFileCommon(path, buffer, NULL);
}

bool downloadFileToDisk(std::string remote_path, std::string local_path)
{
	FILE *out_file = fopen(local_path.c_str(), "wb");
	if (!out_file)
		return false;

	uint8_t *buf = (uint8_t *)malloc(BUF_SIZE); // 8MB.
	if (buf == NULL)
	{
		fclose(out_file);
		return false;
	}

	ntwrk_struct_t data_struct = { buf, BUF_SIZE, 0, out_file };

	if (!downloadFileCommon(remote_path, NULL, &data_struct))
	{
		free(data_struct.data);
		fclose(data_struct.out);
		return false;
	}

	// write remaning data to file before free.
    fwrite(data_struct.data, data_struct.offset, 1, data_struct.out);
    free(data_struct.data);
	fclose(data_struct.out);

	return true;
}

const char* plural(int amount)
{
	return (amount == 1) ? "" : "s";
}

const std::string dir_name(std::string file_path)
{
	// turns "/hi/man/thing.txt to /hi/man"
	size_t pos = file_path.find_last_of("/");

	// no "/" in string, return empty string
	if (pos == std::string::npos)
		return "";

	return file_path.substr(0, pos);
}

// sorting function: put bigger strings at the front
bool compareLen(const std::string& a, const std::string& b)
{
	return (a.size() > b.size());
}

int init_networking()
{
#if defined(SWITCH)
	socketInitializeDefault();
#endif
#if defined(_3DS)
	SOCUBuffer = (u32*)memalign(SOCU_ALIGN, SOCU_BUFFERSIZE);
	socInit(SOCUBuffer, SOCU_BUFFERSIZE);
#endif
#if defined(__WIIU__)
	nn::ac::ConfigIdNum configId;

	// setup network connection
	nn::ac::Initialize();
	nn::ac::GetStartupId(&configId);
	nn::ac::Connect(configId);

	// init socket lib
	socket_lib_init();

	// init nintendo ssl lib
	NSSLInit();
	nsslctx = NSSLCreateContext(0);
#endif
#if defined(WII)
	// TODO: network initialization on the wii is *extremly* slow (~10s)
	// It's probably a good idea to use wiisocket_init_async and
	// show something on the screen during that interval
	wiisocket_init();
#endif

#ifndef NETWORK_MOCK
	curl_global_init(CURL_GLOBAL_ALL);

	// init our curl handle
	curl = curl_easy_init();

#endif
	return 1;
}

int deinit_networking()
{
#ifndef NETWORK_MOCK
	curl_easy_cleanup(curl);
	curl_global_cleanup();
#endif

#if defined(__WIIU__)
	NSSLDestroyContext(nsslctx);
	NSSLFinish();
	socket_lib_finish();
	nn::ac::Finalize();
#endif
#if defined(WII)
	wiisocket_deinit();
#endif

	return 1;
}

void cp(const char* from, const char* to)
{
	std::ifstream src(from, std::ios::binary);
	std::ofstream dst(to, std::ios::binary);

	dst << src.rdbuf();
}

std::string toLower(const std::string& str)
{
	std::string lower;
	transform(str.begin(), str.end(), std::back_inserter(lower), tolower);
	return lower;
}

int remove_empty_dirs(const char* name, int count)
{
	// from incoming path, recursively ensure all directories are deleted
	// return the number of files remaining (0 if totally erased and successful)

	// based on https://stackoverflow.com/a/8438663

	int starting_count = count;

	DIR* dir;
	struct dirent* entry;

	// already deleted
	if (!(dir = opendir(name)))
		return true;

	// go through files in directory
	while ((entry = readdir(dir)) != NULL)
	{
		if (entry->d_type == DT_DIR)
		{
			char path[1024];
			// skip current dir or parent dir
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;

			// update path so far
			snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);

			// recursively go into this directory too
			count += remove_empty_dirs(path, count);
		}
		else
		{
			// file found, increase file count
			count++;
		}
	}

	// now that we've been through this directory, check if it was an empty directory
	if (count == starting_count)
	{
		// empty, try rmdir (should only remove if empty anyway)
		rmdir(name);
	}

	closedir(dir);

	// return number of files at this level (total count minus starting)
	return count - starting_count;
}
