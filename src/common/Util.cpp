#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include <archive.h>
#include <archive_entry.h>


#include "../System.h"
#include "Util.h"
#include "Port.h"

#ifndef _MSC_VER
#define _stricmp strcasecmp
#endif // ! _MSC_VER

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

bool utilIsGBAImage(const char * file)
{
  if(strlen(file) > 4) {
    const char * p = strrchr(file,'.');

    if(p != NULL) {
      if((_stricmp(p, ".agb") == 0) ||
         (_stricmp(p, ".gba") == 0) ||
         (_stricmp(p, ".bin") == 0) ||
         (_stricmp(p, ".elf") == 0))
        return true;
    }
  }

  return false;
}

bool utilLoad(const char *file,
             bool (*accept)(const char *),
             u8 *data,
             int &size)
{
	struct archive *a;
	struct archive_entry *entry;
	int r;
	bool loaded = false;
	bool uncompressedFile = accept(file);

	// Initialize libarchive
	a = archive_read_new();
	
	if (!uncompressedFile) {
		archive_read_support_filter_all(a);
		archive_read_support_format_all(a);
	} else {
		archive_read_support_format_raw(a);
	}
	
	// Open the archive
	r = archive_read_open_filename(a, file, 10240);
	
	if (r != ARCHIVE_OK) {
		systemMessage("Error opening archive %s", file);
		return false;
	}
	
	// Iterate through the archived files
	while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
		if (uncompressedFile || accept(archive_entry_pathname(entry))) {
			size_t buffSize = size;
			size = 0;
			for (;;) {
				ssize_t readSize = archive_read_data(a, data, buffSize);
				
				if (readSize < 0) {
					systemMessage("Error uncompressing %s from %s", archive_entry_pathname(entry), file);
					goto cleanup;
				}
				
				if (readSize == 0) {
					break;
				}
				
				data += readSize;
				buffSize -= readSize;
				size += readSize;
			}
		
			loaded = true;
			break;
		} else {
			archive_read_data_skip(a);
		}
	}
	
	if(!loaded) {
		systemMessage("No image found in file %s", file);
	}
	
	cleanup:
	
	// Free libarchive
	r = archive_read_free(a);
	if (r != ARCHIVE_OK) {
		systemMessage("Error closing archive %s", file);
		return false;
	}

	return loaded;
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

void utilWriteData(gzFile gzFile, variable_desc *data)
{
  while(data->address) {
    utilGzWrite(gzFile, data->address, data->size);
    data++;
  }
}

gzFile utilGzOpen(const char *file, const char *mode)
{
  return gzopen(file, mode);
}

int utilGzWrite(gzFile file, const voidp buffer, unsigned int len)
{
  return gzwrite(file, buffer, len);
}

int utilGzRead(gzFile file, voidp buffer, unsigned int len)
{
  return gzread(file, buffer, len);
}

int utilGzClose(gzFile file)
{
  return gzclose(file);
}

z_off_t utilGzSeek(gzFile file, z_off_t offset, int whence)
{
	return gzseek(file, offset, whence);
}
