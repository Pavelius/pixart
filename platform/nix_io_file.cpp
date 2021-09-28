#include <cstdio>
#include <unistd.h>
#include "crt.h"
#include "io_stream.h"

void printcnf(const char* text) {
	fwrite(text, 1, zlen(text), stdout);
}

io::file::file() : handle(0) {
}

io::file::file(const char* url, unsigned flags) : handle(0) {
	create(url, flags);
}

io::file::~file() {
	close();
}

io::file::find::find(const char* url) {
	path[0] = 0; zcpy(path, url);
	char temp[261];
	zcpy(temp, path);
	zcat(temp, "/*.*");
}

const char* io::file::find::fullname(char* result) {
	zcpy(result, path);
	if(result[0])
		zcat(result, "/");
	zcat(result, name());
	return result;
}

void io::file::find::next() {
	if(!handle)
		return;
}

io::file::find::~find() {
//	if(handle)
//		FindClose(handle);
//	handle = 0;
}

const char* io::file::find::name() {
	return "";
	//return ((WIN32_FIND_DATA*)&reserved)->cFileName;
}

bool io::file::create(const char* url, unsigned flags) {
	if(handle)
		return true;
	const char* mode;
	switch(flags & (StreamWrite|StreamRead)) {
	case StreamRead: mode = "rb"; break;
	case StreamWrite: mode = "wb"; break;
	case StreamRead|StreamWrite: mode = "rwb"; break;
	}
	handle = std::fopen(url, mode);
	return (*this);
}

void io::file::close() {
	if(handle){
		std::fclose((std::FILE*)handle);
		handle = 0;
	}
}

char* io::file::getdir(char* url, int size) {
	return getcwd(url, size);
}

char* io::file::getmodule(char* url, int size) {
	return url;
}

bool io::file::setdir(const char* url) {
	return chdir(url) == 0;
}

bool io::file::remove(const char* url) {
	return ::remove(url)==0;
}

int io::file::read(void* p, int size) {
	return std::fread(p, 1, size, (FILE*)handle);
}

int io::file::write(const void* p, int size) {
	return std::fwrite(p, 1, size, (FILE*)handle);
}

int io::file::seek(int pos, int rel) {
    switch(rel) {
    case SeekEnd: rel = SEEK_END; break;
    case SeekCur: rel = SEEK_CUR; break;
    case SeekSet: rel = SEEK_SET; break;
    }
    if(std::fseek((FILE*)handle, pos, rel))
        return 0;
	return ftell((FILE*)handle);
}

bool io::file::exist(const char* url) {
	return access(url, F_OK ) == 0;
}

bool io::file::makedir(const char* url) {
	return false;
}

bool io::file::getfullurl(const char* short_url, char* url, int size) {
	return false;
}
