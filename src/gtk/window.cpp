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

#include "window.h"

#include <gtkmm/stock.h>
#include <gtkmm/alignment.h>
#include <gtkmm/messagedialog.h>

#include <sys/stat.h>

#include <SDL.h>

#include "../gba/Cartridge.h"
#include "../gba/GBA.h"
#include "../gba/Sound.h"
#include "../gba/Display.h"

#include "tools.h"
#include "intl.h"
#include "screenarea-cairo.h"
#include "screenarea-opengl.h"

extern int emulating;

namespace VBA
{

using Gnome::Glade::Xml;

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

Window::Window(GtkWindow * _pstWindow, const Glib::RefPtr<Xml> & _poXml) :
  Gtk::Window       (_pstWindow),
  m_iGBAScreenWidth (240),
  m_iGBAScreenHeight(160),
  m_iFrameskipMin   (0),
  m_iFrameskipMax   (9),
  m_iScaleMin       (1),
  m_iScaleMax       (6),
  m_iShowSpeedMin   (ShowNone),
  m_iShowSpeedMax   (ShowDetailed),
  m_iSoundSampleRateMin(11025),
  m_iSoundSampleRateMax(48000),
  m_fSoundVolumeMin (0.50f),
  m_fSoundVolumeMax (2.00f),
  m_iJoypadMin      (PAD_1),
  m_iJoypadMax      (PAD_4),
  m_iVideoOutputMin (OutputCairo),
  m_iVideoOutputMax (OutputOpenGL),
  m_bFullscreen     (false)
{
  m_poXml            = _poXml;
  m_poFileOpenDialog = NULL;
  m_iScreenWidth     = m_iGBAScreenWidth;
  m_iScreenHeight    = m_iGBAScreenHeight;
  m_eCartridge       = CartridgeNone;

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
  vUpdateScreen();

  Gtk::MenuItem *      poMI;
  Gtk::CheckMenuItem * poCMI;

  // Menu bar
  m_poMenuBar = dynamic_cast<Gtk::MenuBar *>(_poXml->get_widget("MenuBar"));
  m_poMenuBar->signal_deactivate().connect(sigc::mem_fun(*this, &Window::vOnMenuExit));
  
  poMI = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("FileMenu"));
  poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnMenuEnter));
  poMI = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("EmulationMenu"));
  poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnMenuEnter));
  poMI = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("OptionsMenu"));
  poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnMenuEnter));
  poMI = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("HelpMenu"));
  poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnMenuEnter));

  // File menu
  //
  poMI = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("FileOpen"));
  poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnFileOpen));

  poMI = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("FileLoad"));
  poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnFileLoad));
  m_listSensitiveWhenPlaying.push_back(poMI);

  poMI = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("FileSave"));
  poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnFileSave));
  m_listSensitiveWhenPlaying.push_back(poMI);

  for (int i = 0; i < 10; i++)
  {
    char csName[20];
    snprintf(csName, 20, "LoadGameSlot%d", i + 1);
    m_apoLoadGameItem[i] = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget(csName));
    snprintf(csName, 20, "SaveGameSlot%d", i + 1);
    m_apoSaveGameItem[i] = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget(csName));

    m_apoLoadGameItem[i]->signal_activate().connect(sigc::bind(
                                                      sigc::mem_fun(*this, &Window::vOnLoadGame),
                                                      i + 1));
    m_apoSaveGameItem[i]->signal_activate().connect(sigc::bind(
                                                      sigc::mem_fun(*this, &Window::vOnSaveGame),
                                                      i + 1));
  }
  vUpdateGameSlots();

  poMI = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("LoadGameMostRecent"));
  poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnLoadGameMostRecent));
  m_listSensitiveWhenPlaying.push_back(poMI);

  poCMI = dynamic_cast<Gtk::CheckMenuItem *>(_poXml->get_widget("LoadGameAuto"));
  poCMI->set_active(m_poCoreConfig->oGetKey<bool>("load_game_auto"));
  vOnLoadGameAutoToggled(poCMI);
  poCMI->signal_toggled().connect(sigc::bind(
                                    sigc::mem_fun(*this, &Window::vOnLoadGameAutoToggled),
                                    poCMI));

  poMI = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("SaveGameOldest"));
  poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnSaveGameOldest));
  m_listSensitiveWhenPlaying.push_back(poMI);

  m_poFilePauseItem = dynamic_cast<Gtk::CheckMenuItem *>(_poXml->get_widget("FilePause"));
  m_poFilePauseItem->set_active(false);
  vOnFilePauseToggled(m_poFilePauseItem);
  m_poFilePauseItem->signal_toggled().connect(sigc::bind(
                                                sigc::mem_fun(*this, &Window::vOnFilePauseToggled),
                                                m_poFilePauseItem));
  m_listSensitiveWhenPlaying.push_back(m_poFilePauseItem);

  poMI = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("FileReset"));
  poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnFileReset));
  m_listSensitiveWhenPlaying.push_back(poMI);

  poMI = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("FileClose"));
  poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnFileClose));
  m_listSensitiveWhenPlaying.push_back(poMI);

  poMI = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("FileExit"));
  poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnFileExit));

  // Recent menu
  //
  m_poRecentManager = Gtk::RecentManager::get_default();

  Gtk::RecentFilter oRecentFilter;
  oRecentFilter.add_application( Glib::get_application_name() );

  m_poRecentChooserMenu = Gtk::manage( new Gtk::RecentChooserMenu(m_poRecentManager) );
  m_poRecentChooserMenu->set_show_numbers();
  m_poRecentChooserMenu->set_show_tips();
  m_poRecentChooserMenu->set_local_only();
  m_poRecentChooserMenu->add_filter(oRecentFilter);
  m_poRecentChooserMenu->signal_item_activated().connect(
                                                   sigc::mem_fun(*this, &Window::vOnRecentFile));


  m_poRecentMenu = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("RecentMenu"));
  m_poRecentMenu->set_submenu(static_cast<Gtk::Menu &>(*m_poRecentChooserMenu));

  // Frameskip menu
  //
  struct
  {
    const char * m_csName;
    const int    m_iFrameskip;
  }
  astFrameskip[] =
  {
    { "FrameskipAutomatic", -1 },
    { "Frameskip0",          0 },
    { "Frameskip1",          1 },
    { "Frameskip2",          2 },
    { "Frameskip3",          3 },
    { "Frameskip4",          4 },
    { "Frameskip5",          5 },
    { "Frameskip6",          6 },
    { "Frameskip7",          7 },
    { "Frameskip8",          8 },
    { "Frameskip9",          9 }
  };
  int iDefaultFrameskip;
  if (m_poCoreConfig->sGetKey("frameskip") == "auto")
  {
    iDefaultFrameskip = -1;
  }
  else
  {
    iDefaultFrameskip = m_poCoreConfig->oGetKey<int>("frameskip");
  }
  for (guint i = 0; i < G_N_ELEMENTS(astFrameskip); i++)
  {
    poCMI = dynamic_cast<Gtk::CheckMenuItem *>(_poXml->get_widget(astFrameskip[i].m_csName));
    if (astFrameskip[i].m_iFrameskip == iDefaultFrameskip)
    {
      poCMI->set_active();
      vOnFrameskipToggled(poCMI, iDefaultFrameskip);
    }
    poCMI->signal_toggled().connect(sigc::bind(
                                      sigc::mem_fun(*this, &Window::vOnFrameskipToggled),
                                      poCMI, astFrameskip[i].m_iFrameskip));
  }

  // Emulator menu
  //
  poMI = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("DirectoriesConfigure"));
  poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnDirectories));

  poCMI = dynamic_cast<Gtk::CheckMenuItem *>(_poXml->get_widget("EmulatorPauseWhenInactive"));
  poCMI->set_active(m_poDisplayConfig->oGetKey<bool>("pause_when_inactive"));
  vOnPauseWhenInactiveToggled(poCMI);
  poCMI->signal_toggled().connect(sigc::bind(
                                    sigc::mem_fun(*this, &Window::vOnPauseWhenInactiveToggled),
                                    poCMI));

  // Show speed menu
  //
  struct
  {
    const char *     m_csName;
    const EShowSpeed m_eShowSpeed;
  }
  astShowSpeed[] =
  {
    { "ShowSpeedNone",       ShowNone       },
    { "ShowSpeedPercentage", ShowPercentage },
    { "ShowSpeedDetailed",   ShowDetailed   }
  };
  EShowSpeed eDefaultShowSpeed = (EShowSpeed)m_poDisplayConfig->oGetKey<int>("show_speed");
  for (guint i = 0; i < G_N_ELEMENTS(astShowSpeed); i++)
  {
    poCMI = dynamic_cast<Gtk::CheckMenuItem *>(_poXml->get_widget(astShowSpeed[i].m_csName));
    if (astShowSpeed[i].m_eShowSpeed == eDefaultShowSpeed)
    {
      poCMI->set_active();
      vOnShowSpeedToggled(poCMI, eDefaultShowSpeed);
    }
    poCMI->signal_toggled().connect(sigc::bind(
                                      sigc::mem_fun(*this, &Window::vOnShowSpeedToggled),
                                      poCMI, astShowSpeed[i].m_eShowSpeed));
  }

  // Display menu
  poMI = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("DisplayConfigure"));
  poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnDisplayConfigure));

  // Sound menu
  poMI = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("SoundConfigure"));
  poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnSoundConfigure));

  // Joypad menu
  //
  poMI = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("JoypadConfigure"));
  poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnJoypadConfigure));

  EPad eDefaultJoypad = (EPad)m_poInputConfig->oGetKey<int>("active_joypad");
  inputSetDefaultJoypad(eDefaultJoypad);

  // Fullscreen menu
  //
  poMI = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("VideoFullscreen"));
  poMI->signal_activate().connect(sigc::mem_fun(*this, &Window::vOnVideoFullscreen));

  // Help menu
  //
  poMI = dynamic_cast<Gtk::MenuItem *>(_poXml->get_widget("HelpAbout"));
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

void Window::vInitColors(EColorFormat _eColorFormat)
{
  switch (_eColorFormat)
  {
    case ColorFormatBGR:
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
      Display::initColorMap(3, 11, 19);
#else
      Display::initColorMap(27, 19, 11);
#endif
      break;
    default:
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
      Display::initColorMap(19, 11, 3);
#else
      Display::initColorMap(11, 19, 27);
#endif
      break;
  }
}

void Window::vApplyConfigScreenArea()
{
  EVideoOutput eVideoOutput = (EVideoOutput)m_poDisplayConfig->oGetKey<int>("output");

  Gtk::Alignment * poC;

  poC = dynamic_cast<Gtk::Alignment *>(m_poXml->get_widget("ScreenContainer"));
  poC->remove();
  poC->set(Gtk::ALIGN_CENTER, Gtk::ALIGN_CENTER, 1.0, 1.0);

  try
  {
    switch (eVideoOutput)
    {
      case OutputOpenGL:
        vInitColors(ColorFormatBGR);
        m_poScreenArea = Gtk::manage(new ScreenAreaGl(m_iScreenWidth, m_iScreenHeight));
        break;
      case OutputCairo:
      default:
        vInitColors(ColorFormatRGB);
        m_poScreenArea = Gtk::manage(new ScreenAreaCairo(m_iScreenWidth, m_iScreenHeight));
        break;
    }
  }
  catch (std::exception e)
  {
    fprintf(stderr, "Unable to initialize output, falling back to Cairo\n");
    m_poScreenArea = Gtk::manage(new ScreenAreaCairo(m_iScreenWidth, m_iScreenHeight));
  }

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

  systemFrameSkip = 2;

  emulating = 0;

  m_iFrameCount = 0;

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

  inputSetKeymap(PAD_DEFAULT, KEY_LEFT, GDK_Left);
  inputSetKeymap(PAD_DEFAULT, KEY_RIGHT, GDK_Right);
  inputSetKeymap(PAD_DEFAULT, KEY_UP, GDK_Up);
  inputSetKeymap(PAD_DEFAULT, KEY_DOWN, GDK_Down);
  inputSetKeymap(PAD_DEFAULT, KEY_BUTTON_A, GDK_z);
  inputSetKeymap(PAD_DEFAULT, KEY_BUTTON_B, GDK_x);
  inputSetKeymap(PAD_DEFAULT, KEY_BUTTON_START, GDK_Return);
  inputSetKeymap(PAD_DEFAULT, KEY_BUTTON_SELECT, GDK_BackSpace);
  inputSetKeymap(PAD_DEFAULT, KEY_BUTTON_L, GDK_a);
  inputSetKeymap(PAD_DEFAULT, KEY_BUTTON_R, GDK_s);
  inputSetKeymap(PAD_DEFAULT, KEY_BUTTON_SPEED, GDK_space);
  inputSetKeymap(PAD_DEFAULT, KEY_BUTTON_CAPTURE, GDK_F12);
  inputSetKeymap(PAD_DEFAULT, KEY_BUTTON_AUTO_A, GDK_q);
  inputSetKeymap(PAD_DEFAULT, KEY_BUTTON_AUTO_B, GDK_w);

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
  m_poCoreConfig->vSetKey("frameskip",         "auto"       );
  m_poCoreConfig->vSetKey("bios_file",         ""           );

  // Display section
  //
  m_poDisplayConfig = m_oConfig.poAddSection("Display");
  m_poDisplayConfig->vSetKey("scale",               1              );
  m_poDisplayConfig->vSetKey("show_speed",          ShowPercentage );
  m_poDisplayConfig->vSetKey("pause_when_inactive", true           );
  m_poDisplayConfig->vSetKey("output",              OutputOpenGL   );


  // Sound section
  //
  m_poSoundConfig = m_oConfig.poAddSection("Sound");
  m_poSoundConfig->vSetKey("sample_rate",    44100 );
  m_poSoundConfig->vSetKey("volume",         1.00f );

  // Input section
  //
  m_poInputConfig = m_oConfig.poAddSection("Input");
  m_poInputConfig->vSetKey("active_joypad", m_iJoypadMin );
  for (int i = m_iJoypadMin; i <= m_iJoypadMax; i++)
  {
    char csPrefix[20];
    snprintf(csPrefix, sizeof(csPrefix), "joypadSDL%d_", i);
    std::string sPrefix(csPrefix);

    for (guint j = 0; j < G_N_ELEMENTS(m_astJoypad); j++)
    {
    	m_poInputConfig->vSetKey(sPrefix + m_astJoypad[j].m_csKey,
    			inputGetKeymap(PAD_DEFAULT, m_astJoypad[j].m_eKeyFlag));
    }
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
  if (m_poCoreConfig->sGetKey("frameskip") != "auto")
  {
    iValue = m_poCoreConfig->oGetKey<int>("frameskip");
    iAdjusted = CLAMP(iValue, m_iFrameskipMin, m_iFrameskipMax);
    if (iValue != iAdjusted)
    {
      m_poCoreConfig->vSetKey("frameskip", iAdjusted);
    }
  }

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

  iValue = m_poDisplayConfig->oGetKey<int>("show_speed");
  iAdjusted = CLAMP(iValue, m_iShowSpeedMin, m_iShowSpeedMax);
  if (iValue != iAdjusted)
  {
    m_poDisplayConfig->vSetKey("show_speed", iAdjusted);
  }

  iValue = m_poDisplayConfig->oGetKey<int>("output");
  iAdjusted = CLAMP(iValue, m_iVideoOutputMin, m_iVideoOutputMax);
  if (iValue != iAdjusted)
  {
    m_poDisplayConfig->vSetKey("output", iAdjusted);
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

  // Input section
  //
  iValue = m_poInputConfig->oGetKey<int>("active_joypad");
  iAdjusted = CLAMP(iValue, m_iJoypadMin, m_iJoypadMax);
  if (iValue != iAdjusted)
  {
    m_poInputConfig->vSetKey("active_joypad", iAdjusted);
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

void Window::vHistoryAdd(const std::string & _rsFile)
{
  std::string sURL = "file://" + _rsFile;

  m_poRecentManager->add_item(sURL);
}

void Window::vApplyConfigJoypads()
{
  for (int i = m_iJoypadMin; i <= m_iJoypadMax; i++)
  {
    char csPrefix[20];
    snprintf(csPrefix, sizeof(csPrefix), "joypadSDL%d_", i);
    std::string sPrefix(csPrefix);

    for (guint j = 0; j < G_N_ELEMENTS(m_astJoypad); j++)
    {
    	inputSetKeymap((EPad)i, m_astJoypad[j].m_eKeyFlag,
    			m_poInputConfig->oGetKey<guint>(sPrefix + m_astJoypad[j].m_csKey));
    }
  }
}

void Window::vSaveJoypadsToConfig()
{
  for (int i = m_iJoypadMin; i <= m_iJoypadMax; i++)
  {
	char csPrefix[20];
	snprintf(csPrefix, sizeof(csPrefix), "joypadSDL%d_", i);
	std::string sPrefix(csPrefix);

	for (guint j = 0; j < G_N_ELEMENTS(m_astJoypad); j++)
	{
		m_poInputConfig->vSetKey(sPrefix + m_astJoypad[j].m_csKey,
				inputGetKeymap((EPad)i, m_astJoypad[j].m_eKeyFlag));
	}
  }
}

void Window::vUpdateScreen()
{

  m_iScreenWidth  = m_iGBAScreenWidth;
  m_iScreenHeight = m_iGBAScreenHeight;

  g_return_if_fail(m_iScreenWidth >= 1 && m_iScreenHeight >= 1);

  m_poScreenArea->vSetSize(m_iScreenWidth, m_iScreenHeight);
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

  m_sRomFile = _rsFile;
  const char * csFile = _rsFile.c_str();

  bool bUsableImage = utilIsUsableGBAImage(csFile);
  
  if (!bUsableImage)
  {
    vPopupError(_("Unknown file type %s"), csFile);
    return false;
  }

  if (!CPUInitMemory())
    return false;

  if (!Cartridge::loadDump(csFile))
  {
    CPUCleanUp();
    return false;
  }

  m_eCartridge = CartridgeGBA;
  m_stEmulator = GBASystem;

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
  m_bWasEmulating = false;

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
  // TODO:Remove the cast
  m_poScreenArea->vDrawPixels((u8 *)pix);
  m_iFrameCount++;
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

  if (m_eShowSpeed == ShowPercentage)
  {
    snprintf(csTitle, 50, "VBA-M - %d%%", _iSpeed);
    set_title(csTitle);
  }
  else if (m_eShowSpeed == ShowDetailed)
  {
    snprintf(csTitle, 50, "VBA-M - %d%% (%d, %d fps)",
             _iSpeed, systemFrameSkip, m_iFrameCount);
    set_title(csTitle);
  }
  
  m_iFrameCount = 0;
}

void Window::vComputeFrameskip(int _iRate)
{
  static Glib::TimeVal uiLastTime;
  static int iFrameskipAdjust = 0;

  Glib::TimeVal uiTime;
  uiTime.assign_current_time();

  if (m_bWasEmulating)
  {
    if (m_bAutoFrameskip)
    {
      Glib::TimeVal uiDiff = uiTime - uiLastTime;
      const int iWantedSpeed = 100;
      int iSpeed = iWantedSpeed;

      if (uiDiff != Glib::TimeVal(0, 0))
      {
        iSpeed = (1000000 / _iRate) / (uiDiff.as_double() * 1000);
      }

      if (iSpeed >= iWantedSpeed - 2)
      {
        iFrameskipAdjust++;
        if (iFrameskipAdjust >= 3)
        {
          iFrameskipAdjust = 0;
          if (systemFrameSkip > 0)
          {
            systemFrameSkip--;
          }
        }
      }
      else
      {
        if (iSpeed < iWantedSpeed - 20)
        {
          iFrameskipAdjust -= ((iWantedSpeed - 10) - iSpeed) / 5;
        }
        else if (systemFrameSkip < 9)
        {
          iFrameskipAdjust--;
        }

        if (iFrameskipAdjust <= -4)
        {
          iFrameskipAdjust = 0;
          if (systemFrameSkip < 9)
          {
            systemFrameSkip++;
          }
        }
      }
    }
  }
  else
  {
    m_bWasEmulating = true;
  }

  uiLastTime = uiTime;
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

  const char * acsPattern[] =
  {
    // GBA
    "*.[bB][iI][nN]", "*.[aA][gG][bB]", "*.[gG][bB][aA]",
    // Both
    "*.[mM][bB]", "*.[eE][lL][fF]", "*.[zZ][iI][pP]", "*.[zZ]", "*.[gG][zZ]"
  };

  Gtk::FileFilter oAllGBAFilter;
  oAllGBAFilter.set_name(_("All Gameboy Advance files"));
  for (guint i = 0; i < G_N_ELEMENTS(acsPattern); i++)
  {
    oAllGBAFilter.add_pattern(acsPattern[i]);
  }

  Gtk::FileFilter oGBAFilter;
  oGBAFilter.set_name(_("Gameboy Advance files"));
  for (int i = 0; i < 3; i++)
  {
    oGBAFilter.add_pattern(acsPattern[i]);
  }

  poDialog->add_filter(oAllGBAFilter);
  poDialog->add_filter(oGBAFilter);

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

  sBattery = sDir + "/" + sCutSuffix(Glib::path_get_basename(m_sRomFile)) + ".sav";

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

  sBattery = sDir + "/" + sCutSuffix(Glib::path_get_basename(m_sRomFile)) + ".sav";

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
  m_bWasEmulating = false;
}

void Window::vUpdateGameSlots()
{
  if (m_eCartridge == CartridgeNone)
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

    sFileBase = sDir + "/" + sCutSuffix(Glib::path_get_basename(m_sRomFile));

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
  if(!m_bFullscreen)
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
  while(SDL_PollEvent(&event))
  {
    switch(event.type)
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
