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

#include "OSD.h"
#include "Timer.h"
#include "../common/DisplayDriver.h"
#include "../common/Util.h"

#include <SDL_ttf.h>

struct TextOSD {
	Renderable *renderable;
	Timeout *autoclear;

	gchar *message;
	SDL_Color color;
	gint size;
	gint opacity;

	/** Texture to render */
	SDL_Texture *texture;

	/** Screen position x */
	gint x;

	/** Screen position y */
	gint y;
};

struct ImageOSD {
	Renderable *renderable;

	/** Texture to render */
	SDL_Texture *texture;

	/** Screen position x */
	gint x;

	/** Screen position y */
	gint y;
};

static gboolean text_update_texture(TextOSD *text, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

	SDL_DestroyTexture(text->texture);

	gchar *fontFile = data_get_file_path("fonts", "DroidSans-Bold.ttf");
	TTF_Font *font = TTF_OpenFont(fontFile, text->size);
	g_free(fontFile);

	if (font == NULL) {
		g_set_error(err, DISPLAY_ERROR, G_DISPLAY_ERROR_FAILED,
				"Failed to load font: %s", TTF_GetError());
		return FALSE;
	}

	SDL_Surface *surface = TTF_RenderText_Blended(font, text->message, text->color);
	if (surface == NULL) {
		g_set_error(err, DISPLAY_ERROR, G_DISPLAY_ERROR_FAILED,
				"Failed render text: %s", TTF_GetError());
		return FALSE;
	}

	text->texture = SDL_CreateTextureFromSurface(text->renderable->renderer, surface);
	if (text->texture == NULL) {
		g_set_error(err, DISPLAY_ERROR, G_DISPLAY_ERROR_FAILED,
				"Failed create texture : %s", SDL_GetError());
		return FALSE;
	}

	if (text->opacity != 100) {
		SDL_SetTextureAlphaMod(text->texture, text->opacity * 255 / 100);
	}

	SDL_FreeSurface(surface);
	TTF_CloseFont(font);

	return TRUE;
}

static void osd_render(Renderable *renderable, SDL_Texture *texture, gint x, gint y) {
	int textWidth, textHeight;
	SDL_QueryTexture(texture, NULL, NULL, &textWidth, &textHeight);

	int windowWidth, windowHeight;
	SDL_GetRendererOutputSize(renderable->renderer, &windowWidth, &windowHeight);

	SDL_Rect screenRect;
	screenRect.w = textWidth;
	screenRect.h = textHeight;

	if (x >= 0) {
		screenRect.x = x;
	} else {
		screenRect.x = windowWidth - textWidth + x;
	}

	if (y >= 0) {
		screenRect.y = y;
	} else {
		screenRect.y = windowHeight - textHeight + y;
	}

	SDL_RenderCopy(renderable->renderer, texture, NULL, &screenRect);
}

static void text_osd_render(gpointer entity) {
	TextOSD *text = entity;

	if (text->texture == NULL) {
		text_update_texture(text, NULL);
	}

	osd_render(text->renderable, text->texture, text->x, text->y);
}

TextOSD *text_osd_create(Display *display, const gchar *message, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	TextOSD *text = g_new(TextOSD, 1);
	text->renderable = display_sdl_renderable_create(display, text);
	text->renderable->render = text_osd_render;
	text->autoclear = NULL;

	text->message = g_strdup(message);
	text->size = 10;
	text->opacity = 100;
	text->color.r = 0;
	text->color.g = 0;
	text->color.b = 0;
	text->x = 0;
	text->y = 0;
	text->texture = NULL;

	// Render here to perform error checking
	if (!text_update_texture(text, err)) {
		text_osd_free(text);
		return NULL;
	}

	return text;
}

void text_osd_free(TextOSD *text) {
	if (text == NULL)
		return;

	SDL_DestroyTexture(text->texture);

	display_sdl_renderable_free(text->renderable);
	timeout_free(text->autoclear);

	g_free(text->message);
	g_free(text);
}

void text_osd_set_message(TextOSD *text, const gchar *message) {
	g_assert(text != NULL);

	if (g_strcmp0(message, text->message) == 0)
		return; // Nothing to update

	g_free(text->message);
	text->message = g_strdup(message);

	SDL_DestroyTexture(text->texture);
	text->texture = NULL;
}

void text_osd_set_color(TextOSD *text, guint8 r, guint8 g, guint8 b) {
	g_assert(text != NULL);

	text->color.r = r;
	text->color.g = g;
	text->color.b = b;

	SDL_DestroyTexture(text->texture);
	text->texture = NULL;
}

void text_osd_set_position(TextOSD *text, gint x, gint y) {
	g_assert(text != NULL);

	text->x = x;
	text->y = y;
}

void text_osd_set_opacity(TextOSD *text, gint opacity) {
	g_assert(text != NULL);
	g_assert(opacity >= 0 && opacity <= 100);

	text->opacity = opacity;

	SDL_DestroyTexture(text->texture);
	text->texture = NULL;
}

void text_osd_set_size(TextOSD *text, gint size) {
	g_assert(text != NULL);
	g_assert(size > 0);

	text->size = size;

	SDL_DestroyTexture(text->texture);
	text->texture = NULL;
}

static void text_osd_autoclear(gpointer entity) {
	TextOSD *text = entity;
	text_osd_set_message(text, " ");
}

void text_osd_set_auto_clear(TextOSD *text, guint duration) {
	g_assert(text != NULL);

	timeout_free(text->autoclear);

	text->autoclear = timeout_create(text, text_osd_autoclear);

	timeout_set_duration(text->autoclear, duration);
}

static void image_osd_render(gpointer entity) {
	ImageOSD *image = entity;

	osd_render(image->renderable, image->texture, image->x, image->y);
}

ImageOSD *image_osd_create(Display *display, const gchar *file, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	ImageOSD *image = g_new(ImageOSD, 1);
	image->renderable = display_sdl_renderable_create(display, image);
	image->renderable->render = image_osd_render;

	image->x = 0;
	image->y = 0;
	image->texture = display_sdl_load_png(display, file, err);


	// Render here to perform error checking
	if (image->texture == NULL) {
		image_osd_free(image);
		return NULL;
	}

	return image;
}

void image_osd_free(ImageOSD *image) {
	if (image == NULL)
		return;

	SDL_DestroyTexture(image->texture);

	display_sdl_renderable_free(image->renderable);
	g_free(image);
}

void image_osd_set_position(ImageOSD *image, gint x, gint y) {
	g_assert(image != NULL);

	image->x = x;
	image->y = y;
}
