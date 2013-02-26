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

void trackseg::Deserialise(const deserialiser_input &di, error_collection &ec) {
	CheckTransJsonValue(length, di.json, "length", ec);
	CheckTransJsonValue(elevationdelta, di.json, "elevationdelta", ec);
	CheckTransJsonValue(traincount, di.json, "traincount", ec);
	CheckTransJsonSubObj(trs, di.json, "trs", "trs", ec, di.w);
	CheckTransJsonSubArray(speed_limits, di.json, "speedlimits", "speedlimits", ec, di.w);
	CheckTransJsonSubArray(tractiontypes, di.json, "tractiontypes", "tractiontypes", ec, di.w);
}

void trackseg::Serialise(serialiser_output &so, error_collection &ec) const {
	SerialiseSubObjJson(trs, so, "trs", ec);
	SerialiseValueJson(traincount, so, "traincount");
}

void points::Deserialise(const deserialiser_input &di, error_collection &ec) {
	CheckTransJsonSubObj(trs, di.json, "trs", "trs", ec, di.w);
	CheckTransJsonValueFlag<unsigned int>(pflags, PTF_REV, di.json, "reverse", ec);
	CheckTransJsonValueFlag<unsigned int>(pflags, PTF_OOC, di.json, "ooc", ec);
	CheckTransJsonValueFlag<unsigned int>(pflags, PTF_LOCKED, di.json, "locked", ec);
	CheckTransJsonValueFlag<unsigned int>(pflags, PTF_REMINDER, di.json, "reminder", ec);
	CheckTransJsonValueFlag<unsigned int>(pflags, PTF_FAILED, di.json, "failed", ec);
}

void points::Serialise(serialiser_output &so, error_collection &ec) const {
	SerialiseSubObjJson(trs, so, "trs", ec);
	SerialiseFlagJson<unsigned int>(pflags, PTF_REV, so, "reverse");
	SerialiseFlagJson<unsigned int>(pflags, PTF_OOC, so, "ooc");
	SerialiseFlagJson<unsigned int>(pflags, PTF_LOCKED, so, "locked");
	SerialiseFlagJson<unsigned int>(pflags, PTF_REMINDER, so, "reminder");
	SerialiseFlagJson<unsigned int>(pflags, PTF_FAILED, so, "failed");

}

void track_reservation_state::Deserialise(const deserialiser_input &di, error_collection &ec) {
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
	if(CheckGetJsonValueDef<bool, bool>(di.json, "no_route", false, ec)) reserved_route = 0;
	CheckTransJsonValue(direction, di.json, "direction", ec);
	CheckTransJsonValue(index, di.json, "index", ec);
	CheckTransJsonValue(rr_flags, di.json, "rr_flags", ec);
}

void track_reservation_state::Serialise(serialiser_output &so, error_collection &ec) const {
	if(reserved_route) {
		if(reserved_route->parent) {
			SerialiseValueJson(reserved_route->parent->GetName(), so, "route_parent");
			SerialiseValueJson(reserved_route->index, so, "route_index");
		}
	}
	else SerialiseValueJson(true, so, "no_route");
	SerialiseValueJson(direction, so, "direction");
	SerialiseValueJson(index, so, "index");
	SerialiseValueJson(rr_flags, so, "rr_flags");
}

void speedrestrictionset::Deserialise(const deserialiser_input &di, error_collection &ec) {
	for(rapidjson::SizeType i = 0; i < di.json.Size(); i++) {
		const rapidjson::Value &cur = di.json[i];
		speed_restriction sr;
		if(cur.IsObject() && CheckTransJsonValueDef(sr.speedclass, cur, "speedclass", "", ec) && CheckTransJsonValueDef(sr.speed, cur, "speed", 0, ec)) {
			AddSpeedRestriction(sr);
		}
		else {
			ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation("Invalid speed restriction definition")));
		}
	}
}

void speedrestrictionset::Serialise(serialiser_output &so, error_collection &ec) const {
	return;
}