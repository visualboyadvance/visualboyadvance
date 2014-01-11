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

#ifndef __VBA_GFX_H
#define __VBA_GFX_H

#include <glib.h>
#include "../common/Types.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

void gfx_frame_new();
void gfx_renderer_choose();
void gfx_line_render();
void gfx_buffers_clear(gboolean force);
void gfx_BG2X_update();
void gfx_BG2Y_update();
void gfx_BG3X_update();
void gfx_BG3Y_update();
void gfx_window0_update();
void gfx_window1_update();

void gfx_mode0_line_render();
void gfx_mode0_line_render_no_window();
void gfx_mode0_line_render_all();

void gfx_mode1_line_render();
void gfx_mode1_line_render_no_window();
void gfx_mode1_line_render_all();

void gfx_mode2_line_render();
void gfx_mode2_line_render_no_window();
void gfx_mode2_line_render_all();

void gfx_mode3_line_render();
void gfx_mode3_line_render_no_window();
void gfx_mode3_line_render_all();

void gfx_mode4_line_render();
void gfx_mode4_line_render_no_window();
void gfx_mode4_line_render_all();

void gfx_mode5_line_render();
void gfx_mode5_line_render_no_window();
void gfx_mode5_line_render_all();

extern int gfxCoeff[32];
extern u32 gfxLine0[240];
extern u32 gfxLine1[240];
extern u32 gfxLine2[240];
extern u32 gfxLine3[240];
extern u32 gfxLineOBJ[240];
extern u32 gfxLineOBJWin[240];
extern u32 gfxLineMix[240];
extern gboolean gfxInWin0[240];
extern gboolean gfxInWin1[240];

extern int gfxBG2X;
extern int gfxBG2Y;
extern int gfxBG3X;
extern int gfxBG3Y;

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // __VBA_GFX_H
