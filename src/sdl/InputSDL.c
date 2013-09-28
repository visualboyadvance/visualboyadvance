// VBA-M, A Nintendo Handheld Console Emulator
// Copyright (C) 2008 VBA-M development team
//
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

#include "InputSDL.h"
#include "../common/Settings.h"

// Number of configurable buttons
#define SETTINGS_NUM_BUTTONS 10

static void key_update(uint32_t key, gboolean down);

static gboolean sdlButtons[SETTINGS_NUM_BUTTONS] = {
	FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
	FALSE, FALSE, FALSE, FALSE
};

static gboolean sdlMotionButtons[4] = { FALSE, FALSE, FALSE, FALSE };

static uint32_t default_joypad[SETTINGS_NUM_BUTTONS] = {
	SDL_SCANCODE_LEFT,  SDL_SCANCODE_RIGHT,
	SDL_SCANCODE_UP,    SDL_SCANCODE_DOWN,
	SDL_SCANCODE_Z,     SDL_SCANCODE_X,
	SDL_SCANCODE_RETURN,SDL_SCANCODE_BACKSPACE,
	SDL_SCANCODE_A,     SDL_SCANCODE_S
};

static uint32_t joypad[SETTINGS_NUM_BUTTONS] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


static uint32_t motion[4] = {
	SDL_SCANCODE_KP_4, SDL_SCANCODE_KP_6, SDL_SCANCODE_KP_8, SDL_SCANCODE_KP_2
};

static uint32_t defaultMotion[4] = {
	SDL_SCANCODE_KP_4, SDL_SCANCODE_KP_6, SDL_SCANCODE_KP_8, SDL_SCANCODE_KP_2
};

static int sensorX = 2047;
static int sensorY = 2047;

static GSList *gamecontrollers;

static void controller_open(gint32 index) {
	SDL_GameController *c = SDL_GameControllerOpen(index);
	gamecontrollers = g_slist_append(gamecontrollers, c);
}

static void controllers_close_removed() {
	GSList *it = gamecontrollers;
	while (it != NULL) {
		SDL_GameController *controller = (SDL_GameController *)it->data;
		it = g_slist_next(it);

		if (!SDL_GameControllerGetAttached(controller)) {
			SDL_GameControllerClose(controller);
			gamecontrollers = g_slist_remove(gamecontrollers, controller);
		}
	}
}

static void controllers_close_all() {
	GSList *it = gamecontrollers;
	while (it != NULL) {
		SDL_GameController *controller = (SDL_GameController *)it->data;
		it = g_slist_next(it);

		SDL_GameControllerClose(controller);
		gamecontrollers = g_slist_remove(gamecontrollers, controller);
	}
}

static void controller_update_button(guint8 button, gboolean down) {
	switch (button) {
	case SDL_CONTROLLER_BUTTON_A:
		sdlButtons[KEY_BUTTON_A] = down;
		break;
	case SDL_CONTROLLER_BUTTON_B:
		sdlButtons[KEY_BUTTON_B] = down;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
		sdlButtons[KEY_DOWN] = down;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
		sdlButtons[KEY_LEFT] = down;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
		sdlButtons[KEY_RIGHT] = down;
		break;
	case SDL_CONTROLLER_BUTTON_DPAD_UP:
		sdlButtons[KEY_UP] = down;
		break;
	case SDL_CONTROLLER_BUTTON_START:
		sdlButtons[KEY_BUTTON_START] = down;
		break;
	case SDL_CONTROLLER_BUTTON_BACK:
		sdlButtons[KEY_BUTTON_SELECT] = down;
		break;
	case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
		sdlButtons[KEY_BUTTON_L] = down;
		break;
	case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
		sdlButtons[KEY_BUTTON_R] = down;
		break;
	}
}

uint32_t input_sdl_get_keymap(EKey key)
{
	return joypad[key];
}

void input_sdl_set_keymap(EKey key, uint32_t code)
{
	joypad[key] = code;
}

void input_sdl_set_motion_keymap(EKey key, uint32_t code)
{
	motion[key] = code;
}

static void key_update(SDL_Scancode key, gboolean down)
{
	for (int i = 0 ; i < SETTINGS_NUM_BUTTONS; i++) {
		if (key == joypad[i])
			sdlButtons[i] = down;
	}

	for (int i = 0 ; i < 4; i++) {
		if (key == motion[i])
			sdlMotionButtons[i] = down;
	}
}

static uint32_t input_read_joypad(InputDriver *driver)
{
	uint32_t res = 0;

	if (sdlButtons[KEY_BUTTON_A])
		res |= 1;
	if (sdlButtons[KEY_BUTTON_B])
		res |= 2;
	if (sdlButtons[KEY_BUTTON_SELECT])
		res |= 4;
	if (sdlButtons[KEY_BUTTON_START])
		res |= 8;
	if (sdlButtons[KEY_RIGHT])
		res |= 16;
	if (sdlButtons[KEY_LEFT])
		res |= 32;
	if (sdlButtons[KEY_UP])
		res |= 64;
	if (sdlButtons[KEY_DOWN])
		res |= 128;
	if (sdlButtons[KEY_BUTTON_R])
		res |= 256;
	if (sdlButtons[KEY_BUTTON_L])
		res |= 512;

	// disallow L+R or U+D of being pressed at the same time
	if ((res & 48) == 48)
		res &= ~16;
	if ((res & 192) == 192)
		res &= ~128;

	return res;
}

static void input_update_motion_sensor(InputDriver *driver)
{
	if (sdlMotionButtons[KEY_LEFT]) {
		sensorX += 3;
		if (sensorX > 2197)
			sensorX = 2197;
		if (sensorX < 2047)
			sensorX = 2057;
	} else if (sdlMotionButtons[KEY_RIGHT]) {
		sensorX -= 3;
		if (sensorX < 1897)
			sensorX = 1897;
		if (sensorX > 2047)
			sensorX = 2037;
	} else if (sensorX > 2047) {
		sensorX -= 2;
		if (sensorX < 2047)
			sensorX = 2047;
	} else {
		sensorX += 2;
		if (sensorX > 2047)
			sensorX = 2047;
	}

	if (sdlMotionButtons[KEY_UP]) {
		sensorY += 3;
		if (sensorY > 2197)
			sensorY = 2197;
		if (sensorY < 2047)
			sensorY = 2057;
	} else if (sdlMotionButtons[KEY_DOWN]) {
		sensorY -= 3;
		if (sensorY < 1897)
			sensorY = 1897;
		if (sensorY > 2047)
			sensorY = 2037;
	} else if (sensorY > 2047) {
		sensorY -= 2;
		if (sensorY < 2047)
			sensorY = 2047;
	} else {
		sensorY += 2;
		if (sensorY > 2047)
			sensorY = 2047;
	}
}

static int input_get_sensor_x(InputDriver *driver)
{
	return sensorX;
}

static int input_get_sensor_y(InputDriver *driver)
{
	return sensorY;
}

InputDriver *input_sdl_init(GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER | SDL_INIT_EVENTS)) {
		g_set_error(err, INPUT_ERROR, G_INPUT_ERROR_FAILED,
				"Failed to init joystick support: %s", SDL_GetError());
		return NULL;
	}

	// Open available controllers
	for (int i = 0; i < SDL_NumJoysticks(); i++) {
		if (SDL_IsGameController(i)) {
			controller_open(i);
		}
	}

	// Apply the button mapping from settings
	for (guint i = 0; i < G_N_ELEMENTS(settings_buttons); i++) {
		guint32 keymap = settings_get_button_mapping(settings_buttons[i]);
		input_sdl_set_keymap(settings_buttons[i], keymap);
	}

	// The main joypad has to be entirely defined
	for (int i = 0; i < SETTINGS_NUM_BUTTONS; i++) {
		if (!joypad[i])
			joypad[i] = default_joypad[i];
	}

	InputDriver *driver = g_new(InputDriver, 1);
	driver->driverData = NULL;
	driver->read_joypad = input_read_joypad;
	driver->read_sensor_x = input_get_sensor_x;
	driver->read_sensor_y = input_get_sensor_y;
	driver->update_motion_sensor = input_update_motion_sensor;

	return driver;
}

void input_sdl_free(InputDriver *driver) {
	if (driver == NULL)
		return;

	controllers_close_all();

	g_free(driver->driverData);
	g_free(driver);
}

void input_sdl_process_SDL_event(const SDL_Event *event)
{
	switch (event->type)
	{
	case SDL_KEYDOWN:
		key_update(event->key.keysym.scancode, TRUE);
		break;
	case SDL_KEYUP:
		key_update(event->key.keysym.scancode, FALSE);
		break;
	case SDL_CONTROLLERDEVICEADDED:
		controller_open(event->cdevice.which);
		break;
	case SDL_CONTROLLERDEVICEREMOVED:
		controllers_close_removed();
		break;
	case SDL_CONTROLLERBUTTONDOWN:
		controller_update_button(event->cbutton.button, TRUE);
		break;
	case SDL_CONTROLLERBUTTONUP:
		controller_update_button(event->cbutton.button, FALSE);
		break;
	}
}

