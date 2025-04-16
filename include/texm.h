#pragma once

#include <tmb.h>
#include <file.h>

/* 32 bit */
#define PSMCT32  0
#define PSMZ32   48

/* 24 bit */
#define PSMCT24  1
#define PSMZ24   49

/* 16 bit */
#define PSMCT16  2
#define PSMCT16S 10
#define PSMZ16   50
#define PSMZ16S  58

/* 8 bit */
#define PSMT8    19
#define PSMT8H   27

/* 4 bit */
#define PSMT4    20
#define PSMT4HL  36
#define PSMT4HH  44

/* invalid, not real ps2 pixel formats */
#define PSINV    59
#define PSUNK    60


typedef struct RGBAColor
{
    uint8_t r, g, b, a;
} RGBAColor;

typedef struct TEXMResTextureHeader
{
    uint32_t textureSize;

    ALIGNMENT_PADDING(0, 6);

    uint16_t dbp;

    uint8_t texFlags, sizeFlags,
    offset, unknown;
} TEXMResTextureHeader;

typedef struct TEXMResTexture
{
    TEXMResTextureHeader header;

    uint16_t numColors;

    RGBAColor* table;
} TEXMResTexture;

typedef struct TEXMRes
{
    uint32_t numTextures;

    TEXMResTexture* textures;
} TEXMRes;

typedef struct TEXMTextureHeader
{
    uint32_t textureSize;

    ALIGNMENT_PADDING(0, 8);

    uint8_t dpsm, sizeFlags;

    ALIGNMENT_PADDING(1, 112);
}  TEXMTextureHeader;

typedef struct TEXMTexture
{
    TEXMTextureHeader header;

    uint8_t guessedFormat;

    size_t dataSize;

    uint8_t* data;
} TEXMTexture;

typedef struct TEXM
{
    uint32_t numTextures;

    TEXMTexture* textures;
} TEXM;

EXPORT TEXMRes* load_texmres(char* path);
EXPORT TEXM* load_texm(char* path);

EXPORT void export_texm(TEXMRes* res, TEXM* texm, char* path);