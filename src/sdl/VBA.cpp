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

#include <SDL.h>

#include "../gba/GBA.h"
#include "../gba/Cartridge.h"
#include "../gba/Display.h"
#include "../gba/Sound.h"

#include "DisplaySDL.h"
#include "InputSDL.h"
#include "SoundSDL.h"
#include "Timer.h"
#include "GUI.h"
#include "VBA.h"
#include "GameScreen.h"
#include "PauseScreen.h"
#include "../common/Settings.h"

#include <glib.h>

static GameScreen *game = NULL;
static PauseScreen *pauseScreen = NULL;
static Display *display = NULL;
static SoundDriver *soundDriver = NULL;

static gboolean emulating = FALSE;
static gboolean paused = FALSE;

static gboolean main_process_event(const SDL_Event *event) {
	switch (event->type) {
	case SDL_QUIT:
		emulating = FALSE;
		return TRUE;
	case SDL_KEYUP:
		switch (event->key.keysym.sym) {

		case SDLK_ESCAPE:
			vba_toggle_pause();
			return TRUE;
		case SDLK_f:
			if (!(event->key.keysym.mod & MOD_NOCTRL)
					&& (event->key.keysym.mod & KMOD_CTRL)) {
				display_sdl_toggle_fullscreen(display, NULL);
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
	case SDL_WINDOWEVENT:
		switch (event->window.event) {
		case SDL_WINDOWEVENT_RESIZED:
			display_sdl_resize(display);
			return TRUE;
		}
		break;
	}

	return FALSE;
}

static void events_poll() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (main_process_event(&event)) {
			continue;
		}

		if (actionables_handle_event(&event)) {
			continue;
		}

		if (!vba_is_paused()) {
			gamescreen_process_event(game, &event);
		} else {
			pausescreen_process_event(pauseScreen, &event);
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
	display = display_sdl_init(&err);
	if (display == NULL) {
		g_printerr("%s\n", err->message);
		settings_free();

		g_clear_error(&err);
		exit(1);
	}

	// Init the game screen
	game = gamescreen_create(display, &err);
	if (game == NULL) {
		g_printerr("%s\n", err->message);
		settings_free();

		g_clear_error(&err);
		exit(1);
	}

	display_init(gamescreen_get_display_driver(game));

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

	gamescreen_read_battery(game);

	emulating = TRUE;

	display_sdl_set_window_title(display, cartridge_get_game_title());

	while (emulating) {
		if (!vba_is_paused()) {
			gamescreen_update(game);
		} else {
			pausescreen_update(pauseScreen);
		}
		display_sdl_render(display);
		timers_update();
		events_poll();
	}

	fprintf(stdout, "Shutting down\n");
	gamescreen_write_battery(game);

	soundShutdown();
	cartridge_unload();
	display_free();
	CPUCleanUp();

	gamescreen_free(game);
	sound_sdl_free(soundDriver);
	input_sdl_free(inputDriver);
	display_sdl_free(display);

	settings_free();

	g_free(filename);

	return 0;
}

gboolean vba_toggle_pause() {
	paused = !paused;
	soundPause(paused);

	if (paused) {
		pauseScreen = pausescreen_create(display, NULL);
	} else {
		pausescreen_free(pauseScreen);
		pauseScreen = NULL;
	}

	return paused;
}

gboolean vba_is_paused() {
	return paused;
}

void vba_quit() {
	emulating = FALSE;
}
