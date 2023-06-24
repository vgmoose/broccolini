#ifndef NETWORK_MOCK
#include <curl/curl.h>
#include <curl/easy.h>
#endif

#include <stdio.h>
#include <string>
#include <map>

// the struct to be passed in the write function.
typedef struct
{
    uint8_t *data;
    size_t data_size;
    u_int64_t offset;
    FILE *out;
} ntwrk_struct_t;

#define STATUS_DOWNLOADING	0
#define STATUS_INSTALLING		1
#define STATUS_REMOVING			2
#define STATUS_RELOADING		3
#define STATUS_UPDATING_STATUS	4

// folder stuff
bool mkpath(std::string path);
bool CreateSubfolder(char* cstringpath);

// networking stuff
int init_networking();
bool downloadFileToMemory(std::string path, std::string* buffer, int* httpCode = NULL, std::map<std::string, std::string>* headerResp = NULL); // writes to disk in BUF_SIZE chunks.
bool downloadFileToDisk(std::string remote_path, std::string local_path); // saves file to local_path.

#ifndef NETWORK_MOCK
void setPlatformCurlFlags(CURL* c);
#endif

// callback for networking progress
// if set, will be invoked during the download
extern int (*networking_callback)(void*, double, double, double, double);
extern int (*libget_status_callback)(int, int, int);

// helper methods
const char* plural(int amount);
void cp(const char* from, const char* to);
std::string toLower(const std::string& str);
int remove_empty_dirs(const char* name, int count);

const std::string dir_name(std::string file_path);
bool compareLen(const std::string& a, const std::string& b);
std::string base64_decode(const std::string_view encoded_string);
std::string base64_encode(const std::string_view bytes_to_encode);
std::string just_domain_from_url(const std::string& url);
std::string myReplace(std::string str, std::string substr1, std::string substr2);
bool writeFile(const std::string& path, const std::string& content);
std::string readFile(const std::string& path);
void parseJSON(const std::string& json, std::map<std::string, void*>& map);