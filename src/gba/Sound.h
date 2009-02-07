#ifndef SOUND_H
#define SOUND_H

// Sound emulation setup/options and GBA sound emulation

#include "../Util.h"

//// Setup/options (these affect GBA and GB sound)

// Initializes sound and returns true if successful. Sets sound quality to
// current value in soundQuality global.
bool soundInit();

// sets the Sound throttle
void soundSetThrottle(unsigned short throttle);

// Manages sound volume, where 1.0 is normal
void soundSetVolume( float );
float soundGetVolume();

// Pauses/resumes system sound output
void soundPause();
void soundResume();

// Cleans up sound. Afterwards, soundInit() can be called again.
void soundShutdown();

//// GBA sound options

long soundGetSampleRate();
void soundSetSampleRate(long sampleRate);

//// GBA sound emulation

// GBA sound registers
#define SGCNT0_H 0x82
#define FIFOA_L 0xa0
#define FIFOA_H 0xa2
#define FIFOB_L 0xa4
#define FIFOB_H 0xa6

// Resets emulated sound hardware
void soundReset();

// Emulates write to sound hardware
void soundEvent( u32 addr, u8  data );
void soundEvent( u32 addr, u16 data ); // TODO: error-prone to overload like this

// Notifies emulator that a timer has overflowed
void soundTimerOverflow( int which );

// Notifies emulator that PCM rate may have changed
void interp_rate();

// Notifies emulator that SOUND_CLOCK_TICKS clocks have passed
void psoundTickfn();
extern int SOUND_CLOCK_TICKS;   // Number of 16.8 MHz clocks between calls to soundTick()
extern int soundTicks;          // Number of 16.8 MHz clocks until soundTick() will be called

// Saves/loads emulator state
void soundSaveGame( gzFile );
void soundReadGame( gzFile, int version );

class Multi_Buffer;

void flush_samples(Multi_Buffer * buffer);

#endif // SOUND_H
