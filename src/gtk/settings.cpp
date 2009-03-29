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
#include "intl.h"

namespace VBA
{

SettingsDialog::SettingsDialog(GtkDialog* _pstDialog, const Glib::RefPtr<Gtk::Builder>& refBuilder) :
  Gtk::Dialog(_pstDialog),
  m_poCoreConfig(0),
  m_poSoundConfig(0),
  m_poDisplayConfig(0),
  m_poDirConfig(0)
{
  refBuilder->get_widget("VolumeComboBox", m_poVolumeComboBox);
  refBuilder->get_widget("RateComboBox", m_poRateComboBox);
  refBuilder->get_widget("DefaultScaleComboBox", m_poDefaultScaleComboBox);
  refBuilder->get_widget("OpenGLCheckButton", m_poUseOpenGLCheckButton);
  refBuilder->get_widget("ShowSpeedCheckButton", m_poShowSpeedCheckButton);
  refBuilder->get_widget("BiosFileChooserButton", m_poBiosFileChooserButton);
  refBuilder->get_widget("RomsFileChooserButton", m_poRomsFileChooserButton);
  refBuilder->get_widget("BatteriesFileChooserButton", m_poBatteriesFileChooserButton);
  refBuilder->get_widget("SavesFileChooserButton", m_poSavesFileChooserButton);


  m_poVolumeComboBox->signal_changed().connect(sigc::mem_fun(*this, &SettingsDialog::vOnVolumeChanged));
  m_poRateComboBox->signal_changed().connect(sigc::mem_fun(*this, &SettingsDialog::vOnRateChanged));
  m_poDefaultScaleComboBox->signal_changed().connect(sigc::mem_fun(*this, &SettingsDialog::vOnScaleChanged));
  m_poUseOpenGLCheckButton->signal_toggled().connect(sigc::mem_fun(*this, &SettingsDialog::vOnOutputChanged));
  m_poShowSpeedCheckButton->signal_toggled().connect(sigc::mem_fun(*this, &SettingsDialog::vOnShowSpeedChanged));
  m_poBiosFileChooserButton->signal_file_set().connect(sigc::mem_fun(*this, &SettingsDialog::vOnBiosChanged));
  m_poRomsFileChooserButton->signal_file_set().connect(sigc::mem_fun(*this, &SettingsDialog::vOnRomsChanged));
  m_poBatteriesFileChooserButton->signal_file_set().connect(sigc::mem_fun(*this, &SettingsDialog::vOnBatteriesChanged));
  m_poSavesFileChooserButton->signal_file_set().connect(sigc::mem_fun(*this, &SettingsDialog::vOnSavesChanged));  

  // Bios FileChooserButton
  const char * acsPattern[] =
  {
    "*.[bB][iI][nN]", "*.[aA][gG][bB]", "*.[gG][bB][aA]",
    "*.[bB][iI][oO][sS]", "*.[zZ][iI][pP]", "*.[zZ]", "*.[gG][zZ]"
  };

  Gtk::FileFilter oAllFilter;
  oAllFilter.set_name(_("All files"));
  oAllFilter.add_pattern("*");

  Gtk::FileFilter oBiosFilter;
  oBiosFilter.set_name(_("Gameboy Advance BIOS"));
  for (guint i = 0; i < G_N_ELEMENTS(acsPattern); i++)
  {
    oBiosFilter.add_pattern(acsPattern[i]);
  }

  m_poBiosFileChooserButton->add_filter(oAllFilter);
  m_poBiosFileChooserButton->add_filter(oBiosFilter);
  m_poBiosFileChooserButton->set_filter(oBiosFilter);
}

void SettingsDialog::vSetConfig(Config::Section * _poSoundConfig, Config::Section * _poDisplayConfig, 
    Config::Section * _poCoreConfig, Config::Section * _poDirConfig, VBA::Window * _poWindow)
{
  m_poSoundConfig = _poSoundConfig;
  m_poDisplayConfig = _poDisplayConfig;
  m_poCoreConfig = _poCoreConfig;
  m_poDirConfig = _poDirConfig;
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
  
  std::string sBios = m_poCoreConfig->sGetKey("bios_file");
  m_poBiosFileChooserButton->set_filename(sBios);

  std::string sRoms = m_poDirConfig->sGetKey("gba_roms");
  m_poRomsFileChooserButton->set_current_folder(sRoms);

  std::string sBatteries = m_poDirConfig->sGetKey("batteries");
  m_poBatteriesFileChooserButton->set_current_folder(sBatteries);

  std::string sSaves = m_poDirConfig->sGetKey("saves");
  m_poSavesFileChooserButton->set_current_folder(sSaves);
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

void SettingsDialog::vOnBiosChanged()
{
  std::string sBios = m_poBiosFileChooserButton->get_filename();
  m_poCoreConfig->vSetKey("bios_file", sBios);
}

void SettingsDialog::vOnRomsChanged()
{
  std::string sRoms = m_poRomsFileChooserButton->get_current_folder();
  m_poDirConfig->vSetKey("gba_roms", sRoms);
}

void SettingsDialog::vOnBatteriesChanged()
{
  std::string sBatteries = m_poBatteriesFileChooserButton->get_current_folder();
  m_poDirConfig->vSetKey("batteries", sBatteries);
}

void SettingsDialog::vOnSavesChanged()
{
  std::string sSaves = m_poSavesFileChooserButton->get_current_folder();
  m_poDirConfig->vSetKey("saves", sSaves);
  
  // Needed if saves dir changed
  m_poWindow->vUpdateGameSlots();
}

} // namespace VBA
