//  grass - Generic Rail And Signalling Simulator
//
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

bool tractionset::CanTrainPass(train *t) const {
	for(auto it = t->GetTractionTypes().tractions.begin(); it != t->GetTractionTypes().tractions.end(); ++it) {
		if((*it)->alwaysavailable) return true;
		if(std::find(tractions.begin(), tractions.end(), (*it)) != tractions.end()) return true;
	}
	return false;
}

void tractionset::Deserialise(const deserialiser_input &di, error_collection &ec) {
	for(rapidjson::SizeType i = 0; i < di.json.Size(); i++) {
		const rapidjson::Value &cur = di.json[i];
		if(cur.IsString() && di.w) {
			traction_type *tt = di.w->GetTractionTypeByName(cur.GetString());
			if(tt) tractions.push_back(tt);
			else ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(string_format("No such traction type: \"%s\"", cur.GetString()))));
		}
		else {
			ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation("Invalid traction set definition")));
		}
	}
}

void tractionset::Serialise(serialiser_output &so, error_collection &ec) const {
	return;
}