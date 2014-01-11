// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 2008 VBA-M development team

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

#ifndef __VBA_GFX_HELPERS_H
#define __VBA_GFX_HELPERS_H

#include "../common/Types.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

// Drawing helpers
void gfx_clear_array(u32 *array);
void gfx_text_screen_draw(u16 control, u16 hofs, u16 vofs, u32 *line);
void gfx_rot_screen_draw(u16 control,
                      u16 pa,  u16 pb,
                      u16 pc,  u16 pd,
                      int *currentX, int *currentY,
                      u32 *line);
void gfx_rot_screen_draw_16bit(u16 control,
                           u16 x_l, u16 x_h,
                           u16 y_l, u16 y_h,
                           u16 pa,  u16 pb,
                           u16 pc,  u16 pd,
                           int *currentX, int *currentY,
                           u32 *line);
void gfx_rot_screen_draw_256(u16 control,
                         u16 x_l, u16 x_h,
                         u16 y_l, u16 y_h,
                         u16 pa,  u16 pb,
                         u16 pc,  u16 pd,
                         int *currentX, int *currentY,
                         u32 *line);
void gfx_rot_screen_draw_16bit160(u16 control,
                              u16 x_l, u16 x_h,
                              u16 y_l, u16 y_h,
                              u16 pa,  u16 pb,
                              u16 pc,  u16 pd,
                              int *currentX, int *currentY,
                              u32 *line);
void gfx_sprites_draw(u32 *lineOBJ);
void gfx_obj_win_draw(u32 *lineOBJWin);
u32 gfx_brightness_increase(u32 color, int coeff);
u32 gfx_brightness_decrease(u32 color, int coeff);
u32 gfx_alpha_blend(u32 color, u32 color2, int ca, int cb);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // __VBA_GFX_HELPERS_H
