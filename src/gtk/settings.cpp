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

#include "settings.h"

#include <gtkmm/stock.h>
#include <gtkmm/frame.h>
#include <gtkmm/liststore.h>

#include "intl.h"

namespace VBA
{

SettingsDialog::SettingsDialog(GtkDialog* _pstDialog, const Glib::RefPtr<Gtk::Builder>& refBuilder) :
  Gtk::Dialog(_pstDialog),
  m_poSoundConfig(0),
  m_poDisplayConfig(0)
{
  refBuilder->get_widget("VolumeComboBox", m_poVolumeComboBox);
  refBuilder->get_widget("RateComboBox", m_poRateComboBox);
  refBuilder->get_widget("DefaultScaleComboBox", m_poDefaultScaleComboBox);
  refBuilder->get_widget("OpenGLCheckButton", m_poUseOpenGLCheckButton);
  refBuilder->get_widget("ShowSpeedCheckButton", m_poShowSpeedCheckButton);

  m_poVolumeComboBox->signal_changed().connect(sigc::mem_fun(*this, &SettingsDialog::vOnVolumeChanged));
  m_poRateComboBox->signal_changed().connect(sigc::mem_fun(*this, &SettingsDialog::vOnRateChanged));
  m_poDefaultScaleComboBox->signal_changed().connect(sigc::mem_fun(*this, &SettingsDialog::vOnScaleChanged));
  m_poUseOpenGLCheckButton->signal_toggled().connect(sigc::mem_fun(*this, &SettingsDialog::vOnOutputChanged));
  m_poShowSpeedCheckButton->signal_toggled().connect(sigc::mem_fun(*this, &SettingsDialog::vOnShowSpeedChanged));
}

void SettingsDialog::vSetConfig(Config::Section * _poSoundConfig, Config::Section * _poDisplayConfig, VBA::Window * _poWindow)
{
  m_poSoundConfig = _poSoundConfig;
  m_poDisplayConfig = _poDisplayConfig;
  m_poWindow = _poWindow;

  float fSoundVolume = m_poSoundConfig->oGetKey<float>("volume");

  if (0.0f <= fSoundVolume && fSoundVolume <= 0.25f)
    m_poVolumeComboBox->set_active(1);
  else if (0.25f < fSoundVolume && fSoundVolume <= 0.50f)
    m_poVolumeComboBox->set_active(2);
  else if (1.0f < fSoundVolume && fSoundVolume <= 2.0f)
    m_poVolumeComboBox->set_active(4);
  else
    m_poVolumeComboBox->set_active(3);

  long iSoundSampleRate = m_poSoundConfig->oGetKey<long>("sample_rate");
  switch (iSoundSampleRate)
  {
    case 11025:
      m_poRateComboBox->set_active(0);
      break;
    case 22050:
      m_poRateComboBox->set_active(1);
      break;
    default:
    case 44100:
      m_poRateComboBox->set_active(2);
      break;
    case 48000:
      m_poRateComboBox->set_active(3);
      break;
  }
  
  int iDefaultScale = m_poDisplayConfig->oGetKey<int>("scale");
  m_poDefaultScaleComboBox->set_active(iDefaultScale - 1);

  bool bOpenGL = m_poDisplayConfig->oGetKey<bool>("use_opengl");
  m_poUseOpenGLCheckButton->set_active(bOpenGL);
  
  bool bShowSpeed = m_poDisplayConfig->oGetKey<bool>("show_speed");
  m_poShowSpeedCheckButton->set_active(bShowSpeed);
}

void SettingsDialog::vOnVolumeChanged()
{
  int iVolume = m_poVolumeComboBox->get_active_row_number();
  switch (iVolume)
  {
    case 1: // 25 %
      m_poSoundConfig->vSetKey("volume", 0.25f);
      break;
    case 2: // 50 %
      m_poSoundConfig->vSetKey("volume", 0.50f);
      break;
    case 4: // 200 %
      m_poSoundConfig->vSetKey("volume", 2.00f);
      break;
    case 3: // 100 %
    default:
      m_poSoundConfig->vSetKey("volume", 1.00f);
      break;
  }

  m_poWindow->vApplyConfigVolume();
}

void SettingsDialog::vOnRateChanged()
{
  int iRate = m_poRateComboBox->get_active_row_number();
  switch (iRate)
  {
    case 0: // 11 KHz
      m_poSoundConfig->vSetKey("sample_rate", 11025);
      break;
    case 1: // 22 KHz
      m_poSoundConfig->vSetKey("sample_rate", 22050);
      break;
    case 2: // 44 KHz
    default:
      m_poSoundConfig->vSetKey("sample_rate", 44100);
      break;
    case 3: // 48 KHz
      m_poSoundConfig->vSetKey("sample_rate", 48000);
      break;
  }

  m_poWindow->vApplyConfigSoundSampleRate();
}

void SettingsDialog::vOnOutputChanged()
{
  bool bOldOpenGL = m_poDisplayConfig->oGetKey<bool>("use_opengl");
  bool bNewOpenGL = m_poUseOpenGLCheckButton->get_active();

  m_poDisplayConfig->vSetKey("use_opengl", bNewOpenGL);

  if (bNewOpenGL != bOldOpenGL)
  {
    m_poWindow->vApplyConfigScreenArea();
  }
}

void SettingsDialog::vOnScaleChanged()
{
  int iScale = m_poDefaultScaleComboBox->get_active_row_number() + 1;
  if (iScale > 0)
  {
    m_poDisplayConfig->vSetKey("scale", iScale);
    m_poWindow->vUpdateScreen();
  }
}

void SettingsDialog::vOnShowSpeedChanged()
{
  bool bShowSpeed = m_poShowSpeedCheckButton->get_active();

  m_poDisplayConfig->vSetKey("show_speed", bShowSpeed);
  m_poWindow->vApplyConfigShowSpeed();
}

} // namespace VBA
