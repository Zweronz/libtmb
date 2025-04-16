#include <zip.h>
#include <zlib.h>
#include <stdlib.h>

_Bool is_zip(FILE* file)
{
    uint32_t signature;
    fread(&signature, 4, 1, file);
    
    if (signature != 67323209)
    {
        fpos_t pos = 0;
        fsetpos(file, &pos);

        return 0;
    }

    return 1;
}

ZipHeader zip_read_header(FILE* file)
{
    fpos_t pos = 18;
    fsetpos(file, &pos);

    ZipHeader header;
    fread(&header, sizeof(ZipHeader), 1, file);

    fgetpos(file, &pos);

    /* file name is declared in the header?? */
    pos += header.fileNameLength;
    
    fseek(file, 0, SEEK_END);
    uint32_t fileSize = ftell(file);

    fseek(file, 0, SEEK_SET);
    fsetpos(file, &pos);

    header.deflatedSize = fileSize - pos;
    
    return header;
}

void* zip_inflate_all(ZipHeader header, FILE* file)
{
    z_stream stream;

    stream.zalloc = 0;
    stream.zfree = 0;
    stream.opaque = 0;
    stream.avail_in = 0;
    stream.next_in = 0;

    inflateInit2(&stream, -MAX_WBITS);
    uint8_t* deflatedData = (uint8_t*)malloc(header.deflatedSize);

    /* read all the deflated data */
    fread(deflatedData, 1, header.deflatedSize, file);

    stream.avail_in = header.deflatedSize;
    stream.next_in = deflatedData;

    /* allocate the inflated data */
    uint8_t* inflatedData = (uint8_t*)malloc(header.inflatedSize);
    
    stream.avail_out = header.inflatedSize;
    stream.next_out = inflatedData;
    
    /* inflate the entire file */
    inflate(&stream, Z_NO_FLUSH);
    inflateEnd(&stream);

    free(deflatedData);

    return inflatedData;
}