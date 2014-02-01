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

#include "ErrorScreen.h"
#include "DisplaySDL.h"
#include "GUI.h"
#include "OSD.h"
#include "VBA.h"
#include "../gba/Cartridge.h"
#include "../common/Util.h"

static const int screenWidth = 240;
static const int screenHeight = 160;

struct ErrorScreen {
	Screen *screen;

	Display *display;

	RectOSD *background;
	TextOSD *message;

	Button *quit;
};

GQuark errorscreen_quark() {
	return g_quark_from_static_string("errorscreen_quark");
}

static void errorscreen_on_quit(gpointer entity) {
	vba_quit();
}

static void errorscreen_free(ErrorScreen *error) {
	if (error == NULL)
		return;

	button_free(error->quit);

	text_osd_free(error->message);
	rect_osd_free(error->background);

	screen_free(error->screen);

	g_free(error);
}

static void errorscreen_free_from_entity(gpointer entity) {
	errorscreen_free(entity);
}

static gboolean errorscreen_process_event(gpointer entity, const SDL_Event *event) {
	return TRUE; // No more handlers should be called
}

static void errorscreen_update(gpointer entity) {
	SDL_Delay(50);
}

ErrorScreen *errorscreen_create(Display *display, const gchar *message, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);
	g_assert(display != NULL);

	ErrorScreen *error = g_new0(ErrorScreen, 1);
	error->display = display;
	error->screen = screen_create(error, errorscreen_quark());
	error->screen->update = errorscreen_update;
	error->screen->process_event = errorscreen_process_event;
	error->screen->free = errorscreen_free_from_entity;

	// Create a semi-transparent background
	error->background = rect_osd_create(display, 0, 0, screenWidth, screenHeight, NULL, err);
	if (error->background == NULL) {
		errorscreen_free(error);
		return NULL;
	}
	rect_osd_set_opacity(error->background, 80);
	rect_osd_set_alignment(error->background, ALIGN_CENTER, ALIGN_MIDDLE);

	const Renderable* parent = rect_osd_get_renderable(error->background);

	// Create a pause message
	error->message = text_osd_create(display, message, parent, err);
	if (error->message == NULL) {
		errorscreen_free(error);
		return NULL;
	}
	text_osd_set_alignment(error->message, ALIGN_CENTER, ALIGN_MIDDLE);
	text_osd_set_position(error->message, 0, -20);
	text_osd_set_size(error->message, 240, 7);
	text_osd_set_color(error->message, 220, 220, 220);

	error->quit = button_create(display, "Quit", 90, 80, 60, 15, &errorscreen_on_quit, parent, err);
	if (error->quit == NULL) {
		errorscreen_free(error);
		return NULL;
	}

	return error;
}
