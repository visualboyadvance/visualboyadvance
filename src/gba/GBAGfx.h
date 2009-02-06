#ifndef GFX_H
#define GFX_H

#include "../common/Types.h"

// Line renderers
typedef void (*LineRenderer)();
LineRenderer gfxChooseRenderer();

void mode0RenderLine();
void mode0RenderLineNoWindow();
void mode0RenderLineAll();

void mode1RenderLine();
void mode1RenderLineNoWindow();
void mode1RenderLineAll();

void mode2RenderLine();
void mode2RenderLineNoWindow();
void mode2RenderLineAll();

void mode3RenderLine();
void mode3RenderLineNoWindow();
void mode3RenderLineAll();

void mode4RenderLine();
void mode4RenderLineNoWindow();
void mode4RenderLineAll();

void mode5RenderLine();
void mode5RenderLineNoWindow();
void mode5RenderLineAll();

// Drawing helpers
void gfxClearRenderBuffers(bool force);
void gfxDrawTextScreen(u16, u16, u16, u32 *);
void gfxDrawRotScreen(u16,
			     u16, u16,
			     u16, u16,
			     u16, u16,
			     u16, u16,
			     int&, int&,
			     int,
			     u32*);
void gfxDrawRotScreen16Bit(u16,
				  u16, u16,
				  u16, u16,
				  u16, u16,
				  u16, u16,
				  int&, int&,
				  int,
				  u32*);
void gfxDrawRotScreen256(u16,
				u16, u16,
				u16, u16,
				u16, u16,
				u16, u16,
				int&, int&,
				int,
				u32*);
void gfxDrawRotScreen16Bit160(u16,
				     u16, u16,
				     u16, u16,
				     u16, u16,
				     u16, u16,
				     int&, int&,
				     int,
				     u32*);
void gfxDrawSprites(u32 *);
void gfxDrawOBJWin(u32 *lineOBJWin);
u32 gfxIncreaseBrightness(u32 color, int coeff);
u32 gfxDecreaseBrightness(u32 color, int coeff);
u32 gfxAlphaBlend(u32 color, u32 color2, int ca, int cb);

extern int coeff[32];
extern u32 line0[240];
extern u32 line1[240];
extern u32 line2[240];
extern u32 line3[240];
extern u32 lineOBJ[240];
extern u32 lineOBJWin[240];
extern u32 lineMix[240];
extern bool gfxInWin0[240];
extern bool gfxInWin1[240];
extern int lineOBJpixleft[128];

extern int gfxBG2Changed;
extern int gfxBG3Changed;

extern int gfxBG2X;
extern int gfxBG2Y;
extern int gfxBG3X;
extern int gfxBG3Y;
extern int gfxLastVCOUNT;

#endif // GFX_H
