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
#include "InputDriver.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

#define SETTINGS_SOUND_MAX_VOLUME 2.0

/**
 * Initialize the settings module and set default setting values
 */
void settings_init();

/**
 * Clean up the settings module
 */
void settings_free();

/**
 * Override the current settings with the values from the command line
 *
 * @param argc argument count
 * @param argv argument values
 * @param err return location for a GError, or NULL
 * @return newly allocated string to the ROM file to be loaded
 */
gchar *settings_parse_command_line(gint *argc, gchar ***argv, GError **err);

/**
 * Prints an help text regarding the available command line options
 * on the standard output.
 *
 * @param err optional error message to display
 */
void settings_display_usage(GError *err);

/**
 * Override the current settings with the values from the configuration file
 *
 * @param err return location for a GError, or NULL
 * @return whether reading the configuration file was successful
 */
gboolean settings_read_config_file(GError **err);

/**
 * Validate that the current settings are good enough to allow games to run
 *
 * @param err return location for a GError, or NULL
 * @return whether an error was returned
 */
gboolean settings_check(GError **err);

/** @return path where the battery files are stored */
const gchar *settings_get_battery_dir();

/** @return path where the state files are stored */
const gchar *settings_get_save_dir();

/** @return path of the GBA BIOS ROM file */
const gchar *settings_get_bios();

/** @return whether to start display fullscreen */
gboolean settings_is_fullscreen();

/** @return whether to pause the game when the window is inactive */
gboolean settings_pause_when_inactive();

/** @return whether to always display the emulation speed */
gboolean settings_show_speed();

/** @return whether to disable informational status messages */
gboolean settings_disable_status_messages();

/** @return initial value for the sound volume */
gdouble settings_sound_volume();

/** @return sound sample rate value */
guint settings_sound_sample_rate();

/**
 * @param button emulated button for which to query mapping information
 * @return button mapping code
 */
guint32 settings_get_button_mapping(EKey button);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* VBAM_SDL_SETTINGS_H_ */
