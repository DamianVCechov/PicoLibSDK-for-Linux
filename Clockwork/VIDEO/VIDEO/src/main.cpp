// ****************************************************************************
//
//                                 Main code
//
// ****************************************************************************

#include "../include.h"

extern "C" {
#include "../lib/lib_video_clockwork.h"
}

// Overclocking
#include "../../../_lib/inc/lib_sd.h"
#include "../../../_lib/inc/lib_pwmsnd.h"
#include "../../../_sdk/inc/sdk_spi.h"

#define PATH "/VIDEO" // video path

#define MAXFILES 14

// video file
typedef struct {
	char	name[12+1];	// video file name
	u8	namelen;	// length of file name
	int	len;		// length in seconds
} sVideoFile;

// list of video files
sVideoFile FileList[MAXFILES];
int FileListNum = 0;
int FileListSel = 0;
Bool CtrlIsOn = False; // control is ON
Bool MuteIsOn = False; // mute is ON
s8 Volume = VIDEO_VOLUMEDEF; // current volume

// wait mounting SD card (return False to break)
Bool WaitMount()
{
	DrawClear();
	SelFont8x16();
	DrawText2("Insert SD card", (WIDTH-14*8*2)/2, (HEIGHT-8*2)/2, COL_WHITE);
	DispUpdate();

	while (!DiskMount())
	{
		if (KeyGet() == KEY_Y) return False;
	}
	return True;
}

// load file list (returns False to break)
Bool LoadFileList()
{
	sFile find;
	sFileInfo info;
	sVideoFile* vid;
	sVideoFile tmp;
	int i;

	if (!WaitMount()) return False;

	DrawClear();
	DispUpdate();
	SelFont8x16();

	if (!SetDir(PATH))
	{
		DrawText("No directory " PATH, 0, 0, COL_WHITE);
		DispUpdate();
		WaitMs(2000);
		return False;
	}

	if (!FindOpen(&find, ""))
	{
		DrawText("No video files", 0, 0, COL_WHITE);
		DispUpdate();
		WaitMs(2000);
		return False;
	}

	for (FileListNum = 0; FileListNum < MAXFILES; FileListNum++)
	{
		if (!FindNext(&find, &info, ATTR_ARCH, "*.VID")) break;

		vid = &FileList[FileListNum];
		memcpy(vid->name, info.name, 12+1);
		vid->namelen = info.namelen;
		vid->len = (info.size / VIDEO_FRAMESIZE) / VIDEO_FPS;
	}

	FindClose(&find);

	if (FileListSel >= FileListNum) FileListSel = FileListNum-1;
	if (FileListSel < 0) FileListSel = 0;

	for (i = 0; i < FileListNum-1;)
	{
		if (strcmp(FileList[i].name, FileList[i+1].name) > 0)
		{
			memcpy(&tmp, &FileList[i], sizeof(sVideoFile));
			memcpy(&FileList[i], &FileList[i+1], sizeof(sVideoFile));
			memcpy(&FileList[i+1], &tmp, sizeof(sVideoFile));
			if (i > 0) i--; else i++;
		}
		else i++;
	}

	return True;
}

// display file list
void DispFileList()
{
	int i, len;
	u16 fgcol, bgcol;
	char buf[31];
	sVideoFile* vf;

	DrawClear();
	SelFont8x16();

	if (FileListNum == 0)
	{
		DrawText("No video files", 0, 0, COL_WHITE);
		DispUpdate();
	}

	for (i = 0; i < FileListNum; i++)
	{
		fgcol = COL_WHITE;
		bgcol = COL_BLACK;

		if (i == FileListSel)
		{
			fgcol = COL_BLACK;
			bgcol = COL_WHITE;
		}

		vf = &FileList[i];
		len = vf->len;
		if (len > 5999) len = 5999;
		MemPrint(buf, 30, " % 12s %02d:%02d ", vf->name, len/60, len%60);
		DrawTextBg(buf, 0, i*16, fgcol, bgcol);
	}
	DispUpdate();
}

// play one video
void PlayVideo(const char* filename)
{
	if (!WaitMount()) return;

	DrawClear();
	DispUpdate();

	sVideo video;
	if (SetDir(PATH) && VideoOpen(&video, filename))
	{
		VideoSetCtrl(&video, CtrlIsOn);	
		VideoSetVol(&video, Volume);
		VideoSetMute(&video, MuteIsOn);

		while (video.frame < video.frames)
		{
			if (!video.pause)
			{
				if (!VideoPlayFrame(&video)) break;
			}

			switch (KeyGet())
			{
			case KEY_LEFT:
				VideoShiftPos(&video, -15);
				KeyFlush();
				break;
			case KEY_RIGHT:
				VideoShiftPos(&video, +15);
				KeyFlush();
				break;
			case KEY_UP:
				VideoSetVol(&video, video.volume + 1);
				Volume = video.volume;
				break;
			case KEY_DOWN:
				VideoSetVol(&video, video.volume - 1);
				Volume = video.volume;
				break;
			case KEY_A:
				VideoSetPause(&video, !video.pause);
				break;
			case KEY_B:
				VideoSetMute(&video, !video.mute);
				MuteIsOn = video.mute;
				break;
			case KEY_X:
#if USE_SCREENSHOT
				ScreenShot();
#endif
				VideoSetCtrl(&video, !video.ctrl);
				CtrlIsOn = video.ctrl;
				break;
			case KEY_Y:
				video.frame = video.frames;
				break;
			}
		}
		VideoClose(&video);
	}
	else
	{
		SelFont8x16();
		DrawTextBg("Cannot open video file", (WIDTH-22*8)/2, (HEIGHT-8)/2, COL_YELLOW, COL_RED);
		DispUpdate();
		WaitMs(500);
		KeyFlush();
		while (KeyGet() == NOKEY) {}
	}
	DrawClear();
	DispUpdate();
}

int main()
{
	// Speed CPU pro ILI9488 (320x320)
	ClockPllSysFreq(250000000);

	// Reinitial
#if USE_SD
	SDInit();
#endif

#if USE_PWMSND
	PWMSndInit();
#endif
	
	// SPI (60MHz)
	SPI_Init(DISP_SPI, 60000000); 

	FileListSel = 0;
	Bool ok;

	while (True)
	{
		if (!LoadFileList()) ResetToBootLoader();

		DispFileList();

		ok = False;
		while (!ok)
		{
			switch (KeyGet())
			{
			case KEY_Y:
				ResetToBootLoader();
				break;
			case KEY_UP:
				FileListSel--;
				if (FileListSel < 0) FileListSel = FileListNum-1;
				DispFileList();
				break;
			case KEY_DOWN:
				FileListSel++;
				if (FileListSel >= FileListNum) FileListSel = 0;
				DispFileList();
				break;
			case KEY_LEFT:
				FileListSel = 0;
				DispFileList();
				break;
			case KEY_RIGHT:
				FileListSel = FileListNum - 1;
				if (FileListSel < 0) FileListSel = 0;
				DispFileList();
				break;
			case KEY_A:
				if (FileListSel < FileListNum) ok = True;
				break;
			}
		}
		PlayVideo(FileList[FileListSel].name);
	}
}
