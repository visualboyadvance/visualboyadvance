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

#include "GameScreen.h"
#include "DisplaySDL.h"
#include "InputSDL.h"
#include "OSD.h"
#include "Timer.h"
#include "VBA.h"
#include "../gba/Cartridge.h"
#include "../gba/GBA.h"
#include "../gba/Savestate.h"
#include "../gba/Sound.h"
#include "../common/Settings.h"

static const int screenWidth = 240;
static const int screenHeight = 160;

struct GameScreen {
	Renderable *renderable;
	SDL_Texture *screen;

	DisplayDriver *displayDriver;
	Display *display;

	TextOSD *speed;
	TextOSD *status;
	Timeout *mouseTimeout;

	gboolean inactive;
};

static void gamescreen_update_speed(TextOSD *speed) {
	if (speed == NULL)
		return;

	char buffer[50];
	sprintf(buffer, "%d%%", gba_get_speed());
	text_osd_set_message(speed, buffer);
}

static void gamescreen_update_texture(GameScreen *game, guint16 *pix) {
	g_assert(game != NULL);

	SDL_UpdateTexture(game->screen, NULL, pix, screenWidth * sizeof(*pix));
	// TODO: Error checking

	gamescreen_update_speed(game->speed);
}

static void gamescreen_render(gpointer entity) {
	g_assert(entity != NULL);
	GameScreen *game = (GameScreen *)entity;


	SDL_Rect screenRect;
	screenRect.w = display_sdl_scale(game->display, screenWidth);
	screenRect.h = display_sdl_scale(game->display, screenHeight);
	display_sdl_renderable_get_absolute_position(game->renderable, &screenRect.x, &screenRect.y);

	SDL_RenderCopy(game->renderable->renderer, game->screen, NULL, &screenRect);
}

static void gamescreen_mouse_hide(gpointer entity) {
	SDL_ShowCursor(FALSE);
}

GameScreen *gamescreen_create(Display *display, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);
	g_assert(display != NULL);

	GameScreen *game = g_new(GameScreen, 1);

	game->displayDriver = NULL;
	game->status = NULL;
	game->speed = NULL;
	game->display = display;
	game->renderable = display_sdl_renderable_create(display, game, NULL);
	game->renderable->render = gamescreen_render;
	game->mouseTimeout = timeout_create(game, gamescreen_mouse_hide);
	game->inactive = FALSE;

	display_sdl_renderable_set_size(game->renderable, screenWidth, screenHeight);
	display_sdl_renderable_set_alignment(game->renderable, ALIGN_CENTER, ALIGN_MIDDLE);

	game->screen = SDL_CreateTexture(game->renderable->renderer, SDL_PIXELFORMAT_BGR555,
			SDL_TEXTUREACCESS_STREAMING, screenWidth, screenHeight);

	if (game->screen == NULL) {
		g_set_error(err, DISPLAY_ERROR, G_DISPLAY_ERROR_FAILED,
				"Failed to create screen: %s", SDL_GetError());
		gamescreen_free(game);
		return NULL;
	}

	if (settings_show_speed()) {
		game->speed = text_osd_create(display, " ", NULL, err);
		if (game->speed == NULL) {
			gamescreen_free(game);
			return NULL;
		}

		text_osd_set_color(game->speed, 255, 0, 0);
		text_osd_set_position(game->speed, 5, 5);
		text_osd_set_size(game->speed, 240, 5);
		text_osd_set_opacity(game->speed, 75);
	}

	if (!settings_disable_status_messages()) {
		game->status = text_osd_create(display, " ", NULL, err);
		if (game->status == NULL) {
			gamescreen_free(game);
			return NULL;
		}

		text_osd_set_color(game->status, 255, 0, 0);
		text_osd_set_alignment(game->status, ALIGN_LEFT, ALIGN_BOTTOM);
		text_osd_set_position(game->status, 5, 5);
		text_osd_set_size(game->status, 240, 5);
	}

	return game;
}

void gamescreen_show_status_message(GameScreen *game, const gchar *msg) {
	g_assert(game != NULL);

	if (game->status != NULL) {
		text_osd_set_message(game->status, msg);
		text_osd_set_auto_clear(game->status, 3000);
	}

	g_message("%s", msg);
}

void gamescreen_free(GameScreen *game) {
	if (game == NULL)
		return;

	text_osd_free(game->status);
	text_osd_free(game->speed);

	display_sdl_renderable_free(game->renderable);
	SDL_DestroyTexture(game->screen);
	g_free(game->displayDriver);
	g_free(game);
}

static void gamescreen_draw_screen(const DisplayDriver *driver, guint16 *pix) {
	g_assert(driver != NULL);

	gamescreen_update_texture((GameScreen*)driver->driverData, pix);
}

const DisplayDriver *gamescreen_get_display_driver(GameScreen *game) {
	g_assert(game != NULL);

	if (game->displayDriver != NULL)
		return game->displayDriver;

	DisplayDriver *driver = g_new(DisplayDriver, 1);

	driver->drawScreen = gamescreen_draw_screen;
	driver->driverData = game;

	return driver;
}

static void gamescreen_change_volume(GameScreen *game, float d)
{
	float oldVolume = soundGetVolume();
	float newVolume = oldVolume + d;

	if (newVolume < 0.0) newVolume = 0.0;
	if (newVolume > SETTINGS_SOUND_MAX_VOLUME) newVolume = SETTINGS_SOUND_MAX_VOLUME;

	if (fabs(newVolume - oldVolume) > 0.001) {
		char tmp[32];
		sprintf(tmp, "Volume: %i%%", (int)(newVolume*100.0+0.5));
		gamescreen_show_status_message(game, tmp);
		soundSetVolume(newVolume);
	}
}

static void gamescreen_write_state(GameScreen *game, int num) {
	gchar *message = NULL;
	GError *err = NULL;

	if (!savestate_save_slot(num, &err)) {
		message = strdup(err->message);
		g_clear_error(&err);
	} else {
		message = g_strdup_printf("Wrote state %d", num + 1);
	}

	gamescreen_show_status_message(game, message);
	g_free(message);
}

static void gamescreen_read_state(GameScreen *game, int num) {
	gchar *message = NULL;
	GError *err = NULL;

	if (!savestate_load_slot(num, &err)) {
		message = strdup(err->message);
		g_clear_error(&err);
	} else {
		message = g_strdup_printf("Loaded state %d", num + 1);
	}

	gamescreen_show_status_message(game, message);
	g_free(message);
}

void gamescreen_write_battery(GameScreen *game) {
	gchar *message = NULL;
	GError *err = NULL;

	if (!cartridge_write_battery(&err)) {
		message = strdup(err->message);
		g_clear_error(&err);
	} else {
		message = g_strdup_printf("Wrote battery");
	}

	gamescreen_show_status_message(game, message);
	g_free(message);
}

void gamescreen_read_battery(GameScreen *game) {
	// Ignore errors, we don't care loading battery failed
	gboolean res = cartridge_read_battery(NULL);

	if (res)
		gamescreen_show_status_message(game, "Loaded battery");
}

gboolean gamescreen_process_event(GameScreen *game, const SDL_Event *event) {
	switch (event->type) {
	case SDL_WINDOWEVENT_FOCUS_LOST:
		if (!vba_is_paused() && settings_pause_when_inactive()) {
			game->inactive = TRUE;
			soundPause(game->inactive);
		}
		return FALSE;
	case SDL_WINDOWEVENT_FOCUS_GAINED:
		if (!vba_is_paused() && settings_pause_when_inactive()) {
			game->inactive = FALSE;
			soundPause(game->inactive);
		}
		return FALSE;
	case SDL_MOUSEMOTION:
	case SDL_MOUSEBUTTONUP:
	case SDL_MOUSEBUTTONDOWN:
		SDL_ShowCursor(TRUE);
		if (display_sdl_is_fullscreen(game->display)) {
			timeout_set_duration(game->mouseTimeout, 1000);
		}
		return FALSE;
	case SDL_KEYUP:
		switch (event->key.keysym.sym) {
		case SDLK_r:
			if (!(event->key.keysym.mod & MOD_NOCTRL)
					&& (event->key.keysym.mod & KMOD_CTRL)) {
				CPUReset();
				gamescreen_show_status_message(game, "Reset");

				return TRUE;
			}
			break;

		case SDLK_KP_DIVIDE:
			gamescreen_change_volume(game, -0.1);
			return TRUE;
		case SDLK_KP_MULTIPLY:
			gamescreen_change_volume(game, 0.1);
			return TRUE;
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
				gamescreen_write_state(game, event->key.keysym.sym - SDLK_F1);
				return TRUE;
			} else if (!(event->key.keysym.mod & MOD_KEYS)) {
				gamescreen_read_state(game, event->key.keysym.sym - SDLK_F1);
				return TRUE;
			}
			break;
		}
		break;
	}

	input_sdl_process_SDL_event(event);
	return TRUE; // No more handlers should be called
}

void gamescreen_update(GameScreen *game) {
	if (!game->inactive) {
		CPULoop(250000);
	} else {
		SDL_Delay(500);
	}
}
