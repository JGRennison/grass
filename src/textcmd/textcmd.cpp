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
#include "textcmd/textcmd.h"
#include "util/util.h"
#include "core/world.h"
#include "core/var.h"
#include "core/textpool.h"
#include "core/signal.h"
#include "core/track_ops.h"

#include <vector>

bool ExecuteTextCommand(const std::string &cmd, world &w) {
	auto input_err = [&]() -> bool {
		message_formatter mf;
		mf.RegisterVariableString("input", cmd);
		w.LogUserMessageLocal(LOG_FAILED, mf.FormatMessage(w.GetUserMessageTextpool().GetTextByName("generic/invalidcommand")));
		return false;
	};

	auto cannot_use_err = [&](const std::string &item, const std::string &reasoncode) -> bool {
		message_formatter mf;
		mf.RegisterVariableString("reason", w.GetUserMessageTextpool().GetTextByName(reasoncode));
		mf.RegisterVariableString("item", item);
		w.LogUserMessageLocal(LOG_FAILED, mf.FormatMessage(w.GetUserMessageTextpool().GetTextByName("generic/cannotuse")));
		return false;
	};

	std::vector<std::string> tokens = TokenizeString(cmd, " \t\n\r");
	if(tokens.size() < 1)
		return input_err();

	if(tokens[0] == "reserve") {
		std::vector<routingpoint *> rps;
		bool ok = true;
		for(size_t i = 1; i < tokens.size(); i++) {
			routingpoint *rp = w.FindTrackByNameCast<routingpoint>(tokens[i]);
			if(!rp) {
				cannot_use_err(tokens[i], "track/reservation/notroutingpoint");
				ok = false;
			}
			else {
				rps.emplace_back(rp);
			}
		}
		if(!ok)
			return false;
		if(rps.size() < 2)
			return input_err();

		action_reservepath act(w, rps.front(), rps.back());
		act.SetVias(via_list(rps.begin() + 1, rps.end() - 1)); // Place all unused routing points in the via list

		w.SubmitAction(std::move(act));
		return true;
	}

	if(tokens[0] == "unreserve") {
		if(tokens.size() != 2)
			return input_err();
		genericsignal *rp = w.FindTrackByNameCast<genericsignal>(tokens[1]);
		if(!rp) {
			return cannot_use_err(tokens[1], "track/reservation/notsignal");
		}
		w.SubmitAction(action_unreservetrack(w, *rp));
		return true;
	}

	auto points_cmd = [&](bool reverse) -> bool {
		if(tokens.size() < 2 || tokens.size() > 3)
			return input_err();
		genericpoints *p = w.FindTrackByNameCast<genericpoints>(tokens[1]);
		if(!p) {
			return cannot_use_err(tokens[1], "track/notpoints");
		}
		unsigned int index = 0;
		if(tokens.size() > 2) {
			if(!ownstrtonum(index, tokens[2].c_str(), tokens[2].size()))
				return input_err();
		}
		w.SubmitAction(action_pointsaction(w, *p, index, reverse));
		return true;
	};

	if(tokens[0] == "normalise_points") {
		return points_cmd(false);
	}
	if(tokens[0] == "reverse_points") {
		return points_cmd(true);
	}

	return input_err();
}
