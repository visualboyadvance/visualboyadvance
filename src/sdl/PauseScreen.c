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
#include "InputSDL.h"
#include "OSD.h"
#include "Timer.h"

static const int screenWidth = 240;
static const int screenHeight = 160;

struct PauseScreen {
	Display *display;

	RectOSD *background;
	TextOSD *text;
};

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

	// Create a pause message
	pause->text = text_osd_create(display, "Paused", rect_osd_get_renderable(pause->background), err);
	if (pause->text == NULL) {
		pausescreen_free(pause);
		return NULL;
	}
	text_osd_set_alignment(pause->text, ALIGN_CENTER, ALIGN_MIDDLE);
	text_osd_set_size(pause->text, 200, 20);
	text_osd_set_color(pause->text, 220, 220, 220);

	return pause;
}

void pausescreen_free(PauseScreen *pause) {
	if (pause == NULL)
		return;

	rect_osd_free(pause->background);
	text_osd_free(pause->text);

	g_free(pause);
}

gboolean pausescreen_process_event(PauseScreen *game, const SDL_Event *event) {
	return TRUE; // No more handlers should be called
}

void pausescreen_update(PauseScreen *game) {
	SDL_Delay(50);
}
