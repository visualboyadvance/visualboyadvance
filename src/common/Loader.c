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

#include "Loader.h"

#include <archive.h>
#include <archive_entry.h>

struct RomLoader {
	RomType type;
	gchar *filename;
};

static gboolean loader_is_gba(const gchar *file)
{
	return g_str_has_suffix(file, ".gba");
}

static gboolean loader_is_bios(const gchar *file)
{
	return g_str_has_suffix(file, ".bin");
}

RomLoader *loader_new(RomType type, const gchar *filename) {
	RomLoader *loader = g_new(RomLoader, 1);
	loader->type = type;
	loader->filename = g_strdup(filename);

	return loader;
}

void loader_free(RomLoader *loader) {
	g_free(loader->filename);
	g_free(loader);
}

gboolean loader_load(RomLoader *loader, guint8 *data, int *size, GError **err) {
	gboolean (*accept)(const gchar *) = NULL;

	switch (loader->type) {
	case ROM_BIOS:
		accept = &loader_is_bios;
		break;
	case ROM_GBA:
		accept = &loader_is_gba;
		break;
	default:
		g_return_val_if_reached(FALSE);
		break;
	}

	struct archive *a;
	struct archive_entry *entry;
	int r;
	gboolean loaded = FALSE;
	gboolean uncompressedFile = accept(loader->filename);

	// Initialize libarchive
	a = archive_read_new();

	if (!uncompressedFile) {
		archive_read_support_filter_all(a);
		archive_read_support_format_all(a);
	} else {
		archive_read_support_format_raw(a);
	}

	// Open the archive
	r = archive_read_open_filename(a, loader->filename, 10240);

	if (r != ARCHIVE_OK) {
		g_set_error(err, LOADER_ERROR, G_LOADER_ERROR_FAILED,
				"Error loading %s : %s", loader->filename, archive_error_string(a));
		return FALSE;
	}

	// Iterate through the archived files
	while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
		if (uncompressedFile || accept(archive_entry_pathname(entry))) {
			size_t buffSize = *size;
			*size = 0;
			for (;;) {
				ssize_t readSize = archive_read_data(a, data, buffSize);

				if (readSize < 0) {
					g_set_error(err, LOADER_ERROR, G_LOADER_ERROR_FAILED,
							"Error uncompressing %s from %s : %s", archive_entry_pathname(entry), loader->filename, archive_error_string(a));
					archive_read_free(a);
					return FALSE;
				}

				if (readSize == 0) {
					break;
				}

				data += readSize;
				buffSize -= readSize;
				*size += readSize;
			}

			loaded = TRUE;
			break;
		} else {
			archive_read_data_skip(a);
		}
	}

	if (!loaded) {
		g_set_error(err, LOADER_ERROR, G_LOADER_ERROR_FAILED,
				"No ROM found in file %s", loader->filename);
		archive_read_free(a);
		return FALSE;
	}

	// Free libarchive
	archive_read_free(a);

	return TRUE;
}

GQuark loader_error_quark() {
	return g_quark_from_static_string("loader_error_quark");
}
