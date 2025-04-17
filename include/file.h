#pragma once

#include <zip.h>

#ifdef _WIN32
    #include <direct.h>
#else
    #include <sys/stat.h>
#endif

typedef struct File
{
    uint8_t* buf;

    size_t size, pos;
} File;

File* file_open(char* path);
void file_read(void* buf, size_t len, File* file);

void create_directory(char* path);