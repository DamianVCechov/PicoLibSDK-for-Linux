
// ****************************************************************************
//
//                                 Main code
//
// ****************************************************************************

#include "../include.h"
#include "main.h"
#include "saveonsdcard.h"

#define DIRPATH	"/PAINT/"

void paint()
{
 DrawClear();
 KeyFlush();
 DispUpdate();

 int strx, stry, radius = 5, krok = 1;
 u16 barva = COL_WHITE, barvabg = COL_BLACK;

 strx = WIDTH / 2;
 stry = HEIGHT / 2;

 DrawClear();
 DrawFillCircle(strx, stry, radius, barva);
 DispUpdate();

 while(1)
 {
  switch(KeyGet())
  {
	case KEY_RIGHT:				// Change color bag
		if(KeyPressed(KEY_B))
		{
			switch(barvabg)
			{
				case COL_WHITE:
					barvabg = COL_RED;
					DrawClearCol(barvabg);
					DispUpdate();
					break;
				case COL_RED:
					barvabg = COL_BLUE;
					DrawClearCol(barvabg);
					DispUpdate();
					break;
				case COL_BLUE:
					barvabg = COL_MAGENTA;
					DrawClearCol(barvabg);
					DispUpdate();
					break;
				case COL_MAGENTA:
					barvabg = COL_YELLOW;
					DrawClearCol(barvabg);
					DispUpdate();
					break;
				case COL_YELLOW:
					barvabg = COL_GREEN;
					DrawClearCol(barvabg);
					DispUpdate();
					break;
				case COL_GREEN:
					barvabg = COL_BLACK;
					DrawClearCol(barvabg);
					DispUpdate();
					break;
				case COL_BLACK:
					barvabg = COL_WHITE;
					DrawClearCol(barvabg);
					DispUpdate();
					break;
			}
		break;
		}
		if(KeyPressed(KEY_X))
		{
			switch(krok)
			{
				case 1:
					krok = 5;
					break;
				case 5:
					krok = 10;
					break;
				case 10:
					krok = 20;
					break;
				case 20:
					krok = 1;
					break;
			}
		break;
		}
		strx = strx + krok;
		DrawFillCircle(strx, stry, radius, barva);
		DispUpdate();
		break;
	case KEY_LEFT:
		if(KeyPressed(KEY_B))
		{
			switch(barvabg)
			{
				case COL_WHITE:
					barvabg = COL_BLACK;
					DrawClearCol(barvabg);
					DispUpdate();
					break;
				case COL_RED:
					barvabg = COL_WHITE;
					DrawClearCol(barvabg);
					DispUpdate();
					break;
				case COL_BLUE:
					barvabg = COL_RED;
					DrawClearCol(barvabg);
					DispUpdate();
					break;
				case COL_MAGENTA:
					barvabg = COL_BLUE;
					DrawClearCol(barvabg);
					DispUpdate();
					break;
				case COL_YELLOW:
					barvabg = COL_MAGENTA;
					DrawClearCol(barvabg);
					DispUpdate();
					break;
				case COL_GREEN:
					barvabg = COL_YELLOW;
					DrawClearCol(barvabg);
					DispUpdate();
					break;
				case COL_BLACK:
					barvabg = COL_GREEN;
					DrawClearCol(barvabg);
					DispUpdate();
					break;
			}
		break;
		}
		if(KeyPressed(KEY_X))
		{
			switch(krok)
			{
				case 1:
					krok = 20;
					break;
				case 5:
					krok = 1;
					break;
				case 10:
					krok = 5;
					break;
				case 20:
					krok = 10;
					break;
			}
		break;
		}
		strx = strx - krok;
		DrawFillCircle(strx, stry, radius, barva);
		DispUpdate();
		break;
	case KEY_UP:
		if(KeyPressed(KEY_B))
		{
			switch(barva)
			{
				case COL_WHITE:
					barva = COL_RED;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case COL_RED:
					barva = COL_BLUE;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case COL_BLUE:
					barva = COL_MAGENTA;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case COL_MAGENTA:
					barva = COL_YELLOW;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case COL_YELLOW:
					barva = COL_GREEN;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case COL_GREEN:
					barva = COL_BLACK;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case COL_BLACK:
					barva = COL_WHITE;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
			}
		break;
		}
		if(KeyPressed(KEY_X))
		{
			switch(radius)
			{
				case 1:
					radius = 5;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case 5:
					radius = 10;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case 10:
					radius = 20;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case 20:
					radius = 40;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case 40:
					radius = 1;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
			}
		break;
		}
		stry = stry - krok;
		DrawFillCircle(strx, stry, radius, barva);
		DispUpdate();
		break;
	case KEY_DOWN:
		if(KeyPressed(KEY_B))
		{
			switch(barva)
			{
				case COL_WHITE:
					barva = COL_BLACK;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case COL_RED:
					barva = COL_WHITE;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case COL_BLUE:
					barva = COL_RED;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case COL_MAGENTA:
					barva = COL_BLUE;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case COL_YELLOW:
					barva = COL_MAGENTA;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case COL_GREEN:
					barva = COL_YELLOW;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case COL_BLACK:
					barva = COL_GREEN;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
			}
		break;
		}
		if(KeyPressed(KEY_X))
		{
			switch(radius)
			{
				case 1:
					radius = 40;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case 5:
					radius = 1;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case 10:
					radius = 5;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case 20:
					radius = 10;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
				case 40:
					radius = 20;
					DrawFillCircle(strx, stry, radius, barva);
					DispUpdate();
					break;
			}
		break;
		}
		stry = stry + krok;
		DrawFillCircle(strx, stry, radius, barva);
		DispUpdate();
		break;
	case KEY_A:
		break;
	case KEY_Y:	
		SavePicture();
		DrawTextBg2("Saving picture!", (WIDTH-15*8*2)/2, (HEIGHT-8*2)/2, COL_WHITE, COL_BLACK);
		DispUpdate();
		WaitMs(1000);
		return;
     }
   }
 }

void slide()
{
	int i;
	char name[30];
	sFile file;

	while (True)
	{
RESTART:
		// mounting SD disk
		while (!DiskMount())
		{
			DrawClear();
			DrawText2("Insert SD Card", (WIDTH-14*8*2)/2, (HEIGHT-16)/2, COL_WHITE);
			DispUpdateAll();
			if (KeyGet() == KEY_Y) ResetToBootLoader();
			WaitMs(1000);
		}

		// check file
		while (!FileExist(DIRPATH "1.BMP"))
		{
			DrawClear();
			DrawText("Canot find " DIRPATH "1.BMP", (WIDTH-23*8)/2, (HEIGHT-8)/2, COL_WHITE);
			DispUpdateAll();
			WaitMs(1000);
			DiskUnmount();
			goto RESTART;
		}

		// show files
		for (i = 1; i < 10000; i++)
		{
			// prepare filename
			MemPrint(name, 30, "%s%d.BMP", DIRPATH, i);

			// open file
			if (!FileOpen(&file, name)) goto RESTART;

			// skip header
			FileSeek(&file, 0x46);

			// load image
			FileRead(&file, FrameBuf, FRAMESIZE*sizeof(FRAMETYPE));

			// close vile
			FileClose(&file);

			// update image
			DispUpdateAll();

			// exit
			if (KeyGet() == KEY_Y) return;

			// wait
			WaitMs(4000);
		}
	}
}

int main()
{
RESTORE:
	u8 key;
	DrawClear();
	KeyFlush();
	DrawImgRle(MenuImg, MenuImg_Pal, 0, 0, 320, 240);
	DispUpdate();
	while (True)
	{
		while ((key = KeyGet()) == NOKEY) {}
		switch (key)
		{
			case KEY_LEFT:
				paint();
				goto RESTORE;
				break;

			case KEY_RIGHT:
				slide();
				goto RESTORE;
				break;
			case KEY_Y: ResetToBootLoader();
		}
	}
}

