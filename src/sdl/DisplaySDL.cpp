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

#include "DisplaySDL.h"
#include "text.h"
#include "../common/Settings.h"
#include "../gba/GBA.h"

#include <SDL.h>

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *screen;

	gboolean fullscreen;

	gboolean screenMessage;
	char screenMessageBuffer[21];
	guint32  screenMessageTime;
} DriverData;

void display_sdl_show_screen_message(DisplayDriver *driver, const gchar *msg) {
	g_assert(driver != NULL);
	DriverData *data = (DriverData *)driver->driverData;

	data->screenMessage = TRUE;
	data->screenMessageTime = SDL_GetTicks();
	if (strlen(msg) > 20) {
		strncpy(data->screenMessageBuffer, msg, 20);
		data->screenMessageBuffer[20] = 0;
	} else
		strcpy(data->screenMessageBuffer, msg);

	g_message("%s", msg);
}

static void display_sdl_draw_screen_message(DisplayDriver *driver, guint8 *screen, int pitch, int x, int y, unsigned int duration) {
	g_assert(driver != NULL);
	DriverData *data = (DriverData *)driver->driverData;

	if (data->screenMessage) {
		if (((SDL_GetTicks() - data->screenMessageTime) < duration)
				&& !settings_disable_status_messages()) {
			text_draw(screen, pitch, x, y, data->screenMessageBuffer, FALSE);
		} else {
			data->screenMessage = FALSE;
		}
	}
}

static void display_sdl_draw_speed(guint8 *screen, int pitch, int x, int y) {
	char buffer[50];
	sprintf(buffer, "%d%%", gba_get_speed());

	text_draw(screen, pitch, x, y, buffer, TRUE);
}

static void display_sdl_draw_screen(DisplayDriver *driver, guint32 *pix) {
	g_assert(driver != NULL);
	DriverData *data = (DriverData *)driver->driverData;

	// TODO: Don't draw text on pix
	display_sdl_draw_screen_message(driver, (guint8*) pix, 240 * 4,
			10, 160 - 20, 3000);

	if (settings_show_speed())
		display_sdl_draw_speed((guint8*) pix, 240 * 4, 10, 20);


	SDL_UpdateTexture(data->screen, NULL, pix, 240 * 4);
	// TODO: Error checking

	SDL_RenderCopy(data->renderer, data->screen, NULL, NULL);
	// TODO: Error checking

	SDL_RenderPresent(data->renderer);
}

gboolean display_sdl_create_window(DisplayDriver *driver, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
	g_assert(driver != NULL);
	DriverData *data = (DriverData *)driver->driverData;

	int width = 240;
	int height = 160;

	int flags = data->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;

	data->window = SDL_CreateWindow("Visual Boy Advance",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, flags);
	if (data->window == NULL) {
		g_set_error(err, DISPLAY_ERROR, G_DISPLAY_ERROR_FAILED,
				"Failed to create window: %s", SDL_GetError());
		return FALSE;
	}

	data->renderer = SDL_CreateRenderer(data->window, -1, SDL_RENDERER_ACCELERATED);
	if (data->renderer == NULL) {
		g_set_error(err, DISPLAY_ERROR, G_DISPLAY_ERROR_FAILED,
				"Failed to create renderer: %s", SDL_GetError());
		return FALSE;
	}

	data->screen = SDL_CreateTexture(data->renderer, SDL_PIXELFORMAT_ARGB8888,
			SDL_TEXTUREACCESS_STREAMING, width, height);
	if (data->screen == NULL) {
		g_set_error(err, DISPLAY_ERROR, G_DISPLAY_ERROR_FAILED,
				"Failed to create texture: %s", SDL_GetError());
		return FALSE;
	}

	return TRUE;
}

DisplayDriver *display_sdl_init(GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	DisplayDriver *driver = g_new(DisplayDriver, 1);
	driver->drawScreen = display_sdl_draw_screen;

	DriverData *data = g_new(DriverData, 1);

	data->window = NULL;
	data->renderer = NULL;
	data->screen = NULL;
	data->fullscreen = settings_is_fullscreen();
	data->screenMessage = FALSE;
	data->screenMessageBuffer[0] = '\0';
	data->screenMessageTime = 0;

	driver->driverData = data;

	SDL_InitSubSystem(SDL_INIT_VIDEO);

	if (!display_sdl_create_window(driver, err)) {
		g_free(driver->driverData);
		g_free(driver);
		return NULL;
	}

	return driver;
}

void display_sdl_free(DisplayDriver *driver) {
	if (driver == NULL)
		return;

	DriverData *data = (DriverData *)driver->driverData;

	SDL_DestroyTexture(data->screen);
	SDL_DestroyRenderer(data->renderer);
	SDL_DestroyWindow(data->window);

	SDL_QuitSubSystem(SDL_INIT_VIDEO);

	g_free(driver->driverData);
	g_free(driver);
}

gboolean display_sdl_is_fullscreen(DisplayDriver *driver) {
	g_assert(driver != NULL);
	DriverData *data = (DriverData *)driver->driverData;

	return data->fullscreen;
}

gboolean display_sdl_toggle_fullscreen(DisplayDriver *driver, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
	g_assert(driver != NULL);
	DriverData *data = (DriverData *)driver->driverData;

	if (SDL_SetWindowFullscreen(data->window, data->fullscreen ? 0 : SDL_WINDOW_FULLSCREEN_DESKTOP) < 0) {
		return FALSE;
	}

	data->fullscreen = !data->fullscreen;

	return TRUE;
}

void display_sdl_set_window_title(DisplayDriver *driver, const gchar *title) {
	g_assert(driver != NULL);
	DriverData *data = (DriverData *)driver->driverData;

	if (title != NULL) {
		gchar *fullTitle = g_strdup_printf("%s - Visual Boy Advance", title);
		SDL_SetWindowTitle(data->window, fullTitle);
		g_free (fullTitle);
	} else {
		SDL_SetWindowTitle(data->window, "Visual Boy Advance");
	}
}
