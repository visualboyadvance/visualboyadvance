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

#include "Settings.h"

// Directory within confdir to use for default save location.
#define CONF_DIR "vbam"

// key-file <=> button mapping
static struct {
	const gchar *key;
	EKey button;
} buttons[] = {
	{ "dpad-left",      KEY_LEFT           },
	{ "dpad-right",     KEY_RIGHT          },
	{ "dpad-up",        KEY_UP             },
	{ "dpad-down",      KEY_DOWN           },
	{ "button-a",       KEY_BUTTON_A       },
	{ "button-b",       KEY_BUTTON_B       },
	{ "button-select",  KEY_BUTTON_SELECT  },
	{ "button-start",   KEY_BUTTON_START   },
	{ "trigger-left",   KEY_BUTTON_L       },
	{ "trigger-right",  KEY_BUTTON_R       },
	{ "shortcut-speed", KEY_BUTTON_SPEED   },
	{ "autofire-a",     KEY_BUTTON_AUTO_A  },
	{ "autofire-b",     KEY_BUTTON_AUTO_B  }
};

// Available settings
typedef struct {
	gchar *configFileName;
	gchar *biosFileName;
	gchar *saveDir;
	gchar *batteryDir;

	gboolean fullscreen;
	gboolean pauseWhenInactive;
	gboolean showSpeed;
	gboolean disableStatus;

	guint soundSampleRate;
	gdouble soundVolume;

	guint32 joypad[G_N_ELEMENTS(buttons)];
} Settings;

static Settings settings;
static gchar **filenames = NULL;

static GOptionEntry commandLineOptions[] = {
  { "bios", 'b', 0, G_OPTION_ARG_FILENAME, &settings.biosFileName, "Use given bios file", NULL },
  { "fullscreen", 0, 0, G_OPTION_ARG_NONE, &settings.fullscreen, "Full screen", NULL },
  { "pause-when-inactive", 0, 0, G_OPTION_ARG_NONE, &settings.pauseWhenInactive, "Pause when inactive", NULL },
  { "show-speed", 0, 0, G_OPTION_ARG_NONE, &settings.showSpeed, "Show emulation speed", NULL },
  { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, NULL, "[GBA ROM file]" },
  { NULL }
};

void settings_init() {
	const gchar *userDataDir = g_get_user_data_dir();

	// Setup default values
	settings.configFileName = g_build_filename(userDataDir, CONF_DIR, "config", NULL);
	settings.biosFileName = NULL;
	settings.saveDir = g_build_filename(userDataDir, CONF_DIR, NULL);
	settings.batteryDir = g_build_filename(userDataDir, CONF_DIR, NULL);

	settings.fullscreen = FALSE;
	settings.pauseWhenInactive = FALSE;
	settings.showSpeed = FALSE;
	settings.disableStatus = FALSE;

	settings.soundSampleRate = 44100;
	settings.soundVolume = 1.0f;

	for (guint i = 0; i < G_N_ELEMENTS(buttons); i++) {
		settings.joypad[buttons[i].button] = 0;
	}
}

void settings_free() {
	g_free(settings.configFileName);
	g_free(settings.biosFileName);
	g_free(settings.saveDir);
	g_free(settings.batteryDir);
}

void settings_display_usage(GError *err) {
	GOptionContext *context = g_option_context_new(NULL);
	gchar *usage = g_option_context_get_help(context, FALSE, NULL);
	g_option_context_free(context);

	if (err != NULL) {
		g_printerr("%s\n", err->message);
	}

	g_print("%s", usage);
	g_free(usage);
}

gchar *settings_parse_command_line(gint *argc, gchar ***argv, GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	// Parse command line
	GOptionContext *context;
	gchar *filename = NULL;

	context = g_option_context_new(NULL);
	g_option_context_add_main_entries(context, commandLineOptions, NULL);

	if (!g_option_context_parse(context, argc, argv, err)) {
		g_option_context_free(context);
		return NULL;
	}
	if (filenames == NULL || g_strv_length(filenames) != 1) {
		g_set_error(err,
				G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
				"You must specify exactly one GBA ROM file.");

		g_option_context_free(context);
		return NULL;
	}
	g_option_context_free(context);

	filename = g_strdup(filenames[0]);

	g_strfreev(filenames);
	filenames = NULL;

	return filename;
}

static gboolean is_key_not_found_error(GError *err) {
	if (!err)
		return FALSE;

	return g_error_matches(err, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND)
			|| g_error_matches(err, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_GROUP_NOT_FOUND);
}

static gboolean settings_handle_error(GError **err, GError *tmp_error) {
	if (is_key_not_found_error(tmp_error)) {
		// A missing key is not a fatal error, and should not overwrite the default value
		return TRUE;
	} else {
		// Fatal / unknown error
		g_propagate_prefixed_error(err, tmp_error, "Failed to read configuration file '%s' : ", settings.configFileName);
		return FALSE;
	}
}

static gboolean settings_read_boolean(GKeyFile *file, const gchar *group_name, const gchar *key, gboolean *value, GError **err) {
	GError *tmp_error = NULL;
	gboolean tmp_value = g_key_file_get_boolean(file, group_name, key, &tmp_error);
	if (tmp_error != NULL) {
		return settings_handle_error(err, tmp_error);
	}

	// Overwrite the previous value
	*value = tmp_value;
	return TRUE;
}

static guint settings_read_integer(GKeyFile *file, const gchar *group_name, const gchar *key, guint *value, GError **err) {
	GError *tmp_error = NULL;
	guint tmp_value = g_key_file_get_integer(file, group_name, key, &tmp_error);
	if (tmp_error != NULL) {
		return settings_handle_error(err, tmp_error);
	}

	// Overwrite the previous value
	*value = tmp_value;
	return TRUE;
}

static gboolean settings_read_double(GKeyFile *file, const gchar *group_name, const gchar *key, gdouble *value, GError **err) {
	GError *tmp_error = NULL;
	gdouble tmp_value = g_key_file_get_double(file, group_name, key, &tmp_error);
	if (tmp_error != NULL) {
		return settings_handle_error(err, tmp_error);
	}

	// Overwrite the previous value
	*value = tmp_value;
	return TRUE;
}

static gboolean settings_read_string(GKeyFile *file, const gchar *group_name, const gchar *key, gchar **value, GError **err) {
	GError *tmp_error = NULL;
	gchar *tmp_value = g_key_file_get_string(file, group_name, key, &tmp_error);
	if (tmp_error != NULL) {
		return settings_handle_error(err, tmp_error);
	}

	// Overwrite the previous value
	g_free(*value);
	*value = tmp_value;
	return TRUE;
}

gboolean settings_read_config_file(GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

	GKeyFile *file = g_key_file_new();
	if (!g_key_file_load_from_file(file, settings.configFileName, G_KEY_FILE_KEEP_COMMENTS, err)) {
		g_key_file_free(file);
		g_prefix_error(err, "Failed to read configuration file '%s' : ", settings.configFileName);
		return FALSE;
	}

	if (!settings_read_boolean(file, "display", "fullscreen", &settings.fullscreen, err)) {
		g_key_file_free(file);
		return FALSE;
	}

	if (!settings_read_boolean(file, "display", "showSpeed", &settings.showSpeed, err)) {
		g_key_file_free(file);
		return FALSE;
	}

	if (!settings_read_boolean(file, "display", "pauseWhenInactive", &settings.pauseWhenInactive, err)) {
		g_key_file_free(file);
		return FALSE;
	}

	if (!settings_read_boolean(file, "display", "disableStatus", &settings.disableStatus, err)) {
		g_key_file_free(file);
		return FALSE;
	}

	if (!settings_read_string(file, "paths", "biosFileName", &settings.biosFileName, err)) {
		g_key_file_free(file);
		return FALSE;
	}

	if (!settings_read_string(file, "paths", "batteryDir", &settings.batteryDir, err)) {
		g_key_file_free(file);
		return FALSE;
	}

	if (!settings_read_string(file, "paths", "saveDir", &settings.saveDir, err)) {
		g_key_file_free(file);
		return FALSE;
	}

	if (!settings_read_double(file, "sound", "volume", &settings.soundVolume, err)) {
		g_key_file_free(file);
		return FALSE;
	}

	if (!settings_read_integer(file, "sound", "sampleRate", &settings.soundSampleRate, err)) {
		g_key_file_free(file);
		return FALSE;
	}

	for (guint i = 0; i < G_N_ELEMENTS(buttons); i++) {
		if (!settings_read_integer(file, "input", buttons[i].key, &settings.joypad[buttons[i].button], err)) {
			g_key_file_free(file);
			return FALSE;
		}
	}

	g_key_file_free(file);

	return TRUE;
}

gboolean settings_check(GError **err) {
	g_return_val_if_fail(err == NULL || *err == NULL, FALSE);

	if (settings.biosFileName == NULL || g_strcmp0(settings.biosFileName, "") == 0) {
		g_set_error(err,
			G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
			"You must configure the GBA BIOS ROM file.");
		return FALSE;
	}

	if (settings.soundVolume < 0.0 || settings.soundVolume > SETTINGS_SOUND_MAX_VOLUME) {
		g_set_error(err,
			G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
			"The volume must be between 0 and %f.", SETTINGS_SOUND_MAX_VOLUME);
		return FALSE;
	}

	if (settings.soundSampleRate < 11000 || settings.soundVolume > 48000) {
		g_set_error(err,
			G_OPTION_ERROR, G_OPTION_ERROR_FAILED,
			"The sound sample rate must be between 11000 KHz and 48000 KHz.");
		return FALSE;
	}

	return TRUE;
}

const gchar *settings_get_battery_dir() {
	return settings.batteryDir;
}

const gchar *settings_get_save_dir() {
	return settings.saveDir;
}

const gchar *settings_get_bios() {
	return settings.biosFileName;
}

gboolean settings_is_fullscreen() {
	return settings.fullscreen;
}

gboolean settings_pause_when_inactive() {
	return settings.pauseWhenInactive;
}

gboolean settings_show_speed() {
	return settings.showSpeed;
}

gboolean settings_disable_status_messages() {
	return settings.disableStatus;
}

gdouble settings_sound_volume() {
	return settings.soundVolume;
}

guint settings_sound_sample_rate() {
	return settings.soundSampleRate;
}

guint32 settings_get_button_mapping(EKey button) {
	g_return_val_if_fail(button < G_N_ELEMENTS(settings_buttons) , 0);

	return settings.joypad[button];
}
