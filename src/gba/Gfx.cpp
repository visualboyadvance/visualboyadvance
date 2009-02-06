#include "Gfx.h"
#include "GfxHelpers.h"
#include "GBA.h"

namespace GFX
{

LineRenderer renderLine = mode0RenderLine;

int coeff[32] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
  16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16};

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

int gfxBG2Changed = 0;
int gfxBG3Changed = 0;

int gfxBG2X = 0;
int gfxBG2Y = 0;
int gfxBG3X = 0;
int gfxBG3Y = 0;
int gfxLastVCOUNT = 0;

void chooseRenderer()
{
  bool fxOn = ((BLDMOD>>6)&3) != 0;
  bool windowOn = (layerEnable & 0x6000) ? true : false;

  switch(DISPCNT & 7) {
  case 0:
    if(!fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode0RenderLine;
    else if(fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode0RenderLineNoWindow;
    else
      renderLine = mode0RenderLineAll;
    break;
  case 1:
    if(!fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode1RenderLine;
    else if(fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode1RenderLineNoWindow;
    else
      renderLine = mode1RenderLineAll;
    break;
  case 2:
    if(!fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode2RenderLine;
    else if(fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode2RenderLineNoWindow;
    else
      renderLine = mode2RenderLineAll;
    break;
  case 3:
    if(!fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode3RenderLine;
    else if(fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode3RenderLineNoWindow;
    else
      renderLine = mode3RenderLineAll;
    break;
  case 4:
    if(!fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode4RenderLine;
    else if(fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode4RenderLineNoWindow;
    else
      renderLine = mode4RenderLineAll;
    break;
  case 5:
    if(!fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode5RenderLine;
    else if(fxOn && !windowOn && !(layerEnable & 0x8000))
      renderLine = mode5RenderLineNoWindow;
    else
      renderLine = mode5RenderLineAll;
  default:
    break;
  }
}

void clearRenderBuffers(bool force)
{
  if(!(layerEnable & 0x0100) || force) {
    gfxClearArray(line0);
  }
  if(!(layerEnable & 0x0200) || force) {
    gfxClearArray(line1);
  }
  if(!(layerEnable & 0x0400) || force) {
    gfxClearArray(line2);
  }
  if(!(layerEnable & 0x0800) || force) {
    gfxClearArray(line3);
  }
}

void updateWindow0()
{
  int x00 = WIN0H>>8;
  int x01 = WIN0H & 255;

  if(x00 <= x01) {
    for(int i = 0; i < 240; i++) {
      gfxInWin0[i] = (i >= x00 && i < x01);
    }
  } else {
    for(int i = 0; i < 240; i++) {
      gfxInWin0[i] = (i >= x00 || i < x01);
    }
  }
}

void updateWindow1()
{
  int x00 = WIN1H>>8;
  int x01 = WIN1H & 255;

  if(x00 <= x01) {
    for(int i = 0; i < 240; i++) {
      gfxInWin1[i] = (i >= x00 && i < x01);
    }
  } else {
    for(int i = 0; i < 240; i++) {
      gfxInWin1[i] = (i >= x00 || i < x01);
    }
  }
}

} // namespace GFX
