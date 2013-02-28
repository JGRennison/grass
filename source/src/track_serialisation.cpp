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

void generictrack::DeserialiseGenericTrackCommon(const deserialiser_input &di, error_collection &ec) {
	deserialiser_input subdi(di.json["connect"], "trackconnection", "connect", di);

	if(!subdi.json.IsNull() && subdi.w) {
		auto connfunc = [&](const deserialiser_input &funcdi) {
			bool ok = true;
			if(funcdi.json.IsObject()) {
				DIRTYPE this_entrance_direction = TDIR_NULL;
				DIRTYPE target_entrance_direction = TDIR_NULL;
				std::string target_name;
				ok = CheckTransJsonValue(this_entrance_direction, funcdi, "fromdirection", ec);
				ok &= CheckTransJsonValue(target_entrance_direction, funcdi, "todirection", ec);
				ok &= CheckTransJsonValue(target_name, funcdi, "to", ec);
				
				if(ok) {
					di.w->ConnectTrack(this, this_entrance_direction, target_name, target_entrance_direction, ec);

					/*
					generictrack *gt = di.w->FindTrackByName(target_name);
					if(gt) this->FullConnect(this_entrance_direction, track_target_ptr(gt, target_entrance_direction), ec);
					else ok = false;
					*/
				}
			}
			else ok = false;

			if(!ok) {
				ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(funcdi, "Invalid track connection definition")));
			}
		};

		if(subdi.json.IsArray()) {
			for(rapidjson::SizeType i = 0; i < di.json.Size(); i++) {
				connfunc(deserialiser_input(subdi.json[i], std::to_string(i), subdi));
			}
		}
		else connfunc(subdi);
	}
}

void trackseg::Deserialise(const deserialiser_input &di, error_collection &ec) {
	CheckTransJsonValue(length, di, "length", ec);
	CheckTransJsonValue(elevationdelta, di, "elevationdelta", ec);
	CheckTransJsonValue(traincount, di, "traincount", ec);
	CheckTransJsonSubObj(trs, di, "trs", "trs", ec, di.w);
	CheckTransJsonSubArray(speed_limits, di, "speedlimits", "speedlimits", ec, di.w);
	CheckTransJsonSubArray(tractiontypes, di, "tractiontypes", "tractiontypes", ec, di.w);
	DeserialiseGenericTrackCommon(di, ec);
}

void trackseg::Serialise(serialiser_output &so, error_collection &ec) const {
	SerialiseSubObjJson(trs, so, "trs", ec);
	SerialiseValueJson(traincount, so, "traincount");
}

void points::Deserialise(const deserialiser_input &di, error_collection &ec) {
	CheckTransJsonSubObj(trs, di, "trs", "trs", ec, di.w);
	CheckTransJsonValueFlag<unsigned int>(pflags, PTF_REV, di, "reverse", ec);
	CheckTransJsonValueFlag<unsigned int>(pflags, PTF_OOC, di, "ooc", ec);
	CheckTransJsonValueFlag<unsigned int>(pflags, PTF_LOCKED, di, "locked", ec);
	CheckTransJsonValueFlag<unsigned int>(pflags, PTF_REMINDER, di, "reminder", ec);
	CheckTransJsonValueFlag<unsigned int>(pflags, PTF_FAILED, di, "failed", ec);
	DeserialiseGenericTrackCommon(di, ec);
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
	if(CheckTransJsonValue(targname, di, "route_parent", ec)) {
		unsigned int index;
		CheckTransJsonValueDef(index, di, "route_index", 0, ec);
		if(di.w) {
			generictrack *gt = di.w->FindTrackByName(targname);
			routingpoint *rp = dynamic_cast<routingpoint *>(gt);
			if(rp) {
				reserved_route = rp->GetRouteByIndex(index);
			}
		}
	}
	if(CheckGetJsonValueDef<bool, bool>(di, "no_route", false, ec)) reserved_route = 0;
	CheckTransJsonValue(direction, di, "direction", ec);
	CheckTransJsonValue(index, di, "index", ec);
	CheckTransJsonValue(rr_flags, di, "rr_flags", ec);
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
		deserialiser_input subdi(di.json[i], "speedrestriction", std::to_string(i), di);
		speed_restriction sr;
		if(subdi.json.IsObject() && CheckTransJsonValueDef(sr.speedclass, subdi, "speedclass", "", ec) && CheckTransJsonValueDef(sr.speed, subdi, "speed", 0, ec)) {
			AddSpeedRestriction(sr);
		}
		else {
			ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(subdi, "Invalid speed restriction definition")));
		}
	}
}

void speedrestrictionset::Serialise(serialiser_output &so, error_collection &ec) const {
	return;
}