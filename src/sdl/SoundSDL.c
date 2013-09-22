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

#include "SoundSDL.h"
#include "../common/Settings.h"
#include "../common/RingBuffer.h"

#include <SDL.h>

extern int emulating;
extern gboolean speedup;

typedef struct {
	struct ring_buffer *_rbuf;

	SDL_cond  * _cond;
	SDL_mutex * _mutex;

	gboolean _initialized;
} DriverData;

// Hold up to 100 ms of data in the ring buffer
static const float delay = 0.1f;

static void sound_sdl_read(SoundDriver *driver, guint8 *stream, int len) {
	g_assert(driver != NULL);
	DriverData *data = (DriverData *)driver->driverData;

	if (!data->_initialized || len <= 0 || !emulating)
		return;

	SDL_LockMutex(data->_mutex);

	ring_buffer_read(data->_rbuf, stream, len);

	SDL_CondSignal(data->_cond);
	SDL_UnlockMutex(data->_mutex);
}

static void sound_sdl_write(SoundDriver *driver, guint16 * finalWave, int length) {
	g_assert(driver != NULL);
	DriverData *data = (DriverData *)driver->driverData;

	if (!data->_initialized)
		return;

	if (SDL_GetAudioStatus() != SDL_AUDIO_PLAYING)
		SDL_PauseAudio(0);

	SDL_LockMutex(data->_mutex);

	unsigned int samples = length / 4;

	int avail;
	while ((avail = ring_buffer_avail(data->_rbuf) / 4) < samples)
	{
		ring_buffer_write(data->_rbuf, finalWave, avail * 4);

		finalWave += avail * 2;
		samples -= avail;

		// If emulating and not in speed up mode, synchronize to audio
		// by waiting till there is enough room in the buffer
		if (emulating && !speedup)
		{
			SDL_CondWait(data->_cond, data->_mutex);
		}
		else
		{
			// Drop the remaining of the audio data
			SDL_UnlockMutex(data->_mutex);
			return;
		}
	}

	ring_buffer_write(data->_rbuf, finalWave, samples * 4);

	SDL_UnlockMutex(data->_mutex);
}

static void sound_sdl_pause(SoundDriver *driver, gboolean pause) {
	g_assert(driver != NULL);
	DriverData *data = (DriverData *)driver->driverData;

	if (!data->_initialized)
		return;

	SDL_PauseAudio(pause);
}

static void sound_sdl_reset(SoundDriver *driver) {
	g_assert(driver != NULL);
	DriverData *data = (DriverData *)driver->driverData;

	ring_buffer_reset(data->_rbuf);
}

static void sound_sdl_callback(void *data, guint8 *stream, int len) {
	sound_sdl_read((SoundDriver *)data, stream, len);
}

SoundDriver *sound_sdl_init(GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	SoundDriver *driver = g_new(SoundDriver, 1);
	driver->write = sound_sdl_write;
	driver->pause = sound_sdl_pause;
	driver->reset = sound_sdl_reset;

	guint sampleRate = settings_sound_sample_rate();

	SDL_InitSubSystem(SDL_INIT_AUDIO);

	SDL_AudioSpec audio;
	audio.freq = sampleRate;
	audio.format = AUDIO_S16SYS;
	audio.channels = 2;
	audio.samples = 1024;
	audio.callback = sound_sdl_callback;
	audio.userdata = driver;

	if (SDL_OpenAudio(&audio, NULL)) {
		g_set_error(err, SOUND_ERROR, G_SOUND_ERROR_FAILED,
				"Failed to open audio: %s", SDL_GetError());
		g_free(driver);
		return NULL;
	}

	DriverData *data = g_new(DriverData, 1);
	data->_rbuf = ring_buffer_new(delay * sampleRate * 2 * sizeof(guint16));
	data->_cond = SDL_CreateCond();
	data->_mutex = SDL_CreateMutex();
	data->_initialized = TRUE;

	driver->driverData = data;

	return driver;
}

void sound_sdl_free(SoundDriver *driver) {
	if (driver == NULL)
		return;

	DriverData *data = (DriverData *)driver->driverData;

	if (!data->_initialized)
		return;

	SDL_mutexP(data->_mutex);

	int iSave = emulating;
	emulating = 0;
	SDL_CondSignal(data->_cond);
	SDL_UnlockMutex(data->_mutex);

	SDL_DestroyCond(data->_cond);
	SDL_DestroyMutex(data->_mutex);
	ring_buffer_free(data->_rbuf);

	SDL_CloseAudio();

	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	emulating = iSave;

	g_free(data);
	g_free(driver);
}
