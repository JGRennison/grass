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

#include "common.h"
#include "main/wx_common.h"
#include "main/cmd_input_win.h"
#include "main/wx_util.h"
#include "main/train_win.h"
#include "main/main_gui.h"
#include "text_cmd/text_cmd.h"
#include "util/util.h"

#include <wx/sizer.h>

BEGIN_EVENT_TABLE(main_gui::cmd_input_win, wxFrame)
EVT_TEXT_ENTER(1, main_gui::cmd_input_win::InputEvent)
END_EVENT_TABLE()

main_gui::cmd_input_win::cmd_input_win(std::shared_ptr<world> w_, std::shared_ptr<gr_view_winlist> winlist_)
		: wxFrame(nullptr, wxID_ANY, wxT("Text command input"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE & ~wxRESIZE_BORDER),
		w(std::move(w_)), winlist(std::move(winlist_)) {
	winlist->cmd_input_wins.emplace_back(this);

	input = new wxTextCtrl(this, 1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	Fit();
	SetInitialSize(wxSize(400, -1));
}

main_gui::cmd_input_win::~cmd_input_win() {
	container_unordered_remove(winlist->cmd_input_wins, this);
}

void main_gui::cmd_input_win::InputEvent(wxCommandEvent &event) {
	std::string text = stdstrwx(input->GetValue());
	input->ChangeValue(wxT(""));

	ExecuteTextCommand(text, *w);
}
