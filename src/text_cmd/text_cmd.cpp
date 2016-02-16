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
#include "text_cmd/text_cmd.h"
#include "util/util.h"
#include "core/world.h"
#include "core/var.h"
#include "core/text_pool.h"
#include "core/signal.h"
#include "core/track_ops.h"

#include <vector>

void text_command_handler::RegisterInputError() const {
	message_formatter mf;
	mf.RegisterVariableString("input", cmd);
	w.LogUserMessageLocal(LOG_CATEGORY::FAILED, mf.FormatMessage(w.GetUserMessageTextpool().GetTextByName("generic/invalidcommand")));
};

void text_command_handler::RegisterCannotUseError(const std::string &item, const std::string &reasoncode) const {
	message_formatter mf;
	mf.RegisterVariableString("reason", w.GetUserMessageTextpool().GetTextByName(reasoncode));
	mf.RegisterVariableString("item", item);
	w.LogUserMessageLocal(LOG_CATEGORY::FAILED, mf.FormatMessage(w.GetUserMessageTextpool().GetTextByName("generic/cannotuse")));
};

bool text_command_handler::Execute() {
	std::vector<std::string> tokens = TokenizeString(cmd, " \t\n\r");
	if (tokens.size() < 1) {
		RegisterInputError();
		return false;
	}
	return ProcessTokens(tokens);
}

bool text_command_handler::ProcessTokens(const std::vector<std::string> &tokens) {
	if (tokens[0] == "reserve") {
		std::vector<routing_point *> rps;
		bool ok = true;
		for (size_t i = 1; i < tokens.size(); i++) {
			routing_point *rp = w.FindTrackByNameCast<routing_point>(tokens[i]);
			if (!rp) {
				RegisterCannotUseError(tokens[i], "track/reservation/notroutingpoint");
				ok = false;
			} else {
				rps.emplace_back(rp);
			}
		}
		if (!ok) {
			return false;
		}
		if (rps.size() < 2) {
			RegisterInputError();
			return false;
		}

		action_reserve_path act(w, rps.front(), rps.back());
		act.SetVias(via_list(rps.begin() + 1, rps.end() - 1)); // Place all unused routing points in the via list

		w.SubmitAction(std::move(act));
		return true;
	}

	if (tokens[0] == "unreserve") {
		if (tokens.size() != 2) {
			RegisterInputError();
			return false;
		}
		generic_signal *rp = w.FindTrackByNameCast<generic_signal>(tokens[1]);
		if (!rp) {
			RegisterCannotUseError(tokens[1], "track/reservation/notsignal");
			return false;
		}
		w.SubmitAction(action_unreserve_track(w, *rp));
		return true;
	}

	auto points_cmd = [&](bool reverse) -> bool {
		if (tokens.size() < 2 || tokens.size() > 3) {
			RegisterInputError();
			return false;
		}
		generic_points *p = w.FindTrackByNameCast<generic_points>(tokens[1]);
		if (!p) {
			RegisterCannotUseError(tokens[1], "track/notpoints");
			return false;
		}
		unsigned int index = 0;
		if (tokens.size() > 2) {
			if (!ownstrtonum(index, tokens[2].c_str(), tokens[2].size())) {
				RegisterInputError();
				return false;
			}
		}
		w.SubmitAction(action_points_action(w, *p, index, reverse));
		return true;
	};

	if (tokens[0] == "normalise_points") {
		return points_cmd(false);
	}
	if (tokens[0] == "reverse_points") {
		return points_cmd(true);
	}

	RegisterInputError();
	return false;
}
