// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 1999-2003 Forgotten
// Copyright (C) 2004 Forgotten and the VBA development team

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

#include "inputSDL.h"
#include "../gba/Sound.h"
#include "../common/SoundSDL.h"

#include "window.h"
#include "intl.h"

// Required vars, used by the emulator core
//
int  systemVerbose;
int  systemFrameSkip;
u32  systemColorMap32[0x10000];

int  emulating;

inline VBA::Window * GUI()
{
  return VBA::Window::poGetInstance();
}

void systemMessage(int _iId, const char * _csFormat, ...)
{
  va_list args;
  va_start(args, _csFormat);

  GUI()->vPopupErrorV(_(_csFormat), args);

  va_end(args);
}

void systemDrawScreen()
{
  GUI()->vDrawScreen();
}

bool systemReadJoypads()
{
  return true;
}

u32 systemReadJoypad(int joy)
{
  return inputReadJoypad(joy);
}

void systemShowSpeed(int _iSpeed)
{
  GUI()->vShowSpeed(_iSpeed);
}

void system10Frames(int _iRate)
{
  GUI()->vComputeFrameskip(_iRate);
}

void systemFrame()
{
}

u32 systemGetClock()
{
    Glib::TimeVal time;
    time.assign_current_time();
    return time.as_double() * 1000;
}

void systemUpdateMotionSensor()
{
}

int systemGetSensorX()
{
  return 0;
}

int systemGetSensorY()
{
  return 0;
}

void systemScreenMessage(const char * _csMsg)
{
}

bool systemCanChangeSoundQuality()
{
  return true;
}

bool systemPauseOnFrame()
{
  return false;
}

SoundDriver * systemSoundInit()
{
	soundShutdown();

	return new SoundSDL();
}

void log(const char *defaultMsg, ...)
{
  va_list valist;

  va_start(valist, defaultMsg);
  fprintf(stderr, defaultMsg, valist);

  va_end(valist);
}
