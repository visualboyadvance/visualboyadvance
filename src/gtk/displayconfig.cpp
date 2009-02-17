// -*- C++ -*-
// VisualBoyAdvance - Nintendo Gameboy/GameboyAdvance (TM) emulator.
// Copyright (C) 2008 VBA-M development team

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

#include "displayconfig.h"

#include <gtkmm/stock.h>
#include <gtkmm/frame.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/liststore.h>

#include "intl.h"

namespace VBA
{

DisplayConfigDialog::DisplayConfigDialog(GtkDialog* _pstDialog, const Glib::RefPtr<Gtk::Builder>& refBuilder) :
  Gtk::Dialog(_pstDialog),
  m_poConfig(0)
{
  refBuilder->get_widget("DefaultScaleComboBox", m_poDefaultScaleComboBox);
  refBuilder->get_widget("OutputOpenGL", m_poOutputOpenGLRadioButton);
  refBuilder->get_widget("OutputCairo", m_poOutputCairoRadioButton);

  m_poDefaultScaleComboBox->signal_changed().connect(sigc::mem_fun(*this, &DisplayConfigDialog::vOnScaleChanged));
  m_poOutputOpenGLRadioButton->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &DisplayConfigDialog::vOnOutputChanged), VBA::Window::OutputOpenGL));
  m_poOutputCairoRadioButton->signal_toggled().connect(sigc::bind(sigc::mem_fun(*this, &DisplayConfigDialog::vOnOutputChanged), VBA::Window::OutputCairo));
}

void DisplayConfigDialog::vSetConfig(Config::Section * _poConfig, VBA::Window * _poWindow)
{
  m_poConfig = _poConfig;
  m_poWindow = _poWindow;

  int iDefaultScale = m_poConfig->oGetKey<int>("scale");
  m_poDefaultScaleComboBox->set_active(iDefaultScale - 1);

  // Set the default output module
  VBA::Window::EVideoOutput _eOutput = (VBA::Window::EVideoOutput)m_poConfig->oGetKey<int>("output");
  switch (_eOutput)
  {
    case VBA::Window::OutputOpenGL:
      m_poOutputOpenGLRadioButton->set_active();
      break;
    default:
      m_poOutputCairoRadioButton->set_active();
      break;
  }
}

void DisplayConfigDialog::vOnOutputChanged(VBA::Window::EVideoOutput _eOutput)
{
  VBA::Window::EVideoOutput eOldOutput = (VBA::Window::EVideoOutput)m_poConfig->oGetKey<int>("output");

  if (_eOutput == eOldOutput)
    return;
  
  if (_eOutput == VBA::Window::OutputOpenGL && m_poOutputOpenGLRadioButton->get_active())
    m_poConfig->vSetKey("output", VBA::Window::OutputOpenGL);
  else
    m_poConfig->vSetKey("output", VBA::Window::OutputCairo);

  m_poWindow->vApplyConfigScreenArea();
}

void DisplayConfigDialog::vOnScaleChanged()
{
  int iScale = m_poDefaultScaleComboBox->get_active_row_number() + 1;
  if (iScale > 0)
  {
    m_poConfig->vSetKey("scale", iScale);
    m_poWindow->vUpdateScreen();
  }
}

} // namespace VBA
