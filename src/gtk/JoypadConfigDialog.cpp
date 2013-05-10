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

#include "JoypadConfigDialog.h"

#include "Intl.h"

namespace VBA
{

const JoypadConfigDialog::SJoypadKey JoypadConfigDialog::m_astKeys[] =
{
	{ KEY_UP,             "UpEntry"           },
	{ KEY_DOWN,           "DownEntry"         },
	{ KEY_LEFT,           "LeftEntry"         },
	{ KEY_RIGHT,          "RightEntry"        },
	{ KEY_BUTTON_A,       "ButtonAEntry"      },
	{ KEY_BUTTON_B,       "ButtonBEntry"      },
	{ KEY_BUTTON_L,       "LeftTriggerEntry"  },
	{ KEY_BUTTON_R,       "RightTriggerEntry" },
	{ KEY_BUTTON_SELECT,  "SelectEntry"       },
	{ KEY_BUTTON_START,   "StartEntry"        },
	{ KEY_BUTTON_SPEED,   "SpeedEntry"        },
	{ KEY_BUTTON_AUTO_A,  "AutofireAEntry"    },
	{ KEY_BUTTON_AUTO_B,  "AutofireBEntry"    }
};

JoypadConfigDialog::JoypadConfigDialog(GtkDialog* _pstDialog, const Glib::RefPtr<Gtk::Builder>& refBuilder) :
		Gtk::Dialog(_pstDialog),
		m_bUpdating(false),
		m_iCurrentEntry(-1)
{
	// Joypad buttons
	for (guint i = 0; i < G_N_ELEMENTS(m_astKeys); i++)
	{
		Gtk::Entry * poEntry = 0;
		refBuilder->get_widget(m_astKeys[i].m_csEntryName, poEntry);

		poEntry->signal_focus_in_event().connect(sigc::bind(
		            sigc::mem_fun(*this, &JoypadConfigDialog::bOnEntryFocusIn), i));
		poEntry->signal_focus_out_event().connect(sigc::mem_fun(*this, &JoypadConfigDialog::bOnEntryFocusOut));

		m_oEntries.push_back(poEntry);
	}

	// Dialog validation button
	refBuilder->get_widget("CloseButton", m_poCloseButton);

	// Signals and default values
	m_oConfigSig = Glib::signal_timeout().connect(sigc::mem_fun(*this, &JoypadConfigDialog::bOnConfigIdle),
	               50);

	vUpdateEntries();
}

JoypadConfigDialog::~JoypadConfigDialog()
{
	m_oConfigSig.disconnect();
}

void JoypadConfigDialog::vUpdateEntries()
{
	m_bUpdating = true;

	for (guint i = 0; i < m_oEntries.size(); i++)
	{
		std::string csName;

		guint uiKeyval = inputGetKeymap(m_astKeys[i].m_eKeyFlag);
		int dev = uiKeyval >> 16;
		if (dev == 0)
		{
			csName = gdk_keyval_name(uiKeyval);
		}
		else
		{
			int what = uiKeyval & 0xffff;
			std::stringstream os;
			os << "Joy " << dev;

			if (what >= 128)
			{
				// joystick button
				int button = what - 128;
				os << " Button " << button;
			}
			else if (what < 0x20)
			{
				// joystick axis
				int dir = what & 1;
				what >>= 1;
				os << " Axis " << what << (dir?'-':'+');
			}
			else if (what < 0x30)
			{
				// joystick hat
				int dir = (what & 3);
				what = (what & 15);
				what >>= 2;
				os << " Hat " << what << " ";
				switch (dir)
				{
					case 0: os << "Up"; break;
					case 1: os << "Down"; break;
					case 2: os << "Right"; break;
					case 3: os << "Left"; break;
				}
			}

			csName = os.str();
		}

		if (csName.empty())
		{
			m_oEntries[i]->set_text(_("<Undefined>"));
		}
		else
		{
			m_oEntries[i]->set_text(csName);
		}
	}

	m_bUpdating = false;
}

bool JoypadConfigDialog::bOnEntryFocusIn(GdkEventFocus * _pstEvent,
        guint           _uiEntry)
{
	m_iCurrentEntry = _uiEntry;

	return false;
}

bool JoypadConfigDialog::bOnEntryFocusOut(GdkEventFocus * _pstEvent)
{
	m_iCurrentEntry = -1;

	return false;
}

bool JoypadConfigDialog::on_key_press_event(GdkEventKey * _pstEvent)
{
	if (m_iCurrentEntry < 0)
	{
		return Gtk::Window::on_key_press_event(_pstEvent);
	}

	// Forward the keyboard event by faking a SDL event
	SDL_Event event;
	event.type = SDL_KEYDOWN;
	event.key.keysym.sym = (SDLKey)_pstEvent->keyval;
	vOnInputEvent(event);

	return true;
}

void JoypadConfigDialog::vOnInputEvent(const SDL_Event &event)
{
	if (m_iCurrentEntry < 0)
	{
		return;
	}

	int code = inputGetEventCode(event);
	
	if (!code) return;
	
	inputSetKeymap(m_astKeys[m_iCurrentEntry].m_eKeyFlag, code);
	vUpdateEntries();

	if (m_iCurrentEntry + 1 < (gint)m_oEntries.size())
	{
		m_oEntries[m_iCurrentEntry + 1]->grab_focus();
	}
	else
	{
		m_poCloseButton->grab_focus();
	}
}

bool JoypadConfigDialog::bOnConfigIdle()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
		case SDL_JOYAXISMOTION:
			if (abs(event.jaxis.value) < 16384) continue;
			if (event.jaxis.which != m_oPreviousEvent.jaxis.which
			        || event.jaxis.axis != m_oPreviousEvent.jaxis.axis
			        || (event.jaxis.value > 0 && m_oPreviousEvent.jaxis.value < 0)
			        || (event.jaxis.value < 0 && m_oPreviousEvent.jaxis.value > 0))
			{
				vOnInputEvent(event);
				m_oPreviousEvent = event;
			}
			vEmptyEventQueue();
			break;
		case SDL_JOYHATMOTION:
		case SDL_JOYBUTTONUP:
			vOnInputEvent(event);
			vEmptyEventQueue();
			break;
		}
	}

	return true;
}

void JoypadConfigDialog::vEmptyEventQueue()
{
	// Empty the SDL event queue
	SDL_Event event;
	while (SDL_PollEvent(&event));
}

} // namespace VBA
