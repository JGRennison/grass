//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
//  WEBSITE: http://grss.sourceforge.net
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
#include "core/tractiontype.h"
#include "core/serialisable_impl.h"
#include "core/train.h"

bool tractionset::CanTrainPass(const train *t) const {
	const tractionset &ts = t->GetActiveTractionTypes();
	for(auto it = ts.tractions.begin(); it != ts.tractions.end(); ++it) {
		if((*it)->alwaysavailable) return true;
		if(HasTraction(*it)) return true;
	}
	return false;
}

bool tractionset::HasTraction(const traction_type *tt) const {
	return std::find(tractions.begin(), tractions.end(), tt) != tractions.end();
}

bool tractionset::IsIntersecting(const tractionset &ts) const {
	for(auto &it : tractions) {
		if(ts.HasTraction(it)) return true;
	}
	return false;
}

void tractionset::IntersectWith(const tractionset &ts) {
	container_unordered_remove_if(tractions, [&](traction_type *tt) {
		return !ts.HasTraction(tt);
	});
}

void tractionset::UnionWith(const tractionset &ts) {
	for(auto &it : ts.tractions) {
		AddTractionType(it);
	}
}

void tractionset::Deserialise(const deserialiser_input &di, error_collection &ec) {
	if(!di.json.IsArray()) {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid traction set definition");
		return;
	}
	for(rapidjson::SizeType i = 0; i < di.json.Size(); i++) {
		const rapidjson::Value &cur = di.json[i];
		if(cur.IsString() && di.w) {
			traction_type *tt = di.w->GetTractionTypeByName(cur.GetString());
			if(tt) {
				if(!HasTraction(tt)) tractions.push_back(tt);
			}
			else ec.RegisterNewError<error_deserialisation>(di, string_format("No such traction type: \"%s\"", cur.GetString()));
		}
		else {
			ec.RegisterNewError<error_deserialisation>(di, "Invalid traction set definition");
		}
	}
}

void tractionset::Serialise(serialiser_output &so, error_collection &ec) const {
	for(auto &it : tractions) {
		so.json_out.String(it->name);
	}
	return;
}

std::string tractionset::DumpString() const {
	std::vector<std::string> strs;
	for(auto &it : tractions) {
		strs.push_back(it->name);
	}
	std::sort(strs.begin(), strs.end());
	std::string out;
	for(auto &it : strs) {
		if(!out.empty()) out += ",";
		out += it;
	}
	return out;
}
