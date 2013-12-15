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

#include "PauseScreen.h"
#include "DisplaySDL.h"
#include "OSD.h"
#include "../gba/Cartridge.h"
#include "../common/Util.h"

static const int screenWidth = 240;
static const int screenHeight = 160;

struct PauseScreen {
	Display *display;

	RectOSD *background;
	TextOSD *pauseMessage;
	ImageOSD *flag;
	TextOSD *title;
	TextOSD *publisher;
};

static gchar *region_flag_get_path(const gchar *region) {
	gchar *regionImage = g_strconcat(region, ".png", NULL);

	gchar *flagFile = data_get_file_path("flags", regionImage);
	if (!g_file_test(flagFile, G_FILE_TEST_EXISTS)) {
		g_free(flagFile);
		flagFile = data_get_file_path("flags", "Unknown.png");
	}

	g_free(regionImage);
	return flagFile;
}

PauseScreen *pausescreen_create(Display *display, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);
	g_assert(display != NULL);

	PauseScreen *pause = g_new(PauseScreen, 1);
	pause->display = display;

	// Create a semi-transparent background
	pause->background = rect_osd_create(display, 0, 0, screenWidth, screenHeight, err);
	if (pause->background == NULL) {
		pausescreen_free(pause);
		return NULL;
	}
	rect_osd_set_opacity(pause->background, 80);
	rect_osd_set_alignment(pause->background, ALIGN_CENTER, ALIGN_MIDDLE);

	const Renderable* parent = rect_osd_get_renderable(pause->background);

	// Create a pause message
	pause->pauseMessage = text_osd_create(display, "Paused", parent, err);
	if (pause->pauseMessage == NULL) {
		pausescreen_free(pause);
		return NULL;
	}
	text_osd_set_alignment(pause->pauseMessage, ALIGN_CENTER, ALIGN_MIDDLE);
	text_osd_set_size(pause->pauseMessage, 200, 20);
	text_osd_set_color(pause->pauseMessage, 220, 220, 220);

	// Create the game region flag
	gchar *region = region_flag_get_path(cartridge_get_game_region());
	pause->flag = image_osd_create(display, region, parent, err);
	g_free(region);

	if (pause->flag == NULL) {
		pausescreen_free(pause);
		return NULL;
	}

	image_osd_set_alignment(pause->flag, ALIGN_RIGHT, ALIGN_TOP);
	image_osd_set_position(pause->flag, 4, 0);
	image_osd_set_size(pause->flag, 20, 22);

	// Create the game title text
	pause->title = text_osd_create(display, cartridge_get_game_title(), parent, err);
	if (pause->title == NULL) {
		pausescreen_free(pause);
		return NULL;
	}

	text_osd_set_alignment(pause->title, ALIGN_RIGHT, ALIGN_TOP);
	text_osd_set_position(pause->title, 28, 4);
	text_osd_set_size(pause->title, 212, 8);
	text_osd_set_color(pause->title, 220, 220, 220);

	// Create the game publisher text
	pause->publisher = text_osd_create(display, cartridge_get_game_publisher(), parent, err);
	if (pause->publisher == NULL) {
		pausescreen_free(pause);
		return NULL;
	}

	text_osd_set_alignment(pause->publisher, ALIGN_RIGHT, ALIGN_TOP);
	text_osd_set_position(pause->publisher, 28, 12);
	text_osd_set_size(pause->publisher, 212, 5);
	text_osd_set_color(pause->publisher, 168, 168, 168);

	return pause;
}

void pausescreen_free(PauseScreen *pause) {
	if (pause == NULL)
		return;

	image_osd_free(pause->flag);
	text_osd_free(pause->title);
	text_osd_free(pause->publisher);
	text_osd_free(pause->pauseMessage);
	rect_osd_free(pause->background);

	g_free(pause);
}

gboolean pausescreen_process_event(PauseScreen *game, const SDL_Event *event) {
	return TRUE; // No more handlers should be called
}

void pausescreen_update(PauseScreen *game) {
	SDL_Delay(50);
}
