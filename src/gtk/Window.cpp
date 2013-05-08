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

#include "Tools.h"
#include "Intl.h"
#include "ScreenAreaCairo.h"

extern int emulating;

namespace VBA
{

Window * Window::m_poInstance = NULL;

const Window::SJoypadKey Window::m_astJoypad[] =
{
	{ "left",    KEY_LEFT           },
	{ "right",   KEY_RIGHT          },
	{ "up",      KEY_UP             },
	{ "down",    KEY_DOWN           },
	{ "A",       KEY_BUTTON_A       },
	{ "B",       KEY_BUTTON_B       },
	{ "select",  KEY_BUTTON_SELECT  },
	{ "start",   KEY_BUTTON_START   },
	{ "L",       KEY_BUTTON_L       },
	{ "R",       KEY_BUTTON_R       },
	{ "speed",   KEY_BUTTON_SPEED   },
	{ "autoA",   KEY_BUTTON_AUTO_A  },
	{ "autoB",   KEY_BUTTON_AUTO_B  }
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

	// Get config
	//
	m_sUserDataDir = Glib::get_user_config_dir() + "/gvbam";
	m_sConfigFile  = m_sUserDataDir + "/config";

	vInitConfig();

	if (! Glib::file_test(m_sUserDataDir, Glib::FILE_TEST_EXISTS))
	{
		mkdir(m_sUserDataDir.c_str(), 0777);
	}
	if (Glib::file_test(m_sConfigFile, Glib::FILE_TEST_EXISTS))
	{
		vLoadConfig(m_sConfigFile);
		vCheckConfig();
	}
	else
	{
		vSaveConfig(m_sConfigFile);
	}

	vCreateFileOpenDialog();
	vApplyConfigJoypads();
	vApplyConfigScreenArea();
	vApplyConfigVolume();
	vApplyConfigShowSpeed();
	vUpdateScreen();

	Gtk::MenuItem *      poMI;
	Gtk::CheckMenuItem * poCMI;

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

	_poBuilder->get_widget("LoadGameAuto", poCMI);
	poCMI->set_active(m_poCoreConfig->oGetKey<bool>("load_game_auto"));
	vOnLoadGameAutoToggled(poCMI);
	poCMI->signal_toggled().connect(sigc::bind(
	                                    sigc::mem_fun(*this, &Window::vOnLoadGameAutoToggled),
	                                    poCMI));

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
	vSaveJoypadsToConfig();
	vSaveConfig(m_sConfigFile);

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

	inputSetKeymap(KEY_LEFT, GDK_KEY_Left);
	inputSetKeymap(KEY_RIGHT, GDK_KEY_Right);
	inputSetKeymap(KEY_UP, GDK_KEY_Up);
	inputSetKeymap(KEY_DOWN, GDK_KEY_Down);
	inputSetKeymap(KEY_BUTTON_A, GDK_KEY_z);
	inputSetKeymap(KEY_BUTTON_B, GDK_KEY_x);
	inputSetKeymap(KEY_BUTTON_START, GDK_KEY_Return);
	inputSetKeymap(KEY_BUTTON_SELECT, GDK_KEY_BackSpace);
	inputSetKeymap(KEY_BUTTON_L, GDK_KEY_a);
	inputSetKeymap(KEY_BUTTON_R, GDK_KEY_s);
	inputSetKeymap(KEY_BUTTON_SPEED, GDK_KEY_space);
	inputSetKeymap(KEY_BUTTON_CAPTURE, GDK_KEY_F12);
	inputSetKeymap(KEY_BUTTON_AUTO_A, GDK_KEY_q);
	inputSetKeymap(KEY_BUTTON_AUTO_B, GDK_KEY_w);

	// TODO : remove
	int sdlNumDevices = SDL_NumJoysticks();
	for (int i = 0; i < sdlNumDevices; i++)
		SDL_JoystickOpen(i);

	inputInitJoysticks();

	bDone = true;
}

void Window::vInitConfig()
{
	m_oConfig.vClear();

	// Directories section
	//
	m_poDirConfig = m_oConfig.poAddSection("Directories");
	m_poDirConfig->vSetKey("gba_roms",  Glib::get_home_dir());
	m_poDirConfig->vSetKey("batteries", m_sUserDataDir);
	m_poDirConfig->vSetKey("saves",     m_sUserDataDir);

	// Core section
	//
	m_poCoreConfig = m_oConfig.poAddSection("Core");
	m_poCoreConfig->vSetKey("load_game_auto",    false        );
	m_poCoreConfig->vSetKey("bios_file",         ""           );

	// Display section
	//
	m_poDisplayConfig = m_oConfig.poAddSection("Display");
	m_poDisplayConfig->vSetKey("scale",               1              );
	m_poDisplayConfig->vSetKey("show_speed",          true           );
	m_poDisplayConfig->vSetKey("pause_when_inactive", true           );


	// Sound section
	//
	m_poSoundConfig = m_oConfig.poAddSection("Sound");
	m_poSoundConfig->vSetKey("sample_rate",    44100 );
	m_poSoundConfig->vSetKey("volume",         1.00f );

	// Input section
	//
	m_poInputConfig = m_oConfig.poAddSection("Input");
	for (guint j = 0; j < G_N_ELEMENTS(m_astJoypad); j++)
	{
		m_poInputConfig->vSetKey(std::string("joypadSDL_") + m_astJoypad[j].m_csKey,
		                         inputGetKeymap(m_astJoypad[j].m_eKeyFlag));
	}
}

void Window::vCheckConfig()
{
	int iValue;
	int iAdjusted;
	float fValue;
	float fAdjusted;
	std::string sValue;

	// Directories section
	//
	sValue = m_poDirConfig->sGetKey("gba_roms");
	if (sValue != "" && ! Glib::file_test(sValue, Glib::FILE_TEST_IS_DIR))
	{
		m_poDirConfig->vSetKey("gba_roms", Glib::get_home_dir());
	}
	sValue = m_poDirConfig->sGetKey("batteries");
	if (sValue != "" && ! Glib::file_test(sValue, Glib::FILE_TEST_IS_DIR))
	{
		m_poDirConfig->vSetKey("batteries", m_sUserDataDir);
	}
	sValue = m_poDirConfig->sGetKey("saves");
	if (sValue != "" && ! Glib::file_test(sValue, Glib::FILE_TEST_IS_DIR))
	{
		m_poDirConfig->vSetKey("saves", m_sUserDataDir);
	}

	// Core section
	//
	sValue = m_poCoreConfig->sGetKey("bios_file");
	if (sValue != "" && ! Glib::file_test(sValue, Glib::FILE_TEST_IS_REGULAR))
	{
		m_poCoreConfig->vSetKey("bios_file", "");
	}

	// Display section
	//
	iValue = m_poDisplayConfig->oGetKey<int>("scale");
	iAdjusted = CLAMP(iValue, m_iScaleMin, m_iScaleMax);
	if (iValue != iAdjusted)
	{
		m_poDisplayConfig->vSetKey("scale", iAdjusted);
	}

	// Sound section
	//
	iValue = m_poSoundConfig->oGetKey<int>("sample_rate");
	iAdjusted = CLAMP(iValue, m_iSoundSampleRateMin, m_iSoundSampleRateMax);
	if (iValue != iAdjusted)
	{
		m_poSoundConfig->vSetKey("sample_rate", iAdjusted);
	}

	fValue = m_poSoundConfig->oGetKey<float>("volume");
	fAdjusted = CLAMP(fValue, m_fSoundVolumeMin, m_fSoundVolumeMax);
	if (fValue != fAdjusted)
	{
		m_poSoundConfig->vSetKey("volume", fAdjusted);
	}
}

void Window::vLoadConfig(const std::string & _rsFile)
{
	try
	{
		m_oConfig.vLoad(_rsFile, false, false);
	}
	catch (const Glib::Error & e)
	{
		vPopupError(e.what().c_str());
	}
}

void Window::vSaveConfig(const std::string & _rsFile)
{
	try
	{
		m_oConfig.vSave(_rsFile);
	}
	catch (const Glib::Error & e)
	{
		vPopupError(e.what().c_str());
	}
}

void Window::vApplyConfigVolume()
{
	float fSoundVolume = m_poSoundConfig->oGetKey<float>("volume");
	soundSetVolume(fSoundVolume);
}

void Window::vApplyConfigSoundSampleRate()
{
	long iSoundSampleRate = m_poSoundConfig->oGetKey<int>("sample_rate");
	soundSetSampleRate(iSoundSampleRate);
}

void Window::vApplyConfigShowSpeed()
{
	m_bShowSpeed = m_poDisplayConfig->oGetKey<bool>("show_speed");

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
		inputSetKeymap(m_astJoypad[j].m_eKeyFlag,
		               m_poInputConfig->oGetKey<guint>(std::string("joypadSDL_") + m_astJoypad[j].m_csKey));
	}
}

void Window::vSaveJoypadsToConfig()
{
	for (guint j = 0; j < G_N_ELEMENTS(m_astJoypad); j++)
	{
		m_poInputConfig->vSetKey(std::string("joypadSDL_") + m_astJoypad[j].m_csKey,
		                         inputGetKeymap(m_astJoypad[j].m_eKeyFlag));
	}
}

void Window::vUpdateScreen()
{
	m_poScreenArea->vSetSize(m_iGBAScreenWidth, m_iGBAScreenHeight);
	m_poScreenArea->vSetScale(m_poDisplayConfig->oGetKey<int>("scale"));

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

	if (m_poCoreConfig->sGetKey("bios_file") == "")
	{
		vPopupError(_("Please choose a bios file in the preferences dialog."));
		return false;
	}

	if (!CPULoadBios(m_poCoreConfig->sGetKey("bios_file").c_str()))
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

	if (m_poCoreConfig->oGetKey<bool>("load_game_auto"))
	{
		vOnLoadGameMostRecent();
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
	char csTitle[50];

	if (m_bShowSpeed)
	{
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

	std::string sGBADir = m_poDirConfig->sGetKey("gba_roms");

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
	std::string sBattery;
	std::string sDir = m_poDirConfig->sGetKey("batteries");
	if (sDir == "")
	{
		sDir = m_sUserDataDir;
	}

	sBattery = Glib::build_filename(sDir, Cartridge::getGame().getTitle() + ".sav");

	if (Cartridge::readBatteryFromFile(sBattery.c_str()))
	{
		systemScreenMessage(_("Loaded battery"));
	}
}

void Window::vSaveBattery()
{
	std::string sBattery;
	std::string sDir = m_poDirConfig->sGetKey("batteries");
	if (sDir == "")
	{
		sDir = m_sUserDataDir;
	}

	sBattery = Glib::build_filename(sDir, Cartridge::getGame().getTitle() + ".sav");

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
		std::string sFileBase;
		std::string sDir = m_poDirConfig->sGetKey("saves");
		if (sDir == "")
		{
			sDir = m_sUserDataDir;
		}

		sFileBase = Glib::build_filename(sDir, Cartridge::getGame().getTitle());

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
