#pragma once

#include <tmb.h>

typedef struct ZipHeader
{
    uint32_t deflatedSize, inflatedSize, fileNameLength;
} ZipHeader;

ZipHeader zip_read_header(FILE* file);
void* zip_inflate_all(ZipHeader header, FILE* file);

_Bool is_zip(FILE* file);