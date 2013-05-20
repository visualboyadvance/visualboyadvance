// -*- C++ -*-
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

#ifndef __VBA_WINDOW_H__
#define __VBA_WINDOW_H__

#include <gtkmm.h>

#include "../System.h"
#include "InputSDL.h"

#include "ScreenArea.h"

namespace VBA
{

class Window : public Gtk::Window
{
	friend class Gtk::Builder;

public:
	virtual ~Window();

	inline static Window * poGetInstance() {
		return m_poInstance;
	}

	// GBA screen size
	static const int m_iGBAScreenWidth = 240;
	static const int m_iGBAScreenHeight = 160;

	bool bLoadROM(const std::string & _rsFile);
	void vPopupError(const char * _csFormat, ...);
	void vPopupErrorV(const char * _csFormat, va_list _args);
	void vDrawScreen(u32 *pix);
	void vShowSpeed(int _iSpeed);

protected:
	Window(GtkWindow * _pstWindow,
	       const Glib::RefPtr<Gtk::Builder> & _poBuilder);

	void vApplyConfigScreenArea();
	void vApplyConfigVolume();
	void vApplyConfigSoundSampleRate();
	void vApplyConfigShowSpeed();
	void vUpdateScreen();
	void vUpdateGameSlots();

	void vOnMenuEnter();
	void vOnMenuExit();
	void vOnFileOpen();
	void vOnFileLoad();
	void vOnFileSave();
	void vOnLoadGameMostRecent();
	void vOnLoadGame(int _iSlot);
	void vOnSaveGameOldest();
	void vOnSaveGame(int _iSlot);
	void vOnFilePauseToggled(Gtk::CheckMenuItem * _poCMI);
	void vOnFileReset();
	void vOnRecentFile();
	void vOnFileClose();
	void vOnFileExit();
	void vOnVideoFullscreen();
	void vOnJoypadConfigure();
	void vOnSettings();
	void vOnHelpAbout();
	void vOnSettingsChanged(const Glib::ustring &key);
	bool bOnEmuIdle();

	virtual bool on_focus_in_event(GdkEventFocus * _pstEvent);
	virtual bool on_focus_out_event(GdkEventFocus * _pstEvent);
	virtual bool on_key_press_event(GdkEventKey * _pstEvent);
	virtual bool on_key_release_event(GdkEventKey * _pstEvent);
	virtual bool on_window_state_event(GdkEventWindowState* _pstEvent);

private:
	// Config limits
	static const int m_iScaleMin = 1;
	static const int m_iScaleMax = 6;
	static const int m_iSoundSampleRateMin = 11025;
	static const int m_iSoundSampleRateMax = 48000;
	static const float m_fSoundVolumeMin;
	static const float m_fSoundVolumeMax;

	static Window * m_poInstance;

	Glib::RefPtr<Gtk::Builder> m_poBuilder;

	std::string       m_sUserDataDir;

	Glib::RefPtr<Gio::Settings> m_poSettings;
	Glib::RefPtr<Gio::Settings> m_poJoypadMapping;

	Gtk::FileChooserDialog * m_poFileOpenDialog;

	ScreenArea *         m_poScreenArea;
	Gtk::CheckMenuItem * m_poFilePauseItem;
	Gtk::CheckMenuItem * m_poSoundOffItem;
	Gtk::MenuBar *       m_poMenuBar;

	struct SGameSlot
	{
		bool        m_bEmpty;
		std::string m_sFile;
		time_t      m_uiTime;
	};

	struct SJoypadKey
	{
		const char * m_csKey;
		const EKey   m_eKeyFlag;
	};

	static const SJoypadKey m_astJoypad[];

	Gtk::MenuItem * m_apoLoadGameItem[10];
	Gtk::MenuItem * m_apoSaveGameItem[10];
	SGameSlot       m_astGameSlot[10];

	Glib::RefPtr<Gtk::RecentManager> m_poRecentManager;
	Gtk::MenuItem *                  m_poRecentMenu;
	Gtk::RecentChooserMenu *         m_poRecentChooserMenu;

	std::list<Gtk::Widget *> m_listSensitiveWhenPlaying;

	sigc::connection m_oEmuSig;

	bool m_bFullscreen;
	bool m_bPaused;
	bool m_bShowSpeed;

	void vInitSystem();
	void vUnInitSystem();
	void vInitSDL();
	void vCheckConfig();
	void vInitColors();
	void vHistoryAdd(const std::string & _rsFile);
	void vApplyConfigJoypads();
	void vSaveJoypadsToConfig();
	void vDrawDefaultScreen();
	void vSetDefaultTitle();
	void vCreateFileOpenDialog();
	void vLoadBattery();
	void vSaveBattery();
	void vStartEmu();
	void vStopEmu();
	void vToggleFullscreen();
	void vSDLPollEvents();
};

} // namespace VBA


#endif // __VBA_WINDOW_H__
