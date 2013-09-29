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
#include "GameScreen.h"
#include "OSD.h"
#include "../common/Settings.h"
#include "../gba/GBA.h"

#include <SDL.h>
#include <SDL_ttf.h>
#include <png.h>

static const int screenWidth = 240;
static const int screenHeigth = 160;

typedef struct {
	SDL_Window *window;
	SDL_Renderer *renderer;
	GSList *renderables;
	GameScreen *screen;
	TextOSD *speed;
	TextOSD *status;

	gboolean fullscreen;
} DriverData;

static void display_sdl_render(DisplayDriver *driver) {
	g_assert(driver != NULL);
	DriverData *data = (DriverData *)driver->driverData;

	// Clear the screen
	SDL_RenderClear(data->renderer);

	// Then render the renderables
	GSList *it = data->renderables;
	while (it != NULL) {
		Renderable *renderable = (Renderable *)it->data;
		if (renderable->render) {
			renderable->render(renderable->entity);
		}
		it = g_slist_next(it);
	}

	SDL_RenderPresent(data->renderer);
}

void display_sdl_show_screen_message(DisplayDriver *driver, const gchar *msg) {
	g_assert(driver != NULL);
	DriverData *data = (DriverData *)driver->driverData;

	if (data->status != NULL) {
		text_osd_set_message(data->status, msg);
		text_osd_set_auto_clear(data->status, 3000);
	}

	g_message("%s", msg);
}

static void display_sdl_update_speed(TextOSD *speed) {
	if (speed == NULL)
		return;

	char buffer[50];
	sprintf(buffer, "%d%%", gba_get_speed());
	text_osd_set_message(speed, buffer);
}

static void display_sdl_draw_screen(DisplayDriver *driver, guint16 *pix) {
	g_assert(driver != NULL);
	DriverData *data = (DriverData *)driver->driverData;

	display_sdl_update_speed(data->speed);
	gamescreen_update(data->screen, pix);

	display_sdl_render(driver);
}

static gboolean display_sdl_create_window(DisplayDriver *driver, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);
	g_assert(driver != NULL);
	DriverData *data = (DriverData *)driver->driverData;

	guint zoomFactor = settings_zoom_factor();

	int flags = SDL_WINDOW_RESIZABLE;
	flags |= data->fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0;

	data->window = SDL_CreateWindow("Visual Boy Advance",
			SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth * zoomFactor, screenHeigth * zoomFactor, flags);
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

	data->screen = gamescreen_create(driver, err);
	if (data->screen == NULL) {
		return FALSE;
	}

	if (settings_show_speed()) {
		data->speed = text_osd_create(driver, " ", err);
		if (data->speed == NULL) {
			return FALSE;
		}

		text_osd_set_color(data->speed, 255, 0, 0);
		text_osd_set_position(data->speed, 10, 10);
		text_osd_set_opacity(data->speed, 75);
	}

	if (!settings_disable_status_messages()) {
		data->status = text_osd_create(driver, " ", err);
		if (data->status == NULL) {
			return FALSE;
		}

		text_osd_set_color(data->status, 255, 0, 0);
		text_osd_set_position(data->status, 10, -10);
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
	data->renderables = NULL;
	data->screen = NULL;
	data->speed = NULL;

	data->fullscreen = settings_is_fullscreen();

	driver->driverData = data;

	SDL_InitSubSystem(SDL_INIT_VIDEO);

	if (TTF_Init()) {
		g_set_error(err, DISPLAY_ERROR, G_DISPLAY_ERROR_FAILED,
				"Failed to initialize true type rendering: %s", TTF_GetError());
		return FALSE;
	}

	if (!display_sdl_create_window(driver, err)) {
		display_sdl_free(driver);
		return NULL;
	}

	return driver;
}

void display_sdl_free(DisplayDriver *driver) {
	if (driver == NULL)
		return;

	DriverData *data = (DriverData *)driver->driverData;

	gamescreen_free(data->screen);
	text_osd_free(data->speed);

	SDL_DestroyRenderer(data->renderer);
	SDL_DestroyWindow(data->window);

	SDL_QuitSubSystem(SDL_INIT_VIDEO);

	TTF_Quit();

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

Renderable *display_sdl_renderable_create(DisplayDriver *driver, gpointer entity) {
	g_assert(driver != NULL);
	DriverData *data = (DriverData *)driver->driverData;

	Renderable *renderable = g_new(Renderable, 1);
	renderable->entity = entity;
	renderable->driver = driver;
	renderable->renderer = data->renderer;
	renderable->render = NULL;

	data->renderables = g_slist_append(data->renderables, renderable);

	return renderable;
}

void display_sdl_renderable_free(Renderable *renderable) {
	if (renderable == NULL)
		return;

	DriverData *data = (DriverData *)renderable->driver->driverData;

	data->renderables = g_slist_remove(data->renderables, renderable);

	g_free(renderable);
}

SDL_Texture *display_sdl_load_png(DisplayDriver *driver, const gchar *filename, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);
	g_assert(driver != NULL);
	DriverData *data = (DriverData *) driver->driverData;

	png_image image;
	memset(&image, 0, sizeof(image));
	image.version = PNG_IMAGE_VERSION;

	if (!png_image_begin_read_from_file(&image, filename)) {
		g_set_error(err, DISPLAY_ERROR, G_DISPLAY_ERROR_FAILED,
				"Failed to load png: %s", image.message);
		return NULL;
	}

	png_bytep buffer = (png_bytep) g_malloc(PNG_IMAGE_SIZE(image));
	image.format = PNG_FORMAT_RGBA;

	if (!png_image_finish_read(&image, NULL, buffer, 0, NULL)) {
		g_free(buffer);
		g_set_error(err, DISPLAY_ERROR, G_DISPLAY_ERROR_FAILED,
				"Failed to load png: %s", image.message);
		return NULL;
	}

	SDL_Texture *texture = SDL_CreateTexture(data->renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, image.width, image.height);
	if (texture == NULL) {
		g_free(buffer);
		g_set_error(err, DISPLAY_ERROR, G_DISPLAY_ERROR_FAILED,
				"Failed create texture : %s", SDL_GetError());
		return NULL;
	}

	SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
	SDL_UpdateTexture(texture, NULL, buffer, image.width * sizeof(guint32));

	g_free(buffer);
	return texture;
}
