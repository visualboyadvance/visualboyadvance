#ifndef GFX_HELPERS_H
#define GFX_HELPERS_H

#include "../common/Types.h"

namespace GFX
{

// Drawing helpers
void gfxClearArray(u32 *array);
void gfxDrawTextScreen(u16, u16, u16, u32 *);
void gfxDrawRotScreen(u16,
                      u16, u16,
                      u16, u16,
                      int&, int&,
                      u32*);
void gfxDrawRotScreen16Bit(u16,
                           u16, u16,
                           u16, u16,
                           int&, int&,
                           u32*);
void gfxDrawRotScreen256(u16,
                         u16, u16,
                         u16, u16,
                         int&, int&,
                         u32*);
void gfxDrawRotScreen16Bit160(u16,
                              u16, u16,
                              u16, u16,
                              int&, int&,
                              u32*);
void gfxDrawSprites(u32 *);
void gfxDrawOBJWin(u32 *lineOBJWin);
u32 gfxIncreaseBrightness(u32 color, int coeff);
u32 gfxDecreaseBrightness(u32 color, int coeff);
u32 gfxAlphaBlend(u32 color, u32 color2, int ca, int cb);

} // namespace GFX

#endif // GFX_HELPERS_H
