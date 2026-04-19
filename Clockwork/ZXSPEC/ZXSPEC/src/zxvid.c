/* zxvid.c
 *
 * ZX Spectrum emulator
 * Video handling routines.
 */

#include "zxem.h"
#include "debug.h"
#include <string.h> // PŘIDÁNO: Nutné pro memset

// Definice cílových rozměrů
#define ZOOM_W 320
#define ZOOM_H 240
#define NATIVE_W 256
#define NATIVE_H 192

void ZX_Draw(int nLine) {
    /* Draw the screen */
    static uint8_t flash_flip = 0;
    static uint8_t flash_count = 0;
    
    // PŘIDÁNO: Statická proměnná pro sledování změny barvy borderu
    static int old_border = -1;

    int x = 0, y = 0;
    int i = 0;
    int j = 0;
    uint8_t yp = 0;

    uint8_t ink = 0;
    uint8_t paper = 0;
    uint8_t flash = 0;
    uint8_t bright = 0;
    uint8_t attrib = 0;

    uint16_t pixel_addr = 0;
    uint16_t attrib_addr = 0;
    uint8_t pix;

    // --- NOVÝ KÓD: Kontrola změny borderu na začátku snímku ---
    if (nLine == 0) {
        if (border != old_border) {
            // Pokud se barva změnila, přemázneme celou obrazovku novou barvou
            DrawClearColZx(palette[border]);

            // DŮLEŽITÉ: Musíme zneplatnit cache, abychom donutili emulator
            // znovu vykreslit herní plochu přes nové pozadí.
            // (Jinak by si myslel, že se nic nezměnilo a nechal by tam díru)
            extern uint8_t cached[6912];
            memset(cached, 255, 6912);

            old_border = border;
        }
    }
    // ----------------------------------------------------------

    // Dynamický výpočet posunu podle toho, zda je zapnutý ZOOM
    int current_w = bZoom125 ? ZOOM_W : NATIVE_W;
    int current_h = bZoom125 ? ZOOM_H : NATIVE_H;
    
    int shift_x = (WIDTH - current_w) / 2;
    int shift_y = (HEIGHT - current_h) / 2;

    y = nLine;
    attrib_addr = 6144 + y * 32;
    yp = y * 8;

    pixel_addr = ((yp >> 3) << 5) & 0xFF;
    pixel_addr += ((yp & 0x7) + (((yp >> 6) & 0x3) << 3)) << 8;

    for (x = 0; x < 32; x++) {
        attrib = membank[4][attrib_addr];
        ink = attrib & 0x7;
        paper = (attrib >> 3) & 0x7;
        bright = (attrib >> 6) & 0x1;

        if (bright) {
            ink += 8;
            paper += 8;
        }

        flash = (attrib & 0x80);
        if (flash && flash_flip) {
            paper = paper ^ ink;
            ink = paper ^ ink;
            paper = paper ^ ink;
        }

        for (j = 0; j < 8; j++) {
            if (flash || cachedread(attrib_addr) || cachedread(pixel_addr + j * 256)) {
                cachedwrite(pixel_addr + j * 256, membank[4][pixel_addr + j * 256]);
                pix = membank[4][pixel_addr + j * 256];

                if (bZoom125) {
                    // --- REŽIM ZOOM 1.25x ---
                    int src_y = yp + j;
                    int dst_y1 = (src_y * 5) / 4 + shift_y;
                    int dst_y2 = ((src_y + 1) * 5) / 4 + shift_y;
                    int h = dst_y2 - dst_y1;

                    for (i = 7; i >= 0; i--) {
                        int src_x = x * 8 + (7 - i);
                        int dst_x1 = (src_x * 5) / 4 + shift_x;
                        int dst_x2 = ((src_x + 1) * 5) / 4 + shift_x;
                        int w = dst_x2 - dst_x1;

                        uint8_t color = palette[(pix & (1 << i)) ? ink : paper];

                        if (w == 1 && h == 1) {
                            DrawPointZx(dst_x1, dst_y1, color);
                        } else {
                            DrawRectZx(dst_x1, dst_y1, w, h, color);
                        }
                    }
                } else {
                    // --- REŽIM 1:1 (ORIGINÁL) ---
                    int draw_y = yp + j + shift_y;
                    int block_x_start = x * 8 + shift_x;

                    for (i = 7; i >= 0; i--) {
                        int draw_x = block_x_start + (7 - i);
                        uint8_t color = palette[(pix & (1 << i)) ? ink : paper];
                        DrawPointZx(draw_x, draw_y, color);
                    }
                }
            }
        }
        cachedwrite(attrib_addr, membank[4][attrib_addr]);
        attrib_addr++;
        pixel_addr++;
    }

    if (nLine == 0) {
        flash_count++;
    }
    if (flash_count > 16) {
        flash_flip = !flash_flip;
        flash_count = 0;
    }
}
