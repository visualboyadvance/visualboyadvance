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

#ifndef _WIN32
# include <unistd.h>
# define GETCWD getcwd
#else // _WIN32
# include <direct.h>
# define GETCWD _getcwd
#endif // _WIN32

#ifndef __GNUC__
# define HAVE_DECL_GETOPT 0
# define __STDC__ 1
# include "getopt.h"
#else // ! __GNUC__
# define HAVE_DECL_GETOPT 1
# include <getopt.h>
#endif // ! __GNUC__

SDL_Surface *surface = NULL;

static int systemSpeed = 0;
int systemVerbose = 0;

static int srcWidth = 0;
static int srcHeight = 0;

static int sdlPrintUsage = 0;

static int pauseWhenInactive = 0;
int emulating = 0;
static char filename[2048];
static char biosFileName[2048];
static char saveDir[2048];
static char batteryDir[2048];
static char* homeDir = NULL;

// Directory within homedir to use for default save location.
#define DOT_DIR ".vbam"

#define SYSMSG_BUFFER_SIZE 1024

#define _stricmp strcasecmp

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

static char *arg0;

#define SDL_SOUND_MAX_VOLUME 2.0

struct option sdlOptions[] = {
  { "bios", required_argument, 0, 'b' },
  { "config", required_argument, 0, 'c' },
  { "fullscreen", no_argument, &fullscreen, 1 },
  { "help", no_argument, &sdlPrintUsage, 1 },
  { "no-pause-when-inactive", no_argument, &pauseWhenInactive, 0 },
  { "no-show-speed", no_argument, &showSpeed, 0 },
  { "pause-when-inactive", no_argument, &pauseWhenInactive, 1 },
  { "show-speed-normal", no_argument, &showSpeed, 1 },
  { NULL, no_argument, NULL, 0 }
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

#ifdef __MSC__
#define stat _stat
#define S_IFDIR _S_IFDIR
#endif

void sdlCheckDirectory(char *dir)
{
  struct stat buf;

  int len = strlen(dir);

  char *p = dir + len - 1;

  if(*p == '/' ||
     *p == '\\')
    *p = 0;

  if(stat(dir, &buf) == 0) {
    if(!(buf.st_mode & S_IFDIR)) {
      fprintf(stderr, "Error: %s is not a directory\n", dir);
      dir[0] = 0;
    }
  } else {
    fprintf(stderr, "Error: %s does not exist\n", dir);
    dir[0] = 0;
  }
}

char *sdlGetFilename(char *name)
{
  static char filebuffer[2048];

  int len = strlen(name);

  char *p = name + len - 1;

  while(true) {
    if(*p == '/' ||
       *p == '\\') {
      p++;
      break;
    }
    len--;
    p--;
    if(len == 0)
      break;
  }

  if(len == 0)
    strcpy(filebuffer, name);
  else
    strcpy(filebuffer, p);
  return filebuffer;
}

FILE *sdlFindFile(const char *name)
{
  char buffer[4096];
  char path[2048];

#ifdef _WIN32
#define PATH_SEP ";"
#define FILE_SEP '\\'
#define EXE_NAME "vbam.exe"
#else // ! _WIN32
#define PATH_SEP ":"
#define FILE_SEP '/'
#define EXE_NAME "vbam"
#endif // ! _WIN32

  fprintf(stdout, "Searching for file %s\n", name);

  if(GETCWD(buffer, 2048)) {
    fprintf(stdout, "Searching current directory: %s\n", buffer);
  }

  FILE *f = fopen(name, "r");
  if(f != NULL) {
    return f;
  }

  if(homeDir) {
    fprintf(stdout, "Searching home directory: %s%c%s\n", homeDir, FILE_SEP, DOT_DIR);
    sprintf(path, "%s%c%s%c%s", homeDir, FILE_SEP, DOT_DIR, FILE_SEP, name);
    f = fopen(path, "r");
    if(f != NULL)
      return f;
  }

  fprintf(stdout, "Searching data directory: %s\n", PKGDATADIR);
  sprintf(path, "%s%c%s", PKGDATADIR, FILE_SEP, name);
  f = fopen(path, "r");
  if(f != NULL)
    return f;

//  fprintf(stdout, "Searching system config directory: %s\n", SYSCONFDIR);
//  sprintf(path, "%s%c%s", SYSCONFDIR, FILE_SEP, name);
//  f = fopen(path, "r");
//  if(f != NULL)
//    return f;

  return NULL;
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
      inputSetKeymap(KEY_LEFT, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_Right")) {
      inputSetKeymap(KEY_RIGHT, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_Up")) {
      inputSetKeymap(KEY_UP, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_Down")) {
      inputSetKeymap(KEY_DOWN, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_A")) {
      inputSetKeymap(KEY_BUTTON_A, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_B")) {
      inputSetKeymap(KEY_BUTTON_B, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_L")) {
      inputSetKeymap(KEY_BUTTON_L, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_R")) {
      inputSetKeymap(KEY_BUTTON_R, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_Start")) {
      inputSetKeymap(KEY_BUTTON_START, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_Select")) {
      inputSetKeymap(KEY_BUTTON_SELECT, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_Speed")) {
      inputSetKeymap(KEY_BUTTON_SPEED, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_Capture")) {
      inputSetKeymap(KEY_BUTTON_CAPTURE, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_AutoA")) {
      inputSetKeymap(KEY_BUTTON_AUTO_A, sdlFromHex(value));
    } else if(!strcmp(key, "Joy0_AutoB")) {
      inputSetKeymap(KEY_BUTTON_AUTO_B, sdlFromHex(value));
    } else if(!strcmp(key, "Motion_Left")) {
      inputSetMotionKeymap(KEY_LEFT, sdlFromHex(value));
    } else if(!strcmp(key, "Motion_Right")) {
      inputSetMotionKeymap(KEY_RIGHT, sdlFromHex(value));
    } else if(!strcmp(key, "Motion_Up")) {
      inputSetMotionKeymap(KEY_UP, sdlFromHex(value));
    } else if(!strcmp(key, "Motion_Down")) {
      inputSetMotionKeymap(KEY_DOWN, sdlFromHex(value));
    } else if(!strcmp(key, "fullScreen")) {
      fullscreen = sdlFromHex(value) ? 1 : 0;
    } else if(!strcmp(key, "biosFile")) {
      strcpy(biosFileName, value);
    } else if(!strcmp(key, "disableStatus")) {
      disableStatusMessages = sdlFromHex(value) ? true : false;
    } else if(!strcmp(key, "saveDir")) {
      sdlCheckDirectory(value);
      strcpy(saveDir, value);
    } else if(!strcmp(key, "batteryDir")) {
      sdlCheckDirectory(value);
      strcpy(batteryDir, value);
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
  FILE *f = sdlFindFile("vbam.cfg");

  if(f == NULL) {
    fprintf(stdout, "Configuration file NOT FOUND (using defaults)\n");
    return;
  } else
    fprintf(stdout, "Reading configuration file.\n");

  sdlReadPreferences(f);

  fclose(f);
}

/* returns filename of savestate num, in static buffer (not reentrant, no need to free,
 * but value won't survive much - so if you want to remember it, dup it)
 * You may use the buffer for something else though - until you call sdlStateName again
 */
static char * sdlStateName(int num)
{
  static char stateName[2048];

  if(saveDir[0])
    sprintf(stateName, "%s/%s%d.sgm", saveDir, sdlGetFilename(filename),
            num+1);
  else if (homeDir)
    sprintf(stateName, "%s/%s/%s%d.sgm", homeDir, DOT_DIR, sdlGetFilename(filename), num + 1);
  else
    sprintf(stateName,"%s%d.sgm", filename, num+1);

  return stateName;
}

void sdlWriteState(int num)
{
  char * stateName;

  stateName = sdlStateName(num);

  CPUWriteState(stateName);

	sprintf(stateName, "Wrote state %d", num+1);
	systemScreenMessage(stateName);
}

void sdlReadState(int num)
{
  char * stateName;

  stateName = sdlStateName(num);
  CPUReadState(stateName);

	  sprintf(stateName, "Loaded state %d", num+1);
  systemScreenMessage(stateName);
}

void sdlWriteBattery()
{
  char buffer[1048];

  if(batteryDir[0])
    sprintf(buffer, "%s/%s.sav", batteryDir, sdlGetFilename(filename));
  else if (homeDir)
    sprintf(buffer, "%s/%s/%s.sav", homeDir, DOT_DIR, sdlGetFilename(filename));
  else
    sprintf(buffer, "%s.sav", filename);

  Cartridge::writeBatteryToFile(buffer);

  systemScreenMessage("Wrote battery");
}

void sdlReadBattery()
{
  char buffer[1048];

  if(batteryDir[0])
    sprintf(buffer, "%s/%s.sav", batteryDir, sdlGetFilename(filename));
  else if (homeDir)
    sprintf(buffer, "%s/%s/%s.sav", homeDir, DOT_DIR, sdlGetFilename(filename));
  else
    sprintf(buffer, "%s.sav", filename);

  bool res = false;

  res = Cartridge::readBatteryFromFile(buffer);

  if(res)
    systemScreenMessage("Loaded battery");
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
      inputProcessSDLEvent(event);
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

          if(inputToggleAutoFire(k)) {
            systemScreenMessage(enableMessages[event.key.keysym.sym - SDLK_1]);
          } else {
            systemScreenMessage(disableMessages[event.key.keysym.sym - SDLK_1]);
          }
        }
        break;
      default:
        break;
      }
      inputProcessSDLEvent(event);
      break;
    }
  }
}

void usage(char *cmd)
{
  printf("%s [option ...] file\n", cmd);
  printf("\
\n\
Options:\n\
  -F, --fullscreen             Full screen\n\
  -b, --bios=BIOS              Use given bios file\n\
  -c, --config=FILE            Read the given configuration file\n\
  -h, --help                   Print this help\n\
\n\
Long options only:\n\
      --no-pause-when-inactive Don't pause when inactive\n\
      --no-show-speed          Don't show emulation speed\n\
      --pause-when-inactive    Pause when inactive\n\
      --show-speed-normal      Show emulation speed\n\
");
}

bool loadROM(const char *file)
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

int main(int argc, char **argv)
{
  fprintf(stdout, "VBA-M version %s [SDL]\n", VERSION);

  arg0 = argv[0];

  saveDir[0] = 0;
  batteryDir[0] = 0;

  int op = -1;

  char buf[1024];
  struct stat s;

#ifndef _WIN32
  // Get home dir
  homeDir = getenv("HOME");
  snprintf(buf, 1024, "%s/%s", homeDir, DOT_DIR);
  // Make dot dir if not existent
  if (stat(buf, &s) == -1 || !S_ISDIR(s.st_mode))
    mkdir(buf, 0755);
#else
  homeDir = 0;
#endif

  sdlReadPreferences();

  Display::initColorMap(19, 11, 3);

  sdlPrintUsage = 0;

  while((op = getopt_long(argc,
                          argv,
                           "FNO:T:Y:G:I:D:b:c:df:hi:p::s:t:v:",
                          sdlOptions,
                          NULL)) != -1) {
    switch(op) {
    case 0:
      // long option already processed by getopt_long
      break;
    case 'b':
      if(optarg == NULL) {
        fprintf(stderr, "Missing BIOS file name\n");
        exit(-1);
      }
      strcpy(biosFileName, optarg);
      break;
    case 'c':
      {
        if(optarg == NULL) {
          fprintf(stderr, "Missing config file name\n");
          exit(-1);
        }
        FILE *f = fopen(optarg, "r");
        if(f == NULL) {
          fprintf(stderr, "File not found %s\n", optarg);
          exit(-1);
        }
        sdlReadPreferences(f);
        fclose(f);
      }
      break;
    case 'h':
      sdlPrintUsage = 1;
      break;
    case 'F':
      fullscreen = 1;
      mouseCounter = 120;
      break;
    case '?':
      sdlPrintUsage = 1;
      break;
    break;

    }
  }

  if(sdlPrintUsage) {
    usage(argv[0]);
    exit(-1);
  }

  if(optind < argc) {
    char *szFile = argv[optind];
    u32 len = strlen(szFile);
    if (len > SYSMSG_BUFFER_SIZE)
    {
      fprintf(stderr,"%s :%s: File name too long\n",argv[0],szFile);
      exit(-1);
    }

    soundInit();

    bool failed = !loadROM(szFile);

    if(failed) {
      systemMessage("Failed to load file %s", szFile);
      exit(-1);
    }
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

  inputInitJoysticks();

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
  return inputReadJoypad();
}

void systemUpdateMotionSensor()
{
  inputUpdateMotionSensor();
}

int systemGetSensorX()
{
  return inputGetSensorX();
}

int systemGetSensorY()
{
  return inputGetSensorY();
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
