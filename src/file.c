#include <file.h>

File* file_open(char* path)
{
    File* newFile = (File*)malloc(sizeof(File));
    newFile->pos = 0;

    FILE* file = fopen(path, "rb");

    if (is_zip(file))
    {
        ZipHeader header = zip_read_header(file);
        newFile->size = header.inflatedSize;

        newFile->buf = zip_inflate_all(header, file);
    }
    else
    {
        fseek(file, 0, SEEK_END);
        newFile->size = ftell(file);
    
        fseek(file, 0, SEEK_SET);
    
        newFile->buf = (uint8_t*)malloc(newFile->size);
        fread(newFile->buf, 1, newFile->size, file);
    }

    return newFile;
}

void file_read(void* buf, size_t len, File* file)
{
    memcpy(buf, file->buf + file->pos, len);
    file->pos += len;
}

void create_directory(char* path)
{
#ifdef _WIN32
    _mkdir(path);
#else
    mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
}