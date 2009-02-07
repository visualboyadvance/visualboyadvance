#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>


#include "System.h"
#include "NLS.h"
#include "Util.h"
#include "gba/Flash.h"
#include "gba/GBA.h"
#include "gba/Globals.h"
#include "gba/RTC.h"
#include "common/Port.h"

#include "common/fex.h"

extern "C" {
#include "common/memgzio.h"
}

#ifndef _MSC_VER
#define _stricmp strcasecmp
#endif // ! _MSC_VER

static int (ZEXPORT *utilGzWriteFunc)(gzFile, const voidp, unsigned int) = NULL;
static int (ZEXPORT *utilGzReadFunc)(gzFile, voidp, unsigned int) = NULL;
static int (ZEXPORT *utilGzCloseFunc)(gzFile) = NULL;
static z_off_t (ZEXPORT *utilGzSeekFunc)(gzFile, z_off_t, int) = NULL;

void utilPutDword(u8 *p, u32 value)
{
  *p++ = value & 255;
  *p++ = (value >> 8) & 255;
  *p++ = (value >> 16) & 255;
  *p = (value >> 24) & 255;
}

void utilPutWord(u8 *p, u16 value)
{
  *p++ = value & 255;
  *p = (value >> 8) & 255;
}

extern bool cpuIsMultiBoot;

bool utilIsGBAImage(const char * file)
{
  cpuIsMultiBoot = false;
  if(strlen(file) > 4) {
    const char * p = strrchr(file,'.');

    if(p != NULL) {
      if((_stricmp(p, ".agb") == 0) ||
         (_stricmp(p, ".gba") == 0) ||
         (_stricmp(p, ".bin") == 0) ||
         (_stricmp(p, ".elf") == 0))
        return true;
      if(_stricmp(p, ".mb") == 0) {
        cpuIsMultiBoot = true;
        return true;
      }
    }
  }

  return false;
}

bool utilIsGBImage(const char * file)
{
  if(strlen(file) > 4) {
    const char * p = strrchr(file,'.');

    if(p != NULL) {
      if((_stricmp(p, ".dmg") == 0) ||
         (_stricmp(p, ".gb") == 0) ||
         (_stricmp(p, ".gbc") == 0) ||
         (_stricmp(p, ".cgb") == 0) ||
         (_stricmp(p, ".sgb") == 0))
        return true;
    }
  }

  return false;
}

bool utilIsGzipFile(const char *file)
{
  if(strlen(file) > 3) {
    const char * p = strrchr(file,'.');

    if(p != NULL) {
      if(_stricmp(p, ".gz") == 0)
        return true;
      if(_stricmp(p, ".z") == 0)
        return true;
    }
  }

  return false;
}

// strip .gz or .z off end
void utilStripDoubleExtension(const char *file, char *buffer)
{
  if(buffer != file) // allows conversion in place
    strcpy(buffer, file);

  if(utilIsGzipFile(file)) {
    char *p = strrchr(buffer, '.');

    if(p)
      *p = 0;
  }
}

// Opens and scans archive using accept(). Returns File_Extractor if found.
// If error or not found, displays message and returns NULL.
static File_Extractor* scan_arc(const char *file, bool (*accept)(const char *),
		char (&buffer) [2048] )
{
	fex_err_t err;
	File_Extractor* fe = fex_open( file, &err );
	if(!fe)
	{
		systemMessage(MSG_CANNOT_OPEN_FILE, N_("Cannot open file %s: %s"), file, err);
		return NULL;
	}

	// Scan filenames
	bool found=false;
	while(!fex_done(fe)) {
		strncpy(buffer,fex_name(fe),sizeof buffer);
		buffer [sizeof buffer-1] = '\0';

		utilStripDoubleExtension(buffer, buffer);

		if(accept(buffer)) {
			found = true;
			break;
		}

		fex_err_t err = fex_next(fe);
		if(err) {
			systemMessage(MSG_BAD_ZIP_FILE, N_("Cannot read archive %s: %s"), file, err);
			fex_close(fe);
			return NULL;
		}
	}

	if(!found) {
		systemMessage(MSG_NO_IMAGE_ON_ZIP,
									N_("No image found in file %s"), file);
		fex_close(fe);
		return NULL;
	}
	return fe;
}

static bool utilIsImage(const char *file)
{
	return utilIsGBAImage(file) || utilIsGBImage(file);
}

IMAGE_TYPE utilFindType(const char *file)
{
	char buffer [2048];
	if ( !utilIsImage( file ) ) // TODO: utilIsArchive() instead?
	{
		File_Extractor* fe = scan_arc(file,utilIsImage,buffer);
		if(!fe)
			return IMAGE_UNKNOWN;
		fex_close(fe);
		file = buffer;
	}

	return utilIsGBAImage(file) ? IMAGE_GBA : IMAGE_GB;
}

static int utilGetSize(int size)
{
  int res = 1;
  while(res < size)
    res <<= 1;
  return res;
}

u8 *utilLoad(const char *file,
             bool (*accept)(const char *),
             u8 *data,
             int &size)
{
	// find image file
	char buffer [2048];
	File_Extractor *fe = scan_arc(file,accept,buffer);
	if(!fe)
		return NULL;

	// Allocate space for image
	int fileSize = fex_size(fe);
	if(size == 0)
		size = fileSize;

	u8 *image = data;

	if(image == NULL) {
		// allocate buffer memory if none was passed to the function
		image = (u8 *)malloc(utilGetSize(size));
		if(image == NULL) {
			fex_close(fe);
			systemMessage(MSG_OUT_OF_MEMORY, N_("Failed to allocate memory for %s"),
										"data");
			return NULL;
		}
		size = fileSize;
	}

	// Read image
	int read = fileSize <= size ? fileSize : size; // do not read beyond file
	fex_err_t err = fex_read_once(fe, image, read);
	fex_close(fe);
	if(err) {
		systemMessage(MSG_ERROR_READING_IMAGE,
									N_("Error reading image from %s: %s"), buffer, err);
		if(data == NULL)
			free(image);
		return NULL;
	}

	size = fileSize;

	return image;
}

void utilWriteInt(gzFile gzFile, int i)
{
  utilGzWrite(gzFile, &i, sizeof(int));
}

int utilReadInt(gzFile gzFile)
{
  int i = 0;
  utilGzRead(gzFile, &i, sizeof(int));
  return i;
}

void utilReadData(gzFile gzFile, variable_desc* data)
{
  while(data->address) {
    utilGzRead(gzFile, data->address, data->size);
    data++;
  }
}

void utilReadDataSkip(gzFile gzFile, variable_desc* data)
{
  while(data->address) {
    utilGzSeek(gzFile, data->size, SEEK_CUR);
    data++;
  }
}

void utilWriteData(gzFile gzFile, variable_desc *data)
{
  while(data->address) {
    utilGzWrite(gzFile, data->address, data->size);
    data++;
  }
}

gzFile utilGzOpen(const char *file, const char *mode)
{
  utilGzWriteFunc = (int (ZEXPORT *)(void *,void * const, unsigned int))gzwrite;
  utilGzReadFunc = gzread;
  utilGzCloseFunc = gzclose;
  utilGzSeekFunc = gzseek;

  return gzopen(file, mode);
}

gzFile utilMemGzOpen(char *memory, int available, const char *mode)
{
  utilGzWriteFunc = memgzwrite;
  utilGzReadFunc = memgzread;
  utilGzCloseFunc = memgzclose;

  return memgzopen(memory, available, mode);
}

int utilGzWrite(gzFile file, const voidp buffer, unsigned int len)
{
  return utilGzWriteFunc(file, buffer, len);
}

int utilGzRead(gzFile file, voidp buffer, unsigned int len)
{
  return utilGzReadFunc(file, buffer, len);
}

int utilGzClose(gzFile file)
{
  return utilGzCloseFunc(file);
}

z_off_t utilGzSeek(gzFile file, z_off_t offset, int whence)
{
	return utilGzSeekFunc(file, offset, whence);
}

long utilGzMemTell(gzFile file)
{
  return memtell(file);
}

void utilGBAFindSave(const u8 *data, const int size)
{
  u32 *p = (u32 *)data;
  u32 *end = (u32 *)(data + size);
  int saveType = 0;
  int flashSize = 0x10000;
  bool rtcFound = false;

  while(p  < end) {
    u32 d = READ32LE(p);

    if(d == 0x52504545) {
      if(memcmp(p, "EEPROM_", 7) == 0) {
        if(saveType == 0)
          saveType = 3;
      }
    } else if (d == 0x4D415253) {
      if(memcmp(p, "SRAM_", 5) == 0) {
        if(saveType == 0)
          saveType = 1;
      }
    } else if (d == 0x53414C46) {
      if(memcmp(p, "FLASH1M_", 8) == 0) {
        if(saveType == 0) {
          saveType = 2;
          flashSize = 0x20000;
        }
      } else if(memcmp(p, "FLASH", 5) == 0) {
        if(saveType == 0) {
          saveType = 2;
          flashSize = 0x10000;
        }
      }
    } else if (d == 0x52494953) {
      if(memcmp(p, "SIIRTC_V", 8) == 0)
        rtcFound = true;
    }
    p++;
  }
  // if no matches found, then set it to NONE
  if(saveType == 0) {
    saveType = 5;
  }
  rtcEnable(rtcFound);
  cpuSaveType = saveType;
  flashSetSize(flashSize);
}
