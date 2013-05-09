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

#include <sys/stat.h>

#include <SDL.h>

#include "../gba/Cartridge.h"
#include "../gba/GBA.h"
#include "../gba/Sound.h"
#include "../gba/Display.h"

#include "Intl.h"
#include "ScreenAreaCairo.h"

extern int emulating;

namespace VBA
{

Window * Window::m_poInstance = NULL;

const Window::SJoypadKey Window::m_astJoypad[] =
{
	{ "dpad-left",      KEY_LEFT           },
	{ "dpad-right",     KEY_RIGHT          },
	{ "dpad-up",        KEY_UP             },
	{ "dpad-down",      KEY_DOWN           },
	{ "button-a",       KEY_BUTTON_A       },
	{ "button-b",       KEY_BUTTON_B       },
	{ "button-select",  KEY_BUTTON_SELECT  },
	{ "button-start",   KEY_BUTTON_START   },
	{ "trigger-left",   KEY_BUTTON_L       },
	{ "trigger-right",  KEY_BUTTON_R       },
	{ "shortcut-speed", KEY_BUTTON_SPEED   },
	{ "autofire-a",     KEY_BUTTON_AUTO_A  },
	{ "autofire-b",     KEY_BUTTON_AUTO_B  }
};

const float Window::m_fSoundVolumeMin = 0.50f;
const float Window::m_fSoundVolumeMax = 2.00f;

Window::Window(GtkWindow * _pstWindow, const Glib::RefPtr<Gtk::Builder> & _poBuilder) :
		Gtk::Window       (_pstWindow),
		m_bFullscreen     (false)
{
	m_poBuilder        = _poBuilder;
	m_poFileOpenDialog = NULL;

	vInitSDL();
	vInitSystem();

	vSetDefaultTitle();

	// Path where the batteries and saves are stored
	m_sUserDataDir = Glib::get_user_data_dir() + "/gvbam";

	if (! Glib::file_test(m_sUserDataDir, Glib::FILE_TEST_EXISTS))
	{
		mkdir(m_sUserDataDir.c_str(), 0777);
	}

	m_poSettings = Gio::Settings::create("org.vba.ttb.preferences");
	m_poSettings->signal_changed().connect(sigc::mem_fun(*this, &Window::vOnSettingsChanged));

	m_poJoypadMapping = Gio::Settings::create("org.vba.ttb.joypad");

	vCheckConfig();
	vApplyConfigJoypads();
	vApplyConfigScreenArea();
	vApplyConfigVolume();
	vApplyConfigShowSpeed();
	vUpdateScreen();

	vCreateFileOpenDialog();

	Gtk::MenuItem *      poMI;

	// Menu bar
	_poBuilder->get_widget("MenuBar", m_poMenuBar);
	m_poMenuBar->signal_deactivate().connect(sigc::mem_fun(*this, &Window::vOnMenuExit));

	_poBuilder->get_widget("FileMenu", poMI);
	poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnMenuEnter));
	_poBuilder->get_widget("EmulationMenu", poMI);
	poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnMenuEnter));
	_poBuilder->get_widget("HelpMenu", poMI);
	poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnMenuEnter));

	// File menu
	//
	_poBuilder->get_widget("FileOpen", poMI);
	poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnFileOpen));

	_poBuilder->get_widget("FileLoad", poMI);
	poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnFileLoad));
	m_listSensitiveWhenPlaying.push_back(poMI);

	_poBuilder->get_widget("FileSave", poMI);
	poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnFileSave));
	m_listSensitiveWhenPlaying.push_back(poMI);

	for (int i = 0; i < 10; i++)
	{
		char csName[20];
		snprintf(csName, 20, "LoadGameSlot%d", i + 1);
		_poBuilder->get_widget(csName, m_apoLoadGameItem[i]);
		snprintf(csName, 20, "SaveGameSlot%d", i + 1);
		_poBuilder->get_widget(csName, m_apoSaveGameItem[i]);

		m_apoLoadGameItem[i]->signal_activate().connect(sigc::bind(
		            sigc::mem_fun(*this, &Window::vOnLoadGame),
		            i + 1));
		m_apoSaveGameItem[i]->signal_activate().connect(sigc::bind(
		            sigc::mem_fun(*this, &Window::vOnSaveGame),
		            i + 1));
	}
	vUpdateGameSlots();

	_poBuilder->get_widget("LoadGameMostRecent", poMI);
	poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnLoadGameMostRecent));
	m_listSensitiveWhenPlaying.push_back(poMI);

	_poBuilder->get_widget("SaveGameOldest", poMI);
	poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnSaveGameOldest));
	m_listSensitiveWhenPlaying.push_back(poMI);

	_poBuilder->get_widget("FilePause", m_poFilePauseItem);
	m_poFilePauseItem->set_active(false);
	vOnFilePauseToggled(m_poFilePauseItem);
	m_poFilePauseItem->signal_toggled().connect(sigc::bind(
	            sigc::mem_fun(*this, &Window::vOnFilePauseToggled),
	            m_poFilePauseItem));
	m_listSensitiveWhenPlaying.push_back(m_poFilePauseItem);

	_poBuilder->get_widget("FileReset", poMI);
	poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnFileReset));
	m_listSensitiveWhenPlaying.push_back(poMI);

	_poBuilder->get_widget("FileClose", poMI);
	poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnFileClose));
	m_listSensitiveWhenPlaying.push_back(poMI);

	_poBuilder->get_widget("FileExit", poMI);
	poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnFileExit));

	// Recent menu
	//
	m_poRecentManager = Gtk::RecentManager::get_default();

	Glib::RefPtr<Gtk::RecentFilter> oRecentFilter = Gtk::RecentFilter::create();
	oRecentFilter->add_application( Glib::get_application_name() );

	m_poRecentChooserMenu = Gtk::manage( new Gtk::RecentChooserMenu(m_poRecentManager) );
	m_poRecentChooserMenu->set_show_numbers();
	m_poRecentChooserMenu->set_show_tips();
	m_poRecentChooserMenu->set_local_only();
	m_poRecentChooserMenu->add_filter(oRecentFilter);
	m_poRecentChooserMenu->signal_item_activated().connect(
	    sigc::mem_fun(*this, &Window::vOnRecentFile));


	_poBuilder->get_widget("RecentMenu", m_poRecentMenu);
	m_poRecentMenu->set_submenu(static_cast<Gtk::Menu &>(*m_poRecentChooserMenu));

	// Preferences menu
	_poBuilder->get_widget("Preferences", poMI);
	poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnSettings));

	// Joypad menu
	//
	_poBuilder->get_widget("JoypadConfigure", poMI);
	poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnJoypadConfigure));

	// Fullscreen menu
	//
	_poBuilder->get_widget("VideoFullscreen", poMI);
	poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnVideoFullscreen));

	// Help menu
	//
	_poBuilder->get_widget("HelpAbout", poMI);
	poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnHelpAbout));

	// Init widgets sensitivity
	for (std::list<Gtk::Widget *>::iterator it = m_listSensitiveWhenPlaying.begin();
	        it != m_listSensitiveWhenPlaying.end();
	        it++)
	{
		(*it)->set_sensitive(false);
	}

	if (m_poInstance == NULL)
	{
		m_poInstance = this;
	}
	else
	{
		abort();
	}
}

Window::~Window()
{
	vOnFileClose();
	vUnInitSystem();

	if (m_poFileOpenDialog != NULL)
	{
		delete m_poFileOpenDialog;
	}

	m_poInstance = NULL;
}

void Window::vInitColors()
{
	// RGB color order
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
	Display::initColorMap(19, 11, 3);
#else
	Display::initColorMap(11, 19, 27);
#endif
}

void Window::vApplyConfigScreenArea()
{
	Gtk::Alignment * poC;

	m_poBuilder->get_widget("ScreenContainer", poC);
	poC->remove();
	poC->set(Gtk::ALIGN_CENTER, Gtk::ALIGN_CENTER, 1.0, 1.0);

	vInitColors();
	m_poScreenArea = Gtk::manage(new ScreenAreaCairo(m_iGBAScreenWidth, m_iGBAScreenHeight));

	poC->add(*m_poScreenArea);
	vDrawDefaultScreen();
	m_poScreenArea->show();
}

void Window::vInitSystem()
{
	systemVerbose = 0 //| VERBOSE_SWI
	                //	| VERBOSE_UNALIGNED_MEMORY
	                | VERBOSE_ILLEGAL_WRITE | VERBOSE_ILLEGAL_READ
	                //	| VERBOSE_DMA0 | VERBOSE_DMA1 | VERBOSE_DMA2 | VERBOSE_DMA3
	                | VERBOSE_UNDEFINED | VERBOSE_AGBPRINT | VERBOSE_SOUNDOUTPUT;

	emulating = 0;

	soundInit();
}

void Window::vUnInitSystem()
{
	soundShutdown();
}

void Window::vInitSDL()
{
	static bool bDone = false;

	if (bDone)
		return;

	int iFlags = (SDL_INIT_EVERYTHING | SDL_INIT_NOPARACHUTE);

	if (SDL_Init(iFlags) < 0)
	{
		fprintf(stderr, "Failed to init SDL: %s", SDL_GetError());
		abort();
	}

	// TODO : remove
	int sdlNumDevices = SDL_NumJoysticks();
	for (int i = 0; i < sdlNumDevices; i++)
		SDL_JoystickOpen(i);

	inputInitJoysticks();

	bDone = true;
}

void Window::vCheckConfig()
{
	std::string sValue;

	// Directories section
	//
	sValue = m_poSettings->get_string("gba-roms-dir");
	if (sValue != "" && !Glib::file_test(sValue, Glib::FILE_TEST_IS_DIR))
	{
		m_poSettings->set_string("gba-roms-dir", Glib::get_home_dir());
	}

	// Core section
	//
	sValue = m_poSettings->get_string("gba-bios-path");
	if (sValue != "" && !Glib::file_test(sValue, Glib::FILE_TEST_IS_REGULAR))
	{
		m_poSettings->set_string("gba-bios-path", "");
	}
}

void Window::vApplyConfigVolume()
{
	int iSoundVolume = m_poSettings->get_enum("sound-volume");
	soundSetVolume(static_cast<float>(iSoundVolume) / 100.0);
}

void Window::vApplyConfigSoundSampleRate()
{
	int iSoundSampleRate = m_poSettings->get_enum("sound-sample-rate");
	soundSetSampleRate(iSoundSampleRate);
}

void Window::vApplyConfigShowSpeed()
{
	m_bShowSpeed = m_poSettings->get_boolean("show-speed");

	if (!m_bShowSpeed)
	{
		vSetDefaultTitle();
	}
}

void Window::vHistoryAdd(const std::string & _rsFile)
{
	std::string sURL = "file://" + _rsFile;

	m_poRecentManager->add_item(sURL);
}

void Window::vApplyConfigJoypads()
{
	for (guint j = 0; j < G_N_ELEMENTS(m_astJoypad); j++)
	{
		int iKeyMap = m_poJoypadMapping->get_int(m_astJoypad[j].m_csKey);
		inputSetKeymap(m_astJoypad[j].m_eKeyFlag, iKeyMap);
	}
}

void Window::vSaveJoypadsToConfig()
{
	for (guint j = 0; j < G_N_ELEMENTS(m_astJoypad); j++)
	{
		int iKeyMap = inputGetKeymap(m_astJoypad[j].m_eKeyFlag);
		m_poJoypadMapping->set_int(m_astJoypad[j].m_csKey, iKeyMap);
	}
}

void Window::vUpdateScreen()
{
	m_poScreenArea->vSetSize(m_iGBAScreenWidth, m_iGBAScreenHeight);
	m_poScreenArea->vSetScale(m_poSettings->get_enum("display-scale"));

	resize(1, 1);

	if (emulating)
	{
		Display::drawScreen();
	}
	else
	{
		vDrawDefaultScreen();
	}
}

bool Window::bLoadROM(const std::string & _rsFile)
{
	vOnFileClose();

	if (!CPUInitMemory())
		return false;

	if (!Cartridge::loadRom(_rsFile))
	{
		CPUCleanUp();
		return false;
	}

	if (m_poSettings->get_string("gba-bios-path") == "")
	{
		vPopupError(_("Please choose a bios file in the preferences dialog."));
		return false;
	}

	if (!CPULoadBios(m_poSettings->get_string("gba-bios-path").c_str()))
	{
		return false;
	}

	CPUInit();
	CPUReset();

	vLoadBattery();
	vUpdateScreen();

	emulating = 1;

	vApplyConfigSoundSampleRate();

	vUpdateGameSlots();
	vHistoryAdd(_rsFile);

	for (std::list<Gtk::Widget *>::iterator it = m_listSensitiveWhenPlaying.begin();
	        it != m_listSensitiveWhenPlaying.end();
	        it++)
	{
		(*it)->set_sensitive();
	}

	vStartEmu();

	return true;
}

void Window::vPopupError(const char * _csFormat, ...)
{
	va_list args;
	va_start(args, _csFormat);
	char * csMsg = g_strdup_vprintf(_csFormat, args);
	va_end(args);

	Gtk::MessageDialog oDialog(*this,
	                           csMsg,
	                           false,
	                           Gtk::MESSAGE_ERROR,
	                           Gtk::BUTTONS_OK);
	oDialog.run();
	g_free(csMsg);
}

void Window::vPopupErrorV(const char * _csFormat, va_list _args)
{
	char * csMsg = g_strdup_vprintf(_csFormat, _args);

	Gtk::MessageDialog oDialog(*this,
	                           csMsg,
	                           false,
	                           Gtk::MESSAGE_ERROR,
	                           Gtk::BUTTONS_OK);
	oDialog.run();
	g_free(csMsg);
}

void Window::vDrawScreen(u32 *pix)
{
	m_poScreenArea->vDrawPixels(pix);
}

void Window::vDrawDefaultScreen()
{
	m_poScreenArea->vDrawBlackScreen();
}

void Window::vSetDefaultTitle()
{
	set_title("VBA-M");
}

void Window::vShowSpeed(int _iSpeed)
{
	if (m_bShowSpeed)
	{
		char csTitle[50];

		snprintf(csTitle, 50, "VBA-M - %d%%",
		         _iSpeed);
		set_title(csTitle);
	}
}

void Window::vCreateFileOpenDialog()
{
	if (m_poFileOpenDialog != NULL)
	{
		return;
	}

	std::string sGBADir = m_poSettings->get_string("gba-roms-dir");

	Gtk::FileChooserDialog * poDialog = new Gtk::FileChooserDialog(*this, _("Open"));
	poDialog->add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
	poDialog->add_button(Gtk::Stock::OPEN,   Gtk::RESPONSE_OK);

	try
	{
		if (sGBADir != "")
		{
			poDialog->add_shortcut_folder(sGBADir);
			poDialog->set_current_folder(sGBADir);
		}
	}
	catch (const Gtk::FileChooserError& e)
	{
		// Most likely the shortcut already exists, so do nothing
	}

	const Glib::RefPtr<Gtk::FileFilter> oGBAFilter = Gtk::FileFilter::create();
	oGBAFilter->set_name(_("Gameboy Advance files"));
	oGBAFilter->add_pattern("*.[gG][bB][aA]");

	const Glib::RefPtr<Gtk::FileFilter> oAllFilter = Gtk::FileFilter::create();
	oAllFilter->set_name(_("All files"));
	oAllFilter->add_pattern("*.*");

	poDialog->add_filter(oGBAFilter);
	poDialog->add_filter(oAllFilter);

	m_poFileOpenDialog = poDialog;
}

void Window::vLoadBattery()
{
	std::string sBattery = Glib::build_filename(m_sUserDataDir, Cartridge::getGame().getTitle() + ".sav");

	if (Cartridge::readBatteryFromFile(sBattery.c_str()))
	{
		systemScreenMessage(_("Loaded battery"));
	}
}

void Window::vSaveBattery()
{
	std::string sBattery = Glib::build_filename(m_sUserDataDir, Cartridge::getGame().getTitle() + ".sav");

	if (Cartridge::writeBatteryToFile(sBattery.c_str()))
	{
		systemScreenMessage(_("Saved battery"));
	}
}

void Window::vStartEmu()
{
	if (m_oEmuSig.connected())
	{
		return;
	}

	m_oEmuSig = Glib::signal_idle().connect(sigc::mem_fun(*this, &Window::bOnEmuIdle),
	                                        Glib::PRIORITY_HIGH_IDLE + 30);
}

void Window::vStopEmu()
{
	m_oEmuSig.disconnect();
}

void Window::vUpdateGameSlots()
{
	if (!Cartridge::isPresent())
	{
		std::string sDateTime = _("----/--/-- --:--:--");

		for (int i = 0; i < 10; i++)
		{
			char csPrefix[10];
			snprintf(csPrefix, sizeof(csPrefix), "%2d ", i + 1);

			Gtk::Label * poLabel;
			poLabel = dynamic_cast<Gtk::Label *>(m_apoLoadGameItem[i]->get_child());
			poLabel->set_text(csPrefix + sDateTime);
			m_apoLoadGameItem[i]->set_sensitive(false);

			poLabel = dynamic_cast<Gtk::Label *>(m_apoSaveGameItem[i]->get_child());
			poLabel->set_text(csPrefix + sDateTime);
			m_apoSaveGameItem[i]->set_sensitive(false);

			m_astGameSlot[i].m_bEmpty = true;
		}
	}
	else
	{
		std::string sFileBase = Glib::build_filename(m_sUserDataDir, Cartridge::getGame().getTitle());

		const char * csDateFormat = _("%Y/%m/%d %H:%M:%S");

		for (int i = 0; i < 10; i++)
		{
			char csPrefix[10];
			snprintf(csPrefix, sizeof(csPrefix), "%2d ", i + 1);

			char csSlot[10];
			snprintf(csSlot, sizeof(csSlot), "%d", i + 1);
			m_astGameSlot[i].m_sFile = sFileBase + csSlot + ".sgm";

			std::string sDateTime;
			struct stat stStat;
			if (stat(m_astGameSlot[i].m_sFile.c_str(), &stStat) == -1)
			{
				sDateTime = _("----/--/-- --:--:--");
				m_astGameSlot[i].m_bEmpty = true;
			}
			else
			{
				char csDateTime[30];
				strftime(csDateTime, sizeof(csDateTime), csDateFormat,
				         localtime(&stStat.st_mtime));
				sDateTime = csDateTime;
				m_astGameSlot[i].m_bEmpty = false;
				m_astGameSlot[i].m_uiTime = stStat.st_mtime;
			}

			Gtk::Label * poLabel;
			poLabel = dynamic_cast<Gtk::Label *>(m_apoLoadGameItem[i]->get_child());
			poLabel->set_text(csPrefix + sDateTime);
			m_apoLoadGameItem[i]->set_sensitive(! m_astGameSlot[i].m_bEmpty);

			poLabel = dynamic_cast<Gtk::Label *>(m_apoSaveGameItem[i]->get_child());
			poLabel->set_text(csPrefix + sDateTime);
			m_apoSaveGameItem[i]->set_sensitive();
		}
	}
}

void Window::vToggleFullscreen()
{
	if (!m_bFullscreen)
	{
		fullscreen();
		m_poMenuBar->hide();
	}
	else
	{
		unfullscreen();
		m_poMenuBar->show();
	}
}

void Window::vSDLPollEvents()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_JOYHATMOTION:
		case SDL_JOYBUTTONDOWN:
		case SDL_JOYBUTTONUP:
		case SDL_JOYAXISMOTION:
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			inputProcessSDLEvent(event);
			break;
		}
	}
}

std::string Window::sGetUiFilePath(const std::string &_sFileName)
{
	// Use the ui file from the source folder if it exists
	// to make gvbam runnable without installation
	std::string sUiFile = "data/gtk/ui/" + _sFileName;
	if (!Glib::file_test(sUiFile, Glib::FILE_TEST_EXISTS))
	{
		sUiFile = PKGDATADIR "/ui/" + _sFileName;
	}

	return sUiFile;
}

} // VBA namespace
