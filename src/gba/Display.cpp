#include "Display.h"
#include "../System.h"
#include <algorithm>

namespace Display
{
static const int width = 240;
static const int height = 160;

static u32 colorMap[0x10000];
static u32 *pix;

void initColorMap(int redShift, int greenShift, int blueShift)
{
	for (int i = 0; i < 0x10000; i++)
	{
		colorMap[i] = ((i & 0x1f) << redShift) |
		              (((i & 0x3e0) >> 5) << greenShift) |
		              (((i & 0x7c00) >> 10) << blueShift);
	}
}

void saveState(gzFile gzFile)
{
	utilGzWrite(gzFile, pix, 4 * width * height);
}

void readState(gzFile gzFile)
{
	utilGzRead(gzFile, pix, 4 * width * height);
}

void uninit()
{
	if (pix)
	{
		delete pix;
		pix = 0;
	}
}

bool init()
{
	pix = new u32[width * height];
	return pix != 0;
}

void clear()
{
	std::fill(pix, pix + width * height, 0);
}

void drawLine(int line, u32* src)
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

void drawScreen()
{
	systemDrawScreen(pix);
}

} // namespace Display
