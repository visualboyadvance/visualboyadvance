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

#include "Util.h"

gchar *data_get_file_path(const gchar *filename) {
	// Use the data file from the source folder if it exists
	// to make vbam runnable without installation
	gchar *dataFilePath = g_build_filename("data", filename, NULL);
	if (!g_file_test(dataFilePath, G_FILE_TEST_EXISTS))
	{
		g_free(dataFilePath);
		dataFilePath = g_build_filename("data", "sdl", filename, NULL);
	}
	if (!g_file_test(dataFilePath, G_FILE_TEST_EXISTS))
	{
		g_free(dataFilePath);
		dataFilePath = g_build_filename(PKGDATADIR, filename, NULL);
	}

	return dataFilePath;
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

int utilGzWrite(gzFile file, const voidp buffer, unsigned int len)
{
  return gzwrite(file, buffer, len);
}

int utilGzRead(gzFile file, voidp buffer, unsigned int len)
{
  return gzread(file, buffer, len);
}

