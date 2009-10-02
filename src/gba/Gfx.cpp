#include "Gfx.h"
#include "GfxHelpers.h"
#include "GBA.h"
#include "Globals.h"

namespace GFX
{

typedef void (*InternalLineRenderer)();

struct ModeLineRenderers {
	InternalLineRenderer simple;
	InternalLineRenderer noWindow;
	InternalLineRenderer all;
};

static const ModeLineRenderers lineRenderers[] =
{
	{ mode0RenderLine, mode0RenderLineNoWindow, mode0RenderLineAll },
	{ mode1RenderLine, mode1RenderLineNoWindow, mode1RenderLineAll },
	{ mode2RenderLine, mode2RenderLineNoWindow, mode2RenderLineAll },
	{ mode3RenderLine, mode3RenderLineNoWindow, mode3RenderLineAll },
	{ mode4RenderLine, mode4RenderLineNoWindow, mode4RenderLineAll },
	{ mode5RenderLine, mode5RenderLineNoWindow, mode5RenderLineAll }
};

static InternalLineRenderer internalRenderLine = lineRenderers[0].simple;

int coeff[32] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
};

u32 line0[240];
u32 line1[240];
u32 line2[240];
u32 line3[240];
u32 lineOBJ[240];
u32 lineOBJWin[240];
u32 lineMix[240];
bool gfxInWin0[240];
bool gfxInWin1[240];
int lineOBJpixleft[128];

int gfxBG2X = 0;
int gfxBG2Y = 0;
int gfxBG3X = 0;
int gfxBG3Y = 0;

void renderLine()
{
	if (DISPCNT & 0x80)
	{
		for (int x = 0; x < 240; x++)
		{
			lineMix[x] = 0x7fff;
		}
		return;
	}

	internalRenderLine();
}

void updateBG2X()
{
	gfxBG2X = (BG2X_L) | ((BG2X_H & 0x07FF)<<16);
	if (BG2X_H & 0x0800)
		gfxBG2X |= 0xF8000000;
}

void updateBG2Y()
{
	gfxBG2Y = (BG2Y_L) | ((BG2Y_H & 0x07FF)<<16);
	if (BG2Y_H & 0x0800)
		gfxBG2Y |= 0xF8000000;
}

void updateBG3X()
{
	gfxBG3X = (BG3X_L) | ((BG3X_H & 0x07FF)<<16);
	if (BG3X_H & 0x0800)
		gfxBG3X |= 0xF8000000;
}

void updateBG3Y()
{
	gfxBG3Y = (BG3Y_L) | ((BG3Y_H & 0x07FF)<<16);
	if (BG3Y_H & 0x0800)
		gfxBG3Y |= 0xF8000000;
}

void newFrame()
{
	updateBG2X();
	updateBG2Y();
	updateBG3X();
	updateBG3Y();
}

void chooseRenderer()
{
	bool fxOn = ((BLDMOD>>6)&3) != 0;
	bool windowOn = (layerEnable & 0x6000) ? true : false;
	int mode = DISPCNT & 7;

	if (mode > 5)
		return;

	if (!fxOn && !windowOn && !(layerEnable & 0x8000))
	{
		internalRenderLine = lineRenderers[mode].simple;
	}
	else if (fxOn && !windowOn && !(layerEnable & 0x8000))
	{
		internalRenderLine = lineRenderers[mode].noWindow;
	}
	else
	{
		internalRenderLine = lineRenderers[mode].all;
	}
}

void clearRenderBuffers(bool force)
{
	if (!(layerEnable & 0x0100) || force)
	{
		gfxClearArray(line0);
	}
	if (!(layerEnable & 0x0200) || force)
	{
		gfxClearArray(line1);
	}
	if (!(layerEnable & 0x0400) || force)
	{
		gfxClearArray(line2);
	}
	if (!(layerEnable & 0x0800) || force)
	{
		gfxClearArray(line3);
	}
}

void updateWindow0()
{
	int x00 = WIN0H>>8;
	int x01 = WIN0H & 255;

	if (x00 <= x01)
	{
		for (int i = 0; i < 240; i++)
		{
			gfxInWin0[i] = (i >= x00 && i < x01);
		}
	}
	else
	{
		for (int i = 0; i < 240; i++)
		{
			gfxInWin0[i] = (i >= x00 || i < x01);
		}
	}
}

void updateWindow1()
{
	int x00 = WIN1H>>8;
	int x01 = WIN1H & 255;

	if (x00 <= x01)
	{
		for (int i = 0; i < 240; i++)
		{
			gfxInWin1[i] = (i >= x00 && i < x01);
		}
	}
	else
	{
		for (int i = 0; i < 240; i++)
		{
			gfxInWin1[i] = (i >= x00 || i < x01);
		}
	}
}

} // namespace GFX
