// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 1999-2003 Forgotten
// Copyright (C) 2004 Forgotten and the VBA development team

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

#include "filters.h"


namespace VBA
{

struct {
  char m_csName[30];
  int m_iEnlargeFactor;
  Filter m_apvFunc[2];
}
static const astFilters[] =
{
  { "None",                1, { 0,         0           } }
};

struct {
  char m_csName[30];
  FilterIB m_apvFunc[2];
}
static const astFiltersIB[] =
{
  { "None",                      { 0,         0           } }
};

Filter pvGetFilter(EFilter _eFilter, EFilterDepth _eDepth)
{
  return astFilters[_eFilter].m_apvFunc[_eDepth];
}

char* pcsGetFilterName(const EFilter _eFilter)
{
        return (char*)astFilters[_eFilter].m_csName;
}

FilterIB pvGetFilterIB(EFilterIB _eFilterIB, EFilterDepth _eDepth)
{
  return astFiltersIB[_eFilterIB].m_apvFunc[_eDepth];
}

char* pcsGetFilterIBName(const EFilterIB _eFilterIB)
{
        return (char*)astFiltersIB[_eFilterIB].m_csName;
}

} // namespace VBA
