#include "Gfx.h"
#include "GfxHelpers.h"
#include "Globals.h"

typedef void (*InternalLineRenderer)();

typedef struct ModeLineRenderers ModeLineRenderers;
struct ModeLineRenderers {
	InternalLineRenderer simple;
	InternalLineRenderer noWindow;
	InternalLineRenderer all;
};

static const ModeLineRenderers lineRenderers[] =
{
	{ gfx_mode0_line_render, gfx_mode0_line_render_no_window, gfx_mode0_line_render_all },
	{ gfx_mode1_line_render, gfx_mode1_line_render_no_window, gfx_mode1_line_render_all },
	{ gfx_mode2_line_render, gfx_mode2_line_render_no_window, gfx_mode2_line_render_all },
	{ gfx_mode3_line_render, gfx_mode3_line_render_no_window, gfx_mode3_line_render_all },
	{ gfx_mode4_line_render, gfx_mode4_line_render_no_window, gfx_mode4_line_render_all },
	{ gfx_mode5_line_render, gfx_mode5_line_render_no_window, gfx_mode5_line_render_all }
};

static InternalLineRenderer internalRenderLine = NULL;

int gfxCoeff[32] =
{
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
	16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16
};

u32 gfxLine0[240];
u32 gfxLine1[240];
u32 gfxLine2[240];
u32 gfxLine3[240];
u32 gfxLineOBJ[240];
u32 gfxLineOBJWin[240];
u32 gfxLineMix[240];
gboolean gfxInWin0[240];
gboolean gfxInWin1[240];

int gfxBG2X = 0;
int gfxBG2Y = 0;
int gfxBG3X = 0;
int gfxBG3Y = 0;

void gfx_line_render()
{
	if (DISPCNT & 0x80)
	{
		for (int x = 0; x < 240; x++)
		{
			gfxLineMix[x] = 0x7fff;
		}
		return;
	}

	internalRenderLine();
}

void gfx_BG2X_update()
{
	gfxBG2X = (BG2X_L) | ((BG2X_H & 0x07FF)<<16);
	if (BG2X_H & 0x0800)
		gfxBG2X |= 0xF8000000;
}

void gfx_BG2Y_update()
{
	gfxBG2Y = (BG2Y_L) | ((BG2Y_H & 0x07FF)<<16);
	if (BG2Y_H & 0x0800)
		gfxBG2Y |= 0xF8000000;
}

void gfx_BG3X_update()
{
	gfxBG3X = (BG3X_L) | ((BG3X_H & 0x07FF)<<16);
	if (BG3X_H & 0x0800)
		gfxBG3X |= 0xF8000000;
}

void gfx_BG3Y_update()
{
	gfxBG3Y = (BG3Y_L) | ((BG3Y_H & 0x07FF)<<16);
	if (BG3Y_H & 0x0800)
		gfxBG3Y |= 0xF8000000;
}

void gfx_frame_new()
{
	gfx_BG2X_update();
	gfx_BG2Y_update();
	gfx_BG3X_update();
	gfx_BG3Y_update();
}

void gfx_renderer_choose()
{
	gboolean fxOn = ((BLDMOD>>6)&3) != 0;
	gboolean windowOn = (layerEnable & 0x6000) ? TRUE : FALSE;
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

void gfx_buffers_clear(gboolean force)
{
	if (!(layerEnable & 0x0100) || force)
	{
		gfx_clear_array(gfxLine0);
	}
	if (!(layerEnable & 0x0200) || force)
	{
		gfx_clear_array(gfxLine1);
	}
	if (!(layerEnable & 0x0400) || force)
	{
		gfx_clear_array(gfxLine2);
	}
	if (!(layerEnable & 0x0800) || force)
	{
		gfx_clear_array(gfxLine3);
	}
}

void gfx_window0_update()
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

void gfx_window1_update()
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
