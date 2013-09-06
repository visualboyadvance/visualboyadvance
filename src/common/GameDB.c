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

typedef struct {
	GameInfos *game;

	gboolean isInGameTag;
	gboolean isInTitleTag;
	gboolean isInCodeTag;
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
	
	if (g_strcmp0(element_name, "game") == 0)
	{
		db->isInGameTag = TRUE;
		
		// Reset our cartridge data
		game_infos_reset(db->game);
	}
	else if (g_strcmp0(element_name, "title") == 0)
	{
		db->isInTitleTag = TRUE;
	}
	else if (g_strcmp0(element_name, "code") == 0)
	{
		db->isInCodeTag = TRUE;
	}
	else if (g_strcmp0(element_name, "sram") == 0)
	{
		db->game->hasSRAM = TRUE;
	}
	else if (g_strcmp0(element_name, "hasRTC") == 0)
	{
		db->game->hasRTC = TRUE;
	}
	else if (g_strcmp0(element_name, "eeprom") == 0)
	{
		db->game->hasEEPROM = TRUE;
		int sizeIndex = findv(attribute_names, "size");
		if (sizeIndex >= 0)
		{
			db->game->EEPROMSize = atoi(attribute_values[sizeIndex]);
		}
	}
	else if (g_strcmp0(element_name, "flash") == 0)
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
	
	if (g_strcmp0(element_name, "game") == 0)
	{
		db->isInGameTag = FALSE;
		if (db->foundCode)
		{
			db->gameLoaded = TRUE;
		}
	}
	else if (g_strcmp0(element_name, "title") == 0)
	{
		db->isInTitleTag = FALSE;
	}
	else if (g_strcmp0(element_name, "code") == 0)
	{
		db->isInCodeTag = FALSE;
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
	
	if (db->isInGameTag && db->isInTitleTag)
	{
		g_free(db->game->m_sTitle);
		db->game->m_sTitle = g_strdup(text);
	}
	else if (db->isInGameTag && db->isInCodeTag)
	{
		if (g_strcmp0(text, db->lookupCode) == 0) {
			db->foundCode = TRUE;

			g_free(db->game->m_sCode);
			db->game->m_sCode = g_strdup(text);
		}
	}
}

static gchar *game_db_get_file_path(const gchar *filename)
{
	// Use the data file from the source folder if it exists
	// to make vbam runnable without installation
	gchar *dataFilePath = g_build_filename("data", filename, NULL);
	if (!g_file_test(dataFilePath, G_FILE_TEST_EXISTS))
	{
		g_free(dataFilePath);
		dataFilePath = g_build_filename(PKGDATADIR, filename, NULL);
	}

	return dataFilePath;
}

GameInfos *game_db_lookup_code(const gchar *code)
{
	GameDBParserContext *db = g_new(GameDBParserContext, 1);
	
	db->game = game_infos_new();
	db->lookupCode = code;
	db->foundCode = FALSE;
	db->gameLoaded = FALSE;

	gchar *dbFilePath = game_db_get_file_path("game-db.xml");
	gchar *xmlData = NULL;
	gsize  length = 0;

	if (!g_file_get_contents(dbFilePath, &xmlData, &length, NULL)) {
		g_free(dbFilePath);
		return NULL;
	}

	GMarkupParser parser;
	parser.start_element = &on_start_element;
	parser.end_element = &on_end_element;
	parser.text = &on_text;
	parser.passthrough = NULL;
	parser.error = NULL;

	GMarkupParseContext *context = g_markup_parse_context_new(&parser, (GMarkupParseFlags)0, db, NULL);
	g_markup_parse_context_parse(context, xmlData, length, NULL);
	g_markup_parse_context_end_parse(context, NULL);
	g_markup_parse_context_free(context);

	g_free(xmlData);
	g_free(dbFilePath);

	GameInfos *game = NULL;

	if (db->gameLoaded)
		game = db->game;
	
	g_free(db);

	return game;
}
