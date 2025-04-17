#include <texm.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

TEXMRes* load_texmres(char* path)
{
    File* file = file_open(path);
    TEXMRes* res = (TEXMRes*)malloc(sizeof(TEXMRes));

    /* allocated as single texture so it can reallocated to extend the buffer every iteration */
    res->numTextures = 0;
    res->textures = (TEXMResTexture*)malloc(sizeof(TEXMResTexture));

    for (int i = 0;; i++)
    {
        TEXMResTextureHeader header;
        file_read(&header, sizeof(TEXMResTextureHeader), file);

        /* empty buffer signifies the end of the file */
        if (header.sizeFlags == 0)
        {
            break;
        }

        res->numTextures++;

        /* reallocate buffer, as a fixed size is never declared in the RTX */
        res->textures = realloc(res->textures, sizeof(TEXMResTexture) * res->numTextures);
        res->textures[i].header = header;
        
        /* w/h are stored in a single byte as a power of two multiplier (ie, 8 is 256) */
        uint8_t width = header.sizeFlags & 0xF;
        uint8_t height = (header.sizeFlags >> 4) & 0xF;

        uint16_t numColors = ((1 << width) << height);
        res->textures[i].numColors = numColors;

        /* RTX colors are always 32 bit */
        uint16_t colorTableSize = numColors << 2;

        res->textures[i].table = (RGBAColor*)malloc(colorTableSize);
        file_read(res->textures[i].table, colorTableSize, file);

        /* restructure with offset (idk why you need to do this) */
        if (numColors > 16)
        {
            RGBAColor* restructured = (RGBAColor*)malloc(colorTableSize);
            memcpy(restructured, res->textures[i].table, colorTableSize);
    
            /* seems arbitrary to me, was only able to figure it out by comparing ripped images to original ones */
            for (uint32_t index = header.offset;;)
            {
                if (index + (header.offset * 2) + 1 >= numColors)
                {
                    break;
                }

                for (int j = index; j < index + header.offset; j++)
                {
                    restructured[j] = res->textures[i].table[j + header.offset];
                }

                index += header.offset;

                for (int j = index; j < index + header.offset; j++)
                {
                    restructured[j] = res->textures[i].table[j - header.offset];
                }

                index += header.offset * 3;
            }
    
            /* discard the original table */
            free(res->textures[i].table);
            res->textures[i].table = restructured;
        }
    }

    return res;
}

TEXM* load_texm(char* path)
{
    File* file = file_open(path);
    TEXM* texm = (TEXM*)malloc(sizeof(TEXM));

    /* allocated as single texture so it can reallocated to extend the buffer every iteration */
    texm->numTextures = 0;
    texm->textures = (TEXMTexture*)malloc(sizeof(TEXMTexture));

    /* skip the texm header, as it's unknown what any of the values mean */
    file->pos = 128;

    for (int i = 0;; i++)
    {
        TEXMTextureHeader header;
        file_read(&header, sizeof(TEXMTextureHeader), file);

        texm->numTextures++;

        /* reallocate buffer, as a fixed size is never declared in the TEXM */
        texm->textures = realloc(texm->textures, sizeof(TEXMTexture) * texm->numTextures);
        texm->textures[i].header = header;

        /* w/h are stored in a single byte as a power of two multiplier (ie, 8 is 256) */
        uint8_t width = header.sizeFlags & 0xF;
        uint8_t height = (header.sizeFlags >> 4) & 0xF;
        
        /* the raw size is the header plus the buffer divided by two (or the remaining file) */
        uint32_t textureSize = header.textureSize == 0 ? 0 : ((header.textureSize << 2) - 128);
        uint32_t textureEstimatedSize = (1 << width) * (1 << height);

        bool isLastEntry = false;
        
        /* 0 size marks end of file */
        if (textureSize == 0)
        {
            textureSize = file->size - file->pos;
            isLastEntry = true;
        }

        /* 'estimations' are done until I can be 100% sure the sizes never mismatch
            at which point I will ditch the header's size declaration */
        switch (header.dpsm & 63)
        {
            case PSMCT32: case PSMZ32:
                textureEstimatedSize *= 4;
                break;

            case PSMCT24: case PSMZ24:
                textureEstimatedSize *= 3;
                break;

            case PSMCT16: case PSMCT16S:
            case PSMZ16: case PSMZ16S:
                textureEstimatedSize *= 2;
                break;

            case PSMT4: case PSMT4HL: case PSMT4HH:
                textureEstimatedSize /= 2;
                break;
        }
        
        /* find a more elegant solution */
        if (textureSize != textureEstimatedSize)
        {
            printf("(%i) ERROR CALCULATING SIZE: format: %hhu expected size: %u actual size: %u\n", i, header.dpsm & 63, textureEstimatedSize, textureSize);

            if (textureEstimatedSize == textureSize / 2)
            {
                textureEstimatedSize *= 2;
                
                switch (header.dpsm & 63)
                {
                    case PSMT4:
                        printf("assuming format is PSMT8 (%u %u)\n", textureSize, textureEstimatedSize);
                        texm->textures[i].guessedFormat = PSMT8;
                        break;

                    case PSMT8:
                        printf("assuming format is PSMCT16 (%u %u)\n", textureSize, textureEstimatedSize);
                        texm->textures[i].guessedFormat = PSMCT16;
                        break;

                    default:
                        printf("could not guess format, texture cannot be decoded.\n");

                        /* guessed format being PSUNK means the texture will skip being decoded */
                        texm->textures[i].guessedFormat = PSUNK;
                        textureEstimatedSize = textureSize;

                        break;
                }
            }
            else
            {
                printf("could not guess format, texture cannot be decoded.\n");

                /* guessed format being PSUNK means the texture will skip being decoded */
                texm->textures[i].guessedFormat = PSUNK;
                textureEstimatedSize = textureSize;

                break;
            }
        }
        else
        {
            /* guessed format being PSINV means the texture will use the header's format */
            texm->textures[i].guessedFormat = PSINV;
        }
        
        texm->textures[i].dataSize = textureEstimatedSize;

        /* use the estimated size, if anything is wrong we'll definitely know it. */
        texm->textures[i].data = (uint8_t*)malloc(textureEstimatedSize);
        file_read(texm->textures[i].data, textureEstimatedSize, file);

        if (isLastEntry)
        {
            break;
        }
    }

    return texm;
}

void export_texm(TEXMRes* res, TEXM* texm, char* path)
{
    stbi_flip_vertically_on_write(1);
    printf("%u %u\n", texm->numTextures, res->numTextures);

    for (int i = 0; i < texm->numTextures; i++)
    {
        RGBAColor* data;
        uint8_t format;

        TEXMTexture tex = texm->textures[i];
        TEXMResTexture resTex = res->textures[i + 14];

        _Bool skip = 0;

        if (tex.guessedFormat != PSINV)
        {
            if (tex.guessedFormat == PSUNK)
            {
                continue;
            }

            format = tex.guessedFormat;
        }
        else
        {
            format = tex.header.dpsm & 0x3F;
        }

        switch (format)
        {
            case PSMT4:
            {
                data = (RGBAColor*)calloc(tex.dataSize * 2, sizeof(RGBAColor));
                uint32_t colorIndex = 0;

                for (int j = 0; j < tex.dataSize; j++)
                {
                    data[colorIndex++] = resTex.table[tex.data[j] & 0xF];
                    data[colorIndex++] = resTex.table[tex.data[j] >> 4];
                }

                break;
            }

            case PSMT8:
            {
                data = (RGBAColor*)calloc(tex.dataSize, sizeof(RGBAColor));

                for (int j = 0; j < tex.dataSize; j++)
                {
                    data[j] = resTex.table[tex.data[j]];
                }

                break;
            }

            case PSMCT32:
            {
                data = (RGBAColor*)malloc(tex.dataSize);
                memcpy(data, tex.data, tex.dataSize);

                break;
            }

            default:
            {
                printf("UNHANDLED FORMAT ON IMAGE NUM %i (%hhu)\n", i, format);
                skip = 1;
            }
        }

        if (skip)
        {
            continue;
        }
        
        char fullPath[256];
        sprintf(fullPath, "%s/%i.png", path, i);

        //printf("exporting %i (%hhi)\n", i, tex.header.padding_1[1]);

        stbi_write_png(fullPath, 1 << (tex.header.sizeFlags & 0xF), 1 << ((tex.header.sizeFlags >> 4) & 0xF), 4, data, 0);
        free(data);
    }
}