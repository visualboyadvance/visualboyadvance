#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

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
