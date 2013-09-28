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

#ifndef __VBA_TIMER_SDL_H__
#define __VBA_TIMER_SDL_H__

#include <glib.h>

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Component allowing an entity to run an action at a given time
 */
typedef struct Timeout Timeout;
struct Timeout {
	void (*action)(gpointer entity);

	guint32 time;

	gpointer entity;
};

/**
 * Create a new timeout component and add it to an entity
 *
 * The action runs automatically when the timeout expires
 *
 * @param entity Entity this component applies to
 */
Timeout *timeout_create(gpointer entity);

/**
 * Set the duration of a Timeout
 *
 * @param timeout Timeout to modify
 * @param millis Duration in milliseconds before action is run
 */
void timeout_set_duration(Timeout *timeout, guint32 millis);

/**
 * Set the duration of a Timeout
 *
 * @param timeout Timeout to modify
 * @param millis Duration in milliseconds before action is run
 */
void timeout_free(Timeout *timeout);

/**
 * Update all the timers
 */
void timers_update();

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif // __VBA_TIMER_SDL_H__
