// ****************************************************************************
//
//                                 Main code
//
// ****************************************************************************

#include "../include.h"

#ifdef HEIGHT
#undef HEIGHT
#endif

#define HEIGHT 288

#define EMU_PWMTOP	4095	// PWM sound top (period = EMU_PWMTOP + 1 = 4096)
#define EMU_PWMCLOCK	(AUDIO_SAMPLE_RATE*(EMU_PWMTOP+1)) // PWM clock (= 32768*4096 = 134 217 728)

#define DISP_MINFPS	8	// minimal required display FPS (limit rendering to speed-up program emulation)
#define DISP_MAXFPS	22	// maximal required display FPS (limit rendering to speed-up program emulation)
#define DISP_MINDELAYUS	(1000000/(DISP_MAXFPS*HEIGHT)) // minimal delay per scanline in [us]
#define DISP_MAXDELAYUS	(1000000/(DISP_MINFPS*HEIGHT)) // maximal delay per scanline in [us]
#define DISP_INCDELAY	3	// speed of delay adaptation

// Time synchronization
#define EMU_SYNCTIME	((unsigned)(1000000/VERTICAL_SYNC)) // sync delta time in [us]
u32 OldSyncTime;		// old sync time
u32 LastLineUs;			// time of last line display
volatile u32 DelayLineUs = ((DISP_MINDELAYUS + DISP_MAXDELAYUS)/2); // current delay per line display

// CRAM
int CRamSize;		// size of CRAM to save (0 = no CRAM or no backup battery)
Bool CRamModi;		// CRAM modified
u32 CRamLastWrite;	// last time of write CRAM
u32 CRamBatch;		// batch counter of writes

u8 GPState = 0;		// game pad state

#if DEB_WRITECRAM	// 1=debug display write bytes into CRAM (in game menu)
u32 CRamWriteNum = 0;	// number of writes to CRAM
#endif

// home path
char HomePath[APPPATH_PATHMAX+15+1];
int HomePathLen;

// GameBoy context
struct gb_s gbContext;

// game title CRC
u16 TitleCrc;		// CRC16A of game title, game code, support code and maker code, address 0x0134..0x0145
u8 TitleCrc2;		// game title small CRC (value from address 0x14d)

#if DEB_FPS			// debug display FPS
u32 DebFpsTime;			// last time FPS
int DebFpsCount;		// FPS counter
int DebFps;			// last FPS
u16 DebFpsCol;			// FPS color
#endif

// sound
s16 StreamBuf[AUDIO_SAMPBUF*2];
#if PWMSND_GPIO_R >= 0
u16 SndBuf[AUDIO_SAMPBUF*2];
#else
u16 SndBuf[AUDIO_SAMPBUF];
#endif
int SndBufInx = AUDIO_SAMPBUF;

// --- CHEAT ENGINE & UI VARIABLES ---
volatile bool RequestClearBars = true;
volatile char currentCheatInput[32] = "";
volatile int currentCheatLen = 0;
volatile bool cheatInputActive = false;
volatile bool cheatReadyToParse = false;
volatile bool requestMenu = false;

struct sGB_Cheat {
    char code_str[10];
    u8 type;           // 0 = Game Genie, 1 = GS 01, 0x80 = GS SRAM, 0x90 = GS WRAM
    u8 bank;
    bool active;
    u16 addr;
    u8 new_val;
    u8 old_val;
    bool has_old_val;
};
sGB_Cheat gbCheats[20];
int gbCheatCount = 0;

void AddCheat(const char* code) {
    if (strcmp(code, "CLEAR") == 0 || strcmp(code, "clear") == 0) {
        gbCheatCount = 0;
        return;
    }

    char buf[16]; int len = 0;
    
    for(int i = 0; code[i] != 0 && len < 9; i++) {
        if(code[i] != '-') {
            buf[len++] = (code[i] >= 'a' && code[i] <= 'z') ? code[i] - 32 : code[i];
        }
    }
    buf[len] = 0;
    
    if(len != 6 && len != 8 && len != 9) return;
    
    for (int i = 0; i < gbCheatCount; i++) {
        if (strcmp(gbCheats[i].code_str, buf) == 0) {
            gbCheats[i].active = !gbCheats[i].active;
            return;
        }
    }

    if (gbCheatCount >= 20) return;
    
    int hex[9];
    for(int i = 0; i < len; i++) {
        if(buf[i] >= '0' && buf[i] <= '9') hex[i] = buf[i] - '0';
        else if(buf[i] >= 'A' && buf[i] <= 'F') hex[i] = buf[i] - 'A' + 10;
        else return; 
    }
    
    strcpy(gbCheats[gbCheatCount].code_str, buf);
    gbCheats[gbCheatCount].active = true;

    // --- GAMESHARK (8: TTVVLLHH) ---
    if (len == 8) {
        u8 tt = (hex[0] << 4) | hex[1];
        u8 vv = (hex[2] << 4) | hex[3];
        u8 ll = (hex[4] << 4) | hex[5];
        u8 hh = (hex[6] << 4) | hex[7];

        gbCheats[gbCheatCount].new_val = vv;
        gbCheats[gbCheatCount].addr = (hh << 8) | ll;
        gbCheats[gbCheatCount].has_old_val = false;

        if (tt == 0x01) {
            gbCheats[gbCheatCount].type = 1;
            gbCheats[gbCheatCount].bank = 0;
        } else if ((tt & 0xF0) == 0x80) {
            gbCheats[gbCheatCount].type = 0x80;
            gbCheats[gbCheatCount].bank = tt & 0x0F;
        } else if ((tt & 0xF0) == 0x90) {
            gbCheats[gbCheatCount].type = 0x90;
            gbCheats[gbCheatCount].bank = tt & 0x0F;
        } else {
            return;
        }
    } 
    // --- GAME GENIE (6 or 9) ---
    else {
        gbCheats[gbCheatCount].type = 0;
        gbCheats[gbCheatCount].new_val = (hex[0] << 4) | hex[1];
        gbCheats[gbCheatCount].addr = ((hex[5] << 12) | (hex[2] << 8) | (hex[3] << 4) | hex[4]) ^ 0xF000;
        
        if(len == 9) {
            u8 gi = (hex[6] << 4) | hex[8]; gi ^= 0xFF;
            gi = (gi >> 2) | ((gi & 3) << 6);
            gbCheats[gbCheatCount].old_val = gi ^ 0x45;
            gbCheats[gbCheatCount].has_old_val = true;
        } else {
            gbCheats[gbCheatCount].has_old_val = false;
        }
    }
    
    gbCheatCount++;
}

void ParseCheats(const char* str) {
    while (*str) {
        while (*str == ' ' || *str == '+') str++;
        if (!*str || *str == '*') break;
        char buf[16]; int len = 0;
        while (*str && *str != ' ' && *str != '+') { if (len < 15) buf[len++] = *str; str++; }
        buf[len] = 0; AddCheat(buf);
    }
}

// Callback z klávesnice
void MyRawKeyCallback(u8 status, char c) {
    if (status != 1) return; // Pouze stisk

    // Kontrola klávesy pro Menu (0x85)
    if ((u8)c == 0x85) {
        requestMenu = true;
        return;
    }

    if (c == '\t') {
        cheatInputActive = !cheatInputActive;
        RequestClearBars = true;
    } else if (cheatInputActive) {
        if (c == '\r' || c == '\n') {
            if (currentCheatLen > 0) cheatReadyToParse = true;
            else { cheatInputActive = false; RequestClearBars = true; }
        } else if (c == '\b' || c == 0x7F) {
            if (currentCheatLen > 0) { currentCheatLen--; currentCheatInput[currentCheatLen] = '\0'; RequestClearBars = true; }
        } else if (currentCheatLen < 31 && c >= ' ' && c <= 'z') {
            char up = (c >= 'a' && c <= 'z') ? c - 32 : c;
            currentCheatInput[currentCheatLen++] = up;
            currentCheatInput[currentCheatLen] = '\0';
            RequestClearBars = true;
        }
    }
}

// ----------------------------------------------------------------------------
//                               ROM and cache
// ----------------------------------------------------------------------------

#if USE_PICOPADHSTX
ALIGNED FRAMETYPE FrameBuf2[GB_CACHERAM_NUM*GB_CACHE_SIZE/sizeof(FRAMETYPE)];
#define CacheBuf ((u8*)FrameBuf2)
#else
ALIGNED FRAMETYPE FrameBuf[GB_CACHERAM_NUM*GB_CACHE_SIZE/sizeof(FRAMETYPE)];
#define CacheBuf ((u8*)FrameBuf)
#endif

sGB_Cache GB_CacheDesc[GB_CACHE_NUM];
u8 GB_CacheROM[GB_ROMCACHEMAX];
sFile GB_ROMFile;
Bool GB_ReqExit;

u8 FASTCODE NOFLASH(gbRomReadRaw)(struct gb_s *gb, const uint_fast32_t addr)
{
	if (addr >= gameRomOrig) return 0xff;
	int page = addr >> 14;
	int off = addr & 0x3fff;
	if (page < gameFlashPages) {
		if (off == 0x3fff) return gameRomLastList[page];
		if (off >= gameRomSizeList[page]) return gameRomStuffList[page];
		return gameRom[gameRomOffList[page] + off];
	}
	page = addr >> GB_CACHE_SHIFT;
	if (page >= GB_ROMCACHEMAX) return 0xff;
	off = addr & GB_CACHE_MASK;
	u8 inx = GB_CacheROM[page];
	sGB_Cache* c;
	if (inx == GB_CACHEINX_INV) {
		if (GB_ReqExit) return 0xff;
		c = GB_CacheDesc;
		for (inx = 0; inx < GB_CACHE_NUM; inx++) { if (c->type == GB_CACHETYPE_FREE) break; c++; }
		if (inx == GB_CACHE_NUM) {
			u32 t = Time(), delta, deltabest = 0;
			for (int i = GB_CACHE_NUM-1; i >= 0; i--) {
				if (GB_CacheDesc[i].type == GB_CACHETYPE_VALID) {
					delta = t - GB_CacheDesc[i].last;
					if (delta > deltabest) { inx = (u8)i; deltabest = delta; }
				}
			}
			GB_CacheROM[GB_CacheDesc[inx].rominx] = GB_CACHEINX_INV;
		}
		c = &GB_CacheDesc[inx]; c->type = GB_CACHETYPE_VALID; c->rominx = page; GB_CacheROM[page] = inx;
		u32 romoff = page << GB_CACHE_SHIFT;
		u8* d = (inx < GB_CACHEINX_CRAM) ? &CacheBuf[((int)inx << GB_CACHE_SHIFT)] : &gbContext.cram[((int)(inx - GB_CACHEINX_CRAM) << GB_CACHE_SHIFT)];
		if (!FileIsOpen(&GB_ROMFile)) {
			if (!DiskAutoMount()) return 0xff;
			if (!SetDir(HomePath)) return 0xff;
			GB_PrepSaveFile(-2); Bool res = FileOpen(&GB_ROMFile, HomePath); GB_UnprepSaveFile();
			if (!res) return 0xff;
		}
		FileSeek(&GB_ROMFile, romoff);
		int n = gameRomOrig - romoff; if (n > GB_CACHE_SIZE) n = GB_CACHE_SIZE;
		FileRead(&GB_ROMFile, d, n);
	}
	GB_CacheDesc[inx].last = Time();
	return (inx < GB_CACHEINX_CRAM) ? CacheBuf[((int)inx << GB_CACHE_SHIFT) + off] : gbContext.cram[((int)(inx - GB_CACHEINX_CRAM) << GB_CACHE_SHIFT) + off];
}

u8 FASTCODE NOFLASH(gbRomRead)(struct gb_s *gb, const uint_fast32_t addr) {
    u8 val = gbRomReadRaw(gb, addr);
    if (gbCheatCount == 0) return val;
    u16 virt_addr = (addr < 0x4000) ? addr : (0x4000 | (addr & 0x3FFF));
    for (int i = 0; i < gbCheatCount; i++) {
        // Kontrola, že jde o Game Genie (type == 0)
        if (gbCheats[i].type == 0 && gbCheats[i].active && gbCheats[i].addr == virt_addr) {
            if (!gbCheats[i].has_old_val || gbCheats[i].old_val == val) return gbCheats[i].new_val;
        }
    }
    return val;
}

u8 FASTCODE NOFLASH(gbCartRamRead)(struct gb_s *gb, const uint_fast32_t addr) {
	if ((gbContext.cram_valid & BIT(addr >> GB_CACHE_SHIFT)) == 0) return 0;
	return gbContext.cram[addr];
}

void FASTCODE NOFLASH(gbCartRamWrite)(struct gb_s *gb, const uint_fast32_t addr, const u8 val) {
	int page = addr >> GB_CACHE_SHIFT;
	if ((gbContext.cram_valid & BIT(page)) == 0) {
		gbContext.cram_valid |= BIT(page);
		memset(&gbContext.cram[page << GB_CACHE_SHIFT], 0, GB_CACHE_SIZE);
		sGB_Cache* c = &GB_CacheDesc[page + GB_CACHEINX_CRAM];
		if (c->type == GB_CACHETYPE_VALID) GB_CacheROM[c->rominx] = GB_CACHEINX_INV;
		c->type = GB_CACHETYPE_FORB;
	}
	if (addr < CRamSize) { CRamBatch++; CRamLastWrite = Time(); }
	if (gbContext.cram[addr] != val) { gbContext.cram[addr] = val; if (addr < CRamSize) CRamModi = True; }
}

void gbCartRamSave() {
	sFile f; int num = CRamSize; if (num == 0) return;
	if (DiskAutoMount() && SetDir(HomePath)) {
		GB_PrepSaveFile(-1); FileDelete(HomePath);
		if (FileCreate(&f, HomePath)) {
			u8 buf[512]; memset(buf, 0, 512); u32 mask = B0; int off = 0;
			while (num > 0) {
				int n = (num > GB_CACHE_SIZE) ? GB_CACHE_SIZE : num;
				if (gbContext.cram_valid & mask) FileWrite(&f, &gbContext.cram[off], n);
				else for (int i = n; i > 0; i -= 512) FileWrite(&f, buf, (i > 512) ? 512 : i);
				off += GB_CACHE_SIZE; num -= n; mask <<= 1;
			}
			FileClose(&f);
		}
		GB_UnprepSaveFile();
	}
}

void gbCartRamLoad() {
	sFile f; int num = CRamSize; if (num == 0) return;
	if (DiskAutoMount() && SetDir(HomePath)) {
		GB_PrepSaveFile(-1);
		if (FileOpen(&f, HomePath)) {
			FileRead(&f, gbContext.cram, num); FileClose(&f);
			u32 mask = B0; int off = 0; sGB_Cache* c = &GB_CacheDesc[GB_CACHEINX_CRAM];
			while (num > 0) {
				if (c->type == GB_CACHETYPE_VALID) GB_CacheROM[c->rominx] = GB_CACHEINX_INV;
				int n = (num > GB_CACHE_SIZE) ? GB_CACHE_SIZE : num;
				c->type = GB_CACHETYPE_FREE; gbContext.cram_valid &= ~mask;
				for (int i = 0; i < n; i++) if (gbContext.cram[off + i] != 0) { gbContext.cram_valid |= mask; c->type = GB_CACHETYPE_FORB; break; }
				off += GB_CACHE_SIZE; num -= n; mask <<= 1; c++;
			}
		}
		GB_UnprepSaveFile();
	}
}

u16 convertBGR555toRGB565(u16 color) {
	return (u16)(((color & 0x001F) << 11) | ((color & 0x03E0) << 1) | ((color & 0x0200) >> 4) | ((color & 0x7C00) >> 10));
}

void gbSetPal(u8 info) {
	for (int i = 0; i < PAL_NUM; i++) if (info == gbColorPalSel[i]) { gbContext.display.palinx = (u8)i; break; }
	u8 palette = info & 0x1F, flags = (info & 0xE0) >> 5;
	if (flags == 7) { palette += 0x20; flags = 5; }
	const u16 *obp0 = (flags & 1) ? gbColorizationPaletteData[palette][0] : gbColorizationPaletteData[palette][2];
	const u16 *obp1 = (!(flags & 4)) ? gbColorizationPaletteData[palette][2] : (flags & 2) ? gbColorizationPaletteData[palette][0] : gbColorizationPaletteData[palette][1];
	const u16 *bgp = gbColorizationPaletteData[palette][2];
	for (int i = 0; i < 4; i++) {
		u16 c = convertBGR555toRGB565(*obp0++); for (int j = 0; j < 16; j += 4) gbPalette[i+j] = c;
		c = convertBGR555toRGB565(*obp1++); for (int j = 16; j < 32; j += 4) gbPalette[i+j] = c;
		c = convertBGR555toRGB565(*bgp++); for (int j = 32; j < 64; j += 4) gbPalette[i+j] = c;
	}
}

void gbSelectColorizationPalette() {
	u16 crc = TitleCrc; u8 crc2 = TitleCrc2;
	const u16* c = gbPalExt;
	while (c[0] != 0 || c[1] != 0) {
		if (c[0] == crc) { u8 b = (u8)(c[1] >> 8); if (b == 0 || b == crc2) { gbSetPal(gbColorPalSel[(c[1] & 0xff) - 1]); return; } }
		c += 2;
	}
	int infoIdx = 0;
	if (gameRom[0x014B] == 0x01 || (gameRom[0x014B] == 0x33 && gameRom[0x0144] == 0x30 && gameRom[0x0145] == 0x31)) {
		u8 checksum = 0; for (int i = 0; i < 16; i++) checksum += gameRom[0x0134 + i];
		int idx; for (idx = 0; idx < sizeof(gbColorizationChecksums); idx++) if (gbColorizationChecksums[idx] == checksum) break;
		if (idx < sizeof(gbColorizationChecksums)) {
			if (idx > 0x40) {
				int i = idx - 0x41;
				for (; i < sizeof(gbColorizationDisambigChars); i += 14) if (gameRom[0x0137] == gbColorizationDisambigChars[i]) { infoIdx = idx + (i/14)*14; break; }
			} else infoIdx = idx;
		}
	}
	gbSetPal(gbColorizationPaletteInfo[infoIdx]);
}

void gbErrorHandler(struct gb_s *gb, const enum gb_error_e gb_err, const u16 addr) { reset_usb_boot(0, 0); }


void FASTCODE NOFLASH(core1DrawFrame)()
{
   	if (RequestClearBars) {
		DispStartImg(0, WIDTH, 288, 32);
		for(int y = 0; y < 32; y++) {
            for(int x = 0; x < WIDTH; x++) {
                u16 color = 0x0000;
                if (cheatInputActive) {
                    const char* prefix = "CHEAT: "; int pLen = 7;
                    int ty = y - 8, tx = x - 10;
                    if (ty >= 0 && ty < 16 && tx >= 0) {
                        int charIdx = tx / 8, px = tx % 8; char ch = 0;
                        if (charIdx < pLen) ch = prefix[charIdx];
                        else if (charIdx - pLen < currentCheatLen) ch = currentCheatInput[charIdx - pLen];
                        else if (charIdx - pLen == currentCheatLen) ch = '_';
                        if (ch != 0 && (FontBold8x16[(u8)ch + ty * 256] & (0x80 >> px))) color = 0xFFFF;
                    }
                }
                DispSendImg2(color);
            }
		}
		DispStopImg();
		RequestClearBars = false;
	}

	int rinx = gbContext.frame_read;
	while (rinx == gbContext.frame_write) {
        if (GB_DispMode == GB_DISPMODE_MSG) return;
    }
    
	dmb();
	if (DoEmuScreenShotReq) { DoEmuScreenShot = True; dmb(); DoEmuScreenShotReq = False; dmb(); }
	
    DispStartImg(0, WIDTH, 0, HEIGHT);
    
	for (int y = 0; y < LCD_HEIGHT; y++) {
		rinx = gbContext.frame_read;
		while (rinx == gbContext.frame_write) {
            if (GB_DispMode == GB_DISPMODE_MSG) { DispStopImg(); return; }
        }
		
        u16* s = &gbContext.framebuf[rinx*LCD_WIDTH];
		
        // 1. Zápis prvního fyzického řádku
		for (int x = LCD_WIDTH; x > 0; x--) { 
            u16 c = *s++; 
            DispSendImg2(c); DispSendImg2(c); 
        }
        
        // Pauza po prvním řádku pro stabilizaci LCD řadiče (rovnoměrný tok dat)
        u32 del = DelayLineUs; 
        u32 line = LastLineUs; 
        while ((u32)(Time() - line) < del) {} 
        LastLineUs = Time();
        
        // 2. Zápis druhého fyzického řádku
        s = &gbContext.framebuf[rinx*LCD_WIDTH]; // Návrat na začátek stejného herního řádku
		for (int x = LCD_WIDTH; x > 0; x--) { 
            u16 c = *s++; 
            DispSendImg2(c); DispSendImg2(c); 
        }
        
        // Pauza po druhém řádku
        line = LastLineUs; 
        while ((u32)(Time() - line) < del) {} 
        LastLineUs = Time();
        
        // Okamžité uvolnění paměti pro emulátor
		dmb(); 
        rinx++; 
        if (rinx >= LCD_FRAMEHEIGHT) rinx = 0; 
        gbContext.frame_read = rinx; 
        dmb();
	}
	DispStopImg();
    
    gbContext.frame_rline = 0;
}


void FASTCODE NOFLASH(PWMSndIrq)()
{
	PWM_IntClear(PWMSND_SLICE);
	int i = SndBufInx;
	if (i >= AUDIO_SAMPBUF) {
		int vol = Config.volume;
		if (vol == 0 || GlobalSoundOff) for (i = 0; i < AUDIO_SAMPBUF*(PWMSND_GPIO_R >= 0 ? 2 : 1); i++) SndBuf[i] = EMU_PWMTOP/2;
		else {
			audio_callback(StreamBuf, AUDIO_SAMPBUF);
			s16* s = StreamBuf; u16* d = SndBuf;
			for (int j = AUDIO_SAMPBUF; j > 0; j--) {
				int l = *s++, r = *s++;
#if PWMSND_GPIO_R >= 0
				int cl = ((l*vol) >> 10) + EMU_PWMTOP/2; 
				if (cl < 0) cl = 0; if (cl > EMU_PWMTOP) cl = EMU_PWMTOP;
				*d++ = (u16)cl;

				int cr = ((r*vol) >> 10) + EMU_PWMTOP/2; 
				if (cr < 0) cr = 0; if (cr > EMU_PWMTOP) cr = EMU_PWMTOP;
				*d++ = (u16)cr;
#else
				int m = (((l + r)*vol) >> 11) + EMU_PWMTOP/2; 
				if (m < 0) m = 0; if (m > EMU_PWMTOP) m = EMU_PWMTOP;
				*d++ = (u16)m;
#endif
			}
		}
		i = 0;
	}
	u16 samp = SndBuf[(PWMSND_GPIO_R >= 0 ? 2*i : i)];
	PWM_Comp(PWMSND_SLICE, PWMSND_CHAN, samp);
#if PWMSND_GPIO_R >= 0
	PWM_Comp(PWMSND_SLICE_R, PWMSND_CHAN_R, SndBuf[2*i+1]);
#endif
	SndBufInx = i + 1;
}

volatile Bool RunEmul = False;
void core1_entry() {
	LastLineUs = Time(); RunEmul = True;
	while (RunEmul) {
		if (GB_DispMode == GB_DISPMODE_MSG) { GB_TextUpdate(); LastLineUs = Time(); }
		else core1DrawFrame();
	}
}

void GB_InitSysClk() {
#if !USE_PICOPADHSTX || !USE_DISPHSTX
	u32 clk = gbContext.cgb.cgbMode ? EMU_CLKSYS_GBC : EMU_CLKSYS_GB;
	if (clk >= 285000) { SSI_InitFlash(6); VregSetVoltage(VREG_VOLTAGE_1_30); }
	else if (clk >= 245000) VregSetVoltage(VREG_VOLTAGE_1_30);
	else if (clk >= 220000) VregSetVoltage(VREG_VOLTAGE_1_25);
	else VregSetVoltage(VREG_VOLTAGE_1_10);
	ClockPllSysFreq(clk);
#endif
}

void GB_TermSysClk() {
#if !USE_PICOPADHSTX || !USE_DISPHSTX
	ClockPllSysFreq(PLL_KHZ); VregSetVoltage(VREG_VOLTAGE_1_10); SSI_SetFlashClkDiv(FLASH_CLKDIV);
#endif
}

void GB_InitCore1() {
#if USE_PICOPADHSTX && USE_DISPHSTX
	DispHstxCore1Exec(core1_entry);
#else
	Core1Exec(core1_entry);
#endif
}

void GB_TermCore1() {
#if USE_PICOPADHSTX && USE_DISPHSTX
	RunEmul = False;
#else
	Core1Reset();
#endif
}

void GB_Setup() {
	TitleCrc = Crc16AFast(&gameRom[0x134], 18); TitleCrc2 = gameRom[0x14d];
	memset(GB_CacheDesc, GB_CACHETYPE_FREE, sizeof(GB_CacheDesc));
	memset(GB_CacheROM, GB_CACHEINX_INV, sizeof(GB_CacheROM));
	GB_ReqExit = False; memset(&gbContext, 0, sizeof(gbContext));
	gbSelectColorizationPalette();
	gb_init(&gbContext, &gbRomRead, &gbCartRamRead, &gbCartRamWrite, &gbErrorHandler, nullptr);
	GB_InitSysClk(); CRamSize = gb_get_save_size(&gbContext); gbCartRamLoad(); CRamBatch = 0;
	gb_init_lcd(&gbContext); audio_init(&gbContext);
	PWM_Reset(PWMSND_SLICE); PWM_GpioInit(PWMSND_GPIO);
#if PWMSND_GPIO_R >= 0
	PWM_GpioInit(PWMSND_GPIO_R);
#endif
	SetHandler(IRQ_PWM_WRAP, PWMSndIrq); NVIC_IRQEnable(IRQ_PWM_WRAP);
	PWM_Clock(PWMSND_SLICE, EMU_PWMCLOCK); PWM_Top(PWMSND_SLICE, EMU_PWMTOP);
	PWM_Comp(PWMSND_SLICE, PWMSND_CHAN, EMU_PWMTOP/2);
#if PWMSND_GPIO_R >= 0
	PWM_Comp(PWMSND_SLICE_R, PWMSND_CHAN_R, EMU_PWMTOP/2);
#endif
	PWM_Enable(PWMSND_SLICE); PWM_IntEnable(PWMSND_SLICE);
	GB_InitCore1(); OldSyncTime = Time();
}

int main() {
#if USE_USB_HOST_HID
	UsbHostInit();
#endif
	HomePathLen = GetHomePath(HomePath, "/GAMEBOY");
	GB_DispMode = GB_DISPMODE_EMU; DoEmuScreenShot = False; FileInit(&GB_ROMFile);
	GB_Setup();
	KeySetRawCallback(MyRawKeyCallback); // Registrace callbacku

    while (True) {
		gb_run_frame(&gbContext);
        
		// --- APLIKACE GAMESHARK CHEATŮ ---
		for (int i = 0; i < gbCheatCount; i++) {
			if (!gbCheats[i].active || gbCheats[i].type == 0) continue; // Přeskoč neaktivní a Game Genie

			uint16_t addr = gbCheats[i].addr;
			uint8_t val = gbCheats[i].new_val;

			// Typ 01 (běžný zápis)
			if (gbCheats[i].type == 1) {
				__gb_write(&gbContext, addr, val);
			} 
			// Typ 8X (zápis do SRAM)
			else if (gbCheats[i].type == 0x80) {
				if (gbContext.cart_ram && addr >= CART_RAM_ADDR && addr < WRAM_0_ADDR) {
					uint8_t bank = gbCheats[i].bank;
					if (bank < gbContext.num_ram_banks && gbContext.gb_cart_ram_write) {
						uint32_t real_addr = (addr - CART_RAM_ADDR) + (bank * CRAM_BANK_SIZE);
						gbContext.gb_cart_ram_write(&gbContext, real_addr, val);
					}
				}
			} 
			// Typ 9X (zápis do GBC WRAM)
			else if (gbCheats[i].type == 0x90) {
				if (gbContext.cgb.cgbMode && addr >= WRAM_1_ADDR && addr < ECHO_ADDR) {
					uint8_t bank = gbCheats[i].bank;
					if (bank >= 1 && bank <= 7) {
						uint16_t wram_offset = (addr - WRAM_1_ADDR) + (bank * WRAM_BANK_SIZE);
						if (wram_offset < WRAM_SIZE) {
							gbContext.wram[wram_offset] = val;
						}
					}
				}
			}
		}
		// --- KONEC GAMESHARK ---

		u32 dif = Time() - OldSyncTime;
		u32 del = DelayLineUs;
		if (dif >= EMU_SYNCTIME) { del += DISP_INCDELAY; if (del > DISP_MAXDELAYUS) del = DISP_MAXDELAYUS; }
		else { del -= DISP_INCDELAY; if (del < DISP_MINDELAYUS) del = DISP_MINDELAYUS; }
		DelayLineUs = del;
		while ((u32)(Time() - OldSyncTime) < EMU_SYNCTIME) {}
		if (dif >= 20*EMU_SYNCTIME) OldSyncTime = Time(); else OldSyncTime += EMU_SYNCTIME;

		if (GB_ReqExit) { if (CRamModi) gbCartRamSave(); ResetToBootLoader(); }

        // --- Mapování kláves ---
		Bool press_up = KeyPressed(KEY_UP), press_down = KeyPressed(KEY_DOWN), press_left = KeyPressed(KEY_LEFT), press_right = KeyPressed(KEY_RIGHT);
		Bool press_a = KeyPressed(KEY_A), press_b = KeyPressed(KEY_B), press_x = KeyPressed(KEY_X), press_y = KeyPressed(KEY_Y);

        // --- Zpracování příkazů z Callbacku ---
        if (cheatReadyToParse) {
            ParseCheats((const char*)currentCheatInput);
            currentCheatLen = 0; currentCheatInput[0] = '\0';
            cheatInputActive = false; cheatReadyToParse = false; RequestClearBars = true;
        }

        if (requestMenu) {
            requestMenu = false; goto RunMenuTrigger;
        }

        if (!cheatInputActive) {
            gbContext.direct.joypad_bits.up = !press_up;
            gbContext.direct.joypad_bits.down = !press_down;
            gbContext.direct.joypad_bits.left = !press_left;
            gbContext.direct.joypad_bits.right = !press_right;
            gbContext.direct.joypad_bits.a = !press_a;
            gbContext.direct.joypad_bits.b = !press_b;
            gbContext.direct.joypad_bits.start = !press_x;
            gbContext.direct.joypad_bits.select = !press_y;
        } else {
            gbContext.direct.joypad = 0xFF; // Blokování tlačítek při psaní
        }

		switch (KeyGet()) {
		case KEY_Y: if (press_left || press_right) break; break; // Odstraněna původní kombinace pro menu
		case KEY_UP: break;
RunMenuTrigger: // Skok sem z vlajky requestMenu
			{
				Bool oldoff = GlobalSoundOff; GlobalSoundOff = True;
				if (!GB_Menu()) { if (CRamModi) gbCartRamSave(); ResetToBootLoader(); }
				GlobalSoundOff = oldoff; OldSyncTime = Time(); RequestClearBars = true;
			}
			break;
		case KEY_LEFT: if (press_y) gbSetPal(gbColorPalSel[(gbContext.display.palinx > 0 ? gbContext.display.palinx - 1 : PAL_NUM-1)]); break;
		case KEY_RIGHT: if (press_y) gbSetPal(gbColorPalSel[(gbContext.display.palinx < PAL_NUM-1 ? gbContext.display.palinx + 1 : 0)]); break;
		}

		if (CRamModi && (u32)(Time() - CRamLastWrite) > 2000000) { if (CRamBatch > GB_CRAMBATCH) gbCartRamSave(); CRamModi = False; CRamBatch = 0; }
	}
}
