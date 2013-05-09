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

#include "SettingsDialog.h"
#include "Intl.h"
#include <giomm.h>

namespace VBA
{

SettingsDialog::SettingsDialog(GtkDialog* _pstDialog, const Glib::RefPtr<Gtk::Builder>& refBuilder) :
		Gtk::Dialog(_pstDialog)
{
	Gtk::ComboBox *poVolumeComboBox = 0;
	Gtk::ComboBox *poRateComboBox = 0;
	Gtk::ComboBox *poDefaultScaleComboBox = 0;
	Gtk::CheckButton *poShowSpeedCheckButton = 0;
	Gtk::CheckButton *poPauseOnInactiveCheckButton = 0;

	refBuilder->get_widget("VolumeComboBox", poVolumeComboBox);
	refBuilder->get_widget("RateComboBox", poRateComboBox);
	refBuilder->get_widget("DefaultScaleComboBox", poDefaultScaleComboBox);
	refBuilder->get_widget("ShowSpeedCheckButton", poShowSpeedCheckButton);
	refBuilder->get_widget("BiosFileChooserButton", m_poBiosFileChooserButton);
	refBuilder->get_widget("RomsFileChooserButton", m_poRomsFileChooserButton);
	refBuilder->get_widget("PauseOnInactiveCheckButton", poPauseOnInactiveCheckButton);

	m_oSettings = Gio::Settings::create("org.vba.ttb.preferences");

	m_oSettings->bind("sound-volume", poVolumeComboBox->property_active_id(), Gio::SETTINGS_BIND_DEFAULT);
	m_oSettings->bind("sound-sample-rate", poRateComboBox->property_active_id(), Gio::SETTINGS_BIND_DEFAULT);
	m_oSettings->bind("display-scale", poDefaultScaleComboBox->property_active_id(), Gio::SETTINGS_BIND_DEFAULT);

	m_oSettings->bind("show-speed", poShowSpeedCheckButton->property_active(), Gio::SETTINGS_BIND_DEFAULT);
	m_oSettings->bind("pause-when-inactive", poPauseOnInactiveCheckButton->property_active(), Gio::SETTINGS_BIND_DEFAULT);

	// Can't use bind with FileChooserButton
	m_poBiosFileChooserButton->signal_file_set().connect(sigc::mem_fun(*this, &SettingsDialog::vOnBiosChanged));
	m_poRomsFileChooserButton->signal_file_set().connect(sigc::mem_fun(*this, &SettingsDialog::vOnRomsChanged));

	// Bios FileChooserButton
	const char * acsPattern[] =
	{
		"*.[bB][iI][nN]", "*.[aA][gG][bB]", "*.[gG][bB][aA]",
		"*.[bB][iI][oO][sS]", "*.[zZ][iI][pP]", "*.[zZ]", "*.[gG][zZ]"
	};

	const Glib::RefPtr<Gtk::FileFilter> oAllFilter = Gtk::FileFilter::create();
	oAllFilter->set_name(_("All files"));
	oAllFilter->add_pattern("*");

	const Glib::RefPtr<Gtk::FileFilter> oBiosFilter = Gtk::FileFilter::create();
	oBiosFilter->set_name(_("Gameboy Advance BIOS"));
	for (guint i = 0; i < G_N_ELEMENTS(acsPattern); i++)
	{
		oBiosFilter->add_pattern(acsPattern[i]);
	}

	m_poBiosFileChooserButton->add_filter(oAllFilter);
	m_poBiosFileChooserButton->add_filter(oBiosFilter);
	m_poBiosFileChooserButton->set_filter(oBiosFilter);

	Glib::ustring sBios = m_oSettings->get_string("gba-bios-path");
	m_poBiosFileChooserButton->set_filename(sBios);

	// Roms FileChooserButton
	Glib::ustring sRoms = m_oSettings->get_string("gba-roms-dir");
	m_poRomsFileChooserButton->set_current_folder(sRoms);
}

void SettingsDialog::vOnBiosChanged()
{
	Glib::ustring sBios = m_poBiosFileChooserButton->get_filename();
	m_oSettings->set_string("gba-bios-path", sBios);
}

void SettingsDialog::vOnRomsChanged()
{
	Glib::ustring sRoms = m_poRomsFileChooserButton->get_current_folder();
	m_oSettings->set_string("gba-roms-dir", sRoms);
}

} // namespace VBA
