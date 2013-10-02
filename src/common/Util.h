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

#ifndef VBAM_UTIL_H_
#define VBAM_UTIL_H_

#include "Types.h"
#include <zlib.h>
#include <glib.h>


/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Finds the complete path of a data file distributed with VBA
 *
 * @param folder The name of the folder where the file is
 * @param filename Name of the file to find
 * @return newly allocated string containing the path to the file
 */
gchar *data_get_file_path(const gchar *folder, const gchar *filename);

// save game
typedef struct {
  void *address;
  int size;
} variable_desc;

void utilWriteData(gzFile, variable_desc *);
void utilReadData(gzFile, variable_desc *);
int utilReadInt(gzFile);
void utilWriteInt(gzFile, int);
int utilGzWrite(gzFile file, const voidp buffer, unsigned int len);
int utilGzRead(gzFile file, voidp buffer, unsigned int len);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* VBAM_UTIL_H_ */
