#ifndef SYSTEM_H
#define SYSTEM_H

#include "common/Types.h"

class SoundDriver;

struct EmulatedSystem {
  // main emulation function
  void (*emuMain)(int);
  // reset emulator
  void (*emuReset)();
  // clean up memory
  void (*emuCleanUp)();
  // load battery file
  bool (*emuReadBattery)(const char *);
  // write battery file
  bool (*emuWriteBattery)(const char *);
  // load state
  bool (*emuReadState)(const char *);
  // save state
  bool (*emuWriteState)(const char *);
  // clock ticks to emulate
  int emuCount;
};

extern void log(const char *,...);

extern void systemDrawScreen(u32 *pix);
// return information about the given joystick, -1 for default joystick
extern u32 systemReadJoypad();
extern u32 systemGetClock();
extern void systemMessage(const char *, ...);
extern SoundDriver * systemSoundInit();
extern void systemScreenMessage(const char *);
extern void systemUpdateMotionSensor();
extern int  systemGetSensorX();
extern int  systemGetSensorY();
extern void systemShowSpeed(int);
extern void system10Frames(int);
extern void systemFrame();

extern int systemVerbose;
extern int systemFrameSkip;

#endif // SYSTEM_H
