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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>

#include <SDL.h>

#include "../gba/GBA.h"
#include "../gba/Cartridge.h"
#include "../gba/Display.h"
#include "../gba/Savestate.h"
#include "../gba/Sound.h"
#include "../common/Util.h"

#include "DisplaySDL.h"
#include "InputSDL.h"
#include "SoundSDL.h"
#include "Timer.h"
#include "../common/Settings.h"

#include <glib.h>

DisplayDriver *displayDriver = NULL;
SoundDriver *soundDriver = NULL;

static gboolean emulating = FALSE;

static bool paused = false;
static bool inactive = false;

static int  mouseCounter = 0;

static void sdlChangeVolume(float d)
{
	float oldVolume = soundGetVolume();
	float newVolume = oldVolume + d;

	if (newVolume < 0.0) newVolume = 0.0;
	if (newVolume > SETTINGS_SOUND_MAX_VOLUME) newVolume = SETTINGS_SOUND_MAX_VOLUME;

	if (fabs(newVolume - oldVolume) > 0.001) {
		char tmp[32];
		sprintf(tmp, "Volume: %i%%", (int)(newVolume*100.0+0.5));
		display_sdl_show_screen_message(displayDriver, tmp);
		soundSetVolume(newVolume);
	}
}

static void sdlWriteState(int num)
{
	gchar *message = NULL;
	GError *err = NULL;

	if (!savestate_save_slot(num, &err)) {
		message = strdup(err->message);
		g_clear_error(&err);
	} else {
		message = g_strdup_printf("Wrote state %d", num + 1);
	}

	display_sdl_show_screen_message(displayDriver, message);
	g_free(message);
}

static void sdlReadState(int num) {
	gchar *message = NULL;
	GError *err = NULL;

	if (!savestate_load_slot(num, &err)) {
		message = strdup(err->message);
		g_clear_error(&err);
	} else {
		message = g_strdup_printf("Loaded state %d", num + 1);
	}

	display_sdl_show_screen_message(displayDriver, message);
	g_free(message);
}

static void sdlWriteBattery() {
	gchar *message = NULL;
	GError *err = NULL;

	if (!cartridge_write_battery(&err)) {
		message = strdup(err->message);
		g_clear_error(&err);
	} else {
		message = g_strdup_printf("Wrote battery");
	}

	display_sdl_show_screen_message(displayDriver, message);
	g_free(message);
}

static void sdlReadBattery() {
	// Ignore errors, we don't care loading battery failed
	gboolean res = cartridge_read_battery(NULL);

	if (res)
		display_sdl_show_screen_message(displayDriver, "Loaded battery");
}

#define MOD_KEYS    (KMOD_CTRL|KMOD_SHIFT|KMOD_ALT|KMOD_GUI)
#define MOD_NOCTRL  (KMOD_SHIFT|KMOD_ALT|KMOD_GUI)
#define MOD_NOALT   (KMOD_CTRL|KMOD_SHIFT|KMOD_GUI)
#define MOD_NOSHIFT (KMOD_CTRL|KMOD_ALT|KMOD_GUI)

static gboolean sdlProcessEvent(const SDL_Event *event) {
	switch (event->type) {
	case SDL_QUIT:
		emulating = FALSE;
		return TRUE;
	case SDL_WINDOWEVENT_FOCUS_LOST:
		if (!paused && settings_pause_when_inactive()) {
			inactive = TRUE;
			soundPause(inactive);
		}
		return FALSE;
	case SDL_WINDOWEVENT_FOCUS_GAINED:
		if (!paused && settings_pause_when_inactive()) {
			inactive = FALSE;
			soundPause(inactive);
		}
		return FALSE;
	case SDL_MOUSEMOTION:
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEBUTTONDOWN:
		if (display_sdl_is_fullscreen(displayDriver)) {
			SDL_ShowCursor(SDL_ENABLE);
			mouseCounter = 120;
		}
		return FALSE;
	case SDL_KEYUP:
		switch (event->key.keysym.sym) {
		case SDLK_r:
			if (!(event->key.keysym.mod & MOD_NOCTRL)
					&& (event->key.keysym.mod & KMOD_CTRL)) {
				if (emulating) {
					CPUReset();

					display_sdl_show_screen_message(displayDriver, "Reset");
				}

				return TRUE;
			}
			break;

		case SDLK_KP_DIVIDE:
			sdlChangeVolume(-0.1);
			return TRUE;
		case SDLK_KP_MULTIPLY:
			sdlChangeVolume(0.1);
			return TRUE;

		case SDLK_p:
			if (!(event->key.keysym.mod & MOD_NOCTRL)
					&& (event->key.keysym.mod & KMOD_CTRL)) {
				paused = !paused;
				soundPause(paused);
				g_message(paused ? "Pause on" : "Pause off");

				return TRUE;
			}
			break;
		case SDLK_ESCAPE:
			emulating = FALSE;
			return TRUE;
		case SDLK_f:
			if (!(event->key.keysym.mod & MOD_NOCTRL)
					&& (event->key.keysym.mod & KMOD_CTRL)) {
				display_sdl_toggle_fullscreen(displayDriver, NULL);
				return TRUE;
			}
			break;
		case SDLK_F1:
		case SDLK_F2:
		case SDLK_F3:
		case SDLK_F4:
		case SDLK_F5:
		case SDLK_F6:
		case SDLK_F7:
		case SDLK_F8:
			if (!(event->key.keysym.mod & MOD_NOSHIFT)
					&& (event->key.keysym.mod & KMOD_SHIFT)) {
				sdlWriteState(event->key.keysym.sym - SDLK_F1);
				return TRUE;
			} else if (!(event->key.keysym.mod & MOD_KEYS)) {
				sdlReadState(event->key.keysym.sym - SDLK_F1);
				return TRUE;
			}
			break;
		case SDLK_SPACE:
			sound_sdl_enable_sync(soundDriver, TRUE);
			return TRUE;
		}
		break;
	case SDL_KEYDOWN:
		switch (event->key.keysym.sym) {
		case SDLK_SPACE:
			sound_sdl_enable_sync(soundDriver, FALSE);
			return TRUE;
		}
		break;
	}

	return FALSE;
}

static void sdlPollEvents() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (!sdlProcessEvent(&event)) {
			input_sdl_process_SDL_event(&event);
		}
	}
}

static gboolean loadROM(const char *file, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

	if (!CPUInitMemory(err)) {
		return FALSE;
	}

	if (!cartridge_load_rom(file, err))	{
		CPUCleanUp();
		return FALSE;
	}

	if (!CPULoadBios(settings_get_bios(), err))	{
		CPUCleanUp();
		return FALSE;
	}

	CPUInit();
	CPUReset();

	return TRUE;
}

int main(int argc, char **argv)
{
	fprintf(stdout, "VBA version %s\n", VERSION);

	GError *err = NULL;
	gchar *filename = NULL;

	// Read config file
	settings_init();
	if (!settings_read_config_file(&err)) {
		g_printerr("%s\n", err->message);
		settings_free();

		g_clear_error(&err);
		exit(1);
	}

	// Parse command line
	filename = settings_parse_command_line(&argc, &argv, &err);
	if (filename == NULL) {
		settings_display_usage(err);
		settings_free();

		g_clear_error(&err);
		exit(1);
	}

	// Check the settings
	if (!settings_check(&err)) {
		g_printerr("%s\n", err->message);
		settings_free();

		g_clear_error(&err);
		exit(1);
	}

	// Make sure the batteries dir exists
	const gchar* batteriesDir = settings_get_battery_dir();
	g_mkdir_with_parents(batteriesDir, 0777);

	// Make sure the saves dir exists
	const gchar* savesDir = settings_get_save_dir();
	g_mkdir_with_parents(savesDir, 0777);

	// Init the display driver
	displayDriver = display_sdl_init(&err);
	if (displayDriver == NULL) {
		g_printerr("%s\n", err->message);
		settings_free();

		g_clear_error(&err);
		exit(1);
	}

	display_init(displayDriver);

	// Init the sound driver
	soundDriver = sound_sdl_init(&err);
	if (soundDriver == NULL) {
		g_printerr("%s\n", err->message);
		settings_free();

		g_clear_error(&err);
		exit(1);
	}
	soundSetVolume(settings_sound_volume());
	soundInit(soundDriver);

	// Init the input driver
	InputDriver *inputDriver = input_sdl_init(&err);
	if (inputDriver == NULL) {
		g_printerr("%s\n", err->message);
		settings_free();
		sound_sdl_free(soundDriver);

		g_clear_error(&err);
		exit(1);
	}
	gba_init_input(inputDriver);

    if(!loadROM(filename, &err)) {
		g_printerr("%s\n", err->message);
		settings_free();

		g_clear_error(&err);
		exit(1);
	}

	sdlReadBattery();

	emulating = TRUE;

	display_sdl_set_window_title(displayDriver, cartridge_get_game_title());

	while (emulating) {
		if (!paused && !inactive) {
			CPULoop(250000);
		} else {
			SDL_Delay(500);
		}
		timers_update();
		sdlPollEvents();
		if (mouseCounter) {
			mouseCounter--;
			if (mouseCounter == 0)
				SDL_ShowCursor(SDL_DISABLE);
		}
	}

	fprintf(stdout, "Shutting down\n");
	sdlWriteBattery();

	soundShutdown();
	cartridge_unload();
	display_free();
	CPUCleanUp();

	sound_sdl_free(soundDriver);
	input_sdl_free(inputDriver);
	display_sdl_free(displayDriver);

	SDL_Quit();

	settings_free();

	g_free(filename);

	return 0;
}
