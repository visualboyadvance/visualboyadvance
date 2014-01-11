#include "Globals.h"
#include "Gfx.h"
#include "GfxHelpers.h"
#include "../common/Port.h"

void gfx_mode0_line_render()
{
	u16 *palette = (u16 *)paletteRAM;

	if (layerEnable & 0x0100)
	{
		gfx_text_screen_draw(BG0CNT, BG0HOFS, BG0VOFS, gfxLine0);
	}

	if (layerEnable & 0x0200)
	{
		gfx_text_screen_draw(BG1CNT, BG1HOFS, BG1VOFS, gfxLine1);
	}

	if (layerEnable & 0x0400)
	{
		gfx_text_screen_draw(BG2CNT, BG2HOFS, BG2VOFS, gfxLine2);
	}

	if (layerEnable & 0x0800)
	{
		gfx_text_screen_draw(BG3CNT, BG3HOFS, BG3VOFS, gfxLine3);
	}

	gfx_sprites_draw(gfxLineOBJ);

	u32 backdrop = (READ16LE(&palette[0]) | 0x30000000);

	for (int x = 0; x < 240; x++)
	{
		u32 color = backdrop;
		u8 top = 0x20;

		if (gfxLine0[x] < color)
		{
			color = gfxLine0[x];
			top = 0x01;
		}

		if ((u8)(gfxLine1[x]>>24) < (u8)(color >> 24))
		{
			color = gfxLine1[x];
			top = 0x02;
		}

		if ((u8)(gfxLine2[x]>>24) < (u8)(color >> 24))
		{
			color = gfxLine2[x];
			top = 0x04;
		}

		if ((u8)(gfxLine3[x]>>24) < (u8)(color >> 24))
		{
			color = gfxLine3[x];
			top = 0x08;
		}

		if ((u8)(gfxLineOBJ[x]>>24) < (u8)(color >> 24))
		{
			color = gfxLineOBJ[x];
			top = 0x10;
		}

		if ((top & 0x10) && (color & 0x00010000))
		{
			// semi-transparent OBJ
			u32 back = backdrop;
			u8 top2 = 0x20;

			if ((u8)(gfxLine0[x]>>24) < (u8)(back >> 24))
			{
				back = gfxLine0[x];
				top2 = 0x01;
			}

			if ((u8)(gfxLine1[x]>>24) < (u8)(back >> 24))
			{
				back = gfxLine1[x];
				top2 = 0x02;
			}

			if ((u8)(gfxLine2[x]>>24) < (u8)(back >> 24))
			{
				back = gfxLine2[x];
				top2 = 0x04;
			}

			if ((u8)(gfxLine3[x]>>24) < (u8)(back >> 24))
			{
				back = gfxLine3[x];
				top2 = 0x08;
			}

			if (top2 & (BLDMOD>>8))
				color = gfx_alpha_blend(color, back,
				                      gfxCoeff[COLEV & 0x1F],
				                      gfxCoeff[(COLEV >> 8) & 0x1F]);
			else
			{
				switch ((BLDMOD >> 6) & 3)
				{
				case 2:
					if (BLDMOD & top)
						color = gfx_brightness_increase(color, gfxCoeff[COLY & 0x1F]);
					break;
				case 3:
					if (BLDMOD & top)
						color = gfx_brightness_decrease(color, gfxCoeff[COLY & 0x1F]);
					break;
				}
			}
		}

		gfxLineMix[x] = color;
	}
}

void gfx_mode0_line_render_no_window()
{
	u16 *palette = (u16 *)paletteRAM;

	if (layerEnable & 0x0100)
	{
		gfx_text_screen_draw(BG0CNT, BG0HOFS, BG0VOFS, gfxLine0);
	}

	if (layerEnable & 0x0200)
	{
		gfx_text_screen_draw(BG1CNT, BG1HOFS, BG1VOFS, gfxLine1);
	}

	if (layerEnable & 0x0400)
	{
		gfx_text_screen_draw(BG2CNT, BG2HOFS, BG2VOFS, gfxLine2);
	}

	if (layerEnable & 0x0800)
	{
		gfx_text_screen_draw(BG3CNT, BG3HOFS, BG3VOFS, gfxLine3);
	}

	gfx_sprites_draw(gfxLineOBJ);

	u32 backdrop = (READ16LE(&palette[0]) | 0x30000000);

	int effect = (BLDMOD >> 6) & 3;

	for (int x = 0; x < 240; x++)
	{
		u32 color = backdrop;
		u8 top = 0x20;

		if (gfxLine0[x] < color)
		{
			color = gfxLine0[x];
			top = 0x01;
		}

		if (gfxLine1[x] < (color & 0xFF000000))
		{
			color = gfxLine1[x];
			top = 0x02;
		}

		if (gfxLine2[x] < (color & 0xFF000000))
		{
			color = gfxLine2[x];
			top = 0x04;
		}

		if (gfxLine3[x] < (color & 0xFF000000))
		{
			color = gfxLine3[x];
			top = 0x08;
		}

		if (gfxLineOBJ[x] < (color & 0xFF000000))
		{
			color = gfxLineOBJ[x];
			top = 0x10;
		}

		if (!(color & 0x00010000))
		{
			switch (effect)
			{
			case 0:
				break;
			case 1:
			{
				if (top & BLDMOD)
				{
					u32 back = backdrop;
					u8 top2 = 0x20;
					if (gfxLine0[x] < back)
					{
						if (top != 0x01)
						{
							back = gfxLine0[x];
							top2 = 0x01;
						}
					}

					if (gfxLine1[x] < (back & 0xFF000000))
					{
						if (top != 0x02)
						{
							back = gfxLine1[x];
							top2 = 0x02;
						}
					}

					if (gfxLine2[x] < (back & 0xFF000000))
					{
						if (top != 0x04)
						{
							back = gfxLine2[x];
							top2 = 0x04;
						}
					}

					if (gfxLine3[x] < (back & 0xFF000000))
					{
						if (top != 0x08)
						{
							back = gfxLine3[x];
							top2 = 0x08;
						}
					}

					if (gfxLineOBJ[x] < (back & 0xFF000000))
					{
						if (top != 0x10)
						{
							back = gfxLineOBJ[x];
							top2 = 0x10;
						}
					}

					if (top2 & (BLDMOD>>8))
						color = gfx_alpha_blend(color, back,
						                      gfxCoeff[COLEV & 0x1F],
						                      gfxCoeff[(COLEV >> 8) & 0x1F]);

				}
			}
			break;
			case 2:
				if (BLDMOD & top)
					color = gfx_brightness_increase(color, gfxCoeff[COLY & 0x1F]);
				break;
			case 3:
				if (BLDMOD & top)
					color = gfx_brightness_decrease(color, gfxCoeff[COLY & 0x1F]);
				break;
			}
		}
		else
		{
			// semi-transparent OBJ
			u32 back = backdrop;
			u8 top2 = 0x20;

			if (gfxLine0[x] < back)
			{
				back = gfxLine0[x];
				top2 = 0x01;
			}

			if (gfxLine1[x] < (back & 0xFF000000))
			{
				back = gfxLine1[x];
				top2 = 0x02;
			}

			if (gfxLine2[x] < (back & 0xFF000000))
			{
				back = gfxLine2[x];
				top2 = 0x04;
			}

			if (gfxLine3[x] < (back & 0xFF000000))
			{
				back = gfxLine3[x];
				top2 = 0x08;
			}

			if (top2 & (BLDMOD>>8))
				color = gfx_alpha_blend(color, back,
				                      gfxCoeff[COLEV & 0x1F],
				                      gfxCoeff[(COLEV >> 8) & 0x1F]);
			else
			{
				switch ((BLDMOD >> 6) & 3)
				{
				case 2:
					if (BLDMOD & top)
						color = gfx_brightness_increase(color, gfxCoeff[COLY & 0x1F]);
					break;
				case 3:
					if (BLDMOD & top)
						color = gfx_brightness_decrease(color, gfxCoeff[COLY & 0x1F]);
					break;
				}
			}
		}

		gfxLineMix[x] = color;
	}
}

void gfx_mode0_line_render_all()
{
	u16 *palette = (u16 *)paletteRAM;

	gboolean inWindow0 = FALSE;
	gboolean inWindow1 = FALSE;

	if (layerEnable & 0x2000)
	{
		u8 v0 = WIN0V >> 8;
		u8 v1 = WIN0V & 255;
		inWindow0 = ((v0 == v1) && (v0 >= 0xe8));
		if (v1 >= v0)
			inWindow0 |= (VCOUNT >= v0 && VCOUNT < v1);
		else
			inWindow0 |= (VCOUNT >= v0 || VCOUNT < v1);
	}
	if (layerEnable & 0x4000)
	{
		u8 v0 = WIN1V >> 8;
		u8 v1 = WIN1V & 255;
		inWindow1 = ((v0 == v1) && (v0 >= 0xe8));
		if (v1 >= v0)
			inWindow1 |= (VCOUNT >= v0 && VCOUNT < v1);
		else
			inWindow1 |= (VCOUNT >= v0 || VCOUNT < v1);
	}

	if ((layerEnable & 0x0100))
	{
		gfx_text_screen_draw(BG0CNT, BG0HOFS, BG0VOFS, gfxLine0);
	}

	if ((layerEnable & 0x0200))
	{
		gfx_text_screen_draw(BG1CNT, BG1HOFS, BG1VOFS, gfxLine1);
	}

	if ((layerEnable & 0x0400))
	{
		gfx_text_screen_draw(BG2CNT, BG2HOFS, BG2VOFS, gfxLine2);
	}

	if ((layerEnable & 0x0800))
	{
		gfx_text_screen_draw(BG3CNT, BG3HOFS, BG3VOFS, gfxLine3);
	}

	gfx_sprites_draw(gfxLineOBJ);
	gfx_obj_win_draw(gfxLineOBJWin);

	u32 backdrop = (READ16LE(&palette[0]) | 0x30000000);

	u8 inWin0Mask = WININ & 0xFF;
	u8 inWin1Mask = WININ >> 8;
	u8 outMask = WINOUT & 0xFF;

	for (int x = 0; x < 240; x++)
	{
		u32 color = backdrop;
		u8 top = 0x20;
		u8 mask = outMask;

		if (!(gfxLineOBJWin[x] & 0x80000000))
		{
			mask = WINOUT >> 8;
		}

		if (inWindow1)
		{
			if (gfxInWin1[x])
				mask = inWin1Mask;
		}

		if (inWindow0)
		{
			if (gfxInWin0[x])
			{
				mask = inWin0Mask;
			}
		}

		if ((mask & 1) && (gfxLine0[x] < color))
		{
			color = gfxLine0[x];
			top = 0x01;
		}

		if ((mask & 2) && ((u8)(gfxLine1[x]>>24) < (u8)(color >> 24)))
		{
			color = gfxLine1[x];
			top = 0x02;
		}

		if ((mask & 4) && ((u8)(gfxLine2[x]>>24) < (u8)(color >> 24)))
		{
			color = gfxLine2[x];
			top = 0x04;
		}

		if ((mask & 8) && ((u8)(gfxLine3[x]>>24) < (u8)(color >> 24)))
		{
			color = gfxLine3[x];
			top = 0x08;
		}

		if ((mask & 16) && ((u8)(gfxLineOBJ[x]>>24) < (u8)(color >> 24)))
		{
			color = gfxLineOBJ[x];
			top = 0x10;
		}

		if (color & 0x00010000)
		{
			// semi-transparent OBJ
			u32 back = backdrop;
			u8 top2 = 0x20;

			if ((mask & 1) && ((u8)(gfxLine0[x]>>24) < (u8)(back >> 24)))
			{
				back = gfxLine0[x];
				top2 = 0x01;
			}

			if ((mask & 2) && ((u8)(gfxLine1[x]>>24) < (u8)(back >> 24)))
			{
				back = gfxLine1[x];
				top2 = 0x02;
			}

			if ((mask & 4) && ((u8)(gfxLine2[x]>>24) < (u8)(back >> 24)))
			{
				back = gfxLine2[x];
				top2 = 0x04;
			}

			if ((mask & 8) && ((u8)(gfxLine3[x]>>24) < (u8)(back >> 24)))
			{
				back = gfxLine3[x];
				top2 = 0x08;
			}

			if (top2 & (BLDMOD>>8))
				color = gfx_alpha_blend(color, back,
				                      gfxCoeff[COLEV & 0x1F],
				                      gfxCoeff[(COLEV >> 8) & 0x1F]);
			else
			{
				switch ((BLDMOD >> 6) & 3)
				{
				case 2:
					if (BLDMOD & top)
						color = gfx_brightness_increase(color, gfxCoeff[COLY & 0x1F]);
					break;
				case 3:
					if (BLDMOD & top)
						color = gfx_brightness_decrease(color, gfxCoeff[COLY & 0x1F]);
					break;
				}
			}
		}
		else if (mask & 32)
		{
			// special FX on in the window
			switch ((BLDMOD >> 6) & 3)
			{
			case 0:
				break;
			case 1:
			{
				if (top & BLDMOD)
				{
					u32 back = backdrop;
					u8 top2 = 0x20;
					if ((mask & 1) && (u8)(gfxLine0[x]>>24) < (u8)(back >> 24))
					{
						if (top != 0x01)
						{
							back = gfxLine0[x];
							top2 = 0x01;
						}
					}

					if ((mask & 2) && (u8)(gfxLine1[x]>>24) < (u8)(back >> 24))
					{
						if (top != 0x02)
						{
							back = gfxLine1[x];
							top2 = 0x02;
						}
					}

					if ((mask & 4) && (u8)(gfxLine2[x]>>24) < (u8)(back >> 24))
					{
						if (top != 0x04)
						{
							back = gfxLine2[x];
							top2 = 0x04;
						}
					}

					if ((mask & 8) && (u8)(gfxLine3[x]>>24) < (u8)(back >> 24))
					{
						if (top != 0x08)
						{
							back = gfxLine3[x];
							top2 = 0x08;
						}
					}

					if ((mask & 16) && (u8)(gfxLineOBJ[x]>>24) < (u8)(back >> 24))
					{
						if (top != 0x10)
						{
							back = gfxLineOBJ[x];
							top2 = 0x10;
						}
					}

					if (top2 & (BLDMOD>>8))
						color = gfx_alpha_blend(color, back,
						                      gfxCoeff[COLEV & 0x1F],
						                      gfxCoeff[(COLEV >> 8) & 0x1F]);
				}
			}
			break;
			case 2:
				if (BLDMOD & top)
					color = gfx_brightness_increase(color, gfxCoeff[COLY & 0x1F]);
				break;
			case 3:
				if (BLDMOD & top)
					color = gfx_brightness_decrease(color, gfxCoeff[COLY & 0x1F]);
				break;
			}
		}

		gfxLineMix[x] = color;
	}
}
