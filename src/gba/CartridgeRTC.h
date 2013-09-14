// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 1999-2003 Forgotten
// Copyright (C) 2005-2006 Forgotten and the VBA development team

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
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#ifndef VBAM_GBA_RTC_H_
#define VBAM_GBA_RTC_H_

#include <glib.h>
#include <zlib.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

guint16 cartridge_rtc_read(guint32 address);
gboolean cartridge_rtc_write(guint32 address, guint16 value);
void cartridge_rtc_enable(gboolean enable);
gboolean cartridge_rtc_is_enabled();
void cartridge_rtc_reset();

void cartridge_rtc_load_state(gzFile gzFile);
void cartridge_rtc_save_state(gzFile gzFile);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // VBAM_GBA_RTC_H_

