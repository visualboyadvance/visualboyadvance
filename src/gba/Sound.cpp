#include <string.h>

#include "Sound.h"

#include "GBA.h"
#include "Globals.h"
#include "../System.h"
#include "../common/Port.h"

#include "../apu/Gb_Apu.h"
#include "../apu/Multi_Buffer.h"

#include "../common/SoundDriver.h"

// GBA sound registers
#define SGCNT0_H 0x82
#define FIFOA_L 0xa0
#define FIFOA_H 0xa2
#define FIFOB_L 0xa4
#define FIFOB_H 0xa6
#define NR52 0x84

static SoundDriver * soundDriver = NULL;

extern bool stopState;      // TODO: silence sound when true

static int const SOUND_CLOCK_TICKS_ = 167772; // 1/100 second

static u16   soundFinalWave [1600];
static long  soundSampleRate    = 44100;
static bool  soundInterpolation = true;
static bool  soundPaused        = true;
static float soundFiltering     = 0.5f;
int   SOUND_CLOCK_TICKS  = SOUND_CLOCK_TICKS_;
int   soundTicks         = SOUND_CLOCK_TICKS_;

static float soundVolume     = 1.0f;
static float soundFiltering_ = -1;
static float soundVolume_    = -1;

void interp_rate()
{ /* empty for now */ }

class Gba_Pcm
{
public:
	void init();
	void apply_control( int idx );
	void update( int dac );
	void end_frame( blip_time_t );

private:
	Blip_Buffer* output;
	blip_time_t last_time;
	int last_amp;
	int shift;
};

class Gba_Pcm_Fifo
{
public:
	int     which;
	Gba_Pcm pcm;

	void write_control( int data );
	void write_fifo( int data );
	void timer_overflowed( int which_timer );

	// public only so save state routines can access it
	int  readIndex;
	int  count;
	int  writeIndex;
	u8   fifo [32];
	int  dac;
private:

	int  timer;
	bool enabled;
};

static Gba_Pcm_Fifo     pcm [2];
static Gb_Apu*          gb_apu;
static Stereo_Buffer*   stereo_buffer;

static Blip_Synth<blip_best_quality,1> pcm_synth [3]; // 32 kHz, 16 kHz, 8 kHz

static inline blip_time_t blip_time()
{
	return SOUND_CLOCK_TICKS - soundTicks;
}

void Gba_Pcm::init()
{
	output    = 0;
	last_time = 0;
	last_amp  = 0;
	shift     = 0;
}

void Gba_Pcm::apply_control( int idx )
{
	shift = ~ioMem [SGCNT0_H] >> (2 + idx) & 1;

	int ch = 0;
	if (ioMem [NR52] & 0x80)
		ch = ioMem [SGCNT0_H+1] >> (idx * 4) & 3;

	Blip_Buffer* out = 0;
	switch ( ch )
	{
	case 1:
		out = stereo_buffer->right();
		break;
	case 2:
		out = stereo_buffer->left();
		break;
	case 3:
		out = stereo_buffer->center();
		break;
	}

	if ( output != out )
	{
		if ( output )
		{
			output->set_modified();
			pcm_synth [0].offset( blip_time(), -last_amp, output );
		}
		last_amp = 0;
		output = out;
	}
}

void Gba_Pcm::end_frame( blip_time_t time )
{
	last_time -= time;
	if ( last_time < -2048 )
		last_time = -2048;

	if ( output )
		output->set_modified();
}

void Gba_Pcm::update( int dac )
{
	if ( output )
	{
		blip_time_t time = blip_time();

		dac = (s8) dac >> shift;
		int delta = dac - last_amp;
		if ( delta )
		{
			last_amp = dac;

			int filter = 0;
			if ( soundInterpolation )
			{
				// base filtering on how long since last sample was output
				int period = time - last_time;

				int idx = (unsigned) period / 512;
				if ( idx >= 3 )
					idx = 3;

				static int const filters [4] = { 0, 0, 1, 2 };
				filter = filters [idx];
			}

			pcm_synth [filter].offset( time, delta, output );
		}
		last_time = time;
	}
}

void Gba_Pcm_Fifo::timer_overflowed( int which_timer )
{
	if ( which_timer == timer && enabled )
	{
		if ( count <= 16 )
		{
			// Need to fill FIFO
			CPUCheckDMA( 3, which ? 4 : 2 );
			if ( count <= 16 )
			{
				// Not filled by DMA, so fill with 16 bytes of silence
				int reg = which ? FIFOB_L : FIFOA_L;
				for ( int n = 4; n--; )
				{
					soundEvent(reg  , (u16)0);
					soundEvent(reg+2, (u16)0);
				}
			}
		}

		// Read next sample from FIFO
		count--;
		dac = fifo [readIndex];
		readIndex = (readIndex + 1) & 31;
		pcm.update( dac );
	}
}

void Gba_Pcm_Fifo::write_control( int data )
{
	enabled = (data & 0x0300) ? true : false;
	timer   = (data & 0x0400) ? 1 : 0;

	if ( data & 0x0800 )
	{
		// Reset
		writeIndex = 0;
		readIndex  = 0;
		count      = 0;
		dac        = 0;
		memset( fifo, 0, sizeof fifo );
	}

	pcm.apply_control( which );
	pcm.update( dac );
}

void Gba_Pcm_Fifo::write_fifo( int data )
{
	fifo [writeIndex  ] = data & 0xFF;
	fifo [writeIndex+1] = data >> 8;
	count += 2;
	writeIndex = (writeIndex + 2) & 31;
}

static void apply_control()
{
	pcm [0].pcm.apply_control( 0 );
	pcm [1].pcm.apply_control( 1 );
}

static int gba_to_gb_sound( int addr )
{
	static const int table [0x40] =
	{
		0xFF10,     0,0xFF11,0xFF12,0xFF13,0xFF14,     0,     0,
		0xFF16,0xFF17,     0,     0,0xFF18,0xFF19,     0,     0,
		0xFF1A,     0,0xFF1B,0xFF1C,0xFF1D,0xFF1E,     0,     0,
		0xFF20,0xFF21,     0,     0,0xFF22,0xFF23,     0,     0,
		0xFF24,0xFF25,     0,     0,0xFF26,     0,     0,     0,
		0,     0,     0,     0,     0,     0,     0,     0,
		0xFF30,0xFF31,0xFF32,0xFF33,0xFF34,0xFF35,0xFF36,0xFF37,
		0xFF38,0xFF39,0xFF3A,0xFF3B,0xFF3C,0xFF3D,0xFF3E,0xFF3F,
	};
	if ( addr >= 0x60 && addr < 0xA0 )
		return table [addr - 0x60];
	return 0;
}

void soundEvent(u32 address, u8 data)
{
	int gb_addr = gba_to_gb_sound( address );
	if ( gb_addr )
	{
		ioMem[address] = data;
		gb_apu->write_register( blip_time(), gb_addr, data );

		if ( address == NR52 )
			apply_control();
	}

	// TODO: what about byte writes to SGCNT0_H etc.?
}

static void apply_volume( bool apu_only = false )
{
	if ( !apu_only )
		soundVolume_ = soundVolume;

	if ( gb_apu )
	{
		static float const apu_vols [4] = { 0.25, 0.5, 1, 0.25 };
		gb_apu->volume( soundVolume_ * apu_vols [ioMem [SGCNT0_H] & 3] );
	}

	if ( !apu_only )
	{
		for ( int i = 0; i < 3; i++ )
			pcm_synth [i].volume( 0.66 / 256 * soundVolume_ );
	}
}

static void write_SGCNT0_H( int data )
{
	WRITE16LE( &ioMem [SGCNT0_H], data & 0x770F );
	pcm [0].write_control( data      );
	pcm [1].write_control( data >> 4 );
	apply_volume( true );
}

void soundEvent(u32 address, u16 data)
{
	switch ( address )
	{
	case SGCNT0_H:
		write_SGCNT0_H( data );
		break;

	case FIFOA_L:
	case FIFOA_H:
		pcm [0].write_fifo( data );
		WRITE16LE( &ioMem[address], data );
		break;

	case FIFOB_L:
	case FIFOB_H:
		pcm [1].write_fifo( data );
		WRITE16LE( &ioMem[address], data );
		break;

	case 0x88:
		data &= 0xC3FF;
		WRITE16LE( &ioMem[address], data );
		break;

	default:
		soundEvent( address & ~1, (u8) (data     ) ); // even
		soundEvent( address |  1, (u8) (data >> 8) ); // odd
		break;
	}
}

void soundTimerOverflow(int timer)
{
	pcm [0].timer_overflowed( timer );
	pcm [1].timer_overflowed( timer );
}

static void end_frame( blip_time_t time )
{
	pcm [0].pcm.end_frame( time );
	pcm [1].pcm.end_frame( time );

	gb_apu       ->end_frame( time );
	stereo_buffer->end_frame( time );
}

static void flush_samples(Multi_Buffer * buffer)
{
	// We want to write the data frame by frame to support legacy audio drivers
	// that don't use the length parameter of the write method.
	// TODO: Update the Win32 audio drivers (DS, OAL, XA2), and flush all the
	// samples at once to help reducing the audio delay on all platforms.
	int soundBufferLen = ( soundSampleRate / 60 ) * 4;

	// soundBufferLen should have a whole number of sample pairs
	assert( soundBufferLen % (2 * sizeof *soundFinalWave) == 0 );

	// number of samples in output buffer
	int const out_buf_size = soundBufferLen / sizeof *soundFinalWave;

	// Keep filling and writing soundFinalWave until it can't be fully filled
	while ( buffer->samples_avail() >= out_buf_size )
	{
		buffer->read_samples( (blip_sample_t*) soundFinalWave, out_buf_size );
		if (soundPaused)
			soundPause(FALSE);

		soundDriver->write(soundDriver, soundFinalWave, soundBufferLen);
	}
}

static void apply_filtering()
{
	soundFiltering_ = soundFiltering;

	int const base_freq = (int) (32768 - soundFiltering_ * 16384);
	int const nyquist = stereo_buffer->sample_rate() / 2;

	for ( int i = 0; i < 3; i++ )
	{
		int cutoff = base_freq >> i;
		if ( cutoff > nyquist )
			cutoff = nyquist;
		pcm_synth [i].treble_eq( blip_eq_t( 0, 0, stereo_buffer->sample_rate(), cutoff ) );
	}
}

void psoundTickfn()
{
	if ( gb_apu && stereo_buffer )
	{
		// Run sound hardware to present
		end_frame( SOUND_CLOCK_TICKS );

		flush_samples(stereo_buffer);

		if ( soundFiltering_ != soundFiltering )
			apply_filtering();

		if ( soundVolume_ != soundVolume )
			apply_volume();
	}
}

static void apply_muting()
{
	if ( !stereo_buffer || !ioMem )
		return;

	// PCM
	apply_control();

	if ( gb_apu )
	{
		// APU
		for ( int i = 0; i < 4; i++ )
		{
			gb_apu->set_output( stereo_buffer->center(),
			                    stereo_buffer->left(), stereo_buffer->right(), i );
		}
	}
}

static void reset_apu()
{
	gb_apu->reset( gb_apu->mode_agb, true );

	if ( stereo_buffer )
		stereo_buffer->clear();

	soundTicks = SOUND_CLOCK_TICKS;
}

static void remake_stereo_buffer()
{
	if ( !ioMem )
		return;

	// Clears pointers kept to old stereo_buffer
	pcm [0].pcm.init();
	pcm [1].pcm.init();

	// APU
	if ( !gb_apu )
	{
		gb_apu = new Gb_Apu; // TODO: handle out of memory
		reset_apu();
	}

	// Stereo_Buffer
	delete stereo_buffer;
	stereo_buffer = 0;

	stereo_buffer = new Stereo_Buffer; // TODO: handle out of memory
	stereo_buffer->set_sample_rate( soundSampleRate ); // TODO: handle out of memory
	stereo_buffer->clock_rate( gb_apu->clock_rate );

	// PCM
	pcm [0].which = 0;
	pcm [1].which = 1;
	apply_filtering();

	// Volume Level
	apply_muting();
	apply_volume();
}

void soundShutdown()
{
	soundDriver = NULL;
}

void soundPause(gboolean pause)
{
	soundPaused = pause;
	if (soundDriver)
		soundDriver->pause(soundDriver, pause);
}

void soundSetVolume( float volume )
{
	soundVolume = volume;
}

float soundGetVolume()
{
	return soundVolume;
}

void soundReset()
{
	soundDriver->reset(soundDriver);

	remake_stereo_buffer();
	reset_apu();

	soundPaused = true;
	SOUND_CLOCK_TICKS = SOUND_CLOCK_TICKS_;
	soundTicks        = SOUND_CLOCK_TICKS_;

	soundEvent( NR52, (u8) 0x80 );
}

bool soundInit(SoundDriver *driver)
{
	soundDriver = driver;
	if ( !soundDriver )
		return false;

	soundPaused = true;
	return true;
}

long soundGetSampleRate()
{
	return soundSampleRate;
}

static gb_apu_state_t state;

// State format
static variable_desc gba_state [] =
{
	// PCM
	{ &pcm[0].readIndex,  sizeof(int)    },
	{ &pcm[0].count,      sizeof(int)    },
	{ &pcm[0].writeIndex, sizeof(int)    },
	{ &pcm[0].fifo,       sizeof(u8[32]) },
	{ &pcm[0].dac,        sizeof(int)    },

	{ &pcm[1].readIndex,  sizeof(int)    },
	{ &pcm[1].count,      sizeof(int)    },
	{ &pcm[1].writeIndex, sizeof(int)    },
	{ &pcm[1].fifo,       sizeof(u8[32]) },
	{ &pcm[1].dac,        sizeof(int)    },

	// APU
	{ &state.regs,        sizeof(u8 [0x40]) }, // last values written to registers and wave RAM (both banks)
	{ &state.frame_time,  sizeof(int)       }, // clocks until next frame sequencer action
	{ &state.frame_phase, sizeof(int)       }, // next step frame sequencer will run

	{ &state.sweep_freq,    sizeof(int)     }, // sweep's internal frequency register
	{ &state.sweep_delay,   sizeof(int)     }, // clocks until next sweep action
	{ &state.sweep_enabled, sizeof(int)     },
	{ &state.sweep_neg,     sizeof(int)     }, // obscure internal flag
	{ &state.noise_divider, sizeof(int)     },
	{ &state.wave_buf,      sizeof(int)     }, // last read byte of wave RAM

	{ &state.delay,         sizeof(int [4]) }, // clocks until next channel action
	{ &state.length_ctr,    sizeof(int [4]) },
	{ &state.phase,         sizeof(int [4]) }, // square/wave phase, noise LFSR
	{ &state.enabled,       sizeof(int [4]) }, // internal enabled flag

	{ &state.env_delay,     sizeof(int [3]) }, // clocks until next envelope action
	{ &state.env_volume,    sizeof(int [3]) },
	{ &state.env_enabled,   sizeof(int [3]) },

	{ NULL, 0 }
};

void soundSaveGame( gzFile out )
{
	gb_apu->save_state( &state );

	utilWriteData( out, gba_state );
}

void soundReadGame( gzFile in, int version )
{
	// Prepare APU and default state
	reset_apu();
	gb_apu->save_state( &state );

	utilReadData( in, gba_state );

	gb_apu->load_state( state );
	write_SGCNT0_H( READ16LE( &ioMem [SGCNT0_H] ) & 0x770F );

	apply_muting();
}
