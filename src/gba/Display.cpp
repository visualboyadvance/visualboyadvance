// VBA-M, A Nintendo Handheld Console Emulator
// Copyright (C) 2008 VBA-M development team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2, or(at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include "Display.h"

#include "../common/Util.h"

#include <string.h>

static const int width = 240;
static const int height = 160;

static u32 colorMap[0x10000];
static guint32 *pix;
static DisplayDriver *displayDriver = NULL;

void display_init_color_map(int redShift, int greenShift, int blueShift)
{
	for (int i = 0; i < 0x10000; i++)
	{
		colorMap[i] = ((i & 0x1f) << redShift) |
		              (((i & 0x3e0) >> 5) << greenShift) |
		              (((i & 0x7c00) >> 10) << blueShift);
	}
}

void display_save_state(gzFile gzFile)
{
	utilGzWrite(gzFile, pix, 4 * width * height);
}

void display_read_state(gzFile gzFile)
{
	utilGzRead(gzFile, pix, 4 * width * height);
}

void display_free()
{
	g_free(pix);
	pix = NULL;
	displayDriver = NULL;
}

void display_init(DisplayDriver *driver)
{
	g_assert(driver != NULL);

	displayDriver = driver;

	pix = (guint32 *)g_malloc(width * height * sizeof(guint32));
}

void display_clear()
{
	memset(pix, width * height * sizeof(guint32), 0);
}

void display_draw_line(int line, u32* src)
{
	u32 *dest = pix + width * line;
	for (int x = 0; x < width; )
	{
		*dest++ = colorMap[src[x++] & 0xFFFF];
		*dest++ = colorMap[src[x++] & 0xFFFF];
		*dest++ = colorMap[src[x++] & 0xFFFF];
		*dest++ = colorMap[src[x++] & 0xFFFF];

		*dest++ = colorMap[src[x++] & 0xFFFF];
		*dest++ = colorMap[src[x++] & 0xFFFF];
		*dest++ = colorMap[src[x++] & 0xFFFF];
		*dest++ = colorMap[src[x++] & 0xFFFF];

		*dest++ = colorMap[src[x++] & 0xFFFF];
		*dest++ = colorMap[src[x++] & 0xFFFF];
		*dest++ = colorMap[src[x++] & 0xFFFF];
		*dest++ = colorMap[src[x++] & 0xFFFF];

		*dest++ = colorMap[src[x++] & 0xFFFF];
		*dest++ = colorMap[src[x++] & 0xFFFF];
		*dest++ = colorMap[src[x++] & 0xFFFF];
		*dest++ = colorMap[src[x++] & 0xFFFF];
	}
}

void display_draw_screen()
{
	displayDriver->drawScreen(displayDriver, pix);
}
