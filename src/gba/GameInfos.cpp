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

namespace Cartridge
{

GameInfos::GameInfos()
{
    reset();
}

void GameInfos::reset()
{
	m_bHasSRAM = false;
	m_bHasEEPROM = false;
	m_bHasFlash = false;
	m_bHasRTC = false;
	m_iEEPROMSize = 0x2000;
	m_iFlashSize = 0x10000;
	m_bIsPresent = false;
}

const std::string& GameInfos::getTitle() const
{
	return m_sTitle;
}

const std::string& GameInfos::getRomDump() const
{
	return m_sRomDump;
}

const std::string& GameInfos::getBasePath() const
{
	return m_sBasePath;
}

bool GameInfos::hasSRAM() const
{
	return m_bHasSRAM;
}

bool GameInfos::hasFlash() const
{
	return m_bHasFlash;
}

bool GameInfos::hasEEPROM() const
{
	return m_bHasEEPROM;
}

bool GameInfos::hasRTC() const
{
	return m_bHasRTC;
}

int GameInfos::getEEPROMSize() const
{
	return m_iEEPROMSize;
}

int GameInfos::getFlashSize() const
{
	return m_iFlashSize;
}

bool GameInfos::isPresent() const
{
	return m_bIsPresent;
}

}
