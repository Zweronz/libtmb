#include <stdio.h>
#include <texm.h>


int main(int argc, char** argv)
{
    char resPath[256], texmPath[256], dumpPath[256];

    sprintf(resPath, "%s.RTX", argv[1]);
    sprintf(texmPath, "%s.TEX", argv[1]);
    sprintf(dumpPath, "dumps/%s", argv[1]);

    create_directory("dumps");
    create_directory(dumpPath);

    TEXMRes* res = load_texmres(resPath);
    TEXM* texm = load_texm(texmPath);

    export_texm(res, texm, dumpPath);
    printf("ran successfully! output %u files to %s", texm->numTextures, dumpPath);

    return 0;
}
