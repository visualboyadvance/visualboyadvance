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

#include "Window.h"

#include <SDL.h>

#include "../gba/Cartridge.h"
#include "../gba/GBA.h"
#include "../gba/Sound.h"

#include "InputSDL.h"

#include "Tools.h"
#include "Intl.h"
#include "JoypadConfigDialog.h"
#include "SettingsDialog.h"

extern int emulating;

namespace VBA
{

void Window::vOnMenuEnter()
{
	if (emulating && !m_bPaused)
	{
		vStopEmu();
		soundPause();
	}
}

void Window::vOnMenuExit()
{
	if (emulating && !m_bPaused)
	{
		vStartEmu();
		soundResume();
	}
	else
	{
		vDrawDefaultScreen();
	}
}

void Window::vOnFileOpen()
{
	while (m_poFileOpenDialog->run() == Gtk::RESPONSE_OK)
	{
		if (bLoadROM(m_poFileOpenDialog->get_filename()))
		{
			break;
		}
	}
	m_poFileOpenDialog->hide();
}

void Window::vOnFileLoad()
{
	std::string sSaveDir = m_sUserDataDir;

	Gtk::FileChooserDialog oDialog(*this, _("Load game"));
	oDialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	oDialog.add_button(Gtk::Stock::OPEN,   Gtk::RESPONSE_OK);

	if (sSaveDir != "")
	{
		oDialog.set_current_folder(sSaveDir);
		oDialog.add_shortcut_folder(sSaveDir);
	}

	const Glib::RefPtr<Gtk::FileFilter> oSaveFilter = Gtk::FileFilter::create();
	oSaveFilter->set_name(_("VisualBoyAdvance save game"));
	oSaveFilter->add_pattern("*.[sS][gG][mM]");

	oDialog.add_filter(oSaveFilter);

	while (oDialog.run() == Gtk::RESPONSE_OK)
	{
		if (CPUReadState(oDialog.get_filename().c_str()))
		{
			break;
		}
	}
}

void Window::vOnFileSave()
{
	Glib::ustring sSaveDir = m_sUserDataDir;

	Gtk::FileChooserDialog oDialog(*this, _("Save game"),
	                               Gtk::FILE_CHOOSER_ACTION_SAVE);
	oDialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	oDialog.add_button(Gtk::Stock::SAVE,   Gtk::RESPONSE_OK);

	if (sSaveDir != "")
	{
		oDialog.set_current_folder(sSaveDir);
		oDialog.add_shortcut_folder(sSaveDir);
	}

	oDialog.set_current_name(Cartridge::getGame().getTitle());

	const Glib::RefPtr<Gtk::FileFilter> oSaveFilter = Gtk::FileFilter::create();
	oSaveFilter->set_name(_("VisualBoyAdvance save game"));
	oSaveFilter->add_pattern("*.[sS][gG][mM]");

	oDialog.add_filter(oSaveFilter);

	while (oDialog.run() == Gtk::RESPONSE_OK)
	{
		Glib::ustring sFile = oDialog.get_filename();
		if (! bHasSuffix(sFile, ".sgm", false))
		{
			sFile += ".sgm";
		}

		if (Glib::file_test(sFile, Glib::FILE_TEST_EXISTS))
		{
			Gtk::MessageDialog oConfirmDialog(*this,
			                                  _("File already exists. Overwrite it?"),
			                                  false,
			                                  Gtk::MESSAGE_QUESTION,
			                                  Gtk::BUTTONS_YES_NO);
			if (oConfirmDialog.run() != Gtk::RESPONSE_YES)
			{
				continue;
			}
		}

		if (CPUWriteState(sFile.c_str()))
		{
			break;
		}
	}
}

void Window::vOnLoadGameMostRecent()
{
	int    iMostRecent = -1;
	time_t uiTimeMax = 0;

	for (int i = 0; i < 10; i++)
	{
		if (! m_astGameSlot[i].m_bEmpty
		        && (iMostRecent < 0 || m_astGameSlot[i].m_uiTime > uiTimeMax))
		{
			iMostRecent = i;
			uiTimeMax = m_astGameSlot[i].m_uiTime;
		}
	}

	if (iMostRecent >= 0)
	{
		vOnLoadGame(iMostRecent + 1);
	}
}

void Window::vOnLoadGame(int _iSlot)
{
	int i = _iSlot - 1;
	if (! m_astGameSlot[i].m_bEmpty)
	{
		CPUReadState(m_astGameSlot[i].m_sFile.c_str());
		m_poFilePauseItem->set_active(false);
	}
}

void Window::vOnSaveGameOldest()
{
	int    iOldest = -1;
	time_t uiTimeMin = 0;

	for (int i = 0; i < 10; i++)
	{
		if (! m_astGameSlot[i].m_bEmpty
		        && (iOldest < 0 || m_astGameSlot[i].m_uiTime < uiTimeMin))
		{
			iOldest = i;
			uiTimeMin = m_astGameSlot[i].m_uiTime;
		}
	}

	if (iOldest >= 0)
	{
		vOnSaveGame(iOldest + 1);
	}
	else
	{
		vOnSaveGame(1);
	}
}

void Window::vOnSaveGame(int _iSlot)
{
	int i = _iSlot - 1;
	CPUWriteState(m_astGameSlot[i].m_sFile.c_str());
	vUpdateGameSlots();
}

void Window::vOnFilePauseToggled(Gtk::CheckMenuItem * _poCMI)
{
	m_bPaused = _poCMI->get_active();
	if (emulating)
	{
		if (m_bPaused)
		{
			vStopEmu();
			soundPause();
		}
		else
		{
			vStartEmu();
			soundResume();
		}
	}
}

void Window::vOnFileReset()
{
	if (emulating)
	{
		CPUReset();
		m_poFilePauseItem->set_active(false);
	}
}

void Window::vOnRecentFile()
{
	Glib::ustring sURI = m_poRecentChooserMenu->get_current_uri();

	if (!sURI.empty())
	{
		std::string sFileName = Glib::filename_from_uri(sURI);
		bLoadROM(sFileName);
	}
}

void Window::vOnFileClose()
{
	if (Cartridge::isPresent())
	{
		soundPause();
		vStopEmu();
		vSetDefaultTitle();
		vDrawDefaultScreen();
		vSaveBattery();
		Cartridge::unloadGame();
		CPUCleanUp();
		emulating = 0;

		vUpdateGameSlots();

		for (std::list<Gtk::Widget *>::iterator it = m_listSensitiveWhenPlaying.begin();
		        it != m_listSensitiveWhenPlaying.end();
		        it++)
		{
			(*it)->set_sensitive(false);
		}

		m_poFilePauseItem->set_active(false);
	}
}

void Window::vOnFileExit()
{
	hide();
}

void Window::vOnVideoFullscreen()
{
	vToggleFullscreen();
}

void Window::vOnJoypadConfigure()
{
	std::string sUiFile = sGetUiFilePath("joypad.glade");
	Glib::RefPtr<Gtk::Builder> poBuilder = Gtk::Builder::create_from_file(sUiFile);

	JoypadConfigDialog * poDialog = 0;
	poBuilder->get_widget_derived("JoypadDialog", poDialog);
	poDialog->set_transient_for(*this);
	poDialog->run();
	poDialog->hide();

	vSaveJoypadsToConfig();
}

void Window::vOnSettings()
{
	std::string sUiFile = sGetUiFilePath("settings.glade");
	Glib::RefPtr<Gtk::Builder> poBuilder = Gtk::Builder::create_from_file(sUiFile);

	SettingsDialog * poDialog = 0;
	poBuilder->get_widget_derived("SettingsDialog", poDialog);
	poDialog->set_transient_for(*this);
	poDialog->run();
	poDialog->hide();
}

void Window::vOnHelpAbout()
{
	Gtk::AboutDialog oAboutDialog;
	const char csGPLHeader[] = "This program is free software: you can redistribute it and/or modify\n"
	                           "it under the terms of the GNU General Public License as published by\n"
	                           "the Free Software Foundation, either version 2 of the License, or\n"
	                           "(at your option) any later version.\n\n"
	                           "This program is distributed in the hope that it will be useful,\n"
	                           "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
	                           "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
	                           "GNU General Public License for more details.\n\n"
	                           "You should have received a copy of the GNU General Public License\n"
	                           "along with this program.  If not, see <http://www.gnu.org/licenses/>.";
	const char csCopyright[] = "Copyright (C) 1999-2003 Forgotten\n"
	                           "Copyright (C) 2004-2006 VBA development team\n"
	                           "Copyright (C) 2007-2009 VBA-M development team";

	oAboutDialog.set_transient_for(*this);

	oAboutDialog.set_name("VBA-M");
	oAboutDialog.set_version(VERSION);
	oAboutDialog.set_comments(_("Nintendo GameBoy Advance emulator."));
	oAboutDialog.set_license(csGPLHeader);
	oAboutDialog.set_copyright(csCopyright);
	oAboutDialog.set_logo_icon_name("vbam");

	oAboutDialog.set_website("http://vba-m.ngemu.com/");

	std::vector<Glib::ustring> list_authors;
	list_authors.push_back("Forgotten");
	list_authors.push_back("kxu");
	list_authors.push_back("Pokemonhacker");
	list_authors.push_back("Spacy51");
	list_authors.push_back("mudlord");
	list_authors.push_back("Nach");
	list_authors.push_back("jbo_85");
	list_authors.push_back("bgK");
	oAboutDialog.set_authors(list_authors);

	std::vector<Glib::ustring> list_artists;
	list_artists.push_back("Matteo Drera");
	list_artists.push_back("Jakub Steiner");
	list_artists.push_back("Jones Lee");
	oAboutDialog.set_artists(list_artists);

	oAboutDialog.run();
}

bool Window::bOnEmuIdle()
{
	vSDLPollEvents();

	CPULoop();
	return true;
}

void Window::vOnSettingsChanged(const Glib::ustring &key)
{
	if (key == "sound-volume")
		vApplyConfigVolume();
	else if (key == "sound-sample-rate")
		vApplyConfigSoundSampleRate();
	else if (key == "display-scale")
		vUpdateScreen();
	else if (key == "show-speed")
		vApplyConfigShowSpeed();
}

bool Window::on_focus_in_event(GdkEventFocus * _pstEvent)
{
	if (emulating && !m_bPaused)
	{
		vStartEmu();
		soundResume();
	}
	return false;
}

bool Window::on_focus_out_event(GdkEventFocus * _pstEvent)
{
	if (emulating
	        && ! m_bPaused
	        && m_poSettings->get_boolean("pause-when-inactive"))
	{
		vStopEmu();
		soundPause();
	}
	return false;
}

bool Window::on_key_press_event(GdkEventKey * _pstEvent)
{
	// The menu accelerators are disabled when it is hidden
	if (_pstEvent->keyval == GDK_KEY_F11 && !m_poMenuBar->get_visible())
	{
		vToggleFullscreen();
		return true;
	}

	// Forward the keyboard event to the input module by faking a SDL event
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = (SDLKey)_pstEvent->keyval;
	inputProcessSDLEvent(event);

	return Gtk::Window::on_key_press_event(_pstEvent);
}

bool Window::on_key_release_event(GdkEventKey * _pstEvent)
{
	// Forward the keyboard event to the input module by faking a SDL event
	SDL_Event event;
	event.type = SDL_KEYUP;
	event.key.keysym.sym = (SDLKey)_pstEvent->keyval;
	inputProcessSDLEvent(event);


	return Gtk::Window::on_key_release_event(_pstEvent);
}

bool Window::on_window_state_event(GdkEventWindowState* _pstEvent)
{
	if (_pstEvent->changed_mask & GDK_WINDOW_STATE_FULLSCREEN)
	{
		m_bFullscreen = _pstEvent->new_window_state & GDK_WINDOW_STATE_FULLSCREEN;
	}

	return true;
}

} // namespace VBA
