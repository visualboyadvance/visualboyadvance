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

#include "ScreenAreaCairo.h"

#include <cstring>

namespace VBA
{

template<typename T> T min( T x, T y ) {
	return x < y ? x : y;
}
template<typename T> T max( T x, T y ) {
	return x > y ? x : y;
}

ScreenAreaCairo::ScreenAreaCairo(int _iWidth, int _iHeight, int _iScale) :
		ScreenArea(_iWidth, _iHeight, _iScale)
{
	vUpdateSize();
}

bool ScreenAreaCairo::on_draw(const Cairo::RefPtr<Cairo::Context> &poContext)
{
	DrawingArea::on_draw(poContext);

	Cairo::RefPtr< Cairo::ImageSurface >   poImage;
	Cairo::RefPtr< Cairo::SurfacePattern > poPattern;
	Cairo::Matrix oMatrix;
	const int iScaledPitch = m_iWidth * sizeof(u32);

	poContext->set_identity_matrix();
	poContext->scale(m_dScaleFactor, m_dScaleFactor);

	poImage = Cairo::ImageSurface::create((u8 *)m_puiPixels, Cairo::FORMAT_RGB24,
	                                      m_iWidth, m_iHeight, iScaledPitch);

	cairo_matrix_init_translate(&oMatrix, -m_iAreaLeft, -m_iAreaTop);
	poPattern = Cairo::SurfacePattern::create(poImage);
	poPattern->set_filter(Cairo::FILTER_NEAREST);
	poPattern->set_matrix (oMatrix);
	poContext->set_source_rgb(0.0, 0.0, 0.0);
	poContext->paint();

	poContext->set_source(poPattern);
	poContext->paint();

	return true;
}

void ScreenAreaCairo::vOnWidgetResize()
{
	m_dScaleFactor = min<double>(get_height() / (double)m_iHeight, get_width() / (double)m_iWidth);

	m_iAreaTop = (get_height() / m_dScaleFactor - m_iHeight) / 2;
	m_iAreaLeft = (get_width() / m_dScaleFactor - m_iWidth) / 2;
}

} // namespace VBA
