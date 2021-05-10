#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <conio.h>
#elif defined __linux__
#include <termios.h>
#include <unistd.h>
#endif

void press_any_key()
{
#ifdef _WIN32
    printf("\nPress any key to continue...\n");
    getch();
#elif defined __linux__
    struct termios info, info_copy;
    tcgetattr(STDIN_FILENO, &info);

    info_copy = info;

    info.c_lflag &= ~(ICANON | ECHO);
    info.c_cc[VMIN] = 1;
    info.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &info);

    printf("\nPress any key to continue...\n");
    getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &info_copy);
#endif
}

int get_snd_index(char **argv, char **snd_path, char **audio_path)
{
    FILE *f = fopen(argv[1], "rb");

    if (!f) {
        printf("ERROR: Failed to open %s for reading.\n", argv[1]);
        return -1;
    }

    uint32_t version_1;
    fread(&version_1, 4, 1, f);

    fclose(f);

    if (version_1 != 6 || version_1 != 1179011410 || version_1 != 1399285583) {
        printf("Invalid files.\n");
        return -1;
    }

    FILE *f2 = fopen(argv[2], "rb");

    uint32_t version_2;
    fread(&version_2, 4, 1, f);

    fclose(f2);

    switch (version_2) {
        case 6:
            *snd_path = argv[1];
            *audio_path = argv[2];
            break;
        case 1179011410:
        case 1399285583:
            *snd_path = argv[2];
            *audio_path = argv[1];
            break;
        default:
            printf("Invalid files.\n");
            return -1;
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        printf("DOOM Audio ID Finder v1.0\n\n");
        printf("Usage:\n");
        printf("%s [WEM or OGG file] [SND fileC\n", argv[0]);
        printf("Example:\n");
        printf("%s music_000001.wem music.snd\n", argv[0]);
        return 1;
    }

    char *snd_path, *audio_path;

    if (get_snd_index(argv, &snd_path, &audio_path) == -1) {
        press_any_key();
        return 1;
    }

    FILE *audio = fopen(audio_path, "rb");

    if (!audio) {
        printf("ERROR: Failed to open %s for reading.\n", audio_path);
        press_any_key();
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
        press_any_key();
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

        fseek(snd, 12, SEEK_CUR);
        long current_pos = ftell(snd);

        fseek(snd, offset, SEEK_SET);
        unsigned char *snd_audio_bytes = malloc(size);
        fread(snd_audio_bytes, 1, size, snd);

        if (!memcmp(audio_bytes, snd_audio_bytes, audio_size)) {
            printf("Audio file ID: %u\n", id);
            press_any_key();
            return 0;
        }

        free(snd_audio_bytes);

        fseek(snd, current_pos, SEEK_SET);
    }

    free(audio_bytes);

    printf("Audio file not found in %s!\n", snd_path);
    press_any_key();
    return 1;
}