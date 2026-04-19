// ****************************************************************************
//
//                          Display controller ILI9488
//
// ****************************************************************************

#if USE_ILI9488		// use ILI9488 TFT display (ili9488.c, ili9488.h)

#ifndef _ILI9488_H
#define _ILI9488_H

#ifdef __cplusplus
extern "C" {
#endif

// ILI9488 resolution 320x320 on Picocalc Clockwork
#ifndef WIDTH
#define WIDTH		320		// display width
#endif

#ifndef HEIGHT
#define HEIGHT		320		// display height
#endif

#ifndef COLBITS
#define COLBITS		16		// number of output color bits
#endif

#define COLTYPE		u16		// type of color: u8, u16 or u32
#define FRAMETYPE	u16		// type of frame entry: u8 or u16
#define WIDTHLEN	WIDTH		// length of one line of one plane, in number of frame elements
#define FRAMESIZE 	(WIDTHLEN*HEIGHT) // frame size in number of colors
#define	DISP_STRIP_NUM	1		// number of back strips

// Colors (same as ST7789 RGB565)
#define COLOR_B_BITS	5			// blue color bits
#define COLOR_G_BITS	6			// greem color bits
#define COLOR_R_BITS	5			// red color bits
#define COLOR_B_MASK	0x001F			// blue color mask
#define COLOR_G_MASK	0x07E0			// green color mask
#define COLOR_R_MASK	0xF800			// red color mask
#define COLOR(r,g,b)	((u16)( (((r)&0xf8)<<8) | (((g)&0xfc)<<3) | (((b)&0xf8)>>3) ))

// Backlight control
#define BACKLIGHT_SLICE	PWM_GPIOTOSLICE(DISP_BLK_PIN) // backlight slice index
#define BACKLIGHT_CHAN	PWM_GPIOTOCHAN(DISP_BLK_PIN) // backlight channel index
#define BACKLIGHT_RATE	22050		// backlight rate [Hz]
#define BACKLIGHT_CLOCK	(BACKLIGHT_RATE*256) // PWM clock

#ifndef DISP_OFFSET_X
#define DISP_OFFSET_X	0		// offset of display controller buffer X
#endif

#ifndef DISP_OFFSET_Y
#define DISP_OFFSET_Y	0		// offset of display controller buffer Y
#endif

// Pin definitions fallback (if not defined in device config)
#ifndef DISP_MISO_PIN
#define DISP_MISO_PIN 12
#endif

// Frame buffer in RGB 5-6-5 pixel format
extern ALIGNED FRAMETYPE FrameBuf[];

// Update variables
extern int DispDirtyX1, DispDirtyX2, DispDirtyY1, DispDirtyY2;

#define pDrawBuf FrameBuf	// current draw buffer
#define DispStripInx  0		// current index of back buffer strip
#define DispMinY  0		// minimal Y; base of back buffer strip
#define DispMaxY HEIGHT		// maximal Y + 1; end of back buffer strip

// Functions interface matches ST7789
void DispRotation(u8 rot);
void DispWindow(u16 x1, u16 x2, u16 y1, u16 y2);
void DispStartImg(u16 x1, u16 x2, u16 y1, u16 y2);
void DispSendImg(u8 data);
void DispSendImg2(u16 data); // Sends RGB565 as RGB666 if needed
void DispStopImg();

// --- Compatibility functions for lib_draw (Strip management) ---
INLINE void DispSetStrip(int inx) {}
INLINE void DispSetStripNext() {}
INLINE void DispSetStripOff() {}
INLINE void DispLoad() {}
INLINE void VgaWaitVSync() {}
// ---------------------------------------------------------------

void DispDirtyAll();
void DispDirtyNone();
void DispDirtyRect(int x, int y, int w, int h);
void DispDirtyPoint(int x, int y);
void DispUpdate();
void DispAutoUpdate(u32 ms);
void DispUpdateAll();
void DispBacklight(u8 backlight);
void DispBacklightUpdate();
void DispInit(u8 rot);
void DispTerm();

// Direct draw text
void DispDrawText(const char* text, int x, int y, int w1, int w2, u16 col, u16 bgcol);
void DispDrawTextRow(const char* text, int x, int y, u16 col, u16 bgcol);

// Test function
void DispTest(int stepbystep);

#ifdef __cplusplus
}
#endif

#endif // _ILI9488_H

#endif // USE_ILI9488
