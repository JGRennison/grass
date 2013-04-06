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
#include "trackpiece.h"
#include "points.h"
#include "signal.h"
#include "serialisable_impl.h"
#include "error.h"
#include "world.h"

void generictrack::Deserialise(const deserialiser_input &di, error_collection &ec) {
	world_obj::Deserialise(di, ec);

	deserialiser_input sddi(di.json["sighting"], "sighting", "sighting", di);
	if(!sddi.json.IsNull()) {
		di.RegisterProp("sighting");

		auto sightingfunc = [&](const deserialiser_input &funcdi) {
			bool ok = false;

			if(funcdi.json.IsObject()) {
				EDGETYPE dir;
				unsigned int distance;
				if(CheckTransJsonValue(dir, funcdi, "direction", ec) && CheckTransJsonValue(distance, funcdi, "distance", ec)) {
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

		if(sddi.json.IsUint()) {
			unsigned int distance = sddi.json.GetUint();
			for(auto &it : sighting_distances) {
				it.second = distance;
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
			for(rapidjson::SizeType i = 0; i < di.json.Size(); i++) {
				connfunc(deserialiser_input(subdi.json[i], MkArrayRefName(i), subdi));
			}
		}
		else connfunc(subdi);
	}

	CheckTransJsonValueFlag(gt_privflags, GTPRIVF::REVERSEAUTOCONN, di, "reverseautoconnection", ec);
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

	if(__builtin_popcount(dsflags&DSF::NO_TRACK_MASK) >= 2) {
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

		if(di.w) {
			routingpoint *rp = FastRoutingpointCast(di.w->FindTrackByName(targname));
			if(rp) {
				output = rp->GetRouteByIndex(index);
				return;
			}
			else if(after_layout_init_resolve) {
				world *w = di.w;
				auto resolveroutetarget = [w, targname, index, &output, routetargetresolutionerror](error_collection &ec) {
					routingpoint *rp = FastRoutingpointCast(w->FindTrackByName(targname));
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