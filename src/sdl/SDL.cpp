// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 1999-2003 Forgotten
// Copyright (C) 2005-2006 Forgotten and the VBA development team

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
// Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>

#include "../System.h"

#include <SDL.h>

#include "../gba/GBA.h"
#include "../gba/Cartridge.h"
#include "../gba/Display.h"
#include "../gba/Savestate.h"
#include "../gba/Sound.h"
#include "../common/Util.h"

#include "text.h"
#include "InputSDL.h"
#include "SoundSDL.h"
#include "../common/Settings.h"

#include <glib.h>

static SDL_Surface *surface = NULL;

int systemVerbose = 0;

static int srcWidth = 0;
static int srcHeight = 0;

int emulating = 0;

static bool paused = false;
static bool inactive = false;
static bool fullscreen = false;

static int  mouseCounter = 0;

static bool screenMessage = false;
static char screenMessageBuffer[21];
static u32  screenMessageTime = 0;

static void sdlScreenMessage(const char *msg)
{
  screenMessage = true;
  screenMessageTime = SDL_GetTicks();
  if(strlen(msg) > 20) {
    strncpy(screenMessageBuffer, msg, 20);
    screenMessageBuffer[20] = 0;
  } else
    strcpy(screenMessageBuffer, msg);

  g_message("%s", msg);
}

static void sdlChangeVolume(float d)
{
	float oldVolume = soundGetVolume();
	float newVolume = oldVolume + d;

	if (newVolume < 0.0) newVolume = 0.0;
	if (newVolume > SETTINGS_SOUND_MAX_VOLUME) newVolume = SETTINGS_SOUND_MAX_VOLUME;

	if (fabs(newVolume - oldVolume) > 0.001) {
		char tmp[32];
		sprintf(tmp, "Volume: %i%%", (int)(newVolume*100.0+0.5));
		sdlScreenMessage(tmp);
		soundSetVolume(newVolume);
	}
}

static void sdlWriteState(int num)
{
	gchar *message = NULL;
	GError *err = NULL;

	if (!savestate_save_slot(num, &err)) {
		message = strdup(err->message);
		g_clear_error(&err);
	} else {
		message = g_strdup_printf("Wrote state %d", num + 1);
	}

	sdlScreenMessage(message);
	g_free(message);
}

static void sdlReadState(int num) {
	gchar *message = NULL;
	GError *err = NULL;

	if (!savestate_load_slot(num, &err)) {
		message = strdup(err->message);
		g_clear_error(&err);
	} else {
		message = g_strdup_printf("Loaded state %d", num + 1);
	}

	sdlScreenMessage(message);
	g_free(message);
}

static void sdlWriteBattery() {
	gchar *message = NULL;
	GError *err = NULL;

	if (!cartridge_write_battery(&err)) {
		message = strdup(err->message);
		g_clear_error(&err);
	} else {
		message = g_strdup_printf("Wrote battery");
	}

	sdlScreenMessage(message);
	g_free(message);
}

static void sdlReadBattery() {
	// Ignore errors, we don't care loading battery failed
	gboolean res = cartridge_read_battery(NULL);

	if (res)
		sdlScreenMessage("Loaded battery");
}

//void sdlReadDesktopVideoMode() {
//  const SDL_VideoInfo* vInfo = SDL_GetVideoInfo();
//  desktopWidth = vInfo->current_w;
//  desktopHeight = vInfo->current_h;
//}

static void sdlInitVideo() {
  int flags;

  flags = SDL_ANYFORMAT | (fullscreen ? SDL_FULLSCREEN : 0);
  flags |= SDL_HWSURFACE | SDL_DOUBLEBUF;

  surface = SDL_SetVideoMode(srcWidth, srcHeight, 32, flags);

  if(surface == NULL) {
	  g_printerr("Failed to set video mode\n");
    SDL_Quit();
    exit(-1);
  }
}

#define MOD_KEYS    (KMOD_CTRL|KMOD_SHIFT|KMOD_ALT|KMOD_META)
#define MOD_NOCTRL  (KMOD_SHIFT|KMOD_ALT|KMOD_META)
#define MOD_NOALT   (KMOD_CTRL|KMOD_SHIFT|KMOD_META)
#define MOD_NOSHIFT (KMOD_CTRL|KMOD_ALT|KMOD_META)

static void sdlPollEvents()
{
  SDL_Event event;
  while(SDL_PollEvent(&event)) {
    switch(event.type) {
    case SDL_QUIT:
      emulating = 0;
      break;
    case SDL_ACTIVEEVENT:
      if(!paused && settings_pause_when_inactive() && (event.active.state & SDL_APPINPUTFOCUS)) {
        inactive = !event.active.gain;
        soundPause(inactive);
      }
      break;
    case SDL_MOUSEMOTION:
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEBUTTONDOWN:
      if(fullscreen) {
        SDL_ShowCursor(SDL_ENABLE);
        mouseCounter = 120;
      }
      break;
    case SDL_JOYHATMOTION:
    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP:
    case SDL_JOYAXISMOTION:
    case SDL_KEYDOWN:
      input_sdl_process_SDL_event(&event);
      break;
    case SDL_KEYUP:
      switch(event.key.keysym.sym) {
      case SDLK_r:
        if(!(event.key.keysym.mod & MOD_NOCTRL) &&
           (event.key.keysym.mod & KMOD_CTRL)) {
          if(emulating) {
            CPUReset();

            sdlScreenMessage("Reset");
          }
        }
        break;
	break;

      case SDLK_KP_DIVIDE:
        sdlChangeVolume(-0.1);
        break;
      case SDLK_KP_MULTIPLY:
        sdlChangeVolume(0.1);
        break;

      case SDLK_p:
        if(!(event.key.keysym.mod & MOD_NOCTRL) &&
           (event.key.keysym.mod & KMOD_CTRL)) {
          paused = !paused;
          soundPause(paused);
	  g_message(paused?"Pause on":"Pause off");
        }
        break;
      case SDLK_ESCAPE:
        emulating = 0;
        break;
      case SDLK_f:
        if(!(event.key.keysym.mod & MOD_NOCTRL) &&
           (event.key.keysym.mod & KMOD_CTRL)) {
          fullscreen = !fullscreen;
          sdlInitVideo();
        }
        break;
      case SDLK_F1:
      case SDLK_F2:
      case SDLK_F3:
      case SDLK_F4:
      case SDLK_F5:
      case SDLK_F6:
      case SDLK_F7:
      case SDLK_F8:
        if(!(event.key.keysym.mod & MOD_NOSHIFT) &&
           (event.key.keysym.mod & KMOD_SHIFT)) {
        	sdlWriteState(event.key.keysym.sym - SDLK_F1);
        } else if(!(event.key.keysym.mod & MOD_KEYS)) {
        	sdlReadState(event.key.keysym.sym - SDLK_F1);
	}
        break;
      case SDLK_1:
      case SDLK_2:
      case SDLK_3:
      case SDLK_4:
        if(!(event.key.keysym.mod & MOD_NOALT) &&
           (event.key.keysym.mod & KMOD_ALT)) {
          const char *disableMessages[4] =
            { "autofire A disabled",
              "autofire B disabled",
              "autofire R disabled",
              "autofire L disabled"};
          const char *enableMessages[4] =
            { "autofire A",
              "autofire B",
              "autofire R",
              "autofire L"};

	  EKey k = KEY_BUTTON_A;
	  if (event.key.keysym.sym == SDLK_1)
	    k = KEY_BUTTON_A;
	  else if (event.key.keysym.sym == SDLK_2)
	    k = KEY_BUTTON_B;
	  else if (event.key.keysym.sym == SDLK_3)
	    k = KEY_BUTTON_R;
	  else if (event.key.keysym.sym == SDLK_4)
	    k = KEY_BUTTON_L;

          if(input_sdl_toggle_autofire(k)) {
            sdlScreenMessage(enableMessages[event.key.keysym.sym - SDLK_1]);
          } else {
            sdlScreenMessage(disableMessages[event.key.keysym.sym - SDLK_1]);
          }
        }
        break;
      default:
        break;
      }
      input_sdl_process_SDL_event(&event);
      break;
    }
  }
}

static gboolean loadROM(const char *file, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

	if (!CPUInitMemory(err)) {
		return FALSE;
	}

	if (!cartridge_load_rom(file, err))	{
		CPUCleanUp();
		return FALSE;
	}

	if (!CPULoadBios(settings_get_bios(), err))	{
		CPUCleanUp();
		return FALSE;
	}

	CPUInit();
	CPUReset();

	return TRUE;
}

int main(int argc, char **argv)
{
	fprintf(stdout, "VBA version %s\n", VERSION);

	GError *err = NULL;
	gchar *filename = NULL;

	// Read config file
	settings_init();
	if (!settings_read_config_file(&err)) {
		g_printerr("%s\n", err->message);
		settings_free();

		g_clear_error(&err);
		exit(1);
	}

	// Parse command line
	filename = settings_parse_command_line(&argc, &argv, &err);
	if (filename == NULL) {
		settings_display_usage(err);
		settings_free();

		g_clear_error(&err);
		exit(1);
	}

	// Check the settings
	if (!settings_check(&err)) {
		g_printerr("%s\n", err->message);
		settings_free();

		g_clear_error(&err);
		exit(1);
	}

	// Make sure the batteries dir exists
	const gchar* batteriesDir = settings_get_battery_dir();
	g_mkdir_with_parents(batteriesDir, 0777);

	// Make sure the saves dir exists
	const gchar* savesDir = settings_get_save_dir();
	g_mkdir_with_parents(savesDir, 0777);

	display_init_color_map(19, 11, 3);

	// Init the sound driver
	SoundDriver *soundDriver = sound_sdl_init(&err);
	if (soundDriver == NULL) {
		g_printerr("%s\n", err->message);
		settings_free();

		g_clear_error(&err);
		exit(1);
	}
	soundSetVolume(settings_sound_volume());
	soundInit(soundDriver);

	// Init the input driver
	InputDriver *inputDriver = input_sdl_init(&err);
	if (inputDriver == NULL) {
		g_printerr("%s\n", err->message);
		settings_free();
		sound_sdl_free(soundDriver);

		g_clear_error(&err);
		exit(1);
	}
	gba_init_input(inputDriver);

    if(!loadROM(filename, &err)) {
		g_printerr("%s\n", err->message);
		settings_free();

		g_clear_error(&err);
		exit(1);
    }

  sdlReadBattery();

  int flags = SDL_INIT_VIDEO|SDL_INIT_AUDIO|
    SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE;

  if(SDL_Init(flags)) {
	  g_printerr("Failed to init SDL: %s\n", SDL_GetError());
    exit(-1);
  }

  srcWidth = 240;
  srcHeight = 160;

  fullscreen = settings_is_fullscreen();
  sdlInitVideo();

  emulating = 1;

  gchar *windowTitle = g_strdup_printf("%s - VBA", cartridge_get_game_title());
  SDL_WM_SetCaption(windowTitle, NULL);
  g_free(windowTitle);

  while(emulating) {
    if(!paused && !inactive) {
      CPULoop(250000);
    } else {
      SDL_Delay(500);
    }
    sdlPollEvents();
    if(mouseCounter) {
      mouseCounter--;
      if(mouseCounter == 0)
        SDL_ShowCursor(SDL_DISABLE);
    }
  }

  emulating = 0;
  fprintf(stdout,"Shutting down\n");
  soundShutdown();
  sound_sdl_free(soundDriver);
  input_sdl_free(inputDriver);

  sdlWriteBattery();
  cartridge_unload();
  CPUCleanUp();

  SDL_Quit();

  settings_free();

  g_free(filename);

  return 0;
}

static void drawScreenMessage(u8 *screen, int pitch, int x, int y, unsigned int duration)
{
  if(screenMessage) {
    if(((SDL_GetTicks() - screenMessageTime) < duration) &&
       !settings_disable_status_messages()) {
      drawText(screen, pitch, x, y,
               screenMessageBuffer, false);
    } else {
      screenMessage = false;
    }
  }
}

static void drawSpeed(u8 *screen, int pitch, int x, int y)
{
  char buffer[50];
  sprintf(buffer, "%d%%", gba_get_speed());

  drawText(screen, pitch, x, y, buffer, true);
}

void systemDrawScreen(u32 *pix)
{
  u8 *screen = (u8*)surface->pixels;

    SDL_LockSurface(surface);

    for (uint l = 0; l < srcHeight; l++) {
    	memcpy(screen, pix, srcWidth * 4);
    	pix += srcWidth;
        screen += surface->pitch;
    }

    screen = (u8*)surface->pixels;

  drawScreenMessage(screen, srcWidth * 4, 10, srcHeight - 20, 3000);

  if (settings_show_speed())
    drawSpeed(screen, srcWidth * 4, 10, 20);

  SDL_UnlockSurface(surface);
  SDL_Flip(surface);
}
