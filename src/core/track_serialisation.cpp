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

#include "common.h"
#include "util/error.h"
#include "core/track.h"
#include "core/trackpiece.h"
#include "core/points.h"
#include "core/signal.h"
#include "core/serialisable_impl.h"
#include "core/world.h"
#include "core/deserialisation_scalarconv.h"
#include "core/trackcircuit.h"

void generictrack::Deserialise(const deserialiser_input &di, error_collection &ec) {
	world_obj::Deserialise(di, ec);

	CheckIterateJsonArrayOrType<json_object>(di, "layout", "layout", ec, [&](const deserialiser_input &di, error_collection &ec) {
		if(di.ws) di.ws->gui_layout_generictrack(this, di, ec);
	});

	deserialiser_input sddi(di.json["sighting"], "sighting", "sighting", di);
	if(!sddi.json.IsNull()) {
		di.RegisterProp("sighting");

		auto sightingfunc = [&](const deserialiser_input &funcdi) {
			bool ok = false;

			if(funcdi.json.IsObject()) {
				EDGETYPE dir;
				unsigned int distance;
				if(CheckTransJsonValue(dir, funcdi, "direction", ec) && CheckTransJsonValueProc(distance, funcdi, "distance", ec, dsconv::Length)) {
					for(auto &it : sighting_distances) {
						if(it.first == dir) {
							it.second = distance;
							ok = true;
							break;
						}
					}
				}
			}

			if(!ok) {
				ec.RegisterNewError<error_deserialisation>(funcdi, "Invalid sighting distance definition");
			}
		};

		if(sddi.json.IsUint() || sddi.json.IsString()) {
			unsigned int distance;
			if(TransJsonValueProc(sddi.json, distance, di, "sighting", ec, dsconv::Length)) {
				for(auto &it : sighting_distances) {
					it.second = distance;
				}
			}
		}
		else if(sddi.json.IsArray()) {
			for(rapidjson::SizeType i = 0; i < sddi.json.Size(); i++) {
				sightingfunc(deserialiser_input(sddi.json[i], MkArrayRefName(i), sddi));
			}
		}
		else sightingfunc(sddi);
	}

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
			for(rapidjson::SizeType i = 0; i < subdi.json.Size(); i++) {
				connfunc(deserialiser_input(subdi.json[i], MkArrayRefName(i), subdi));
			}
		}
		else connfunc(subdi);
	}

	CheckTransJsonValueFlag(gt_privflags, GTPRIVF::REVERSEAUTOCONN, di, "reverseautoconnection", ec);

	if(CanHaveBerth()) {
		deserialiser_input bdi(di.json["berth"], "berth", "berth", di);
		if(!bdi.json.IsNull()) {
			di.RegisterProp("berth");

			bool berthval = false;
			EDGETYPE berthedge = EDGE_NULL;
			if(bdi.json.IsBool()) berthval = bdi.json.GetBool();
			else if(IsType<EDGETYPE>(bdi.json)) {
				berthval = true;
				berthedge = GetType<EDGETYPE>(bdi.json);
			}
			else ec.RegisterNewError<error_deserialisation>(bdi, "Invalid track berth definition");

			if(berthval) {
				if(!berth) berth.reset(new trackberth);
				berth->direction = berthedge;
			}
			else berth.reset();
		}
		if(berth) {
			CheckTransJsonValue(berth->contents, di, "berthstr", ec);
			CheckIterateJsonArrayOrType<json_object>(di, "berthlayout", "layout", ec, [&](const deserialiser_input &di, error_collection &ec) {
				if(di.ws) di.ws->gui_layout_trackberth(berth.get(), this, di, ec);
			});
		}
	}

	std::string tracksegname;
	if(CheckTransJsonValue(tracksegname, di, "trackcircuit", ec)) {
		tc = GetWorld().track_circuits.FindOrMakeByName(tracksegname);
		tc->RegisterTrack(this);
	}
}

void generictrack::Serialise(serialiser_output &so, error_collection &ec) const {
	world_obj::Serialise(so, ec);
	if(berth) {
		SerialiseValueJson(berth->contents, so, "berthstr");
	}
}

void trackseg::Deserialise(const deserialiser_input &di, error_collection &ec) {
	generictrack::Deserialise(di, ec);

	CheckTransJsonValueProc(length, di, "length", ec, dsconv::Length);
	CheckTransJsonValueProc(elevationdelta, di, "elevationdelta", ec, dsconv::Length);
	CheckTransJsonValue(traincount, di, "traincount", ec);
	CheckTransJsonSubObj(trs, di, "trs", "trs", ec);
	CheckTransJsonSubArray(speed_limits, di, "speedlimits", "speedlimits", ec);
	CheckTransJsonSubArray(tractiontypes, di, "tractiontypes", "tractiontypes", ec);

	CheckIterateJsonArrayOrType<std::string>(di, "tracktriggers", "tracktrigger", ec, [&](const deserialiser_input &sdi, error_collection &ec) {
		track_train_counter_block *ttcb = GetWorld().track_triggers.FindOrMakeByName(GetType<std::string>(sdi.json));
		ttcbs.push_back(ttcb);
		ttcb->RegisterTrack(this);
	});
}

void trackseg::Serialise(serialiser_output &so, error_collection &ec) const {
	generictrack::Serialise(so, ec);

	SerialiseSubObjJson(trs, so, "trs", ec);
	SerialiseValueJson(traincount, so, "traincount");
}

void DeserialisePointFlags(genericpoints::PTF &pflags, const deserialiser_input &di, error_collection &ec) {
	CheckTransJsonValueFlag(pflags, genericpoints::PTF::REV, di, "reverse", ec);
	CheckTransJsonValueFlag(pflags, genericpoints::PTF::OOC, di, "ooc", ec);
	CheckTransJsonValueFlag(pflags, genericpoints::PTF::LOCKED, di, "locked", ec);
	CheckTransJsonValueFlag(pflags, genericpoints::PTF::REMINDER, di, "reminder", ec);
	CheckTransJsonValueFlag(pflags, genericpoints::PTF::FAILEDNORM, di, "failednorm", ec);
	CheckTransJsonValueFlag(pflags, genericpoints::PTF::FAILEDREV, di, "failedrev", ec);
}

void SerialisePointFlags(genericpoints::PTF pflags, serialiser_output &so, error_collection &ec) {
	SerialiseFlagJson(pflags, genericpoints::PTF::REV, so, "reverse");
	SerialiseFlagJson(pflags, genericpoints::PTF::OOC, so, "ooc");
	SerialiseFlagJson(pflags, genericpoints::PTF::LOCKED, so, "locked");
	SerialiseFlagJson(pflags, genericpoints::PTF::REMINDER, so, "reminder");
	SerialiseFlagJson(pflags, genericpoints::PTF::FAILEDNORM, so, "failednorm");
	SerialiseFlagJson(pflags, genericpoints::PTF::FAILEDREV, so, "failedrev");
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
	genericpoints::PTF *pflags;
	genericpoints::PTF inpflags;

	public:
	pointsflagssubobj(genericpoints::PTF *pf) : pflags(pf), inpflags(*pf) { }
	pointsflagssubobj(genericpoints::PTF pf) : pflags(0), inpflags(pf) { }

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

	CheckTransJsonValueFlag(dsflags, DSF::NO_FL_BL, di, "notrack_fl_bl", ec);
	CheckTransJsonValueFlag(dsflags, DSF::NO_FR_BL, di, "notrack_fr_bl", ec);
	CheckTransJsonValueFlag(dsflags, DSF::NO_FL_BR, di, "notrack_fl_br", ec);
	CheckTransJsonValueFlag(dsflags, DSF::NO_FR_BR, di, "notrack_fr_br", ec);

	if(__builtin_popcount(flag_unwrap<DSF>(dsflags&DSF::NO_TRACK_MASK)) >= 2) {
		ec.RegisterNewError<error_deserialisation>(di, "Cannot remove more than one track edge from a double-slip, use points or a crossover instead");
		return;
	}

	UpdatePointsFixedStatesFromMissingTrackEdges();
	UpdateInternalCoupling();

	auto deserialisepointsflags = [&](EDGETYPE direction, const char *prop) {
		genericpoints::PTF pf = GetCurrentPointFlags(direction);
		pointsflagssubobj ps(&pf);
		CheckTransJsonSubObj(ps, di, prop, "", ec);
		SetPointsFlagsMasked(GetPointsIndexByEdge(direction), pf, genericpoints::PTF::SERIALISABLE);
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
		auto routeindexresolutionerror = [](error_collection &ec, const std::string &targname, unsigned int index) {
			ec.RegisterNewError<generic_error_obj>("Cannot resolve route target: " + targname + ", index: " + std::to_string(index));
		};

		if(di.w) {
			routingpoint *rp = FastRoutingpointCast(di.w->FindTrackByName(targname));
			if(rp) {
				output = rp->GetRouteByIndex(index);
				if(!output) routeindexresolutionerror(ec, targname, index);
				return;
			}
			else if(after_layout_init_resolve) {
				world *w = di.w;
				auto resolveroutetarget = [w, targname, index, &output, routetargetresolutionerror, routeindexresolutionerror](error_collection &ec) {
					routingpoint *rp = FastRoutingpointCast(w->FindTrackByName(targname));
					if(rp) {
						output = rp->GetRouteByIndex(index);
						if(!output) routeindexresolutionerror(ec, targname, index);
					}
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

void speedrestrictionset::Deserialise(const deserialiser_input &di, error_collection &ec) {
	for(rapidjson::SizeType i = 0; i < di.json.Size(); i++) {
		deserialiser_input subdi(di.json[i], "speedrestriction", MkArrayRefName(i), di);
		speed_restriction sr;
		if(subdi.json.IsObject() && CheckTransJsonValueDef(sr.speedclass, subdi, "speedclass", "", ec)
				&& CheckTransJsonValueDefProc(sr.speed, subdi, "speed", 0, ec, dsconv::Speed)) {
			AddSpeedRestriction(sr);
			subdi.PostDeserialisePropCheck(ec);
		}
		else {
			ec.RegisterNewError<error_deserialisation>(subdi, "Invalid speed restriction definition");
		}
	}
}

void speedrestrictionset::Serialise(serialiser_output &so, error_collection &ec) const {
	return;
}

void DeserialisePointsCoupling(const deserialiser_input &di, error_collection &ec) {
	bool ok = true;
	deserialiser_input subdi(di.json["points"], "pointscouplingarray", "points", di);
	if(subdi.json.IsArray() && subdi.json.Size() >= 2) {
		di.RegisterProp("points");
		auto params = std::make_shared<std::vector<std::pair<std::string, EDGETYPE> > >();
		for(rapidjson::SizeType i = 0; i < subdi.json.Size(); i++) {
			deserialiser_input itemdi(subdi.json[i], "pointscoupling", MkArrayRefName(i), subdi);
			std::string name;
			EDGETYPE direction;
			if(itemdi.json.IsObject() && CheckTransJsonValueDef(name, itemdi, "name", "", ec) && CheckTransJsonValueDef(direction, itemdi, "edge", EDGE_NULL, ec)) {
				params->emplace_back(name, direction);
				itemdi.PostDeserialisePropCheck(ec);
			}
			else ok = false;
		}
		if(ok) {
			world *w = di.w;
			w->layout_init_final_fixups.AddFixup([params, w](error_collection &ec) {
				std::vector<vartrack_target_ptr<genericpoints> > points_list;
				points_list.reserve(params->size());
				for(auto it : *params) {
					genericpoints* p = dynamic_cast<genericpoints*>(w->FindTrackByName(it.first));
					if(!p) {
						ec.RegisterNewError<generic_error_obj>("Points coupling: no such points: " + it.first);
						return;
					}
					if(!p->IsCoupleable(it.second)) {
						ec.RegisterNewError<generic_error_obj>("Points coupling: points cannot be coupled: " + it.first);
						return;
					}
					points_list.emplace_back(p, it.second);
				}
				for(auto it = points_list.begin(); it != points_list.end(); ++it) {
					for(auto jt = it + 1; jt != points_list.end(); ++jt) {
						std::vector<genericpoints::points_coupling> pci;
						std::vector<genericpoints::points_coupling> pcj;
						it->track->GetCouplingPointsFlagsByEdge(it->direction, pci);
						jt->track->GetCouplingPointsFlagsByEdge(jt->direction, pcj);
						for(auto kt : pci) {
							for(auto lt : pcj) {
								genericpoints::PTF xormask = kt.xormask ^ lt.xormask;
								kt.targ->CouplePointsFlagsAtIndexTo(kt.index, genericpoints::points_coupling(lt.pflags, xormask, lt.targ, lt.index));
								lt.targ->CouplePointsFlagsAtIndexTo(lt.index, genericpoints::points_coupling(kt.pflags, xormask, kt.targ, kt.index));
							}
						}
					}
					for(unsigned int i = 0; i < it->track->GetPointsCount(); i++) {
						it->track->SetPointsFlagsMasked(i, it->track->GetPointsFlags(i)&genericpoints::PTF::SERIALISABLE, genericpoints::PTF::SERIALISABLE);
					}
				}
			});
		}
	}
	else ok = false;


	if(!ok) {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid points coupling definition");
	}
}

template<> void track_target_ptr::Deserialise(const std::string &name, const deserialiser_input &di, error_collection &ec) {
	auto parse = [&](const deserialiser_input &edi, error_collection &ec) {
		std::string piece;
		if(!edi.w || !CheckTransJsonValue(piece, edi, "piece", ec)) {
			Reset();
			return;
		}
		track = edi.w->FindTrackByName(piece);
		if(!track) {
			ec.RegisterNewError<error_deserialisation>(edi, "Invalid track target definition: no such piece");
			return;
		}
		CheckTransJsonValueDef(direction, edi, "dir", EDGETYPE::EDGE_NULL, ec);
	};

	if(name.empty()) {
		parse(di, ec);
	}
	else {
		CheckTransJsonTypeFunc<json_object>(di, name.c_str(), "track_target_ptr", ec, parse);
	}
}

template<> void track_target_ptr::Serialise(const std::string &name, serialiser_output &so, error_collection &ec) const {
	if(!name.empty()) {
		so.json_out.String(name);
		so.json_out.StartObject();
	}
	if(IsValid()) {
		SerialiseValueJson(track->GetName(), so, "piece");
		SerialiseValueJson(direction, so, "dir");
	}
	if(!name.empty()) so.json_out.EndObject();
}

template<> void track_location::Deserialise(const std::string &name, const deserialiser_input &di, error_collection &ec) {
	auto parse = [&](const deserialiser_input &edi, error_collection &ec) {
		trackpiece.Deserialise("", edi, ec);
		CheckTransJsonValueDef(offset, edi, "offset", 0, ec);
	};

	if(name.empty()) {
		parse(di, ec);
	}
	else {
		CheckTransJsonTypeFunc<json_object>(di, name.c_str(), "track_location", ec, parse);
	}
}

template<> void track_location::Serialise(const std::string &name, serialiser_output &so, error_collection &ec) const {
	if(!name.empty()) {
		so.json_out.String(name);
		so.json_out.StartObject();
	}
	if(IsValid()) {
		trackpiece.Serialise("", so, ec);
		SerialiseValueJson(offset, so, "offset");
	}
	if(!name.empty()) so.json_out.EndObject();
}
