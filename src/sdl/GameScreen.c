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

static const int screenWidth = 240;
static const int screenHeigth = 160;

struct GameScreen {
	Renderable *renderable;
	SDL_Texture *texture;

};

void gamescreen_update(GameScreen *screen, guint16 *pix) {
	g_assert(screen != NULL);

	SDL_UpdateTexture(screen->texture, NULL, pix, screenWidth * sizeof(*pix));
	// TODO: Error checking
}

void gamescreen_render(gpointer entity) {
	g_assert(entity != NULL);
	GameScreen *screen = (GameScreen *)entity;

	// Do letterboxing to preserve aspect ratio regardless of the window size
	int windowWidth, windowHeight;
	SDL_GetWindowSize(screen->renderable->window, &windowWidth, &windowHeight);

	double scale = MIN(windowHeight / (double)screenHeigth, windowWidth / (double)screenWidth);
	SDL_Rect screenRect;
	screenRect.w = screenWidth * scale;
	screenRect.h = screenHeigth * scale;
	screenRect.x = (windowWidth - screenRect.w) / 2;
	screenRect.y = (windowHeight - screenRect.h) / 2;

	SDL_RenderCopy(screen->renderable->renderer, screen->texture, NULL, &screenRect);
}

GameScreen *gamescreen_create(DisplayDriver *driver, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);
	g_assert(driver != NULL);

	GameScreen *screen = g_new(GameScreen, 1);

	screen->renderable = display_sdl_renderable_create(driver, screen);
	screen->renderable->render = gamescreen_render;

	screen->texture = SDL_CreateTexture(screen->renderable->renderer, SDL_PIXELFORMAT_BGR555,
			SDL_TEXTUREACCESS_STREAMING, screenWidth, screenHeigth);

	if (screen->texture == NULL) {
		g_set_error(err, DISPLAY_ERROR, G_DISPLAY_ERROR_FAILED,
				"Failed to create texture: %s", SDL_GetError());
		gamescreen_free(screen);
		return NULL;
	}

	return screen;
}

void gamescreen_free(GameScreen *screen) {
	if (screen == NULL)
		return;

	display_sdl_renderable_free(screen->renderable);
	SDL_DestroyTexture(screen->texture);
	g_free(screen);
}
