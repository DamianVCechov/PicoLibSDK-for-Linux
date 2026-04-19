// ****************************************************************************
//
//                          Display controller ILI9488
//
// ****************************************************************************

#include "../../global.h"	// globals

#if USE_ILI9488		// use ILI9488 TFT display

#include "ili9488.h"
#include "../../_sdk/inc/sdk_spi.h"
#include "../../_sdk/inc/sdk_gpio.h"
#include "../../_sdk/inc/sdk_timer.h"
#include "../../_sdk/inc/sdk_pwm.h"
#include "../../_sdk/inc/sdk_dma.h"
#include "../../_lib/inc/lib_config.h"
#include "../../_devices/picopad/picopad_ss.h"
#include "../../_lib/inc/lib_text.h"
#include "../../_lib/inc/lib_draw.h"

//#define ILI9488_USE_16BIT_SPI	1

// ILI9488 commands
#define ILI9488_NOP     0x00
#define ILI9488_SWRESET 0x01
#define ILI9488_RDDID   0x04
#define ILI9488_RDDST   0x09

#define ILI9488_SLPIN   0x10
#define ILI9488_SLPOUT  0x11
#define ILI9488_PTLON   0x12
#define ILI9488_NORON   0x13

#define ILI9488_INVOFF  0x20
#define ILI9488_INVON   0x21
#define ILI9488_DISPOFF 0x28
#define ILI9488_DISPON  0x29
#define ILI9488_CASET   0x2A
#define ILI9488_RASET   0x2B
#define ILI9488_RAMWR   0x2C
#define ILI9488_RAMRD   0x2E

#define ILI9488_MADCTL  0x36
#define ILI9488_COLMOD  0x3A

#define ILI9488_FRMCTR1 0xB1
#define ILI9488_DISCTRL 0xB6
#define ILI9488_PWCTRL1 0xC0
#define ILI9488_PWCTRL2 0xC1
#define ILI9488_VMCTRL1 0xC5
#define ILI9488_PGAMCTRL 0xE0
#define ILI9488_NGAMCTRL 0xE1

// MADCTL bits
#define ILI9488_MADCTL_MY  0x80 // Row Address Order
#define ILI9488_MADCTL_MX  0x40 // Column Address Order
#define ILI9488_MADCTL_MV  0x20 // Row/Column Exchange
#define ILI9488_MADCTL_ML  0x10 // Vertical Refresh Order
#define ILI9488_MADCTL_BGR 0x08 // RGB-BGR Order
#define ILI9488_MADCTL_MH  0x04 // Horizontal Refresh Order

// GPIO Macros
#define CS_ON	GPIO_Out0(DISP_CS_PIN); cb()
#define CS_OFF	cb(); GPIO_Out1(DISP_CS_PIN)
#define RES_ON	GPIO_Out0(DISP_RES_PIN); cb()
#define RES_OFF	cb(); GPIO_Out1(DISP_RES_PIN)
#define DC_CMD	GPIO_Out0(DISP_DC_PIN); cb()
#define DC_DATA	GPIO_Out1(DISP_DC_PIN); cb()

#if USE_FRAMEBUF
// frame buffer in RGB 5-6-5 pixel format
ALIGNED u16 FrameBuf[FRAMESIZE];
#endif

// Rotation table
const u8 RotationTab[4] = {
	ILI9488_MADCTL_MX | ILI9488_MADCTL_BGR, // 0: Portrait
	ILI9488_MADCTL_MV | ILI9488_MADCTL_BGR, // 1: Landscape
	ILI9488_MADCTL_MY | ILI9488_MADCTL_BGR, // 2: Inverted Portrait
	ILI9488_MADCTL_MX | ILI9488_MADCTL_MY | ILI9488_MADCTL_MV | ILI9488_MADCTL_BGR, // 3: Inverted Landscape
};

// dirty window to update
int DispDirtyX1, DispDirtyX2, DispDirtyY1, DispDirtyY2;
u32 DispAutoUpdateLast;

// Write command
void DispWriteCmd(u8 cmd)
{
	CS_ON; DC_CMD;
	SPI_Send8(DISP_SPI, &cmd, 1);
	CS_OFF;
}

// Write data
void DispWriteData(const void* data, int len)
{
	CS_ON; DC_DATA;
	SPI_Send8(DISP_SPI, data, len);
	CS_OFF;
}

// Write command and data
void DispWriteCmdData(u8 cmd, const void* data, int len)
{
	CS_ON; DC_CMD;
	SPI_Send8(DISP_SPI, &cmd, 1);
	DC_DATA;
	SPI_Send8(DISP_SPI, data, len);
	CS_OFF;
}

void DispHardReset()
{
	// Robust reset sequence
	CS_ON; 
	RES_OFF;
	WaitMs(20);
	RES_ON;
	WaitMs(50); // Longer reset pulse
	RES_OFF;
	WaitMs(150); // Longer wait for initialization
	CS_OFF;
}

void DispSoftReset()
{
	DispWriteCmd(ILI9488_SWRESET);
	WaitMs(150); // Must be >120ms
}

void DispRotation(u8 rot)
{
	DispWriteCmdData(ILI9488_MADCTL, &RotationTab[rot & 3], 1);
}

void DispWindow(u16 x1, u16 x2, u16 y1, u16 y2)
{
	u8 buf[4];
	
	// Column Address Set
	buf[0] = (x1 + DISP_OFFSET_X) >> 8;
	buf[1] = (x1 + DISP_OFFSET_X) & 0xFF;
	buf[2] = (x2 - 1 + DISP_OFFSET_X) >> 8;
	buf[3] = (x2 - 1 + DISP_OFFSET_X) & 0xFF;
	DispWriteCmdData(ILI9488_CASET, buf, 4);

	// Row Address Set
	buf[0] = (y1 + DISP_OFFSET_Y) >> 8;
	buf[1] = (y1 + DISP_OFFSET_Y) & 0xFF;
	buf[2] = (y2 - 1 + DISP_OFFSET_Y) >> 8;
	buf[3] = (y2 - 1 + DISP_OFFSET_Y) & 0xFF;
	DispWriteCmdData(ILI9488_RASET, buf, 4);

	DispWriteCmd(ILI9488_RAMWR);
}

// Start sending image data
void DispStartImg(u16 x1, u16 x2, u16 y1, u16 y2)
{
	// Dummy byte to sync if needed
	u8 d = 0xff;
	CS_OFF; DC_CMD;
	SPI_Send8(DISP_SPI, &d, 1);

	DispWindow(x1, x2, y1, y2);
	CS_ON; DC_DATA;
}

// Send one byte (raw)
void DispSendImg(u8 data)
{
	while (SPI_TxIsFull(DISP_SPI)) {}
	SPI_Write(DISP_SPI, data);
	SPI_RxFlush(DISP_SPI);
}

// Send one pixel (u16 RGB565) - handles 18-bit conversion for ILI9488
void DispSendImg2(u16 data)
{
#ifdef ILI9488_USE_16BIT_SPI
	// Native 16-bit support
	while (SPI_TxIsFull(DISP_SPI)) {}
	SPI_Write(DISP_SPI, (u8)(data >> 8));
	while (SPI_TxIsFull(DISP_SPI)) {}
	SPI_Write(DISP_SPI, (u8)data);
#else
	// Convert RGB565 to RGB666 (3 bytes)
	u8 r = (data & 0xF800) >> 8;
	u8 g = (data & 0x07E0) >> 3;
	u8 b = (data & 0x001F) << 3;

	while (SPI_TxIsFull(DISP_SPI)) {}
	SPI_Write(DISP_SPI, r);
	while (SPI_TxIsFull(DISP_SPI)) {}
	SPI_Write(DISP_SPI, g);
	while (SPI_TxIsFull(DISP_SPI)) {}
	SPI_Write(DISP_SPI, b);
#endif
	SPI_RxFlush(DISP_SPI);
}

void DispStopImg()
{
	while (SPI_IsBusy(DISP_SPI)) SPI_RxFlush(DISP_SPI);
	SPI_RxFlush(DISP_SPI);
	CS_OFF;
}

// Dirty Rect management
void DispDirtyAll() { DispDirtyX1 = 0; DispDirtyX2 = WIDTH; DispDirtyY1 = 0; DispDirtyY2 = HEIGHT; }
void DispDirtyNone() { DispDirtyX1 = WIDTH; DispDirtyX2 = 0; DispDirtyY1 = HEIGHT; DispDirtyY2 = 0; }
void DispDirtyRect(int x, int y, int w, int h)
{
	if (x < 0) { w += x; x = 0; }
	if (x + w > WIDTH) w = WIDTH - x;
	if (w <= 0) return;
	if (y < 0) { h += y; y = 0; }
	if (y + h > HEIGHT) h = HEIGHT - y;
	if (h <= 0) return;
	if (x < DispDirtyX1) DispDirtyX1 = x;
	if (x + w > DispDirtyX2) DispDirtyX2 = x + w;
	if (y < DispDirtyY1) DispDirtyY1 = y;
	if (y + h > DispDirtyY2) DispDirtyY2 = y + h;
}
void DispDirtyPoint(int x, int y)
{
	if (((u32)x < (u32)WIDTH) && ((u32)y < (u32)HEIGHT)) {
		if (x < DispDirtyX1) DispDirtyX1 = x;
		if (x + 1 > DispDirtyX2) DispDirtyX2 = x + 1;
		if (y < DispDirtyY1) DispDirtyY1 = y;
		if (y + 1 > DispDirtyY2) DispDirtyY2 = y + 1;
	}
}

// Update display from FrameBuf
void DispUpdate()
{
	if ((DispDirtyX1 < DispDirtyX2) && (DispDirtyY1 < DispDirtyY2))
	{
		// Sync
		u8 d = 0xff;
		CS_OFF; DC_CMD;
		SPI_Send8(DISP_SPI, &d, 1);

		DispWindow((u16)DispDirtyX1, (u16)DispDirtyX2, (u16)DispDirtyY1, (u16)DispDirtyY2);
		
		u16* s0 = &FrameBuf[DispDirtyX1 + DispDirtyY1*WIDTH];
		int w = DispDirtyX2 - DispDirtyX1;
		int h = DispDirtyY2 - DispDirtyY1;

#ifdef ILI9488_USE_16BIT_SPI
		int i;
		for (i = 0; i < h; i++)
		{
			DispWriteData(s0, w * 2);
			s0 += WIDTH;
		}
#else
		// Convert RGB565 to RGB666 (3 bytes/pixel)
		u8 lineBuf[WIDTH * 3]; // Allocate for max width
		int y, x, k;

		for (y = 0; y < h; y++)
		{
			k = 0;
			for (x = 0; x < w; x++)
			{
				u16 c = s0[x];
				lineBuf[k++] = (c & 0xF800) >> 8; // R
				lineBuf[k++] = (c & 0x07E0) >> 3; // G
				lineBuf[k++] = (c & 0x001F) << 3; // B
			}
			DispWriteData(lineBuf, k);
			s0 += WIDTH;
		}
#endif
		DispDirtyNone();
	}
}

void DispAutoUpdate(u32 ms)
{
	u32 us = ms*1000;
	if ((u32)(Time() - DispAutoUpdateLast) >= us) {
		DispUpdate();
		DispAutoUpdateLast = Time();
	}
}

void DispUpdateAll()
{
	DispDirtyAll();
	DispUpdate();
}

void DispBacklight(u8 backlight)
{
	PWM_Comp(BACKLIGHT_SLICE, BACKLIGHT_CHAN, backlight);
}

void DispBacklightUpdate()
{
#if USE_CONFIG
	DispBacklight(Config.backlight);
#else
	DispBacklight(255);
#endif
}

void DispInit(u8 rot)
{
	// 1. PIN INIT - CRITICAL: Set CS High FIRST to avoid SPI noise during init
	GPIO_Out1(DISP_CS_PIN); // Deselect display immediately
	GPIO_DirOut(DISP_CS_PIN);
	GPIO_Fnc(DISP_CS_PIN, GPIO_FNC_SIO);

	GPIO_Out1(DISP_DC_PIN); 
	GPIO_DirOut(DISP_DC_PIN); 
	GPIO_Fnc(DISP_DC_PIN, GPIO_FNC_SIO);
	
	GPIO_Out1(DISP_RES_PIN); 
	GPIO_DirOut(DISP_RES_PIN); 
	GPIO_Fnc(DISP_RES_PIN, GPIO_FNC_SIO);

	// 2. SPI Init
	SPI_Init(DISP_SPI, DISP_SPI_BAUD);
	SPI_Pol(DISP_SPI, 0); // ILI9488 often Mode 0 or 3
	SPI_Phase(DISP_SPI, 0); 

	GPIO_Out1(DISP_SCK_PIN); 
	GPIO_DirOut(DISP_SCK_PIN); 
	GPIO_Fnc(DISP_SCK_PIN, GPIO_FNC_SPI);
	
	GPIO_Out1(DISP_MOSI_PIN); 
	GPIO_DirOut(DISP_MOSI_PIN); 
	GPIO_Fnc(DISP_MOSI_PIN, GPIO_FNC_SPI);
	
	// MISO
	GPIO_DirIn(DISP_MISO_PIN); 
	GPIO_Fnc(DISP_MISO_PIN, GPIO_FNC_SPI);

	// 3. PWM Init
	PWM_Reset(BACKLIGHT_SLICE);
	PWM_GpioInit(DISP_BLK_PIN);
	PWM_Clock(BACKLIGHT_SLICE, BACKLIGHT_CLOCK);
	PWM_Top(BACKLIGHT_SLICE, 255);
	DispBacklight(0);
	PWM_Enable(BACKLIGHT_SLICE);

	// 4. Reset & Config sequence
	DispHardReset();
	DispSoftReset();

	// Initialization Sequence for ILI9488
	DispWriteCmd(0xE0); // PGAMCTRL
	u8 gammaP[] = {0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78, 0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F};
	DispWriteData(gammaP, 15);

	DispWriteCmd(0xE1); // NGAMCTRL
	u8 gammaN[] = {0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45, 0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F};
	DispWriteData(gammaN, 15);

	DispWriteCmdData(ILI9488_PWCTRL1, "\x17\x15", 2); // Power Control 1
	DispWriteCmdData(ILI9488_PWCTRL2, "\x41", 1);     // Power Control 2
	DispWriteCmdData(ILI9488_VMCTRL1, "\x00\x12\x80", 3); // VCOM Control

#ifdef ILI9488_USE_16BIT_SPI
	u8 pixelFmt = 0x55; // 16-bit Interface, 16-bit RGB
#else
	u8 pixelFmt = 0x66; // 18-bit Interface, 18-bit RGB (Safe mode for 4-wire SPI)
#endif
	DispWriteCmdData(ILI9488_COLMOD, &pixelFmt, 1);

	DispWriteCmdData(ILI9488_FRMCTR1, "\xA0", 1); // Frame Rate
	DispWriteCmdData(ILI9488_DISCTRL, "\x02\x02", 2); // Display Function Control
	
	DispRotation(rot);
	DispWriteCmd(ILI9488_SLPOUT);
	WaitMs(120);

	DispWriteCmd(ILI9488_INVON); // Inversion ON

	DispWriteCmd(ILI9488_DISPON);
	WaitMs(20);
	
	DispBacklightUpdate();

	// Clear screen
	DispDirtyAll();
#if USE_FRAMEBUF
	// Fill FrameBuf with 0
	int i;
	for (i = 0; i < FRAMESIZE; i++) FrameBuf[i] = 0;
	DispUpdate();

	// *** TEST START ***
//	DispTest();
//	WaitMs(3000); // Wait to see the pattern
	// *** TEST END ***
#endif
}

void DispTerm()
{
	DispWriteCmd(ILI9488_DISPOFF);
	PWM_Disable(BACKLIGHT_SLICE);
	
	// Set pins to safe idle state (CS High), don't reset to floating!
	GPIO_Out1(DISP_CS_PIN); // Deselect
	GPIO_Out0(DISP_BLK_PIN); // Backlight off
	
	// Reset others if needed
	GPIO_Reset(DISP_DC_PIN);
	GPIO_Reset(DISP_SCK_PIN);
	GPIO_Reset(DISP_MOSI_PIN);
	GPIO_Reset(DISP_MISO_PIN); 
	GPIO_Reset(DISP_RES_PIN);
	
	// Keep CS as output high to prevent noise
	GPIO_Fnc(DISP_CS_PIN, GPIO_FNC_SIO);
	GPIO_DirOut(DISP_CS_PIN);
	GPIO_Out1(DISP_CS_PIN);
}

void DispDrawText(const char* text, int x, int y, int w1, int w2, u16 col, u16 bgcol)
{
	int len = StrLen(text);
	int w0 = DrawFontWidth;
	int w = len*w0;
	if (w > WIDTH) len = WIDTH/w0;
	w = WIDTH - len*w0;

	if (w1 < 0) w1 = 0;
	if (w1 > w) w1 = w;
	w -= w1;

	if (w2 < 0) w2 = 0;
	if (w2 > w) w2 = w;

	w = w1 + len*w0 + w2;
	int h = DrawFontHeight;
	DispStartImg((u16)x, (u16)(x+w), (u16)y, (u16)(y+h));

	int i, j;
	u8 ch;
	const u8* fnt = pDrawFont;
	const char* t;
	for (; h > 0; h--)
	{
		for (i = w1; i > 0; i--) DispSendImg2(bgcol);
		t = text;
		for (i = len; i > 0; i--)
		{
			ch = *t++;
			ch = fnt[ch];
			for (j = w0; j > 0; j--)
			{
				DispSendImg2(((ch & 0x80) != 0) ? col : bgcol);
				ch <<= 1;
			}
		}
		for (i = w2; i > 0; i--) DispSendImg2(bgcol);
		fnt += 256;
	}
	DispStopImg();
}

void DispDrawTextRow(const char* text, int x, int y, u16 col, u16 bgcol)
{
	DispDrawText(text, 0, y, x, WIDTH - x - StrLen(text)*DrawFontWidth, col, bgcol);
}

// Test function: Draws a checkerboard pattern
void DispTest(int stepbystep)
{
	int x, y;
	int squareSize = 40; // Size of squares in pixels
	u16 color;

	for (y = 0; y < HEIGHT; y++) {
		for (x = 0; x < WIDTH; x++) {
			// Decide color based on position
			bool isRed = ((x / squareSize) + (y / squareSize)) % 2 == 0;
			if (stepbystep == 0) color = isRed ? COLOR(255, 0, 0) : COLOR(0, 0, 255);
			if (stepbystep == 1) color = isRed ? COLOR(0, 255, 0) : COLOR(0, 0, 255);
			if (stepbystep == 2) color = isRed ? COLOR(0, 0, 255) : COLOR(255, 0, 255);
			FrameBuf[x + y * WIDTH] = color;
		}
	}
	DispUpdateAll();
}

#endif // USE_ILI9488
