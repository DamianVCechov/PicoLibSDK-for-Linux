// ****************************************************************************
//
//                             PicoPad Buttons
//
// ****************************************************************************

#if USE_CLOCKWORK

#ifndef _CLOCKWORK_KEY_H
#define _CLOCKWORK_KEY_H

#ifdef __cplusplus
extern "C" {
#endif

// buttons codes
#define KEY_UP		1
#define KEY_LEFT	2
#define KEY_RIGHT	3
#define KEY_DOWN	4
#define KEY_X		5
#define KEY_Y		6
#define KEY_A		7
#define KEY_B		8

#define KEY_NUM		8
#define MAX_KEYS    128
#define KEY_MASK	0x7f
#define KEY_RELEASE 0x80
#define KEY_REL_TIME	50

extern volatile Bool KeyPressMap[MAX_KEYS];

void KeyInit();
void KeyTerm();

// check if button KEY_* is currently pressed
// (Automatically updates I2C keyboard state)
Bool KeyPressed(u8 key);
INLINE Bool KeyPressedFast(u8 key) { return KeyPressMap[key-1]; }

void KeyScan(); // SysTick handler

u8 KeyGetRel();
u8 KeyGet();
char KeyChar();
void KeyFlush();
void KeyWaitNoPressed();
void KeyRet(u8 key);

typedef void (*pKeyRawCallback)(u8 status, char ch);
void KeySetRawCallback(pKeyRawCallback cb);

#ifdef __cplusplus
}
#endif

#endif // _PICOPAD_KEY_H

#endif // USE_PICOPAD
