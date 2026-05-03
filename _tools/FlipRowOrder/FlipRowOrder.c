#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Pouziti: %s <soubor.bmp>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    
    FILE *f = fopen(filename, "rb+");
    if (!f) {
        printf("Chyba: Nelze otevrit soubor '%s'.\n", filename);
        return 1;
    }

    uint8_t signature[2];
    fread(signature, 1, 2, f);
    if (signature[0] != 'B' || signature[1] != 'M') {
        printf("Chyba: Soubor '%s' neni platny BMP soubor.\n", filename);
        fclose(f);
        return 1;
    }

    uint32_t data_offset;
    int32_t width, height;
    uint16_t bpp; 

    fseek(f, 0x0A, SEEK_SET);
    fread(&data_offset, sizeof(uint32_t), 1, f);

    fseek(f, 0x12, SEEK_SET);
    fread(&width, sizeof(int32_t), 1, f);
    fread(&height, sizeof(int32_t), 1, f);

    fseek(f, 0x1C, SEEK_SET);
    fread(&bpp, sizeof(uint16_t), 1, f);

    int32_t abs_height = height < 0 ? -height : height;

    int row_padded = ((width * bpp + 31) / 32) * 4;

    printf("Analyzovan obrazek: %dx%d, %d bpp\n", width, abs_height, bpp);
    printf("Fyzicke prevraceni obrazovych dat...\n");

    uint8_t *row_top = (uint8_t *)malloc(row_padded);
    uint8_t *row_bottom = (uint8_t *)malloc(row_padded);

    if (!row_top || !row_bottom) {
        printf("Chyba: Nedostatek pameti.\n");
        fclose(f);
        return 1;
    }

    for (int y = 0; y < abs_height / 2; y++) {
        long offset_top = data_offset + y * row_padded;
        long offset_bottom = data_offset + (abs_height - 1 - y) * row_padded;

        fseek(f, offset_top, SEEK_SET);
        fread(row_top, 1, row_padded, f);

        fseek(f, offset_bottom, SEEK_SET);
        fread(row_bottom, 1, row_padded, f);

        fseek(f, offset_top, SEEK_SET);
        fwrite(row_bottom, 1, row_padded, f);

        fseek(f, offset_bottom, SEEK_SET);
        fwrite(row_top, 1, row_padded, f);
    }

    free(row_top);
    free(row_bottom);

    int32_t new_height = -height;
    
    fseek(f, 0x16, SEEK_SET);
    fwrite(&new_height, sizeof(int32_t), 1, f);

    printf("Hotovo. Data prevracena a vyska v hlavicce zmenena z %d na %d.\n", height, new_height);

    fclose(f);
    return 0;
}
