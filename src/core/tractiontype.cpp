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
//  2013 - Jonathan Rennison <j.g.rennison@gmail.com>
//==========================================================================

#include <algorithm>

#include "common.h"
#include "core/traction_type.h"
#include "core/serialisable_impl.h"
#include "core/train.h"

bool traction_set::CanTrainPass(const train *t) const {
	const traction_set &ts = t->GetActiveTractionTypes();
	for (auto &it : ts.tractions) {
		if (it->always_available) {
			return true;
		}
		if (HasTraction(it)) {
			return true;
		}
	}
	return false;
}

bool traction_set::HasTraction(const traction_type *tt) const {
	return std::find(tractions.begin(), tractions.end(), tt) != tractions.end();
}

bool traction_set::IsIntersecting(const traction_set &ts) const {
	for (auto &it : tractions) {
		if (ts.HasTraction(it)) {
			return true;
		}
	}
	return false;
}

void traction_set::IntersectWith(const traction_set &ts) {
	container_unordered_remove_if(tractions, [&](traction_type *tt) {
		return !ts.HasTraction(tt);
	});
}

void traction_set::UnionWith(const traction_set &ts) {
	for (auto &it : ts.tractions) {
		AddTractionType(it);
	}
}

void traction_set::Deserialise(const deserialiser_input &di, error_collection &ec) {
	if (!di.json.IsArray()) {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid traction set definition");
		return;
	}
	for (rapidjson::SizeType i = 0; i < di.json.Size(); i++) {
		const rapidjson::Value &cur = di.json[i];
		if (cur.IsString() && di.w) {
			traction_type *tt = di.w->GetTractionTypeByName(cur.GetString());
			if (tt) {
				if (!HasTraction(tt)) {
					tractions.push_back(tt);
				}
			} else {
				ec.RegisterNewError<error_deserialisation>(di, string_format("No such traction type: \"%s\"", cur.GetString()));
			}
		} else {
			ec.RegisterNewError<error_deserialisation>(di, "Invalid traction set definition");
		}
	}
}

void traction_set::Serialise(serialiser_output &so, error_collection &ec) const {
	for (auto &it : tractions) {
		so.json_out.String(it->name);
	}
	return;
}

std::string traction_set::DumpString() const {
	std::vector<std::string> strs;
	for (auto &it : tractions) {
		strs.push_back(it->name);
	}
	std::sort(strs.begin(), strs.end());
	std::string out;
	for (auto &it : strs) {
		if (!out.empty()) {
			out += ",";
		}
		out += it;
	}
	return out;
}
