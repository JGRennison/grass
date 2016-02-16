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
//  2016 - Jonathan Rennison <j.g.rennison@gmail.com>
//==========================================================================

#include "common.h"
#include "main/wx_common.h"
#include "main/main.h"
#include "main/text_cmd_main.h"
#include "draw/draw_options.h"

bool text_cmd_main::ProcessTokens(const std::vector<std::string> &tokens) {
	auto parse_bool_opt = [&](bool &opt, size_t index) -> bool {
		if (tokens.size() <= index) {
			RegisterInputError();
			return false;
		}
		if (tokens[index] == "on") {
			opt = true;
		} else if (tokens[index] == "off") {
			opt = false;
		} else {
			return false;
		}

		return true;
	};

	if (tokens[0] == "display") {
		if (tokens.size() < 2) {
			RegisterInputError();
			return false;
		}

		if (tokens[1] == "tc_gaps") {
			bool ok = parse_bool_opt(wxGetApp().GetDrawOptions()->tc_gaps, 2);
			if (ok) wxGetApp().RefreshAllViews();
			return ok;
		}

		RegisterInputError();
		return false;
	}

	return text_command_handler::ProcessTokens(tokens);
}
