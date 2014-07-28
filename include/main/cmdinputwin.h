//  grass - Generic Rail And Signalling Simulator
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version. See: COPYING-GPL.txt
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <http://www.gnu.org/licenses/>.
//
//  2014 - Jonathan Rennison <j.g.rennison@gmail.com>
//==========================================================================

#ifndef INC_MAIN_CMDINPUTWIN_ALREADY
#define INC_MAIN_CMDINPUTWIN_ALREADY

#include <memory>
#include <vector>
#include <wx/grid.h>
#include <wx/frame.h>
#include <wx/textctrl.h>

class world;
class train;

namespace maingui {

	class grviewwinlist;

	class cmdinputwin : public wxFrame {
		std::shared_ptr<world> w;
		std::shared_ptr<grviewwinlist> winlist;

		wxTextCtrl *input = nullptr;

		public:
		cmdinputwin(std::shared_ptr<world> w_, std::shared_ptr<grviewwinlist> winlist_);
		~cmdinputwin();
		void InputEvent(wxCommandEvent &event);

		virtual bool ShouldPreventAppExit() const override { return false; }

		DECLARE_EVENT_TABLE()
	};
};

#endif
