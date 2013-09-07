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
#include <cmath>

#include <time.h>

#include "../version.h"

#include <SDL.h>

#include "../gba/GBA.h"
#include "../gba/Cartridge.h"
#include "../gba/Display.h"
#include "../gba/Sound.h"
#include "../common/Util.h"

#include "text.h"
#include "InputSDL.h"
#include "../common/SoundSDL.h"

#include <glib.h>

SDL_Surface *surface = NULL;

static int systemSpeed = 0;
int systemVerbose = 0;

static int srcWidth = 0;
static int srcHeight = 0;

static int pauseWhenInactive = 0;
int emulating = 0;
static gchar **filename = NULL;
static gchar *biosFileName = NULL;
static gchar *configFileName = NULL;
static gchar *saveDir = NULL;
static gchar *batteryDir = NULL;

// Directory within confdir to use for default save location.
#define CONF_DIR "vbam"

#define SYSMSG_BUFFER_SIZE 1024

static int showSpeed = 1;
static bool disableStatusMessages = false;
static bool paused = false;
static int fullscreen = 0;

/* forward */
void systemConsoleMessage(const char*);

static int  mouseCounter = 0;

static bool screenMessage = false;
static char screenMessageBuffer[21];
static u32  screenMessageTime = 0;

#define SDL_SOUND_MAX_VOLUME 2.0

static GOptionEntry sdlOptions[] = {
  { "bios", 'b', 0, G_OPTION_ARG_FILENAME, &biosFileName, "Use given bios file", NULL },
  { "config", 'c', 0, G_OPTION_ARG_FILENAME, &configFileName, "Read the given configuration file", NULL },
  { "fullscreen", 0, 0, G_OPTION_ARG_NONE, &fullscreen, "Full screen", NULL },
  { "pause-when-inactive", 0, 0, G_OPTION_ARG_NONE, &pauseWhenInactive, "Pause when inactive", NULL },
  { "show-speed", 0, 0, G_OPTION_ARG_NONE, &showSpeed, "Show emulation speed", NULL },
  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &filename, NULL, "[GBA ROM file]" },
  { NULL }
};

static void sdlChangeVolume(float d)
{
	float oldVolume = soundGetVolume();
	float newVolume = oldVolume + d;

	if (newVolume < 0.0) newVolume = 0.0;
	if (newVolume > SDL_SOUND_MAX_VOLUME) newVolume = SDL_SOUND_MAX_VOLUME;

	if (fabs(newVolume - oldVolume) > 0.001) {
		char tmp[32];
		sprintf(tmp, "Volume: %i%%", (int)(newVolume*100.0+0.5));
		systemScreenMessage(tmp);
		soundSetVolume(newVolume);
	}
}

u32 sdlFromHex(char *s)
{
  u32 value;
  sscanf(s, "%x", &value);
  return value;
}

u32 sdlFromDec(char *s)
{
  u32 value = 0;
  sscanf(s, "%u", &value);
  return value;
}

static bool sdlCheckDirectory(char *dir)
{
  if (g_file_test(dir, G_FILE_TEST_EXISTS)) {
    if(!g_file_test(dir, G_FILE_TEST_IS_DIR)) {
      fprintf(stderr, "Error: %s is not a directory\n", dir);
      return false;
    }
  } else {
    fprintf(stderr, "Error: %s does not exist\n", dir);
    return false;
  }

  return true;
}

FILE *sdlFindFile(const char *name)
{
  const gchar *configDir = g_get_user_config_dir();
  gchar *currentDir = g_get_current_dir();
  gchar *filename = NULL;
  FILE *f = NULL;

  if (f == NULL&& currentDir != NULL) {
    filename = g_build_filename(currentDir, name, NULL);
    g_print("Searching current directory: %s\n", filename);
    f = fopen(filename, "r");
    g_free(filename);
  }

  if (f == NULL && configDir) {
    filename = g_build_filename(configDir, CONF_DIR, name, NULL);
    g_print("Searching user config directory: %s\n", filename);
    f = fopen(filename, "r");
    g_free(filename);
  }

  g_free(currentDir);
  return f;
}

void sdlReadPreferences(FILE *f)
{
  char buffer[2048];

  while(1) {
    char *s = fgets(buffer, 2048, f);

    if(s == NULL)
      break;

    char *p  = strchr(s, '#');

    if(p)
      *p = 0;

    char *token = strtok(s, " \t\n\r=");

    if(!token)
      continue;

    if(strlen(token) == 0)
      continue;

    char *key = token;
    char *value = strtok(NULL, "\t\n\r");

    if(value == NULL) {
      fprintf(stdout, "Empty value for key %s\n", key);
      continue;
    }

    if(!strcmp(key,"Joy0_Left")) {
      input_set_keymap(KEY_LEFT, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_Right")) {
      input_set_keymap(KEY_RIGHT, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_Up")) {
      input_set_keymap(KEY_UP, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_Down")) {
      input_set_keymap(KEY_DOWN, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_A")) {
      input_set_keymap(KEY_BUTTON_A, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_B")) {
      input_set_keymap(KEY_BUTTON_B, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_L")) {
      input_set_keymap(KEY_BUTTON_L, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_R")) {
      input_set_keymap(KEY_BUTTON_R, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_Start")) {
      input_set_keymap(KEY_BUTTON_START, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_Select")) {
      input_set_keymap(KEY_BUTTON_SELECT, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_Speed")) {
      input_set_keymap(KEY_BUTTON_SPEED, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_Capture")) {
      input_set_keymap(KEY_BUTTON_CAPTURE, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_AutoA")) {
      input_set_keymap(KEY_BUTTON_AUTO_A, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_AutoB")) {
      input_set_keymap(KEY_BUTTON_AUTO_B, sdlFromHex(value));
    } else if(!strcmp(key, "Motion_Left")) {
      input_set_motion_keymap(KEY_LEFT, sdlFromHex(value));
    } else if(!strcmp(key, "Motion_Right")) {
      input_set_motion_keymap(KEY_RIGHT, sdlFromHex(value));
    } else if(!strcmp(key, "Motion_Up")) {
      input_set_motion_keymap(KEY_UP, sdlFromHex(value));
    } else if(!strcmp(key, "Motion_Down")) {
      input_set_motion_keymap(KEY_DOWN, sdlFromHex(value));
    } else if(!strcmp(key, "fullScreen")) {
      fullscreen = sdlFromHex(value) ? 1 : 0;
    } else if(!strcmp(key, "biosFile")) {
      g_free(biosFileName);
      biosFileName = g_strdup(value);
    } else if(!strcmp(key, "disableStatus")) {
      disableStatusMessages = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "saveDir")) {
      if (sdlCheckDirectory(value)) {
        g_free(saveDir);
        saveDir = g_strdup(value);
      }
    } else if(!strcmp(key, "batteryDir")) {
      if (sdlCheckDirectory(value)) {
        g_free(batteryDir);
        batteryDir = g_strdup(value);
      }
    } else if(!strcmp(key, "soundQuality")) {
      int soundQuality = sdlFromHex(value);
      switch(soundQuality) {
      case 1:
      case 2:
      case 4:
        break;
      default:
        fprintf(stdout, "Unknown sound quality %d. Defaulting to 22Khz\n",
                soundQuality);
        soundQuality = 2;
        break;
      }
      soundSetSampleRate(44100 / soundQuality);
    } else if(!strcmp(key, "soundVolume")) {
      float volume = sdlFromDec(value) / 100.0;
      if (volume < 0.0 || volume > SDL_SOUND_MAX_VOLUME)
        volume = 1.0;
      soundSetVolume(volume);
    } else if(!strcmp(key, "showSpeed")) {
      showSpeed = sdlFromHex(value);
      if(showSpeed < 0 || showSpeed > 2)
        showSpeed = 1;
    } else if(!strcmp(key, "pauseWhenInactive")) {
      pauseWhenInactive = sdlFromHex(value) ? true : false;
    } else {
      fprintf(stderr, "Unknown configuration key %s\n", key);
    }
  }
}

void sdlReadPreferences()
{
  g_print("Searching for configuration file\n");
  FILE *f = sdlFindFile("vbam.cfg");

  if(f == NULL) {
    fprintf(stdout, "Configuration file NOT FOUND (using defaults)\n");
    return;
  } else
    fprintf(stdout, "Reading configuration file.\n");

  sdlReadPreferences(f);

  fclose(f);
}

static gchar *conf_get_battery_dir() {
	gchar *dir = NULL;
	if (batteryDir) {
		dir = g_build_filename(batteryDir, NULL);
	} else {
		const gchar *userDataDir = g_get_user_data_dir();
		dir = g_build_filename(userDataDir, CONF_DIR, NULL);
	}

	return dir;
}

static gchar *conf_get_save_dir() {
	gchar *dir = NULL;
	if (saveDir) {
		dir = g_build_filename(saveDir, NULL);
	} else {
		const gchar *userDataDir = g_get_user_data_dir();
		dir = g_build_filename(userDataDir, CONF_DIR, NULL);
	}

	return dir;
}

static gchar *sdlStateName(int num)
{
  gchar *stateNum = g_strdup_printf("%d", num + 1);
  gchar *baseName = g_path_get_basename(filename[0]);
  gchar *saveDir = conf_get_save_dir();
  gchar *fileName = g_strconcat(baseName, stateNum, ".sgm", NULL);
  gchar *stateName = g_build_filename(batteryDir, fileName, NULL);

  g_free(saveDir);
  g_free(fileName);
  g_free(baseName);
  g_free(stateNum);

  return stateName;
}

static gchar *sdlBatteryName()
{
  gchar *batteryDir = conf_get_battery_dir();
  gchar *baseName = g_path_get_basename(filename[0]);
  gchar *fileName = g_strconcat(baseName, ".sav", NULL);
  gchar *batteryFile = g_build_filename(batteryDir, fileName, NULL);

  g_free(batteryDir);
  g_free(fileName);
  g_free(baseName);

  return batteryFile;
}

void sdlWriteState(int num)
{
  gchar *stateName = sdlStateName(num);
  CPUWriteState(stateName);
  g_free(stateName);

  gchar *message = g_strdup_printf("Wrote state %d", num + 1);
  systemScreenMessage(message);
  g_free(message);
}

void sdlReadState(int num)
{
  gchar *stateName = sdlStateName(num);
  CPUReadState(stateName);
  g_free(stateName);

  gchar *message = g_strdup_printf("Loaded state %d", num + 1);
  systemScreenMessage(message);
  g_free(message);
}

void sdlWriteBattery()
{
  gchar *batteryFile = sdlBatteryName();

  Cartridge::writeBatteryToFile(batteryFile);

  systemScreenMessage("Wrote battery");

  g_free(batteryFile);
}

void sdlReadBattery()
{
  gchar *batteryFile = sdlBatteryName();

  bool res = false;

  res = Cartridge::readBatteryFromFile(batteryFile);

  if(res)
    systemScreenMessage("Loaded battery");

  g_free(batteryFile);
}

//void sdlReadDesktopVideoMode() {
//  const SDL_VideoInfo* vInfo = SDL_GetVideoInfo();
//  desktopWidth = vInfo->current_w;
//  desktopHeight = vInfo->current_h;
//}

void sdlInitVideo() {
  int flags;

  flags = SDL_ANYFORMAT | (fullscreen ? SDL_FULLSCREEN : 0);
  flags |= SDL_HWSURFACE | SDL_DOUBLEBUF;

  surface = SDL_SetVideoMode(srcWidth, srcHeight, 32, flags);

  if(surface == NULL) {
    systemMessage("Failed to set video mode");
    SDL_Quit();
    exit(-1);
  }
}

#define MOD_KEYS    (KMOD_CTRL|KMOD_SHIFT|KMOD_ALT|KMOD_META)
#define MOD_NOCTRL  (KMOD_SHIFT|KMOD_ALT|KMOD_META)
#define MOD_NOALT   (KMOD_CTRL|KMOD_SHIFT|KMOD_META)
#define MOD_NOSHIFT (KMOD_CTRL|KMOD_ALT|KMOD_META)



/*
 * handle the F* keys (for savestates)
 * given the slot number and state of the SHIFT modifier, save or restore
 * (in savemode 3, saveslot is stored in saveSlotPosition and num means:
 *  4 .. F5: decrease slot number (down to 0)
 *  5 .. F6: increase slot number (up to 7, because 8 and 9 are reserved for backups)
 *  6 .. F7: save state
 *  7 .. F8: load state
 *  (these *should* be configurable)
 *  other keys are ignored
 * )
 */
static void sdlHandleSavestateKey(int num, int shifted)
{
	int action	= -1;
	// 0: load
	// 1: save

		if (shifted)
			action	= 1; // save
		else	action	= 0; // load

	if (action < 0 || action > 1)
	{
		fprintf(
				stderr,
				"sdlHandleSavestateKey(%d,%d): unexpected action %d.\n",
				num,
				shifted,
				action
		);
	}

	if (action)
	{        /* save */
		sdlWriteState(num);
	} else { /* load */
		sdlReadState(num);
    }

} // sdlHandleSavestateKey

void sdlPollEvents()
{
  SDL_Event event;
  while(SDL_PollEvent(&event)) {
    switch(event.type) {
    case SDL_QUIT:
      emulating = 0;
      break;
    case SDL_ACTIVEEVENT:
      if(!paused && pauseWhenInactive && (event.active.state & SDL_APPINPUTFOCUS)) {
    	  paused = event.active.gain;
          if(!paused) {
              soundResume();
          } else if(paused) {
              soundPause();
          }
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
      input_process_SDL_event(&event);
      break;
    case SDL_KEYUP:
      switch(event.key.keysym.sym) {
      case SDLK_r:
        if(!(event.key.keysym.mod & MOD_NOCTRL) &&
           (event.key.keysym.mod & KMOD_CTRL)) {
          if(emulating) {
            CPUReset();

            systemScreenMessage("Reset");
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
          SDL_PauseAudio(paused);
	  systemConsoleMessage(paused?"Pause on":"Pause off");
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
		sdlHandleSavestateKey( event.key.keysym.sym - SDLK_F1, 1); // with SHIFT
        } else if(!(event.key.keysym.mod & MOD_KEYS)) {
		sdlHandleSavestateKey( event.key.keysym.sym - SDLK_F1, 0); // without SHIFT
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

          if(input_toggle_autofire(k)) {
            systemScreenMessage(enableMessages[event.key.keysym.sym - SDLK_1]);
          } else {
            systemScreenMessage(disableMessages[event.key.keysym.sym - SDLK_1]);
          }
        }
        break;
      default:
        break;
      }
      input_process_SDL_event(&event);
      break;
    }
  }
}

static bool loadROM(const char *file)
{
	if (!CPUInitMemory())
		return false;

	if (!Cartridge::loadRom(file))
	{
		CPUCleanUp();
		return false;
	}

	if (biosFileName == NULL)
	{
		return false;
	}

	if (!CPULoadBios(biosFileName))
	{
		return false;
	}

	CPUInit();
	CPUReset();

	return true;
}

static void usage(GOptionContext *context) {
    gchar *usage = g_option_context_get_help(context, FALSE, NULL);
    g_print("%s", usage);
    g_free(usage);
}

int main(int argc, char **argv)
{
  fprintf(stdout, "VBA-M version %s [SDL]\n", VERSION);

  // Parse command line
  GError *error = NULL;
  GOptionContext *context;

  context = g_option_context_new(NULL);
  g_option_context_add_main_entries (context, sdlOptions, NULL);

  if (!g_option_context_parse (context, &argc, &argv, &error)) {
      g_print("Command line parsing failed: %s.\n", error->message);

      usage(context);

      g_option_context_free(context);
      exit (1);
  }
  if (filename == NULL || g_strv_length(filename) != 1) {
      g_print("You must specify exactly one GBA ROM file.\n");

      usage(context);

      g_option_context_free(context);
      exit (1);
  }
  g_option_context_free(context);


  sdlReadPreferences();

  // Make sure the batteries dir exists
  gchar* batteriesDir = conf_get_battery_dir();
  g_mkdir_with_parents(batteriesDir, 0777);
  g_free(batteriesDir);

  // Make sure the saves dir exists
  gchar* savesDir = conf_get_save_dir();
  g_mkdir_with_parents(savesDir, 0777);
  g_free(savesDir);

  Display::initColorMap(19, 11, 3);

    u32 len = strlen(filename[0]);
    if (len > SYSMSG_BUFFER_SIZE)
    {
      fprintf(stderr,"%s :%s: File name too long\n",argv[0],filename[0]);
      exit(-1);
    }

    soundInit();

    bool failed = !loadROM(filename[0]);

    if(failed) {
      systemMessage("Failed to load file %s", filename[0]);
      exit(-1);
    }

  sdlReadBattery();

  int flags = SDL_INIT_VIDEO|SDL_INIT_AUDIO|
    SDL_INIT_TIMER|SDL_INIT_NOPARACHUTE;

  if(SDL_Init(flags)) {
    systemMessage("Failed to init SDL: %s", SDL_GetError());
    exit(-1);
  }

  if(SDL_InitSubSystem(SDL_INIT_JOYSTICK)) {
    systemMessage("Failed to init joystick support: %s", SDL_GetError());
  }

  input_init_joysticks();

  srcWidth = 240;
  srcHeight = 160;

  sdlInitVideo();

  emulating = 1;

  SDL_WM_SetCaption("VBA-M", NULL);

  while(emulating) {
    if(!paused) {
      CPULoop();
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

  sdlWriteBattery();
  Cartridge::unloadGame();
  CPUCleanUp();

  SDL_Quit();

  g_strfreev(filename);
  g_free(biosFileName);
  g_free(configFileName);
  g_free(saveDir);
  g_free(batteryDir);

  return 0;
}

void drawScreenMessage(u8 *screen, int pitch, int x, int y, unsigned int duration)
{
  if(screenMessage) {
    if(((systemGetClock() - screenMessageTime) < duration) &&
       !disableStatusMessages) {
      drawText(screen, pitch, x, y,
               screenMessageBuffer, false);
    } else {
      screenMessage = false;
    }
  }
}

void drawSpeed(u8 *screen, int pitch, int x, int y)
{
  char buffer[50];
  sprintf(buffer, "%d%%", systemSpeed);

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

  if (showSpeed && fullscreen)
    drawSpeed(screen, srcWidth * 4, 10, 20);

  SDL_UnlockSurface(surface);
  SDL_Flip(surface);
}

void systemSetTitle(const char *title)
{
  SDL_WM_SetCaption(title, NULL);
}

void systemShowSpeed(int speed)
{
  systemSpeed = speed;

  if(!fullscreen && showSpeed) {
    char buffer[80];
    sprintf(buffer, "VBA-M - %d%%", systemSpeed);

    systemSetTitle(buffer);
  }
}

u32 systemGetClock()
{
  return SDL_GetTicks();
}

/* xKiv: added timestamp */
void systemConsoleMessage(const char *msg)
{
  time_t now_time;
  struct tm now_time_broken;

  now_time		= time(NULL);
  now_time_broken	= *(localtime( &now_time ));
  fprintf(
		stdout,
		"%02d:%02d:%02d %02d.%02d.%4d: %s\n",
		now_time_broken.tm_hour,
		now_time_broken.tm_min,
		now_time_broken.tm_sec,
		now_time_broken.tm_mday,
		now_time_broken.tm_mon + 1,
		now_time_broken.tm_year + 1900,
		msg
  );
}

void systemScreenMessage(const char *msg)
{

  screenMessage = true;
  screenMessageTime = systemGetClock();
  if(strlen(msg) > 20) {
    strncpy(screenMessageBuffer, msg, 20);
    screenMessageBuffer[20] = 0;
  } else
    strcpy(screenMessageBuffer, msg);

  systemConsoleMessage(msg);
}

u32 systemReadJoypad()
{
  return input_read_joypad();
}

void systemUpdateMotionSensor()
{
  input_update_motion_sensor();
}

int systemGetSensorX()
{
  return input_get_sensor_x();
}

int systemGetSensorY()
{
  return input_get_sensor_y();
}

SoundDriver * systemSoundInit()
{
	soundShutdown();

	return new SoundSDL();
}

void systemMessage(const char *msg, ...)
{
  char buffer[SYSMSG_BUFFER_SIZE*2];
  va_list valist;

  va_start(valist, msg);
  vsprintf(buffer, msg, valist);

  fprintf(stderr, "%s\n", buffer);
  va_end(valist);
}

void log(const char *defaultMsg, ...)
{
  static FILE *out = NULL;

  if(out == NULL) {
    out = fopen("trace.log","w");
  }

  va_list valist;

  va_start(valist, defaultMsg);
  vfprintf(out, defaultMsg, valist);
  va_end(valist);
}
