#ifndef GFX_H
#define GFX_H

#include "../common/Types.h"

namespace GFX
{

typedef void (*LineRenderer)();
extern LineRenderer renderLine;

void chooseRenderer();
void clearRenderBuffers(bool force);
void updateWindow0();
void updateWindow1();

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

} // namespace GFX

#endif // GFX_H
