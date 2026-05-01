// ****************************************************************************
//
//                            Main code for keyboard
//
// ****************************************************************************

#include "include.h" 
#include "qrcodegen.h"
#include <string.h>  

#define COLOR_CODE          COL_BLACK
#define COLOR_BACKGROUND    COL_WHITE

// Virtual keyboard height, rows a cols
const int kbHeight = 130;
const int kbRows = 5;
const int kbCols = 10;
const int kbOffsetY = 15;

// Dynamic virtual keyboard key width
int keyWidths[kbRows];
const int keyMargin = 2;

// Text field parameters
const int txtFldHeight = 185;  
const int txtFldMargin = 2;     

char currentText[7100] = "";   
int currentTextLen = 0;
bool textChanged = false; 

int startRow = 0;
int startCol = 0;

bool actShift = false;
bool actChar = false;
bool actSmile = false;

const int keyCols[kbRows] = {5, 10, 10, 10, 9};

const char* smallLabel[kbRows][kbCols] = {
    {"CHAR", "SMILE", "\\", "TEST5", "123456"},
    {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
    {"q", "w", "e", "r", "t", "z", "u", "i", "o", "p"},
    {"a", "s", "d", "f", "g", "h", "j", "k", "l", "BACKSPACE"},
    {"SHIFT", "y", "x", "c", "v", "b", "n", "m", " ", ""}
};

const char* upperLabel[kbRows][kbCols] = {
     {"CHAR", "SMILE", "\\", "TEST5", "123456"},
    {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
    {"Q", "W", "E", "R", "T", "Z", "U", "I", "O", "P"},
    {"A", "S", "D", "F", "G", "H", "J", "K", "L", "BACKSPACE"},
    {"SHIFT", "Y", "X", "C", "V", "B", "N", "M", " ", ""}
};

const char* charLabel[kbRows][kbCols] = {
     {"CHAR", "SMILE", "\\", "TEST5", "123456"},
    {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"},
    {"@", "#", "$", "_", "&", "-", "+", "(", ")", "/"},
    {"*", "'", ":", ";", "!", "?", "|", "^", "", "BACKSPACE"},
    {"{", "}", "~", "%", "<", ">", ",", ".", " ", ""}
};

const char* smileLabel[kbRows][kbCols] = {
     {"CHAR", "SMILE", "\\", "TEST5", "123456"},
    {":)", ":))", ";)", ":D", "xD", ":'D", ":P", "8)", ">:>", ":|"},
    {":(", ":'(", ":o", "8O", ":?", ":x", ":S", "*-)", "o_o", ">_<"},
    {":*", ":oO", "=)(=", ">]", "|)", ":a", ";>)", "<3", "@=", "BACKSPACE"},
    {"", "", "", "", "", "", "", "", " ", ""}
};

void intToStr(int num, char* str);
void i2c_keyboard_callback(u8 status, char ch);
void toggleShift();
void toggleChar();
void toggleSmile();
void setKeyWidth();
void drawUpArrow(int x, int y, int width, int height, u16 color);
void drawLeftArrow(int x, int y, int width, int height, u16 color);
void drawKey(int row, int col, int keyWidth);
void drawKeyboard();
void drawTextField();

void intToStr(int num, char* str) {
    if (num == 0) {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    int temp = num, len = 0;
    while (temp > 0) { len++; temp /= 10; }
    str[len] = '\0';
    while (num > 0) {
        str[--len] = (num % 10) + '0';
        num /= 10;
    }
}

void i2c_keyboard_callback(u8 status, char ch) {
    if (status == 1) { 
        if (ch >= 32 && ch <= 126) { 
            if (currentTextLen < 7089) {
                currentText[currentTextLen++] = ch;
                currentText[currentTextLen] = '\0';
                textChanged = true;
            }
        } else if (ch == 8 || ch == 127) { // Backspace / Delete
            if (currentTextLen > 0) {
                currentText[--currentTextLen] = '\0';
                textChanged = true;
            }
        } else if (ch == '\n' || ch == '\r') { // Enter
            if (currentTextLen < 7089) {
                currentText[currentTextLen++] = '\n';
                currentText[currentTextLen] = '\0';
                textChanged = true;
            }
        }
    }
}
// -------------------------------------------------------------------------

void toggleShift() { actShift = !actShift; }
void toggleChar()  { actChar = !actChar; }
void toggleSmile() { actSmile = !actSmile; }

void setKeyWidth() {
    for (int i = 0; i < kbRows; i++) {
        keyWidths[i] = (WIDTH / 10) - (keyMargin * 2);
    }
}

void drawUpArrow(int x, int y, int width, int height, u16 color) {
    int centerX = x + width / 2;
    int centerY = y + height / 2;
    DrawLine(centerX, centerY - 5, centerX, centerY + 5, color);
    DrawLine(centerX, centerY - 5, centerX - 5, centerY, color);
    DrawLine(centerX, centerY - 5, centerX + 5, centerY, color);
}

void drawLeftArrow(int x, int y, int width, int height, u16 color) {
    int centerX = x + width / 2;
    int centerY = y + height / 2;
    DrawLine(centerX - 5, centerY, centerX + 5, centerY, color);
    DrawLine(centerX - 5, centerY, centerX, centerY - 5, color);
    DrawLine(centerX - 5, centerY, centerX, centerY + 5, color);
}

void drawKey(int row, int col, int keyWidth) {
    const char* label;
    if (actChar) label = charLabel[row][col];
    else if (actSmile) label = smileLabel[row][col];
    else if (actShift) label = upperLabel[row][col];
    else label = smallLabel[row][col];
    
    int x, y, width;
    
    if (row == 0) {
        width = 2 * (WIDTH / kbCols) - (2 * keyMargin);
        x = col * (2 * (keyWidths[row] + keyMargin * 2)) + keyMargin;
    } else if (row == 4 && strcmp(label, " ") == 0) {
        width = 2 * (WIDTH / kbCols) - (2 * keyMargin);
        x = 8 * (keyWidths[row] + keyMargin * 2) + keyMargin;
    } else {
        width = keyWidths[row];
        x = col * (width + keyMargin * 2) + keyMargin;
    }

    y = HEIGHT - kbHeight + row * ((kbHeight / kbRows) - (keyMargin * 2)) + keyMargin + kbOffsetY;
    
    u16 fillColor;
    bool specialKey = (row == 0) && (strcmp(label, "CHAR") == 0 || strcmp(label, "SMILE") == 0 || strcmp(label, "\\") == 0 || strcmp(label, "TEST5") == 0 || strcmp(label, "123456") == 0);

    if (specialKey && !(row == startRow && col == startCol)) {
        fillColor = COL_DKGRAY;
    } else {
        fillColor = (row == startRow && col == startCol) ? COL_DKYELLOW : COL_BLACK;
    }

    int textX = x + (width / 2) - (DrawFontWidth / 2 * strlen(label));
    int textY = y + ((kbHeight / kbRows) / 2) - (DrawFontHeight / 2);

    DrawFrame(x, y, width, (kbHeight / kbRows) - (keyMargin * 2), COL_WHITE);
    DrawRect(x + 1, y + 1, width - 2, (kbHeight / kbRows) - (keyMargin * 2) - 2, fillColor);

    if (strcmp(label, "SHIFT") == 0) {
        drawUpArrow(x, y, width, (kbHeight / kbRows) - (keyMargin * 2), COL_WHITE);
    } else if (strcmp(label, "BACKSPACE") == 0) {
        drawLeftArrow(x, y, width, (kbHeight / kbRows) - (keyMargin * 2), COL_WHITE);
    } else {
        DrawText(label, textX, textY, COL_WHITE);
    }
}

void drawKeyboard() {
    for (int row = 0; row < kbRows; row++) {
        for (int col = 0; col < keyCols[row]; col++) {
            drawKey(row, col, keyWidths[row]);
        }
    }
}

void drawTextField() {
    int x = txtFldMargin;
    int y = txtFldMargin;
    int width = WIDTH - 2 * txtFldMargin;
    int height = txtFldHeight;
    
    DrawFrame(x, y, width, height, COL_WHITE);
    DrawRect(x + 1, y + 1, width - 2, height - 2, COL_BLACK);
    
    int maxLines = 14; 
    int linesShown = 0;
    int startPos = 0;
    char lineBuf[102]; 
    
    bool showCursor = (Time() / 500000) % 2 == 0; 
    
    while (linesShown < maxLines && startPos <= currentTextLen) {
        int endPos = startPos;
        while (endPos < currentTextLen && currentText[endPos] != '\n') {
            endPos++;
        }
        
        int copyLen = endPos - startPos;
        if (copyLen > 99) copyLen = 99; 
        
        memcpy(lineBuf, &currentText[startPos], copyLen);
        lineBuf[copyLen] = '\0'; 
        
        if (endPos == currentTextLen && showCursor) {
            lineBuf[copyLen] = '_';
            lineBuf[copyLen + 1] = '\0';
        }
        
        DrawText(lineBuf, x + 5, y + 5 + linesShown * 13, COL_WHITE);
        startPos = endPos + 1;
        linesShown++;
        
        if (endPos == currentTextLen) break;
    }
}

int main() {
    DrawClear();
    SelFont8x8();
    
    KeySetRawCallback(i2c_keyboard_callback); 
    
    drawTextField();
    setKeyWidth();
    drawKeyboard();
    
    DispUpdate();
    KeyFlush();

    bool needsUpdate = false;
    bool lastCursorState = false;

    while(true) {
        u8 key = KeyGetRel();

        if (textChanged) {
            needsUpdate = true;
            textChanged = false;
        }

        bool currentCursorState = (Time() / 500000) % 2 == 0;
        if (currentCursorState != lastCursorState) {
            lastCursorState = currentCursorState;
            needsUpdate = true; 
        }
        
        if (key != NOKEY && (key & KEY_RELEASE) == 0) { 
            switch (key) {
                case KEY_UP:
                    if (startRow > 0) {
                        if (startRow == 1) {
                            if (startCol < 2) startCol = 0; 
                            else if (startCol < 4) startCol = 1; 
                            else if (startCol < 6) startCol = 2; 
                            else if (startCol < 8) startCol = 3; 
                            else startCol = 4; 
                        }
                        startRow--;
                    }
                    needsUpdate = true;
                    break;
                    
                case KEY_DOWN:
                    if (startRow < kbRows - 1) {
                        if (startRow == 0) {
                            if (startCol == 0) startCol = 1; 
                            else if (startCol == 1) startCol = 3; 
                            else if (startCol == 2) startCol = 5; 
                            else if (startCol == 3) startCol = 7; 
                            else if (startCol == 4) startCol = 9; 
                        }
                        startRow++;
                        if (startRow == 4 && startCol > 7) startCol = 8;
                    }
                    needsUpdate = true;
                    break;
                    
                case KEY_LEFT:
                    if (startCol > 0) startCol--;
                    needsUpdate = true;
                    break;
                    
                case KEY_RIGHT:
                    if (startCol < keyCols[startRow] - 1) startCol++;
                    needsUpdate = true;
                    break;
                    
                case KEY_A:
                {
                    const char* selectedKey;
                    if (actChar) selectedKey = charLabel[startRow][startCol];
                    else if (actSmile) selectedKey = smileLabel[startRow][startCol];
                    else if (actShift) selectedKey = upperLabel[startRow][startCol];
                    else selectedKey = smallLabel[startRow][startCol];

                    if (strcmp(selectedKey, "SHIFT") == 0) { toggleShift(); needsUpdate = true; break; }
                    if (strcmp(selectedKey, "CHAR") == 0)  { toggleChar(); needsUpdate = true; break; }
                    if (strcmp(selectedKey, "SMILE") == 0) { toggleSmile(); needsUpdate = true; break; }

                    int lastNewLine = -1;
                    for (int i = currentTextLen - 1; i >= 0; i--) {
                        if (currentText[i] == '\n') {
                            lastNewLine = i;
                            break;
                        }
                    }
                    
                    int lineLength = (lastNewLine == -1) ? currentTextLen : (currentTextLen - 1 - lastNewLine);
                    int maxCharsLine = (WIDTH / DrawFontWidth) - 1;

                    if (lineLength >= maxCharsLine - 1 && strcmp(selectedKey, "BACKSPACE") != 0) {
                        if (currentTextLen + 1 < 7089) {
                            strcat(currentText, "\n");
                            currentTextLen++;
                        }
                    }

                    if (strcmp(selectedKey, "BACKSPACE") == 0) {
                        if (currentTextLen > 0) currentText[--currentTextLen] = '\0';
                    } else {
                        int len = strlen(selectedKey);
                        if (currentTextLen + len < 7089) {
                            strcat(currentText, selectedKey);
                            currentTextLen += len;
                        }
                    }
                    needsUpdate = true;
                    break;
                }
                
                case KEY_B:
                    if (currentTextLen + 1 < 7089) {
                        strcat(currentText, "\n");
                        currentTextLen++;
                    }
                    needsUpdate = true;
                    break;

                case KEY_X:
                {
                    DrawClearCol(COLOR_BACKGROUND);
                    enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW;
                    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
                    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];

                    bool ok = qrcodegen_encodeText(currentText, tempBuffer, qrcode, errCorLvl,
                    qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);

                    if (ok) {
                        int size = qrcodegen_getSize(qrcode);
                        int pixmodul = HEIGHT / size;
                        int pixelQR = size * pixmodul;
                        int osa_x = (WIDTH % pixelQR) / 2;
                        int osa_x_restore = osa_x;
                        int osa_y = (HEIGHT % pixelQR) / 2;

                        for (int y = 0; y < size; y++) {
                            for (int x = 0; x < size; x++) {
                                if (qrcodegen_getModule(qrcode, x, y)) DrawRect(osa_x, osa_y, pixmodul, pixmodul, COLOR_CODE);
                                else DrawRect(osa_x, osa_y, pixmodul, pixmodul, COLOR_BACKGROUND);
                                osa_x = osa_x + pixmodul;                
                            }
                            osa_y = osa_y + pixmodul;
                            osa_x = osa_x_restore;
                        }
                    } else {
                        DrawClearCol(COL_BLACK);
                        DrawText("Error, continue press Y", ((23*8)-WIDTH)/2, (HEIGHT-8)/2, COL_RED);
                    }

                    DispUpdate();
                    
                    while (true) {
                        u8 k = KeyGet();
                        if (k == KEY_B) break;
                        if (k == KEY_Y) ResetToBootLoader();
                    }
                    
                    DrawClear();
                    needsUpdate = true; 
                    break;
                }
                case KEY_Y:
                    ResetToBootLoader();
                    break;
            }
        }

        if (needsUpdate) {
            drawTextField();
            drawKeyboard();
            
            char pocetznaku_v_char[15];
            intToStr(currentTextLen, pocetznaku_v_char);
            
            int counterX = WIDTH - (5 * 8); 
            int counterY = txtFldMargin + txtFldHeight + 4; 
            
            DrawRect(counterX, counterY, 5 * 8, 8, COL_BLACK); 
            DrawText(pocetznaku_v_char, counterX, counterY, COL_WHITE);

            DispUpdate();
            needsUpdate = false;
        }
    }
    return 0;
}
