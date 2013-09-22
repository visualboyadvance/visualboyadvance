// VBA-M, A Nintendo Handheld Console Emulator
// Copyright (C) 2008 VBA-M development team
//
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

#ifndef DISPLAY_H
#define DISPLAY_H

#include <glib.h>
#include <zlib.h>
#include "../common/DisplayDriver.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

void display_init(DisplayDriver *driver);
void display_free();

void display_read_state(gzFile gzFile);
void display_save_state(gzFile gzFile);

void display_draw_line(int line, guint32* src);
void display_draw_screen();
void display_clear();

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // DISPLAY_H
