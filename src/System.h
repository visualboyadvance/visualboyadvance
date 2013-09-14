#ifndef SYSTEM_H
#define SYSTEM_H

#include "common/Types.h"

class SoundDriver;

extern void log(const char *,...);

extern void systemDrawScreen(u32 *pix);
// return information about the given joystick, -1 for default joystick
extern u32 systemReadJoypad();
extern u32 systemGetClock();
extern SoundDriver * systemSoundInit();
extern void systemUpdateMotionSensor();
extern int  systemGetSensorX();
extern int  systemGetSensorY();
extern void systemShowSpeed(int);

#define VERBOSE_SWI                  1
#define VERBOSE_UNALIGNED_MEMORY     2
#define VERBOSE_ILLEGAL_WRITE        4
#define VERBOSE_ILLEGAL_READ         8
#define VERBOSE_DMA0                16
#define VERBOSE_DMA1                32
#define VERBOSE_DMA2                64
#define VERBOSE_DMA3               128
#define VERBOSE_UNDEFINED          256
#define VERBOSE_AGBPRINT           512
#define VERBOSE_SOUNDOUTPUT       1024

extern int systemVerbose;

#endif // SYSTEM_H
