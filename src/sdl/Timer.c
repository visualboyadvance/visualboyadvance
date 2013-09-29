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

#include "Timer.h"

#include <SDL.h>

struct Timeout {
	TimerAction action;
	guint32 time;
	gboolean expired;

	gpointer entity;
};

static GSList *timeouts = NULL;

Timeout *timeout_create(gpointer entity, TimerAction action) {

	Timeout *timeout = g_new(Timeout, 1);
	timeout->entity = entity;
	timeout->action = action;
	timeout->time = 0;
	timeout->expired = TRUE;

	timeouts = g_slist_append(timeouts, timeout);

	return timeout;
}

void timeout_free(Timeout *timeout) {
	if (timeout == NULL)
		return;

	timeouts = g_slist_remove(timeouts, timeout);

	g_free(timeout);
}

void timeout_set_duration(Timeout *timeout, guint32 millis) {
	g_assert(timeout != NULL);
	g_assert(millis > 0);

	timeout->time = SDL_GetTicks() + millis;
	timeout->expired = FALSE;
}

void timers_update() {
	GSList *it = timeouts;
	while (it != NULL) {
		Timeout *timeout = (Timeout *)it->data;
		it = g_slist_next(it);
		if (SDL_GetTicks() >= timeout->time && !timeout->expired && timeout->action) {
			timeout->action(timeout->entity);
			timeout->expired = TRUE;
		}
	}
}
