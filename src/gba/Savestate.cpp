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

#include "Savestate.h"

#include "GBA.h"
#include "Cartridge.h"
#include "../common/Settings.h"

#include <errno.h>
#include <string.h>
#include <zlib.h>

gboolean savestate_load_from_file(const gchar *file, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

	gzFile gzFile = gzopen(file, "rb");
	if (gzFile == NULL) {
		SaveStateError code = G_SAVESTATE_ERROR_FAILED;
		if (errno == ENOENT) {
			code = G_SAVESTATE_NOT_FOUND;
		}

		g_set_error(err, SAVESTATE_ERROR, code,
				"Failed to load state: %s", g_strerror(errno));
		return FALSE;
	}

	gboolean res = CPUReadState(gzFile, err);

	gzclose(gzFile);

	return res;
}

gboolean savestate_save_to_file(const gchar *file, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

	gzFile gzFile = gzopen(file, "wb");
	if (gzFile == NULL) {
		g_set_error(err, SAVESTATE_ERROR, G_SAVESTATE_ERROR_FAILED,
				"Failed to save state: %s", g_strerror(errno));
		return FALSE;
	}

	CPUWriteState(gzFile);

	gzclose(gzFile);

	return TRUE;
}

static gchar *get_slot_filename(gint num) {
	const gchar *saveDir = settings_get_save_dir();

	//TODO: Ensure the filename is safe
	const gchar *gameTitle = Cartridge::getGameTitle();

	gchar *stateNum = g_strdup_printf("%d", num + 1);
	gchar *baseName = g_path_get_basename(gameTitle);
	gchar *fileName = g_strconcat(baseName, "_", stateNum, ".sgm", NULL);
	gchar *stateName = g_build_filename(saveDir, fileName, NULL);

	g_free(fileName);
	g_free(baseName);
	g_free(stateNum);

	return stateName;
}

gboolean savestate_load_slot(gint num, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

	gchar *stateName = get_slot_filename(num);
	gboolean success = savestate_load_from_file(stateName, err);
	g_free(stateName);

	if (g_error_matches(*err, SAVESTATE_ERROR, G_SAVESTATE_NOT_FOUND)) {
		g_clear_error(err);
		g_set_error(err, SAVESTATE_ERROR, G_SAVESTATE_NOT_FOUND,
				"No save in slot %d", num + 1);
	}

	return success;
}

gboolean savestate_save_slot(gint num, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

	gchar *stateName = get_slot_filename(num);
	gboolean success = savestate_save_to_file(stateName, err);
	g_free(stateName);

	return success;
}

GQuark savestate_error_quark() {
	return g_quark_from_static_string("savestate_error_quark");
}
