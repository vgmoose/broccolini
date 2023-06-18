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

#include <map>
#include <algorithm>
#include <cstring>
#include <regex>

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

// record the headers into a map
size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
	std::map<std::string, std::string>* headerResp = (std::map<std::string, std::string>*)userdata;

	std::string header(buffer);
	std::string::size_type pos = header.find(':');
	if (std::string::npos == pos) {
		// Malformed header?
		return nitems * size;
	}
	
	std::string name = header.substr(0, pos);
	std::string value = header.substr(pos + 1);

	// Remove whitespace
	value.erase(value.begin(), std::find_if(value.begin(), value.end(), [](int ch) {
		return !std::isspace(ch);
	}));
	// https://stackoverflow.com/a/313990/4953343
	std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c){ return std::tolower(c); });
	(*headerResp)[name] = value;

	return nitems * size;
}

// https://gist.github.com/alghanmi/c5d7b761b2c9ab199157
// if data_struct is specified, file will go straight to disk as it downloads
bool downloadFileCommon(
	std::string path,
	std::string* buffer = NULL,
	ntwrk_struct_t* data_struct = NULL,
	int* http_code = NULL,
	std::map<std::string, std::string>* headerResp = NULL
)
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

	// get header info as map
	curl_slist* curlHeaders = NULL;
	if (headerResp != NULL) {
		// iterate through each header from the curl resp
		curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
		curl_easy_setopt(curl, CURLOPT_HEADERDATA, headerResp);
	}

	if (skipDisk)
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer);
	else
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, data_struct);

	bool ret = curl_easy_perform(curl) == CURLE_OK;
	// get status code info
	if (http_code != NULL) {
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, http_code);
	}

	return ret;
#else
  return true;
#endif
}

bool downloadFileToMemory(
	std::string path,
	std::string* buffer,
	int* httpCode,
	std::map<std::string, std::string>* headerResp
)
{
	return downloadFileCommon(path, buffer, NULL, httpCode, headerResp);
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

// https://stackoverflow.com/a/44562527/4953343
std::string base64_decode(const std::string_view in) {
  // table from '+' to 'z'
  const uint8_t lookup[] = {
      62,  255, 62,  255, 63,  52,  53, 54, 55, 56, 57, 58, 59, 60, 61, 255,
      255, 0,   255, 255, 255, 255, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
      10,  11,  12,  13,  14,  15,  16, 17, 18, 19, 20, 21, 22, 23, 24, 25,
      255, 255, 255, 255, 63,  255, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
      36,  37,  38,  39,  40,  41,  42, 43, 44, 45, 46, 47, 48, 49, 50, 51};
  static_assert(sizeof(lookup) == 'z' - '+' + 1);

  std::string out;
  int val = 0, valb = -8;
  for (uint8_t c : in) {
    if (c < '+' || c > 'z')
      break;
    c -= '+';
    if (lookup[c] >= 64)
      break;
    val = (val << 6) + lookup[c];
    valb += 6;
    if (valb >= 0) {
      out.push_back(char((val >> valb) & 0xFF));
      valb -= 8;
    }
  }
  return out;
}


std::string base64_encode(const std::string_view in) {
  // table from '+' to 'z'
  const char lookup[] =
	  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  std::string out;
  out.reserve(((in.size() + 2) / 3) * 4);

  uint32_t val = 0;
  int32_t valb = -6;
  for (uint8_t c : in) {
	val = (val << 8) + c;
	valb += 8;
	while (valb >= 0) {
	  out.push_back(lookup[(val >> valb) & 0x3F]);
	  valb -= 6;
	}
  }
  if (valb > -6) {
	out.push_back(lookup[((val << 8) >> (valb + 8)) & 0x3F]);
  }
  while (out.size() % 4) {
	out.push_back('=');
  }
  return out;
}

std::string just_domain_from_url(const std::string& url)
{
	// use regex to grab between protocol and first slash
	std::regex domain_regex("^(?:https?:\\/\\/)?(?:[^@\\/\\n]+@)?(?:www\\.)?([^:\\/\\n]+)");
	std::smatch domain_match;

	if (std::regex_search(url, domain_match, domain_regex))
	{
		return domain_match[1];
	}

	return "n/a";
}