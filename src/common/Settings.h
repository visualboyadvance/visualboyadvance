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

#ifndef VBAM_SDL_SETTINGS_H_
#define VBAM_SDL_SETTINGS_H_

#include <glib.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define SETTINGS_SOUND_MAX_VOLUME 2.0

typedef enum {
	KEY_LEFT,
	KEY_RIGHT,
	KEY_UP,
	KEY_DOWN,
	KEY_BUTTON_A,
	KEY_BUTTON_B,
	KEY_BUTTON_START,
	KEY_BUTTON_SELECT,
	KEY_BUTTON_L,
	KEY_BUTTON_R,
	KEY_BUTTON_SPEED,
	KEY_BUTTON_AUTO_A,
	KEY_BUTTON_AUTO_B
} EKey;

static EKey settings_buttons[] = {
		KEY_LEFT,
		KEY_RIGHT,
		KEY_UP,
		KEY_DOWN,
		KEY_BUTTON_A,
		KEY_BUTTON_B,
		KEY_BUTTON_START,
		KEY_BUTTON_SELECT,
		KEY_BUTTON_L,
		KEY_BUTTON_R,
		KEY_BUTTON_SPEED,
		KEY_BUTTON_AUTO_A,
		KEY_BUTTON_AUTO_B
};

void settings_init();
void settings_free();
gchar *settings_parse_command_line(gint *argc, gchar ***argv, GError **err);
void settings_display_usage(GError *err);
gboolean settings_read_config_file(GError **err);
gboolean settings_check(GError **err);

const gchar *settings_get_battery_dir();
const gchar *settings_get_save_dir();
const gchar *settings_get_bios();
gboolean settings_is_fullscreen();
gboolean settings_pause_when_inactive();
gboolean settings_show_speed();
gboolean settings_disable_status_messages();
gdouble settings_sound_volume();
guint settings_sound_sample_rate();
guint32 settings_get_button_mapping(EKey button);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* VBAM_SDL_SETTINGS_H_ */
