// ****************************************************************************
//
//                       Pico Commander - file manager
//                       Dvoupanelový správce souborů
//
//                          Author: @DamianVCechov
//
// ****************************************************************************

#include "../include.h"
#include <string.h>
#include <stdio.h>

// --- CONFIGURATION ---
#define MAX_FILES 150        // Max files in directory
#define MAX_PATH_LEN 256     // Max lenght path
#define VISIBLE_FILES ((HEIGHT - 34) / 12) // Dynamic number of displayed files by display height

// --- COLOR THEME ---
#define MC_BG        COL_BLUE    // Dashboard Background
#define MC_BORDER    COL_CYAN    // Border
#define MC_DIR       COL_YELLOW  // Color Directories
#define MC_FILE      COL_WHITE   // Color Files
#define MC_SEL_BG    COL_CYAN    // Color Selection Background
#define MC_SEL_TEXT  COL_BLACK   // Color Text Selection
#define MC_BOTTOM    COL_CYAN    // Color Menu hint bar
#define MC_DIM       COL_LTGRAY  // Color text for inactive elements
#define MC_MARKED    COL_RED     // Color marked items

// --- STRUCTURS ---
struct FileEntry {
    char name[13];  // 8.3 format + null terminator
    bool isDir;
    u32 size;       // File size
    bool marked;
};

struct Panel {
    char path[MAX_PATH_LEN];
    FileEntry files[MAX_FILES];
    int count;      // Total number of items loaded
    int selected;   // Index of the currently selected item
    int scroll;     // First viewed item index (for scrolling)
};

Panel leftPanel;
Panel rightPanel;
bool leftActive = true; // True = left dashboard is aktive, False = right

// --- POMOCNÉ FUNKCE PRO CESTY ---

void GoUpDir(char* path) {
    int len = strlen(path);
    if (len <= 1) return; 

    char* lastBackslash = strrchr(path, '\\');
    if (lastBackslash != NULL) {
        if (lastBackslash == path) {
            path[1] = '\0'; 
        } else {
            *lastBackslash = '\0'; 
        }
    }
}

void MakeFullPath(char* dest, const char* path, const char* filename) {
    strcpy(dest, path);
    int len = strlen(dest);
    if (len > 0 && dest[len - 1] != '\\') {
        strcat(dest, "\\"); 
    }
    strcat(dest, filename);
}

// Case-insensitive
int CompareNames(const char* a, const char* b) {
    while (*a && *b) {
        char ca = (*a >= 'a' && *a <= 'z') ? *a - 32 : *a;
        char cb = (*b >= 'a' && *b <= 'z') ? *b - 32 : *b;
        if (ca != cb) return ca - cb;
        a++; b++;
    }
    return *a - *b;
}

void LoadDirectory(Panel* p) {
    sFile dir; 
    sFileInfo fno;
    
    p->count = 0;
    p->selected = 0;
    p->scroll = 0;

    if (FindOpen(&dir, p->path)) {
        if (strcmp(p->path, "\\") != 0) {
            strcpy(p->files[p->count].name, "..");
            p->files[p->count].isDir = true;
            p->files[p->count].size = 0;
            p->files[p->count].marked = false;
            p->count++;
        }

        while (FindNext(&dir, &fno, 0xFF, "*.*")) {
            if (fno.name[0] == 0) continue; 
            if (p->count >= MAX_FILES) break;
            if (fno.attr & ATTR_VOL) continue;
            if (strcmp(fno.name, ".") == 0 || strcmp(fno.name, "..") == 0) continue;

            strcpy(p->files[p->count].name, fno.name);
            p->files[p->count].isDir = (fno.attr & ATTR_DIR) != 0;
            p->files[p->count].size = fno.size;
            p->files[p->count].marked = false;
            
            p->count++;
        }
        FindClose(&dir);

        // Ordering
        int startIdx = (p->count > 0 && strcmp(p->files[0].name, "..") == 0) ? 1 : 0;
        for (int i = startIdx; i < p->count - 1; i++) {
            for (int j = i + 1; j < p->count; j++) {
                bool swap = false;
                
                if (p->files[i].isDir && !p->files[j].isDir) {
                    swap = false;
                } else if (!p->files[i].isDir && p->files[j].isDir) {
                    swap = true; // Directory first
                } else {
                    if (CompareNames(p->files[i].name, p->files[j].name) > 0) {
                        swap = true;
                    }
                }

                if (swap) {
                    FileEntry temp = p->files[i];
                    p->files[i] = p->files[j];
                    p->files[j] = temp;
                }
            }
        }

    } else {
        strcpy(p->files[p->count].name, "<Read Error>");
        p->files[p->count].isDir = false;
        p->files[p->count].size = 0;
        p->files[p->count].marked = false;
        p->count++;
    }
}

bool CopyFile(const char* srcPath, const char* dstPath) {
    sFile fSrc, fDst;
    
    if (!FileOpen(&fSrc, srcPath)) return false;
    
    if (!FileCreate(&fDst, dstPath)) {
        FileClose(&fSrc);
        return false;
    }

    u32 bw, br;
    u8 buffer[512]; 
    bool success = true;

    while ((br = FileRead(&fSrc, buffer, sizeof(buffer))) > 0) {
        bw = FileWrite(&fDst, buffer, br);
        if (bw < br) {
            success = false;
            break;
        }
    }

    FileClose(&fSrc);
    FileClose(&fDst);
    return success;
}

void ProcessAction(Panel* srcPanel, Panel* dstPanel, int fileIdx, bool isCopy, bool isMove, bool isDelete) {
    char srcPath[MAX_PATH_LEN];
    char dstPath[MAX_PATH_LEN];
    
    MakeFullPath(srcPath, srcPanel->path, srcPanel->files[fileIdx].name);
    
    if (isCopy || isMove) {
        MakeFullPath(dstPath, dstPanel->path, srcPanel->files[fileIdx].name);
    }

    if (isCopy) {
        CopyFile(srcPath, dstPath);
    } 
    else if (isMove) {
        if (CopyFile(srcPath, dstPath)) {
            FileDelete(srcPath); 
        }
    } 
    else if (isDelete) {
        FileDelete(srcPath); 
    }
}

void ShowMessage(const char* msg) {
    int msgX = (WIDTH - 160) / 2;
    int msgY = (HEIGHT - 40) / 2;
    DrawRect(msgX, msgY, 160, 40, MC_BORDER); 
    DrawRect(msgX + 1, msgY + 1, 158, 38, MC_BG);     
    DrawText(msg, (WIDTH - (strlen(msg) * 8)) / 2, msgY + 16, MC_FILE); 
    DispUpdate();
}

// --- USER INTERFACE  ---

void DrawPanel(Panel* p, int x, int y, int w, int h, bool isActive) {
    u16 borderColor = isActive ? MC_BORDER : MC_DIM; 
    
    DrawRect(x, y, w, 1, borderColor);
    DrawRect(x, y + h - 1, w, 1, borderColor);
    DrawRect(x, y, 1, h, borderColor);
    DrawRect(x + w - 1, y, 1, h, borderColor);
    DrawRect(x + 1, y + 1, w - 2, h - 2, MC_BG);

    const char* displayPath = p->path;
    int pathLen = strlen(displayPath);
    char shortPath[24];
    if (pathLen > 18) {
        strcpy(shortPath, "..");
        strcat(shortPath, displayPath + pathLen - 16);
        displayPath = shortPath;
        pathLen = strlen(displayPath);
    }
    
    int pathX = x + (w - (pathLen * 8)) / 2; 
    DrawText(displayPath, pathX, y + 2, isActive ? MC_DIR : MC_DIM);

    int listY = y + 16; 
    for (int i = 0; i < VISIBLE_FILES; i++) {
        int fileIdx = p->scroll + i;
        if (fileIdx >= p->count) break;

        int itemY = listY + (i * 12); 
        
        // Cursor
        if (isActive && fileIdx == p->selected) {
            DrawRect(x + 1, itemY + 1, w - 2, 12, MC_SEL_BG);
        }

        // Marked (*)
        u16 textColor;
        if (isActive && fileIdx == p->selected) {
            textColor = MC_SEL_TEXT; 
        } else if (p->files[fileIdx].marked) {
            textColor = MC_MARKED;
        } else {
            textColor = p->files[fileIdx].isDir ? MC_DIR : MC_FILE; 
        }

        char displayName[16];
        if (p->files[fileIdx].isDir) {
            snprintf(displayName, sizeof(displayName), "[%s]", p->files[fileIdx].name);
        } else {
            strcpy(displayName, p->files[fileIdx].name);
        }

        if (p->files[fileIdx].marked) {
            DrawText("*", x + 4, itemY, textColor);
            DrawText(displayName, x + 12, itemY, textColor);
        } else {
            DrawText(displayName, x + 4, itemY, textColor);
        }

        if (!p->files[fileIdx].isDir && p->files[fileIdx].size > 0) {
            char sizeStr[16];
            int nameLen = strlen(p->files[fileIdx].name);
            
            snprintf(sizeStr, sizeof(sizeStr), "%lu", p->files[fileIdx].size);
            
            // If name file + size in byte >= 19 characters, then kB
            if (nameLen + strlen(sizeStr) >= 19) {
                snprintf(sizeStr, sizeof(sizeStr), "%lukB", p->files[fileIdx].size / 1024);
                
                // If name file + size in kB >= 19 characters, then MB
                if (nameLen + strlen(sizeStr) >= 19) {
                    snprintf(sizeStr, sizeof(sizeStr), "%luMB", p->files[fileIdx].size / (1024 * 1024));
                }
            }
            
            int sizeX = x + w - (strlen(sizeStr) * 8) - 4;
            DrawText(sizeStr, sizeX, itemY, textColor);
        }
    }
}

// --- MAIN ---

int main() {
    DeviceInit();
    DiskMount();

    strcpy(leftPanel.path, "\\");
    strcpy(rightPanel.path, "\\");

    LoadDirectory(&leftPanel);
    LoadDirectory(&rightPanel);

    while (true) {
        Panel* activePanel = leftActive ? &leftPanel : &rightPanel;
        Panel* targetPanel = leftActive ? &rightPanel : &leftPanel;

        DrawClear();
        
        int panelWidth = WIDTH / 2;
        int panelHeight = HEIGHT - 18;
        DrawPanel(&leftPanel, 0, 0, panelWidth, panelHeight, leftActive);
        DrawPanel(&rightPanel, panelWidth, 0, panelWidth, panelHeight, !leftActive);

        DrawRect(0, panelHeight, WIDTH, 18, MC_BOTTOM); 
        DrawText("F1:Open/Menu F2:Back F3:Mark F4:Exit", (WIDTH-36*8)/2, HEIGHT-8*2, COL_BLACK);
        
        DispUpdate();

        char key = KeyGet();
        
        if (key != 0) { 
            if (key == KEY_UP) {
                if (activePanel->selected > 0) {
                    activePanel->selected--;
                    if (activePanel->selected < activePanel->scroll) {
                        activePanel->scroll = activePanel->selected;
                    }
                }
            } 
            else if (key == KEY_DOWN) {
                if (activePanel->selected < activePanel->count - 1) {
                    activePanel->selected++;
                    if (activePanel->selected >= activePanel->scroll + VISIBLE_FILES) {
                        activePanel->scroll = activePanel->selected - VISIBLE_FILES + 1;
                    }
                }
            }
            else if (key == KEY_LEFT) {
                leftActive = true;
            }
            else if (key == KEY_RIGHT) {
                leftActive = false;
            }
            else if (key == KEY_B) { // Back
                GoUpDir(activePanel->path);
                LoadDirectory(activePanel);
            }
            else if (key == KEY_X) { // Mark
                if (strcmp(activePanel->files[activePanel->selected].name, "..") != 0 && 
                    strcmp(activePanel->files[activePanel->selected].name, "<Read Error>") != 0) {
                    
                    activePanel->files[activePanel->selected].marked = !activePanel->files[activePanel->selected].marked;
                    
                    if (activePanel->selected < activePanel->count - 1) {
                        activePanel->selected++;
                        if (activePanel->selected >= activePanel->scroll + VISIBLE_FILES) {
                            activePanel->scroll = activePanel->selected - VISIBLE_FILES + 1;
                        }
                    }
                }
            }
            else if (key == KEY_A) { 
                bool hasMarked = false;
                for (int i = 0; i < activePanel->count; i++) {
                    if (activePanel->files[i].marked) {
                        hasMarked = true;
                        break;
                    }
                }

                // If nothing is marked and we are on the folder, key_A will open it right away
                if (!hasMarked && activePanel->count > 0 && activePanel->files[activePanel->selected].isDir) {
                    if (strcmp(activePanel->files[activePanel->selected].name, "..") == 0) {
                        GoUpDir(activePanel->path);
                    } else if (strcmp(activePanel->files[activePanel->selected].name, "<Read Error>") != 0) {
                        char tempPath[MAX_PATH_LEN];
                        MakeFullPath(tempPath, activePanel->path, activePanel->files[activePanel->selected].name);
                        strcpy(activePanel->path, tempPath);
                        
                    }
                    LoadDirectory(activePanel);
                    continue;                 
                }

                // ACTION MENU
                DrawRect(0, HEIGHT - 18, WIDTH, 18, COL_MAGENTA);
                DrawText("F1:Copy F2:Move F3:Delete F4:Storno", (WIDTH-35*8)/2, HEIGHT-8*2, COL_WHITE);
                DispUpdate();

                char actionKey = 0;

                while(true) {
                    actionKey = KeyGet();
                    if (actionKey == KEY_A || actionKey == KEY_B || actionKey == KEY_X || actionKey == KEY_Y) {
                        break;
                    }
                }

                if (actionKey != KEY_Y) { 
                    bool actionCopy = (actionKey == KEY_A);
                    bool actionMove = (actionKey == KEY_B);
                    bool actionDelete = (actionKey == KEY_X);
                    
                    ShowMessage("Working...");
                    WaitMs(50);

                    if (hasMarked) {
                        for (int i = 0; i < activePanel->count; i++) {
                            if (activePanel->files[i].marked) {
                                ProcessAction(activePanel, targetPanel, i, actionCopy, actionMove, actionDelete);
                            }
                        }
                    } else {
                        if (strcmp(activePanel->files[activePanel->selected].name, "..") != 0 &&
                            strcmp(activePanel->files[activePanel->selected].name, "<Read Error>") != 0) {
                            ProcessAction(activePanel, targetPanel, activePanel->selected, actionCopy, actionMove, actionDelete);
                        }
                    }
                    
                    LoadDirectory(&leftPanel);
                    LoadDirectory(&rightPanel);
                }
            }
            else if (key == KEY_Y) { // Exit
#if USE_SCREENSHOT		// use screen shots
	            ScreenShot();
#endif
                ResetToBootLoader();
            }
        }
    }
    return 0;
}
