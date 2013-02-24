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

#include "common.h"
#include "track.h"
#include "signal.h"
#include "serialisable_impl.h"
#include "error.h"
#include "world.h"

bool trackseg::Deserialise(const deserialiser_input &di, error_collection &ec) {
	CheckTransJsonValue(length, di.json, "length", ec);
	CheckTransJsonValue(elevationdelta, di.json, "elevationdelta", ec);
	CheckTransJsonValue(traincount, di.json, "traincount", ec);
	CheckTransJsonSubObj(trs, di.json, "trs", "trs", ec, di.w);
	return true;
}

bool trackseg::Serialise(serialiser_output &so, error_collection &ec) const {
	SerialiseSubObjJson(trs, so, "trs", ec);
	SerialiseValueJson(traincount, so, "traincount");
	return true;
}

bool points::Deserialise(const deserialiser_input &di, error_collection &ec) {
	return false;
}

bool points::Serialise(serialiser_output &so, error_collection &ec) const {
	return false;
}

bool track_reservation_state::Deserialise(const deserialiser_input &di, error_collection &ec) {
	std::string targname;
	if(CheckTransJsonValue(targname, di.json, "route_parent", ec)) {
		unsigned int index;
		CheckTransJsonValueDef(index, di.json, "route_index", 0, ec);
		if(di.w) {
			generictrack *gt = di.w->FindTrackByName(targname);
			routingpoint *rp = dynamic_cast<routingpoint *>(gt);
			if(rp) {
				reserved_route = rp->GetRouteByIndex(index);
			}
		}
	}
	CheckTransJsonValue(direction, di.json, "direction", ec);
	CheckTransJsonValue(index, di.json, "index", ec);
	CheckTransJsonValue(rr_flags, di.json, "rr_flags", ec);
	return true;
}

bool track_reservation_state::Serialise(serialiser_output &so, error_collection &ec) const {
	return false;
}
