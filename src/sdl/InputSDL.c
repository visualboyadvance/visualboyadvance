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
#define SETTINGS_NUM_BUTTONS 13

static void key_update(uint32_t key, gboolean down);
static void button_update(int which, int button, gboolean pressed);
static void hat_update(int which, int hat, int value);
static void axis_update(int which, int axis, int value);
static gboolean key_check(int key);

static gboolean sdlButtons[SETTINGS_NUM_BUTTONS] = {
	FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
	FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,
	FALSE
};

static gboolean sdlMotionButtons[4] = { FALSE, FALSE, FALSE, FALSE };

static int sdlNumDevices = 0;
static SDL_Joystick **sdlDevices = NULL;

static int autoFire = 0;
static gboolean autoFireToggle = FALSE;
static int autoFireCountdown = 0;
static int autoFireMaxCount = 1;

static uint32_t default_joypad[SETTINGS_NUM_BUTTONS] = {
	SDLK_LEFT,  SDLK_RIGHT,
	SDLK_UP,    SDLK_DOWN,
	SDLK_z,     SDLK_x,
	SDLK_RETURN,SDLK_BACKSPACE,
	SDLK_a,     SDLK_s,
	SDLK_SPACE,
	SDLK_q,     SDLK_w,
};

static uint32_t joypad[SETTINGS_NUM_BUTTONS] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


static uint32_t motion[4] = {
	SDLK_KP4, SDLK_KP6, SDLK_KP8, SDLK_KP2
};

static uint32_t defaultMotion[4] = {
	SDLK_KP4, SDLK_KP6, SDLK_KP8, SDLK_KP2
};

static int sensorX = 2047;
static int sensorY = 2047;

static uint32_t hat_get_code(const SDL_Event *event)
{
    if (!event->jhat.value) return 0;
    
	return (
	           ((event->jhat.which + 1) << 16) |
	           32 |
	           (event->jhat.hat << 2) |
	           (
	               event->jhat.value & SDL_HAT_UP ? 0 :
	               event->jhat.value & SDL_HAT_DOWN ? 1 :
	               event->jhat.value & SDL_HAT_RIGHT ? 2 :
	               event->jhat.value & SDL_HAT_LEFT ? 3 : 0
	           )
	       );
}

static uint32_t button_get_code(const SDL_Event *event)
{
	return (
	           ((event->jbutton.which + 1) << 16) |
	           (event->jbutton.button + 0x80)
	       );
}

static uint32_t axis_get_code(const SDL_Event *event)
{
	if (event->jaxis.value >= -16384 && event->jaxis.value <= 16384) return 0;

	return (
	           ((event->jaxis.which + 1) << 16) |
	           (event->jaxis.axis << 1) |
	           (
	               event->jaxis.value > 16384 ? 1 :
	               event->jaxis.value < -16384 ? 0 : 0
	           )
	       );
}

uint32_t input_sdl_get_event_code(const SDL_Event *event)
{
	switch (event->type)
	{
	case SDL_KEYDOWN:
	case SDL_KEYUP:
		return event->key.keysym.sym;
		break;
	case SDL_JOYHATMOTION:
		return hat_get_code(event);
		break;
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
		return button_get_code(event);
		break;
	case SDL_JOYAXISMOTION:
		return axis_get_code(event);
		break;
	default:
		return 0;
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

gboolean input_sdl_get_autofire(EKey key)
{
	int mask = 0;

	switch (key)
	{
	case KEY_BUTTON_A:
		mask = 1 << 0;
		break;
	case KEY_BUTTON_B:
		mask = 1 << 1;
		break;
	case KEY_BUTTON_R:
		mask = 1 << 8;
		break;
	case KEY_BUTTON_L:
		mask = 1 << 9;
		break;
	default:
		break;
	}

	return !(autoFire & mask);
}

gboolean input_sdl_toggle_autofire(EKey key)
{
	int mask = 0;

	switch (key)
	{
	case KEY_BUTTON_A:
		mask = 1 << 0;
		break;
	case KEY_BUTTON_B:
		mask = 1 << 1;
		break;
	case KEY_BUTTON_R:
		mask = 1 << 8;
		break;
	case KEY_BUTTON_L:
		mask = 1 << 9;
		break;
	default:
		break;
	}

	if (autoFire & mask)
	{
		autoFire &= ~mask;
		return FALSE;
	}
	else
	{
		autoFire |= mask;
		return TRUE;
	}
}

static void key_update(uint32_t key, gboolean down)
{
	int i;

	for (i = 0 ; i < SETTINGS_NUM_BUTTONS; i++) {
		if ((joypad[i] & 0xffff0000) == 0) {
			if (key == joypad[i])
				sdlButtons[i] = down;
		}
	}

	for (i = 0 ; i < 4; i++) {
		if ((motion[i] & 0xffff0000) == 0) {
			if (key == motion[i])
				sdlMotionButtons[i] = down;
		}
	}
}

static void button_update(int which,
                               int button,
                               gboolean pressed)
{
	int i;

	for (i = 0; i < SETTINGS_NUM_BUTTONS; i++) {
		int dev = (joypad[i] >> 16);
		int b = joypad[i] & 0xffff;
		if (dev) {
			dev--;

			if ((dev == which) && (b >= 128) && (b == (button+128))) {
				sdlButtons[i] = pressed;
			}
		}
	}

	for (i = 0; i < 4; i++) {
		int dev = (motion[i] >> 16);
		int b = motion[i] & 0xffff;
		if (dev) {
			dev--;

			if ((dev == which) && (b >= 128) && (b == (button+128))) {
				sdlMotionButtons[i] = pressed;
			}
		}
	}
}

static void hat_update(int which,
                            int hat,
                            int value)
{
	int i;

	for (i = 0; i < SETTINGS_NUM_BUTTONS; i++) {
		int dev = (joypad[i] >> 16);
		int a = joypad[i] & 0xffff;
		if (dev) {
			dev--;

			if ((dev == which) && (a>=32) && (a < 48) && (((a&15)>>2) == hat)) {
				int dir = a & 3;
				int v = 0;
				switch (dir) {
				case 0:
					v = value & SDL_HAT_UP;
					break;
				case 1:
					v = value & SDL_HAT_DOWN;
					break;
				case 2:
					v = value & SDL_HAT_RIGHT;
					break;
				case 3:
					v = value & SDL_HAT_LEFT;
					break;
				}
				sdlButtons[i] = (v ? TRUE : FALSE);
			}
		}
	}

	for (i = 0; i < 4; i++) {
		int dev = (motion[i] >> 16);
		int a = motion[i] & 0xffff;
		if (dev) {
			dev--;

			if ((dev == which) && (a>=32) && (a < 48) && (((a&15)>>2) == hat)) {
				int dir = a & 3;
				int v = 0;
				switch (dir) {
				case 0:
					v = value & SDL_HAT_UP;
					break;
				case 1:
					v = value & SDL_HAT_DOWN;
					break;
				case 2:
					v = value & SDL_HAT_RIGHT;
					break;
				case 3:
					v = value & SDL_HAT_LEFT;
					break;
				}
				sdlMotionButtons[i] = (v ? TRUE : FALSE);
			}
		}
	}
}

static void axis_update(int which,
                             int axis,
                             int value)
{
	int i;

	for (i = 0; i < SETTINGS_NUM_BUTTONS; i++) {
		int dev = (joypad[i] >> 16);
		int a = joypad[i] & 0xffff;
		if (dev) {
			dev--;

			if ((dev == which) && (a < 32) && ((a>>1) == axis)) {
				sdlButtons[i] = (a & 1) ? (value > 16384) : (value < -16384);
			}
		}
	}

	for (i = 0; i < 4; i++) {
		int dev = (motion[i] >> 16);
		int a = motion[i] & 0xffff;
		if (dev) {
			dev--;

			if ((dev == which) && (a < 32) && ((a>>1) == axis)) {
				sdlMotionButtons[i] = (a & 1) ? (value > 16384) : (value < -16384);
			}
		}
	}
}

static gboolean key_check(int key)
{
	int dev = (key >> 16) - 1;
	int what = key & 0xffff;

	if (what >= 128) {
		// joystick button
		int button = what - 128;

		if (button >= SDL_JoystickNumButtons(sdlDevices[dev]))
			return FALSE;
	} else if (what < 0x20) {
		// joystick axis
		what >>= 1;
		if (what >= SDL_JoystickNumAxes(sdlDevices[dev]))
			return FALSE;
	} else if (what < 0x30) {
		// joystick hat
		what = (what & 15);
		what >>= 2;
		if (what >= SDL_JoystickNumHats(sdlDevices[dev]))
			return FALSE;
	}

	// no problem found
	return TRUE;
}

static uint32_t input_read_joypad(InputDriver *driver)
{
	int realAutoFire  = autoFire;

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
	if (sdlButtons[KEY_BUTTON_AUTO_A])
		realAutoFire ^= 1;
	if (sdlButtons[KEY_BUTTON_AUTO_B])
		realAutoFire ^= 2;

	// disallow L+R or U+D of being pressed at the same time
	if ((res & 48) == 48)
		res &= ~16;
	if ((res & 192) == 192)
		res &= ~128;

	if (sdlButtons[KEY_BUTTON_SPEED])
		res |= 1024;

	if (realAutoFire) {
		res &= (~realAutoFire);
		if (autoFireToggle)
			res |= realAutoFire;
		autoFireCountdown--; // this needs decrementing even when autoFireToggle is toggled,
		// so that autoFireMaxCount==1 (the default) will alternate at the maximum possible
		// frequency (every time this code is reached). Which is what it did before
		// introducing autoFireCountdown.
		if (autoFireCountdown <= 0) {
			autoFireToggle = !autoFireToggle;
			autoFireCountdown = autoFireMaxCount;
		}
	}

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

	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK)) {
		g_set_error(err, INPUT_ERROR, G_INPUT_ERROR_FAILED,
				"Failed to init joystick support: %s", SDL_GetError());
		return NULL;
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

	sdlNumDevices = SDL_NumJoysticks();

	if (sdlNumDevices)
		sdlDevices = (SDL_Joystick **)calloc(1,sdlNumDevices *
		                                     sizeof(SDL_Joystick **));
	gboolean usesJoy = FALSE;

	for (int i = 0; i < SETTINGS_NUM_BUTTONS; i++) {
		int dev = joypad[i] >> 16;
		if (dev) {
			dev--;
			gboolean ok = FALSE;

			if (sdlDevices) {
				if (dev < sdlNumDevices) {
					if (sdlDevices[dev] == NULL) {
						sdlDevices[dev] = SDL_JoystickOpen(dev);
					}

					ok = key_check(joypad[i]);
				} else
					ok = FALSE;
			}

			if (!ok)
				joypad[i] = default_joypad[i];
			else
				usesJoy = TRUE;
		}
	}

	for (int i = 0; i < 4; i++) {
		int dev = motion[i] >> 16;
		if (dev) {
			dev--;
			gboolean ok = FALSE;

			if (sdlDevices) {
				if (dev < sdlNumDevices) {
					if (sdlDevices[dev] == NULL) {
						sdlDevices[dev] = SDL_JoystickOpen(dev);
					}

					ok = key_check(motion[i]);
				} else
					ok = FALSE;
			}

			if (!ok)
				motion[i] = defaultMotion[i];
			else
				usesJoy = TRUE;
		}
	}

	if (usesJoy)
		SDL_JoystickEventState(SDL_ENABLE);

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

	g_free(driver->driverData);
	g_free(driver);
}

void input_sdl_process_SDL_event(const SDL_Event *event)
{
//	fprintf(stdout, "%x\n", inputGetEventCode(event));

	switch (event->type)
	{
	case SDL_KEYDOWN:
		key_update(event->key.keysym.sym, TRUE);
		break;
	case SDL_KEYUP:
		key_update(event->key.keysym.sym, FALSE);
		break;
	case SDL_JOYHATMOTION:
		hat_update(event->jhat.which,
		                event->jhat.hat,
		                event->jhat.value);
		break;
	case SDL_JOYBUTTONDOWN:
	case SDL_JOYBUTTONUP:
		button_update(event->jbutton.which,
		                   event->jbutton.button,
		                   event->jbutton.state == SDL_PRESSED);
		break;
	case SDL_JOYAXISMOTION:
		axis_update(event->jaxis.which,
		                 event->jaxis.axis,
		                 event->jaxis.value);
		break;
	}
}

