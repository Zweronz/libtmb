#pragma once

#include <zip.h>

typedef struct File
{
    uint8_t* buf;

    size_t size, pos;
} File;

File* file_open(char* path);
void file_read(void* buf, size_t len, File* file);