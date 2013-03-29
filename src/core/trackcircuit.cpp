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

#include "trackcircuit.h"
#include "serialisable_impl.h"

void track_circuit::TrainEnter(train *t) {
	traincount++;
	for(auto it = occupying_trains.begin(); it != occupying_trains.end(); ++it) {
		if(it->t == t) {
			it->count++;
			return;
		}
	}
	occupying_trains.emplace_back(t, 1);
}
void track_circuit::TrainLeave(train *t) {
	traincount--;
	for(auto it = occupying_trains.begin(); it != occupying_trains.end(); ++it) {
		if(it->t == t) {
			it->count--;
			if(it->count == 0) {
				*it = *occupying_trains.rbegin();
				occupying_trains.pop_back();
			}
			return;
		}
	}
}

void track_circuit::Deserialise(const deserialiser_input &di, error_collection &ec) {
	world_obj::Deserialise(di, ec);

	//CheckTransJsonValue(traincount, di, "traincount", ec);
}

void track_circuit::Serialise(serialiser_output &so, error_collection &ec) const {
	world_obj::Serialise(so, ec);

	//SerialiseValueJson(traincount, so, "traincount");
}

track_circuit::TCF track_circuit::GetTCFlags() const {
	return tc_flags;
}

track_circuit::TCF track_circuit::SetTCFlagsMasked(TCF bits, TCF mask) {
	tc_flags = (tc_flags & ~mask) | (bits & mask);
	return tc_flags;
}
