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

#include "GameInfos.h"

#include "glib.h"

GameInfos *game_infos_new() {
	GameInfos *game = g_new(GameInfos, 1);

	game_infos_reset(game);

	return game;
}

void game_infos_free(GameInfos *game)
{
	if (game == NULL)
		return;

	g_free(game->m_sCode);
	g_free(game->m_sTitle);
	g_free(game);
}

void game_infos_reset(GameInfos *game)
{
	game->hasSRAM = FALSE;
	game->hasEEPROM = FALSE;
	game->hasFlash = FALSE;
	game->hasRTC = FALSE;
	game->EEPROMSize = 0x2000;
	game->flashSize = 0x10000;
	game->m_sTitle = NULL;
	game->m_sCode = NULL;
}
