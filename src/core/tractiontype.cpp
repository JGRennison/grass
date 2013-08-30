//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
//  WEBSITE: http://grss.sourceforge.net
//
//  NOTE: This software is licensed under the GPL. See: COPYING-GPL.txt
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.
//
//  Jonathan Rennison (or anybody else) is in no way responsible, or liable
//  for this program or its use in relation to users, 3rd parties or to any
//  persons in any way whatsoever.
//
//  You  should have  received a  copy of  the GNU  General Public
//  License along  with this program; if  not, write to  the Free Software
//  Foundation, Inc.,  59 Temple Place,  Suite 330, Boston,  MA 02111-1307
//  USA
//
//  2013 - Jonathan Rennison <j.g.rennison@gmail.com>
//==========================================================================

#include <algorithm>

#include "common.h"
#include "tractiontype.h"
#include "serialisable_impl.h"
#include "train.h"

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
