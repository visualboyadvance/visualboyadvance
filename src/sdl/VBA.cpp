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
#include "ErrorScreen.h"
#include "GameScreen.h"
#include "PauseScreen.h"
#include "../common/Settings.h"

#include <glib.h>

static Display *display = NULL;
static SoundDriver *soundDriver = NULL;
static InputDriver *inputDriver = NULL;
gchar *filename = NULL;

static gboolean emulating = FALSE;

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

		screens_process_event_current(&event);
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

static void vba_free() {
	soundShutdown();
	cartridge_unload();
	display_free();
	CPUCleanUp();

	screens_free_all();
	sound_sdl_free(soundDriver);
	input_sdl_free(inputDriver);
	display_sdl_free(display);

	settings_free();

	g_free(filename);
}

__attribute__((noreturn)) static void vba_fatal_error(GError *err) {
	if (display == NULL) {
		g_printerr("%s\n", err->message);
		g_clear_error(&err);
	} else {
		screens_free_all();

		errorscreen_create(display, err->message, NULL);
		g_clear_error(&err);

		emulating = TRUE;

		while (emulating) {
			screens_update_current();
			display_sdl_render(display);
			events_poll();
		}
	}

	vba_free();
	exit(1);
}

int main(int argc, char **argv)
{
	fprintf(stdout, "VBA version %s\n", VERSION);

	GError *err = NULL;

	// Read config file
	settings_init();
	if (!settings_read_config_file(&err)) {
		vba_fatal_error(err);
	}

	// Parse command line
	filename = settings_parse_command_line(&argc, &argv, &err);
	if (filename == NULL) {
		settings_display_usage();
		vba_fatal_error(err);
	}

	// Init the display driver
	display = display_sdl_init(&err);
	if (display == NULL) {
		vba_fatal_error(err);
	}

	// Check the settings
	if (!settings_check(&err)) {
		vba_fatal_error(err);
	}

	// Make sure the batteries dir exists
	const gchar* batteriesDir = settings_get_battery_dir();
	g_mkdir_with_parents(batteriesDir, 0777);

	// Make sure the saves dir exists
	const gchar* savesDir = settings_get_save_dir();
	g_mkdir_with_parents(savesDir, 0777);

	// Init the game screen
	GameScreen *game = gamescreen_create(display, &err);
	if (game == NULL) {
		vba_fatal_error(err);
	}

	display_init(gamescreen_get_display_driver(game));

	// Init the sound driver
	soundDriver = sound_sdl_init(&err);
	if (soundDriver == NULL) {
		vba_fatal_error(err);
	}
	soundSetVolume(settings_sound_volume());
	soundInit(soundDriver);

	// Init the input driver
	inputDriver = input_sdl_init(&err);
	if (inputDriver == NULL) {
		vba_fatal_error(err);
	}
	gba_init_input(inputDriver);

    if(!loadROM(filename, &err)) {
		vba_fatal_error(err);
	}

	gamescreen_read_battery(game);

	emulating = TRUE;

	display_sdl_set_window_title(display, cartridge_get_game_title());

	while (emulating) {
		screens_update_current();
		display_sdl_render(display);
		timers_update();
		events_poll();
	}

	fprintf(stdout, "Shutting down\n");
	gamescreen_write_battery(game);

	vba_free();

	return 0;
}

gboolean vba_toggle_pause() {
	gboolean paused = !vba_is_paused();
	soundPause(paused);

	if (paused) {
		pausescreen_create(display, NULL);
	} else {
		screens_pop();
	}

	return paused;
}

gboolean vba_is_paused() {
	return screens_current_is(PAUSE_SCREEN);
}

void vba_quit() {
	emulating = FALSE;
}
