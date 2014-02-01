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

typedef gboolean (*LoaderAccept)(const gchar *);

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

static LoaderAccept loader_accept_func(RomLoader *loader) {
	switch (loader->type) {
	case ROM_BIOS:
		return &loader_is_bios;
		break;
	case ROM_GBA:
		return &loader_is_gba;
		break;
	default:
		g_return_val_if_reached(FALSE);
		break;
	}
}

static struct archive *loader_open_archive(RomLoader *loader, GError **err) {
	LoaderAccept accept = loader_accept_func(loader);

	// Initialize libarchive
	struct archive *a = archive_read_new();

	if (!accept(loader->filename)) {
		archive_read_support_filter_all(a);
		archive_read_support_format_all(a);
	} else {
		archive_read_support_format_raw(a);
	}

	// Open the archive
	int r = archive_read_open_filename(a, loader->filename, 10240);

	if (r != ARCHIVE_OK) {
		g_set_error(err, LOADER_ERROR, G_LOADER_ERROR_FAILED,
				"Loading error : %s", archive_error_string(a));
		archive_read_free(a);
		return NULL;
	}

	return a;
}

gboolean loader_load(RomLoader *loader, guint8 *data, int *size, GError **err) {
	struct archive_entry *entry;
	LoaderAccept accept = loader_accept_func(loader);
	gboolean loaded = FALSE;
	gboolean uncompressedFile = accept(loader->filename);

	// Open the archive
	struct archive *a = loader_open_archive(loader, err);
	if (a == NULL) {
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

	// Free libarchive
	archive_read_free(a);

	if (!loaded) {
		g_set_error(err, LOADER_ERROR, G_LOADER_ERROR_FAILED,
				"No ROM found in file %s", loader->filename);
		return FALSE;
	}

	return TRUE;
}

gchar *loader_read_code(RomLoader *loader, GError **err) {
	static const size_t HEADER_SIZE = 192;

	struct archive_entry *entry;
	LoaderAccept accept = loader_accept_func(loader);
	gboolean uncompressedFile = accept(loader->filename);
	guint8 header[HEADER_SIZE];
	gchar *code = NULL;

	// Open the archive
	struct archive *a = loader_open_archive(loader, err);
	if (a == NULL) {
		return NULL;
	}

	// Iterate through the archived files
	while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
		if (uncompressedFile || accept(archive_entry_pathname(entry))) {
			ssize_t readSize = archive_read_data(a, header, HEADER_SIZE);

			if (readSize != HEADER_SIZE) {
				g_set_error(err, LOADER_ERROR, G_LOADER_ERROR_FAILED,
						"Error uncompressing %s from %s : %s", archive_entry_pathname(entry), loader->filename, archive_error_string(a));
				archive_read_free(a);
				return FALSE;
			}

			code = g_strndup((gchar *) &header[0xac], 4);

			break;
		} else {
			archive_read_data_skip(a);
		}
	}

	// Free libarchive
	archive_read_free(a);

	if (code == NULL) {
		g_set_error(err, LOADER_ERROR, G_LOADER_ERROR_FAILED,
				"No ROM found in file %s", loader->filename);
		return NULL;
	}

	return code;
}

GQuark loader_error_quark() {
	return g_quark_from_static_string("loader_error_quark");
}
