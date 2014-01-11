// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 2009 VBA-M development team

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
// Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include <stdlib.h>

#include "GameDB.h"
#include "Loader.h"
#include "Util.h"

typedef struct {
	GameInfos *game;

	gboolean foundCode;
	gboolean gameLoaded;

	const gchar *lookupCode;
} GameDBParserContext;

static int findv(const gchar **strings, const gchar *needle) {
	for (int i = 0; i < g_strv_length((gchar **)strings); i++) {
		if (g_strcmp0(strings[i], needle) == 0) {
			return i;
		}
	}

	return -1;
}

static gboolean g_markup_is_in_element(GMarkupParseContext *context, ...) {
	va_list ap;

	va_start(ap, context);

	// Get the first items
	const GSList *current_stack = g_markup_parse_context_get_element_stack(context);
	const gchar *item = va_arg(ap, const gchar *);

	// Compare items from both lists
	while (current_stack != NULL && item != NULL && !g_strcmp0(current_stack->data, item)) {
		current_stack = g_slist_next(current_stack);
		item = va_arg(ap, const gchar *);
	}

	va_end(ap);

	// Ensure the end of both lists was reached
	return current_stack == NULL && item == NULL;
}

static void on_start_element(GMarkupParseContext *context,
                          const gchar         *element_name,
                          const gchar        **attribute_names,
                          const gchar        **attribute_values,
                          gpointer             user_data,
                          GError             **error)
{
	GameDBParserContext *db = (GameDBParserContext *)user_data;
	
	if (db->gameLoaded) {
		return;
	}
	
	if (g_markup_is_in_element(context, "game", "games", NULL))
	{
		// Reset our cartridge data
		game_infos_free(db->game);
		db->game = game_infos_new();
	}
	else if (g_markup_is_in_element(context, "sram", "cartridge", "game", "games", NULL))
	{
		db->game->hasSRAM = TRUE;
	}
	else if (g_markup_is_in_element(context, "hasRTC", "cartridge", "game", "games", NULL))
	{
		db->game->hasRTC = TRUE;
	}
	else if (g_markup_is_in_element(context, "eeprom", "cartridge", "game", "games", NULL))
	{
		db->game->hasEEPROM = TRUE;
		int sizeIndex = findv(attribute_names, "size");
		if (sizeIndex >= 0)
		{
			db->game->EEPROMSize = atoi(attribute_values[sizeIndex]);
		}
	}
	else if (g_markup_is_in_element(context, "flash", "cartridge", "game", "games", NULL))
	{
		db->game->hasFlash = TRUE;
		int sizeIndex = findv(attribute_names, "size");
		if (sizeIndex >= 0)
		{
			db->game->flashSize = atoi(attribute_values[sizeIndex]);
		}
	}
}

static void on_end_element(GMarkupParseContext *context,
                          const gchar         *element_name,
                          gpointer             user_data,
                          GError             **error)
{
	GameDBParserContext *db = (GameDBParserContext *)user_data;
	
	if (g_markup_is_in_element(context, "game", "games", NULL))
	{
		if (db->foundCode)
		{
			db->gameLoaded = TRUE;
		}
	}
}

static void on_text(GMarkupParseContext *context,
                          const gchar         *text,
                          gsize                text_len,
                          gpointer             user_data,
                          GError             **error)
{
	GameDBParserContext *db = (GameDBParserContext *)user_data;
	
	if (db->gameLoaded) {
		return;
	}
	
	if (g_markup_is_in_element(context, "title", "game", "games", NULL))
	{
		g_free(db->game->title);
		db->game->title = g_strdup(text);
	}
	else if (g_markup_is_in_element(context, "region", "game", "games", NULL))
	{
		g_free(db->game->region);
		db->game->region = g_strdup(text);
	}
	else if (g_markup_is_in_element(context, "publisher", "game", "games", NULL))
	{
		g_free(db->game->publisher);
		db->game->publisher = g_strdup(text);
	}
	else if (g_markup_is_in_element(context, "code", "cartridge", "game", "games", NULL))
	{
		if (g_strcmp0(text, db->lookupCode) == 0) {
			db->foundCode = TRUE;

			g_free(db->game->code);
			db->game->code = g_strdup(text);
		}
	}
}

GameInfos *game_db_lookup_code(const gchar *code, GError **err)
{
	g_return_val_if_fail(err == NULL || *err == NULL, NULL);

	gchar *dbFilePath = data_get_file_path("db", "game-db.xml");
	gchar *xmlData = NULL;
	gsize  length = 0;

	if (!g_file_get_contents(dbFilePath, &xmlData, &length, err)) {
		g_free(dbFilePath);
		return NULL;
	}
	g_free(dbFilePath);

	GameDBParserContext *db = g_new(GameDBParserContext, 1);
	db->game = NULL;
	db->lookupCode = code;
	db->foundCode = FALSE;
	db->gameLoaded = FALSE;

	GMarkupParser parser;
	parser.start_element = &on_start_element;
	parser.end_element = &on_end_element;
	parser.text = &on_text;
	parser.passthrough = NULL;
	parser.error = NULL;

	GMarkupParseContext *context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, db, NULL);
	if (!g_markup_parse_context_parse(context, xmlData, length, err)) {
		game_infos_free(db->game);
		g_free(db);
		g_free(xmlData);
		g_markup_parse_context_free(context);
		return NULL;
	}
	g_free(xmlData);

	if (!g_markup_parse_context_end_parse(context, err)) {
		game_infos_free(db->game);
		g_free(db);
		g_markup_parse_context_free(context);
		return NULL;
	}

	g_markup_parse_context_free(context);

	GameInfos *game;

	if (db->gameLoaded) {
		game = db->game;
	} else {
		game_infos_free(db->game);
		game = NULL;
		g_set_error(err, LOADER_ERROR, G_LOADER_ERROR_NOT_IN_DB,
				"Game '%s' was not found in '%s'", code, "game-db.xml");
	}
	
	g_free(db);

	return game;
}
