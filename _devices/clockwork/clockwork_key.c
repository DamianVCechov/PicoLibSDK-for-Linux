// ****************************************************************************
//
//                             PicoPad Buttons (I2C Mod)
//
// ****************************************************************************
// Upraveno pro Picocalc: I2C DRIVER ONLY (No Legacy GPIO conflict)

#include "../../global.h"

#ifndef KEYBUF_SIZE
#define KEYBUF_SIZE 10
#endif

#if USE_CLOCKWORK

#include "../../_sdk/inc/sdk_gpio.h"
#include "../../_sdk/inc/sdk_timer.h"
#if USE_USBPAD
#include "../../_sdk/usb_inc/sdk_usb_phy.h"
#include "../../_sdk/usb_inc/sdk_usb_hid.h"
#include "../../_sdk/usb_inc/sdk_usb_host.h"
#include "../../_sdk/usb_inc/sdk_usb_host_hid.h"
#endif
#include "../../_display/minivga/minivga.h"
#include "clockwork_key.h"

// --- PICOCALC CONFIGURATION ---
#ifndef USE_PICOCALC_I2C
#define USE_PICOCALC_I2C  1
#endif

#if USE_PICOCALC_I2C
#include "../../_sdk/inc/sdk_i2c.h"
#define SAFETY_TIMEOUT  500     // ms
#define SCAN_INTERVAL   20000   // us (20ms) - interval dotazování I2C
#endif
// -----------------------------

// Původní GPIO definice použijeme jen pokud NEJEDEME přes I2C
#if !USE_PICOPAD_LED && !USE_PICOCALC_I2C
const u8 KeyGpio[KEY_NUM] = {	4,	3,	2,	5,	17,	16,	18,	19 };
#endif

volatile Bool KeyPressMap[KEY_NUM];
u8 KeyBuf[KEYBUF_SIZE];
volatile u8 KeyWriteOff = 0;
volatile u8 KeyReadOff = 0;
u8 KeyRetBuf = NOKEY;

// Mapování kódů kláves na ASCII znaky (pro KeyChar)
const char KeyMapToChar[KEY_NUM+1] = {
    NOCHAR, // 0
    NOCHAR, // 1 KEY_UP
    NOCHAR, // 2 KEY_LEFT
    NOCHAR, // 3 KEY_RIGHT
    NOCHAR, // 4 KEY_DOWN
    NOCHAR, // 5 KEY_X
    NOCHAR, // 6 KEY_Y
    ' ',    // 7 KEY_A (Zde si můžete dát znak, jaký chcete, např. 'A')
    0x1B,   // 8 KEY_B
};

#if USE_PICOCALC_I2C
volatile u16 PicocalcTimeout[KEY_NUM];
volatile u32 LastI2CScanTime = 0; 
volatile Bool I2C_Active = False;

// Mapování RAW dat z I2C (např. 0x81) na interní kódy PicoPadu (KEY_A = 7)
u8 PicocalcMapKey(u8 ch) {
    switch (ch) {
        case 0xB5: return KEY_UP;
        case 0xB6: return KEY_DOWN;
        case 0xB4: return KEY_LEFT;
        case 0xB7: return KEY_RIGHT;
        case 0x81: return KEY_A; 
        case 0x82: return KEY_B;
        case 0x83: return KEY_X;
        case 0x84: return KEY_Y;
        default: return 0;
    }
}

void I2C_BusRecovery() {
    // Reset sběrnice pro případ zaseknutí slave zařízení
    GPIO_Init(KEY_I2C_SDA); GPIO_DirIn(KEY_I2C_SDA);
    GPIO_Init(KEY_I2C_SCL); GPIO_DirOut(KEY_I2C_SCL);
    
    for (int i = 0; i < 9; i++) {
        if (GPIO_In(KEY_I2C_SDA)) break;
        GPIO_Out(KEY_I2C_SCL, 0); WaitUs(10);
        GPIO_Out(KEY_I2C_SCL, 1); WaitUs(10);
    }
    
    GPIO_DirOut(KEY_I2C_SDA);
    GPIO_Out(KEY_I2C_SDA, 0); GPIO_Out(KEY_I2C_SCL, 1); WaitUs(10);
    GPIO_Out(KEY_I2C_SDA, 1); WaitUs(10);
}

pKeyRawCallback KeyRawCb = NULL;
void KeySetRawCallback(pKeyRawCallback cb) {
    KeyRawCb = cb;
}

// Jádro ovladače - volá se z KeyGet, KeyPressed atd.
void Picocalc_Update() {
    if (!I2C_Active) return;
    
    // Omezení frekvence dotazování
    u32 t = Time();
    if ((t - LastI2CScanTime) < SCAN_INTERVAL) return;
    LastI2CScanTime = t;

    u8 reg = 0x09;
    u8 rx_data[2] = {0,0};
    u8 key = NOKEY;
    u8 key_rel = NOKEY;

    // Dotaz na I2C klávesnici
    int w_ret = I2C_SendMsg(KEY_I2C_PORT, KEY_ADDR, &reg, 1, True, 1000);
    if (w_ret == 1) { 
        int r_ret = I2C_RecvMsg(KEY_I2C_PORT, KEY_ADDR, rx_data, 2, False, 1000);
        if (r_ret == 2) {
            u8 status = rx_data[0];
            u8 ch = rx_data[1];

            if (ch != 0 && KeyRawCb != NULL) KeyRawCb(status, (char)ch);

            if (ch != 0) {
                u8 mapped = PicocalcMapKey(ch); // Zde se 0x81 změní na KEY_A (7)
                if (mapped != 0) {
                    // Logic: 0x01 = Press, 0x03 = Release
                    if (status == 0x01) {
                        // Edge detection: Pokud klávesa nebyla stisknuta, zapiš do bufferu
                        if (!KeyPressMap[mapped-1]) {
                            KeyPressMap[mapped-1] = True;
                            key = mapped; // Toto pošle KEY_A do KeyBuf
                        }
                        PicocalcTimeout[mapped-1] = SAFETY_TIMEOUT; // Reset timeoutu
                    }
                    else if (status == 0x03) {
                        if (KeyPressMap[mapped-1]) {
                            KeyPressMap[mapped-1] = False;
                            key_rel = mapped;
                        }
                        PicocalcTimeout[mapped-1] = 0;
                    }
                }
            }
        }
    }

    // Zápis do kruhového bufferu (FIFO)
    if (key != NOKEY) {
        u8 k_code = key & KEY_MASK;
        u8 w = KeyWriteOff; u8 w2 = w + 1;
        if (w2 >= KEYBUF_SIZE) w2 = 0;
        if (w2 != KeyReadOff) { KeyBuf[w] = k_code; KeyWriteOff = w2; }
    }
    if (key_rel != NOKEY) {
        u8 w = KeyWriteOff; u8 w2 = w + 1;
        if (w2 >= KEYBUF_SIZE) w2 = 0;
        if (w2 != KeyReadOff) { KeyBuf[w] = key_rel | KEY_RELEASE; KeyWriteOff = w2; }
    }
}
#endif

void KeyInit()
{
    // --- I2C INICIALIZACE ---
    #if USE_PICOCALC_I2C
    I2C_BusRecovery();
    I2C_Init(KEY_I2C_PORT, I2C_BAUDRATE);
    GPIO_Fnc(KEY_I2C_SDA, GPIO_FNC_I2C);
    GPIO_Fnc(KEY_I2C_SCL, GPIO_FNC_I2C);
    GPIO_PullUp(KEY_I2C_SDA);
    GPIO_PullUp(KEY_I2C_SCL);
    
    for(int i=0; i<KEY_NUM; i++) PicocalcTimeout[i] = 0;
    LastI2CScanTime = Time();
    I2C_Active = True; 
    
    // ZDE JSEM ODSTRANIL KONFLIKTNÍ INITIALIZACI STARÝCH GPIO
    // Pokud je aktivní I2C, nesaháme na piny znovu!
    
    #else 
    // --- STARÁ GPIO INICIALIZACE (jen pokud není I2C) ---
    #if !USE_PICOPAD_LED
    int i;
    for (i = 0; i < KEY_NUM; i++) { GPIO_Init(KeyGpio[i]); GPIO_PullUp(KeyGpio[i]); }
    #endif
    
    #endif // USE_PICOCALC_I2C

    // Reset stavů
    for (int i = 0; i < KEY_NUM; i++) KeyPressMap[i] = False;
    KeyWriteOff = 0; KeyReadOff = 0; KeyRetBuf = NOKEY;
}

void KeyTerm()
{
    #if USE_PICOCALC_I2C
    KeyWaitNoPressed();
    #endif
    
    #if !USE_PICOPAD_LED && !USE_PICOCALC_I2C
    int i;
    for (i = 0; i < KEY_NUM; i++) GPIO_Reset(KeyGpio[i]);
    #endif
}

Bool KeyPressed(u8 key)
{
    #if USE_PICOCALC_I2C
    Picocalc_Update(); 
    #endif
    if ((key < 1) || (key > KEY_NUM)) return False;
    return KeyPressMap[key-1];
}

// Voláno ze SysTick interruptu
void KeyScan()
{
    #if USE_PICOCALC_I2C
    // I2C se nesmí volat z interruptu, zde řešíme jen timeouty
    for(int i=0; i<KEY_NUM; i++) {
        if (PicocalcTimeout[i] > 0) {
            PicocalcTimeout[i]--;
            if (PicocalcTimeout[i] == 0) {
                KeyPressMap[i] = False; // Bezpečnostní uvolnění klávesy
            }
        }
    }
    #else
    // Staré skenování GPIO (jen pro legacy)
    #if !USE_PICOPAD_LED
    int i;
    for (i = 0; i < KEY_NUM; i++) {
        if (GPIO_In(KeyGpio[i]) == 0) {
            if (!KeyPressMap[i]) { KeyPressMap[i] = True; }
        } else {
            if (KeyPressMap[i]) KeyPressMap[i] = False;
        }
    }
    #endif
    #endif
}

u8 KeyGetRel()
{
    #if USE_PICOCALC_I2C
    Picocalc_Update();
    #endif
    u8 ch = KeyRetBuf;
    if (ch != NOKEY) { KeyRetBuf = NOKEY; return ch; }
    u8 r = KeyReadOff;
    dmb();
    if (r == KeyWriteOff) return NOKEY;
    ch = KeyBuf[r];
    r++;
    if (r >= KEYBUF_SIZE) r = 0;
    KeyReadOff = r;
    return ch;
}

u8 KeyGet() {
    u8 ch;
    for (;;) {
        ch = KeyGetRel();
        if ((ch & KEY_RELEASE) == 0) return ch;
    }
}

char KeyChar() {
    // Opraveno: Vždy číst přes buffer, aby se zachovala synchronizace s KeyGet
    // Pokud potřebujete, aby KeyChar vracel znaky, upravte tabulku KeyMapToChar nahoře.
    return KeyMapToChar[(u8)KeyGet()];
}

void KeyFlush() {
    KeyWriteOff = KeyReadOff;
    KeyRetBuf = NOKEY;
}

void KeyRet(u8 key) {
    KeyRetBuf = key;
}

void KeyWaitNoPressed()
{
    #if USE_PICOCALC_I2C
    if (I2C_Active) {
        I2C_Term(KEY_I2C_PORT);
        GPIO_PullDown(KEY_I2C_SDA);
        GPIO_PullDown(KEY_I2C_SCL);
        GPIO_Reset(KEY_I2C_SDA); 
        GPIO_Reset(KEY_I2C_SCL); 
        I2C_Active = False;
    }
    #endif
}

#endif
