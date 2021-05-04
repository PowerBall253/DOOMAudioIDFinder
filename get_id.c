#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int main(int argc, char **argv)
{
    if (argc < 3) {
        printf("DOOM Audio ID Finder v1.0\n\n");
        printf("Usage:\n");
        printf("%s [WEM or OGG file] [SND file]\n\n", argv[0]);
        printf("Example:\n");
        printf("%s music_000001.wem music.snd\n", argv[0]);
        return 1;
    }

    char *audio_path = argv[1];
    char *snd_path = argv[2];

    FILE *audio = fopen(audio_path, "rb");

    if (!audio) {
        printf("ERROR: Failed to open %s for reading.\n", audio_path);
        return 1;
    }

    fseek(audio, 0, SEEK_END);
    long audio_size = ftell(audio);
    fseek(audio, 0, SEEK_SET);

    unsigned char *audio_bytes = malloc(audio_size);
    fread(audio_bytes, 1, audio_size, audio);

    fclose(audio);

    FILE *snd = fopen(snd_path, "rb");

    if (!snd) {
        printf("ERROR: Failed to open %s for reading.\n", snd_path);
        return 1;
    }

    fseek(snd, 4, SEEK_SET);

    uint32_t info_size;
    fread(&info_size, 4, 1, snd);

    uint32_t header_size;
    fread(&header_size, 4, 1, snd);

    fseek(snd, header_size, SEEK_CUR);

    for (int i = 0; i < (info_size - header_size) / 32; i++) {
        fseek(snd, 8, SEEK_CUR);

        uint32_t id;
        fread(&id, 4, 1, snd);

        uint32_t size;
        fread(&size, 4, 1, snd);

        if (size != audio_size) {
            fseek(snd, 16, SEEK_CUR);
            continue;
        }

        uint32_t offset;
        fread(&offset, 4, 1, snd);

        fseek(snd, 4, SEEK_CUR);
        uint16_t format;
        fread(&format, 2, 1, snd);

        fseek(snd, 6, SEEK_CUR);
        long current_pos = ftell(snd);

        fseek(snd, offset, SEEK_SET);
        unsigned char *snd_audio_bytes = malloc(size);
        fread(snd_audio_bytes, 1, size, snd);

        if (!memcmp(audio_bytes, snd_audio_bytes, audio_size)) {
            printf("Audio file ID: %u\n", id);
            return 0;
        }

        free(snd_audio_bytes);

        fseek(snd, current_pos, SEEK_SET);
    }

    free(audio_bytes);

    printf("Audio file not found in %s!\n", snd_path);
    return 1;
}