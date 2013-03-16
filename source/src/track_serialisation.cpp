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
#include "points.h"
#include "signal.h"
#include "serialisable_impl.h"
#include "error.h"
#include "world.h"

void generictrack::Deserialise(const deserialiser_input &di, error_collection &ec) {
	world_obj::Deserialise(di, ec);

	deserialiser_input subdi(di.json["connect"], "trackconnection", "connect", di);

	if(!subdi.json.IsNull() && subdi.w) {
		di.RegisterProp("connect");
		auto connfunc = [&](const deserialiser_input &funcdi) {
			bool ok = true;
			if(funcdi.json.IsObject()) {
				EDGETYPE this_entrance_direction = EDGE_NULL;
				EDGETYPE target_entrance_direction = EDGE_NULL;
				std::string target_name;
				bool have_directions;
				have_directions = CheckTransJsonValue(this_entrance_direction, funcdi, "fromdirection", ec);
				have_directions &= CheckTransJsonValue(target_entrance_direction, funcdi, "todirection", ec);
				ok = CheckTransJsonValue(target_name, funcdi, "to", ec);

				if(ok) {
					if(have_directions) di.w->ConnectTrack(this, this_entrance_direction, target_name, target_entrance_direction, ec);
					else {
						world *w = di.w;
						auto resolveconnection = [w, this, this_entrance_direction, target_entrance_direction, target_name](error_collection &ec) mutable {
							auto checkconnection = [&](generictrack *gt, EDGETYPE &dir) {
								if(dir != EDGE_NULL) return;
								if(!gt) {
									ec.RegisterNewError<generic_error_obj>(string_format("Partial track connection declaration: no such piece: %s", target_name.c_str()));
									return;
								}

								std::vector<edgelistitem> edges;
								gt->GetListOfEdges(edges);
								EDGETYPE lastfound;
								unsigned int freeedgecount = 0;
								for(auto it = edges.begin(); it != edges.end(); ++it) {
									if(! it->target->IsValid()) {
										lastfound = it->edge;
										freeedgecount++;
									}
								}
								if(freeedgecount == 1) dir = lastfound;
								else {
									ec.RegisterNewError<generic_error_obj>(string_format("Ambiguous partial track connection declaration: piece: %s, has: %d unconnected edges", gt->GetName().c_str(), freeedgecount));
								}
							};
							checkconnection(this, this_entrance_direction);
							checkconnection(w->FindTrackByName(target_name), target_entrance_direction);
							w->ConnectTrack(this, this_entrance_direction, target_name, target_entrance_direction, ec);
						};
						di.w->layout_init_final_fixups.AddFixup(resolveconnection);
					}
				}
			}
			else ok = false;

			if(!ok) {
				ec.RegisterNewError<error_deserialisation>(funcdi, "Invalid track connection definition");
			}
		};

		if(subdi.json.IsArray()) {
			for(rapidjson::SizeType i = 0; i < di.json.Size(); i++) {
				connfunc(deserialiser_input(subdi.json[i], MkArrayRefName(i), subdi));
			}
		}
		else connfunc(subdi);
	}

	CheckTransJsonValueFlag<unsigned int>(gt_privflags, GTPRIVF_REVERSEAUTOCONN, di, "reverseautoconnection", ec);
}

void trackseg::Deserialise(const deserialiser_input &di, error_collection &ec) {
	generictrack::Deserialise(di, ec);

	CheckTransJsonValue(length, di, "length", ec);
	CheckTransJsonValue(elevationdelta, di, "elevationdelta", ec);
	CheckTransJsonValue(traincount, di, "traincount", ec);
	CheckTransJsonSubObj(trs, di, "trs", "trs", ec);
	CheckTransJsonSubArray(speed_limits, di, "speedlimits", "speedlimits", ec);
	CheckTransJsonSubArray(tractiontypes, di, "tractiontypes", "tractiontypes", ec);

	std::string tracksegname;
	if(CheckTransJsonValue(tracksegname, di, "trackcircuit", ec)) {
		tc = GetWorld().FindOrMakeTrackCircuitByName(tracksegname);
	}
}

void trackseg::Serialise(serialiser_output &so, error_collection &ec) const {
	generictrack::Serialise(so, ec);

	SerialiseSubObjJson(trs, so, "trs", ec);
	SerialiseValueJson(traincount, so, "traincount");
}

void DeserialisePointFlags(unsigned int &pflags, const deserialiser_input &di, error_collection &ec) {
	CheckTransJsonValueFlag<unsigned int>(pflags, genericpoints::PTF_REV, di, "reverse", ec);
	CheckTransJsonValueFlag<unsigned int>(pflags, genericpoints::PTF_OOC, di, "ooc", ec);
	CheckTransJsonValueFlag<unsigned int>(pflags, genericpoints::PTF_LOCKED, di, "locked", ec);
	CheckTransJsonValueFlag<unsigned int>(pflags, genericpoints::PTF_REMINDER, di, "reminder", ec);
	CheckTransJsonValueFlag<unsigned int>(pflags, genericpoints::PTF_FAILEDNORM, di, "failednorm", ec);
	CheckTransJsonValueFlag<unsigned int>(pflags, genericpoints::PTF_FAILEDREV, di, "failedrev", ec);
}

void SerialisePointFlags(unsigned int pflags, serialiser_output &so, error_collection &ec) {
	SerialiseFlagJson<unsigned int>(pflags, genericpoints::PTF_REV, so, "reverse");
	SerialiseFlagJson<unsigned int>(pflags, genericpoints::PTF_OOC, so, "ooc");
	SerialiseFlagJson<unsigned int>(pflags, genericpoints::PTF_LOCKED, so, "locked");
	SerialiseFlagJson<unsigned int>(pflags, genericpoints::PTF_REMINDER, so, "reminder");
	SerialiseFlagJson<unsigned int>(pflags, genericpoints::PTF_FAILEDNORM, so, "failednorm");
	SerialiseFlagJson<unsigned int>(pflags, genericpoints::PTF_FAILEDREV, so, "failedrev");
}

void points::Deserialise(const deserialiser_input &di, error_collection &ec) {
	genericpoints::Deserialise(di, ec);

	CheckTransJsonSubObj(trs, di, "trs", "trs", ec);
	DeserialisePointFlags(pflags, di, ec);
}

void points::Serialise(serialiser_output &so, error_collection &ec) const {
	genericpoints::Serialise(so, ec);

	SerialiseSubObjJson(trs, so, "trs", ec);
	SerialisePointFlags(pflags, so, ec);
}

void catchpoints::Deserialise(const deserialiser_input &di, error_collection &ec) {
	genericpoints::Deserialise(di, ec);

	CheckTransJsonSubObj(trs, di, "trs", "trs", ec);
	DeserialisePointFlags(pflags, di, ec);
}

void catchpoints::Serialise(serialiser_output &so, error_collection &ec) const {
	genericpoints::Serialise(so, ec);

	SerialiseSubObjJson(trs, so, "trs", ec);
	SerialisePointFlags(pflags, so, ec);
}

void springpoints::Deserialise(const deserialiser_input &di, error_collection &ec) {
	genericzlentrack::Deserialise(di, ec);

	CheckTransJsonSubObj(trs, di, "trs", "trs", ec);
	CheckTransJsonValue(sendreverse, di, "sendreverse", ec);
}

void springpoints::Serialise(serialiser_output &so, error_collection &ec) const {
	genericzlentrack::Serialise(so, ec);

	SerialiseSubObjJson(trs, so, "trs", ec);
}

void crossover::Deserialise(const deserialiser_input &di, error_collection &ec) {
	genericzlentrack::Deserialise(di, ec);

	CheckTransJsonSubObj(trs, di, "trs", "trs", ec);
}

void crossover::Serialise(serialiser_output &so, error_collection &ec) const {
	genericzlentrack::Serialise(so, ec);

	SerialiseSubObjJson(trs, so, "trs", ec);
}

class pointsflagssubobj : public serialisable_obj {
	unsigned int *pflags;
	unsigned int inpflags;

	public:
	pointsflagssubobj(unsigned int *pf) : pflags(pf), inpflags(*pf) { }
	pointsflagssubobj(unsigned int pf) : pflags(0), inpflags(pf) { }

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) {
		if(pflags) DeserialisePointFlags(*pflags, di, ec);
	}
	virtual void Serialise(serialiser_output &so, error_collection &ec) const {
		SerialisePointFlags(inpflags, so, ec);
	}
};

void doubleslip::Deserialise(const deserialiser_input &di, error_collection &ec) {
	genericpoints::Deserialise(di, ec);

	CheckTransJsonSubObj(trs, di, "trs", "trs", ec);
	if(CheckTransJsonValue(dof, di, "degreesoffreedom", ec)) {
		if(dof != 1 && dof != 2 && dof != 4) {
			ec.RegisterNewError<error_deserialisation>(di, "Invalid double slip degrees of freedom: " + std::to_string(dof));
		}
	}

	CheckTransJsonValueFlag<unsigned int>(dsflags, DSF_NO_FL_BL, di, "notrack_fl_bl", ec);
	CheckTransJsonValueFlag<unsigned int>(dsflags, DSF_NO_FR_BL, di, "notrack_fr_bl", ec);
	CheckTransJsonValueFlag<unsigned int>(dsflags, DSF_NO_FL_BR, di, "notrack_fl_br", ec);
	CheckTransJsonValueFlag<unsigned int>(dsflags, DSF_NO_FR_BR, di, "notrack_fr_br", ec);

	if(__builtin_popcount(dsflags&DSF_NO_TRACK_MASK) >= 2) {
		ec.RegisterNewError<error_deserialisation>(di, "Cannot remove more than one track edge from a double-slip, use points or a crossover instead");
		return;
	}

	UpdatePointsFixedStatesFromMissingTrackEdges();

	auto deserialisepointsflags = [&](EDGETYPE direction, const char *prop) {
		unsigned int pf = GetCurrentPointFlags(direction);
		pointsflagssubobj ps(&pf);
		CheckTransJsonSubObj(ps, di, prop, "", ec);
		SetPointFlagsMasked(GetCurrentPointIndex(direction), pf, genericpoints::PTF_SERIALISABLE);
	};
	deserialisepointsflags(EDGE_DS_FL, "leftfrontpoints");
	deserialisepointsflags(EDGE_DS_FR, "rightfrontpoints");
	deserialisepointsflags(EDGE_DS_BR, "rightbackpoints");
	deserialisepointsflags(EDGE_DS_BL, "leftbackpoints");
}

void doubleslip::Serialise(serialiser_output &so, error_collection &ec) const {
	genericpoints::Serialise(so, ec);

	SerialiseSubObjJson(trs, so, "trs", ec);
	SerialiseSubObjJson(pointsflagssubobj(GetCurrentPointFlags(EDGE_DS_FL)), so, "forwardleftpoints", ec);
	if(dof>=2) SerialiseSubObjJson(pointsflagssubobj(GetCurrentPointFlags(EDGE_DS_BR)), so, "reverseleftpoints", ec);
	if(dof>=4) {
		SerialiseSubObjJson(pointsflagssubobj(GetCurrentPointFlags(EDGE_DS_FR)), so, "forwardrightpoints", ec);
		SerialiseSubObjJson(pointsflagssubobj(GetCurrentPointFlags(EDGE_DS_BL)), so, "reverserightpoints", ec);
	}
}

//if after_layout_init_resolve is true, and the target piece cannot be found (does not exist yet),
//then a fixup is added to after_layout_init in world to resolve it and set output,
//such that output *must* remain in scope throughout the layout initialisation phase
void DeserialiseRouteTargetByParentAndIndex(const route *& output, const deserialiser_input &di, error_collection &ec, bool after_layout_init_resolve) {
	std::string targname;
	if(CheckTransJsonValue(targname, di, "route_parent", ec)) {
		unsigned int index;
		CheckTransJsonValueDef(index, di, "route_index", 0, ec);

		auto routetargetresolutionerror = [](error_collection &ec, const std::string &targname) {
			ec.RegisterNewError<generic_error_obj>("Cannot resolve route target: " + targname);
		};

		if(di.w) {
			routingpoint *rp = dynamic_cast<routingpoint *>(di.w->FindTrackByName(targname));
			if(rp) {
				output = rp->GetRouteByIndex(index);
				return;
			}
			else if(after_layout_init_resolve) {
				world *w = di.w;
				auto resolveroutetarget = [w, targname, index, &output, routetargetresolutionerror](error_collection &ec) {
					routingpoint *rp = dynamic_cast<routingpoint *>(w->FindTrackByName(targname));
					if(rp) output = rp->GetRouteByIndex(index);
					else routetargetresolutionerror(ec, targname);
				};
				di.w->layout_init_final_fixups.AddFixup(resolveroutetarget);
			}
			else routetargetresolutionerror(ec, targname);
		}
	}
	output = 0;
	return;
}

void track_reservation_state::Deserialise(const deserialiser_input &di, error_collection &ec) {
	itrss.clear();
	CheckIterateJsonArrayOrType<json_object>(di, "reservations", "track_reservation", ec, [&](const deserialiser_input &innerdi, error_collection &ec) {
		itrss.emplace_back();
		inner_track_reservation_state &itrs = itrss.back();

		DeserialiseRouteTargetByParentAndIndex(itrs.reserved_route, innerdi, ec, true);
		CheckTransJsonValue(itrs.direction, innerdi, "direction", ec);
		CheckTransJsonValue(itrs.index, innerdi, "index", ec);
		CheckTransJsonValue(itrs.rr_flags, innerdi, "rr_flags", ec);
		innerdi.PostDeserialisePropCheck(ec);
	});
}

void track_reservation_state::Serialise(serialiser_output &so, error_collection &ec) const {
	if(!itrss.empty()) {
		so.json_out.String("reservations");
		so.json_out.StartArray();
		for(auto it = itrss.begin(); it != itrss.end(); ++it) {
			so.json_out.StartObject();
			if(it->reserved_route) {
				if(it->reserved_route->parent) {
					SerialiseValueJson(it->reserved_route->parent->GetName(), so, "route_parent");
					SerialiseValueJson(it->reserved_route->index, so, "route_index");
				}
			}
			SerialiseValueJson(it->direction, so, "direction");
			SerialiseValueJson(it->index, so, "index");
			SerialiseValueJson(it->rr_flags, so, "rr_flags");
			so.json_out.EndObject();
		}
		so.json_out.EndArray();
	}
}

void speedrestrictionset::Deserialise(const deserialiser_input &di, error_collection &ec) {
	for(rapidjson::SizeType i = 0; i < di.json.Size(); i++) {
		deserialiser_input subdi(di.json[i], "speedrestriction", MkArrayRefName(i), di);
		speed_restriction sr;
		if(subdi.json.IsObject() && CheckTransJsonValueDef(sr.speedclass, subdi, "speedclass", "", ec) && CheckTransJsonValueDef(sr.speed, subdi, "speed", 0, ec)) {
			AddSpeedRestriction(sr);
		}
		else {
			ec.RegisterNewError<error_deserialisation>(subdi, "Invalid speed restriction definition");
		}
	}
}

void speedrestrictionset::Serialise(serialiser_output &so, error_collection &ec) const {
	return;
}
