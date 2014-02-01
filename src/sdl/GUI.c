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

#include "GUI.h"
#include "OSD.h"

typedef struct Actionable Actionable;
struct Actionable {

	const Renderable *renderable;

	ActionableAction action;

	gpointer entity;
};

static GSList *actionables = NULL;

Actionable *actionable_create(gpointer entity, ActionableAction action, const Renderable *renderable) {
	g_assert(entity != NULL && action != NULL && renderable != NULL);

	Actionable *actionable = g_new(Actionable, 1);
	actionable->entity = entity;
	actionable->action = action;
	actionable->renderable = renderable;

	actionables = g_slist_append(actionables, actionable);

	return actionable;
}

void actionable_free(Actionable *actionable) {
	if (actionable == NULL)
		return;

	actionables = g_slist_remove(actionables, actionable);

	g_free(actionable);
}

static gboolean renderable_is_point_inside(const Renderable *renderable, gint x, gint y) {
	g_assert(renderable != NULL);

	gint left, top;
	display_sdl_renderable_get_absolute_position(renderable, &left, &top);

	gint right = left + display_sdl_scale(renderable->display, renderable->width);
	gint bottom = top + display_sdl_scale(renderable->display, renderable->height);

	if (left <= x && x <= right
			&& top <= y && y <= bottom) {
		return TRUE;
	}

	return FALSE;
}

static gboolean actionable_handle_event(Actionable *actionable, const SDL_Event *event) {
	g_assert(actionable != NULL && event != NULL);

	const Renderable *renderable = actionable->renderable;

	switch (event->type) {
	case SDL_MOUSEBUTTONUP:
		if (renderable_is_point_inside(renderable, event->motion.x, event->motion.y)) {
			if (actionable->action) {
				actionable->action(actionable->entity);
			}
			return TRUE; // The event has been handled
		}
		break;
	}

	return FALSE;
}

gboolean actionables_handle_event(const SDL_Event *event) {
	GSList *it = actionables;
	while (it != NULL) {
		Actionable *actionable = (Actionable *)it->data;
		it = g_slist_next(it);
		if (actionable_handle_event(actionable, event)) {
			return TRUE;
		}
	}

	return FALSE;
}

struct Button {
	RectOSD *background;
	TextOSD *caption;
	Actionable *action;
};

Button *button_create(Display *display, const gchar *caption, gint x, gint y, guint w, guint h, ActionableAction action, const Renderable *parent, GError **err) {
	g_assert(display != NULL && action != NULL);

	Button *button = g_new(Button, 1);

	button->background = rect_osd_create(display, x, y, w, h, parent, err);
	if (button->background == NULL) {
		button_free(button);
		return NULL;
	}
	rect_osd_set_opacity(button->background, 80);
	rect_osd_set_color(button->background, 0, 0, 64);
	rect_osd_set_border(button->background, 1, 220, 220, 220);

	const Renderable *backgroundRenderable = rect_osd_get_renderable(button->background);

	button->action = actionable_create(button, action, backgroundRenderable);
	g_assert(button->action != NULL);

	button->caption = text_osd_create(display, caption, backgroundRenderable, err);
	if (button->caption == NULL) {
		button_free(button);
		return NULL;
	}
	text_osd_set_alignment(button->caption, ALIGN_CENTER, ALIGN_MIDDLE);
	text_osd_set_size(button->caption, w, h * 0.60);
	text_osd_set_color(button->caption, 220, 220, 220);

	return button;
}

void button_free(Button *button) {
	if (button == NULL)
		return;

	actionable_free(button->action);
	text_osd_free(button->caption);
	rect_osd_free(button->background);

	g_free(button);
}

static GSList *screens = NULL;

Screen *screen_create(gpointer entity, GQuark type) {
	g_assert(entity != NULL && type != 0);

	Screen *screen = g_new0(Screen, 1);
	screen->entity = entity;
	screen->type = type;

	screens = g_slist_prepend(screens, screen);

	return screen;
}

void screen_free(Screen *screen) {
	if (screen == NULL)
		return;

	screens = g_slist_remove(screens, screen);

	g_free(screen);
}

void screens_update_current() {
	if (screens == NULL)
		return;

	Screen *current = screens->data;
	current->update(current->entity);
}

void screens_process_event_current(const SDL_Event *event) {
	if (screens == NULL)
		return;

	Screen *current = screens->data;
	current->process_event(current->entity, event);
}

void screens_free_all() {
	GSList *it = screens;
	while (it != NULL) {
		Screen *screen = (Screen *)it->data;
		it = g_slist_next(it);
		screen->free(screen->entity);
	}
}

void screens_pop() {
	if (screens == NULL)
		return;

	Screen *current = screens->data;
	current->free(current->entity);
}

gboolean screens_current_is(GQuark type) {
	if (screens == NULL)
		return FALSE;

	Screen *current = screens->data;

	return current->type == type;
}
