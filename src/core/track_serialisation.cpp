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
#include "core/track_piece.h"
#include "core/points.h"
#include "core/signal.h"
#include "core/serialisable_impl.h"
#include "core/world.h"
#include "core/deserialisation_scalarconv.h"
#include "core/track_circuit.h"

void generic_track::Deserialise(const deserialiser_input &di, error_collection &ec) {
	world_obj::Deserialise(di, ec);

	CheckIterateJsonArrayOrType<json_object>(di, "layout", "layout", ec, [&](const deserialiser_input &di, error_collection &ec) {
		if (di.ws) {
			di.ws->gui_layout_generic_track(this, di, ec);
		}
	});

	deserialiser_input sddi(di.json["sighting"], "sighting", "sighting", di);
	if (!sddi.json.IsNull()) {
		di.RegisterProp("sighting");

		auto sightingfunc = [&](const deserialiser_input &funcdi) {
			bool ok = false;

			if (funcdi.json.IsObject()) {
				EDGE dir {};
				unsigned int distance;
				if (CheckTransJsonValue(dir, funcdi, "direction", ec) && CheckTransJsonValueProc(distance, funcdi, "distance", ec, dsconv::Length)) {
					for (auto &it : sighting_distances) {
						if (it.first == dir) {
							it.second = distance;
							ok = true;
							break;
						}
					}
				}
			}

			if (!ok) {
				ec.RegisterNewError<error_deserialisation>(funcdi, "Invalid sighting distance definition");
			}
		};

		if (sddi.json.IsUint() || sddi.json.IsString()) {
			unsigned int distance;
			if (TransJsonValueProc(sddi.json, distance, di, "sighting", ec, dsconv::Length)) {
				for (auto &it : sighting_distances) {
					it.second = distance;
				}
			}
		} else if (sddi.json.IsArray()) {
			for (rapidjson::SizeType i = 0; i < sddi.json.Size(); i++) {
				sightingfunc(deserialiser_input(sddi.json[i], "", MkArrayRefName(i), sddi));
			}
		} else {
			sightingfunc(sddi);
		}
	}

	deserialiser_input subdi(di.json["connect"], "track_connection", "connect", di);
	if (!subdi.json.IsNull() && subdi.w) {
		di.RegisterProp("connect");
		auto connfunc = [&](const deserialiser_input &funcdi) {
			bool ok = true;
			if (funcdi.json.IsObject()) {
				EDGE this_entrance_direction = EDGE::INVALID;
				EDGE target_entrance_direction = EDGE::INVALID;
				std::string target_name;
				bool have_from_direction = CheckTransJsonValue(this_entrance_direction, funcdi, "from_direction", ec);
				bool have_to_direction = CheckTransJsonValue(target_entrance_direction, funcdi, "to_direction", ec);
				ok = CheckTransJsonValue(target_name, funcdi, "to", ec);

				if (ok) {
					if (!have_from_direction) {
						std::vector<edgelistitem> edges;
						GetListOfEdges(edges);
						if (edges.size() == 1) {
							this_entrance_direction = edges[0].edge;
							have_from_direction = true;
						}
					}
					if (have_from_direction && have_to_direction) {
						di.w->ConnectTrack(this, this_entrance_direction, target_name, target_entrance_direction, ec);
					} else {
						if (this_entrance_direction != EDGE::INVALID) {
							if (ConnectionIsEdgeReserved(this_entrance_direction)) {
								ec.RegisterNewError<error_deserialisation>(funcdi, string_format("Track connection cannot re-use previously used edge: %s", SerialiseDirectionName(this_entrance_direction)));
								return;
							}
							ConnectionReserveEdge(this_entrance_direction);
						}
						world *w = di.w;
						auto resolveconnection = [w, this, this_entrance_direction, target_entrance_direction, target_name](error_collection &ec) mutable {
							if (this_entrance_direction != EDGE::INVALID) {
								ConnectionUnreserveEdge(this_entrance_direction);
							}

							auto checkconnection = [&](generic_track *gt, EDGE &dir) {
								if (dir != EDGE::INVALID) {
									return;
								}
								if (!gt) {
									ec.RegisterNewError<generic_error_obj>(
										string_format("Partial track connection declaration: no such piece: %s", target_name.c_str()));
									return;
								}

								std::vector<edgelistitem> edges;
								gt->GetListOfEdges(edges);
								EDGE lastfound;
								unsigned int freeedgecount = 0;
								for (auto &it : edges) {
									if (! it.target->IsValid()) {
										lastfound = it.edge;
										freeedgecount++;
									}
								}
								if (freeedgecount == 1) {
									dir = lastfound;
								} else {
									ec.RegisterNewError<generic_error_obj>(
											string_format("Ambiguous partial track connection declaration: piece: %s, has: %d unconnected edges",
												gt->GetName().c_str(), freeedgecount));
								}
							};
							checkconnection(this, this_entrance_direction);
							checkconnection(w->FindTrackByName(target_name), target_entrance_direction);
							w->ConnectTrack(this, this_entrance_direction, target_name, target_entrance_direction, ec);
						};
						di.w->layout_init_final_fixups.AddFixup(resolveconnection);
					}
				}
			} else {
				ok = false;
			}

			if (!ok) {
				ec.RegisterNewError<error_deserialisation>(funcdi, "Invalid track connection definition");
			}
		};

		if (subdi.json.IsArray()) {
			for (rapidjson::SizeType i = 0; i < subdi.json.Size(); i++) {
				connfunc(deserialiser_input(subdi.json[i], "", MkArrayRefName(i), subdi));
			}
		} else {
			connfunc(subdi);
		}
	}

	CheckTransJsonValueFlag(gt_privflags, GTPRIVF::REVERSE_AUTO_CONN, di, "reverse_auto_connection", ec);

	if (CanHaveBerth()) {
		deserialiser_input bdi(di.json["berth"], "berth", "berth", di);
		if (!bdi.json.IsNull()) {
			di.RegisterProp("berth");

			bool berthval = false;
			EDGE berthedge = EDGE::INVALID;
			if (bdi.json.IsBool()) {
				berthval = bdi.json.GetBool();
			} else if (IsType<EDGE>(bdi.json)) {
				berthval = true;
				berthedge = GetType<EDGE>(bdi.json);
			} else {
				ec.RegisterNewError<error_deserialisation>(bdi, "Invalid track berth definition");
			}

			if (berthval) {
				if (!berth) {
					berth.reset(new track_berth);
				}
				berth->direction = berthedge;
			} else {
				berth.reset();
			}
		}
		if (berth) {
			CheckTransJsonValue(berth->contents, di, "berth_str", ec);
			CheckIterateJsonArrayOrType<json_object>(di, "berth_layout", "layout", ec, [&](const deserialiser_input &di, error_collection &ec) {
				if (di.ws) {
					di.ws->gui_layout_track_berth(berth.get(), this, di, ec);
				}
			});
		}
	}

	std::string track_circuit_name;
	if (CheckTransJsonValue(track_circuit_name, di, "track_circuit", ec)) {
		tc = GetWorld().track_circuits.FindOrMakeByName(track_circuit_name);
		tc->RegisterTrack(this);
	}
}

void generic_track::DeserialiseReservationState(track_reservation_state &trs, const deserialiser_input &di, const char *name, error_collection &ec) {
	if (CheckTransJsonSubObj(trs, di, name, "trs", ec)) {
		UpdateTrackCircuitReservationState();
	}
}

void generic_track::Serialise(serialiser_output &so, error_collection &ec) const {
	world_obj::Serialise(so, ec);
	if (berth) {
		SerialiseValueJson(berth->contents, so, "berth_str");
	}
}

void track_seg::Deserialise(const deserialiser_input &di, error_collection &ec) {
	generic_track::Deserialise(di, ec);

	CheckTransJsonValueProc(length, di, "length", ec, dsconv::Length);
	CheckTransJsonValueProc(elevation_delta, di, "elevation_delta", ec, dsconv::Length);
	CheckTransJsonValue(train_count, di, "train_count", ec);
	DeserialiseReservationState(trs, di, "trs", ec);
	CheckTransJsonSubArray(speed_limits, di, "speed_limits", "speed_limits", ec);
	CheckTransJsonSubArray(traction_types, di, "traction_types", "traction_types", ec);

	CheckIterateJsonArrayOrType<std::string>(di, "track_triggers", "tracktrigger", ec, [&](const deserialiser_input &sdi, error_collection &ec) {
		track_train_counter_block *ttcb = GetWorld().track_triggers.FindOrMakeByName(GetType<std::string>(sdi.json));
		ttcbs.push_back(ttcb);
		ttcb->RegisterTrack(this);
	});
}

void track_seg::Serialise(serialiser_output &so, error_collection &ec) const {
	generic_track::Serialise(so, ec);

	SerialiseSubObjJson(trs, so, "trs", ec);
	SerialiseValueJson(train_count, so, "train_count");
}

void DeserialisePointFlags(generic_points::PTF &pflags, const deserialiser_input &di, error_collection &ec) {
	CheckTransJsonValueFlag(pflags, generic_points::PTF::REV, di, "reverse", ec);
	CheckTransJsonValueFlag(pflags, generic_points::PTF::OOC, di, "ooc", ec);
	CheckTransJsonValueFlag(pflags, generic_points::PTF::LOCKED, di, "locked", ec);
	CheckTransJsonValueFlag(pflags, generic_points::PTF::REMINDER, di, "reminder", ec);
	CheckTransJsonValueFlag(pflags, generic_points::PTF::FAILED_NORM, di, "failed_norm", ec);
	CheckTransJsonValueFlag(pflags, generic_points::PTF::FAILED_REV, di, "failed_rev", ec);
	CheckTransJsonValueFlag(pflags, generic_points::PTF::AUTO_NORMALISE, di, "auto_normalise", ec);
}

void SerialisePointFlags(generic_points::PTF pflags, serialiser_output &so, error_collection &ec) {
	SerialiseFlagJson(pflags, generic_points::PTF::REV, so, "reverse");
	SerialiseFlagJson(pflags, generic_points::PTF::OOC, so, "ooc");
	SerialiseFlagJson(pflags, generic_points::PTF::LOCKED, so, "locked");
	SerialiseFlagJson(pflags, generic_points::PTF::REMINDER, so, "reminder");
	SerialiseFlagJson(pflags, generic_points::PTF::FAILED_NORM, so, "failed_norm");
	SerialiseFlagJson(pflags, generic_points::PTF::FAILED_REV, so, "failed_rev");
}

void points::Deserialise(const deserialiser_input &di, error_collection &ec) {
	generic_points::Deserialise(di, ec);

	DeserialiseReservationState(trs, di, "trs", ec);
	DeserialisePointFlags(pflags, di, ec);
}

void points::Serialise(serialiser_output &so, error_collection &ec) const {
	generic_points::Serialise(so, ec);

	SerialiseSubObjJson(trs, so, "trs", ec);
	SerialisePointFlags(pflags, so, ec);
}

void catchpoints::Deserialise(const deserialiser_input &di, error_collection &ec) {
	generic_points::Deserialise(di, ec);

	DeserialiseReservationState(trs, di, "trs", ec);
	DeserialisePointFlags(pflags, di, ec);
}

void catchpoints::Serialise(serialiser_output &so, error_collection &ec) const {
	generic_points::Serialise(so, ec);

	SerialiseSubObjJson(trs, so, "trs", ec);
	SerialisePointFlags(pflags, so, ec);
}

void spring_points::Deserialise(const deserialiser_input &di, error_collection &ec) {
	generic_zlen_track::Deserialise(di, ec);

	DeserialiseReservationState(trs, di, "trs", ec);
	CheckTransJsonValue(sendreverse, di, "send_reverse", ec);
}

void spring_points::Serialise(serialiser_output &so, error_collection &ec) const {
	generic_zlen_track::Serialise(so, ec);

	SerialiseSubObjJson(trs, so, "trs", ec);
}

void crossover::Deserialise(const deserialiser_input &di, error_collection &ec) {
	generic_zlen_track::Deserialise(di, ec);

	DeserialiseReservationState(trs, di, "trs", ec);
}

void crossover::Serialise(serialiser_output &so, error_collection &ec) const {
	generic_zlen_track::Serialise(so, ec);

	SerialiseSubObjJson(trs, so, "trs", ec);
}

class pointsflagssubobj : public serialisable_obj {
	generic_points::PTF *pflags;
	generic_points::PTF inpflags;

	public:
	pointsflagssubobj(generic_points::PTF *pf) : pflags(pf), inpflags(*pf) { }
	pointsflagssubobj(generic_points::PTF pf) : pflags(0), inpflags(pf) { }

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) {
		if (pflags) {
			DeserialisePointFlags(*pflags, di, ec);
		}
	}

	virtual void Serialise(serialiser_output &so, error_collection &ec) const {
		SerialisePointFlags(inpflags, so, ec);
	}
};

void double_slip::Deserialise(const deserialiser_input &di, error_collection &ec) {
	generic_points::Deserialise(di, ec);

	DeserialiseReservationState(trs, di, "trs", ec);
	if (CheckTransJsonValue(dof, di, "degrees_of_freedom", ec)) {
		if (dof != 1 && dof != 2 && dof != 4) {
			ec.RegisterNewError<error_deserialisation>(di, "Invalid double slip degrees of freedom: " + std::to_string(dof));
		}
	}

	CheckTransJsonValueFlag(dsflags, DSF::NO_FL_BL, di, "no_track_fl_bl", ec);
	CheckTransJsonValueFlag(dsflags, DSF::NO_FR_BL, di, "no_track_fr_bl", ec);
	CheckTransJsonValueFlag(dsflags, DSF::NO_FL_BR, di, "no_track_fl_br", ec);
	CheckTransJsonValueFlag(dsflags, DSF::NO_FR_BR, di, "no_track_fr_br", ec);

	if (__builtin_popcount(flag_unwrap<DSF>(dsflags&DSF::NO_TRACK_MASK)) >= 2) {
		ec.RegisterNewError<error_deserialisation>(di, "Cannot remove more than one track edge from a double-slip, use points or a crossover instead");
		return;
	}

	UpdatePointsFixedStatesFromMissingTrackEdges();
	UpdateInternalCoupling();

	auto deserialisepointsflags = [&](EDGE direction, const char *prop) {
		generic_points::PTF pf = GetCurrentPointFlags(direction);
		pointsflagssubobj ps(&pf);
		CheckTransJsonSubObj(ps, di, prop, "", ec);
		if (dof != 4 && pf & PTF::AUTO_NORMALISE) {
			ec.RegisterNewError<error_deserialisation>(di, "Self-coupled double-slips cannot have auto-normalise set");
		}
		SetPointsFlagsMasked(GetPointsIndexByEdge(direction), pf, generic_points::PTF::ALL);
	};
	deserialisepointsflags(EDGE::DS_FL, "left_front_points");
	deserialisepointsflags(EDGE::DS_FR, "right_front_points");
	deserialisepointsflags(EDGE::DS_BR, "right_back_points");
	deserialisepointsflags(EDGE::DS_BL, "left_back_points");
}

void double_slip::Serialise(serialiser_output &so, error_collection &ec) const {
	generic_points::Serialise(so, ec);

	SerialiseSubObjJson(trs, so, "trs", ec);
	SerialiseSubObjJson(pointsflagssubobj(GetCurrentPointFlags(EDGE::DS_FL)), so, "left_front_points", ec);
	if (dof >= 2) {
		SerialiseSubObjJson(pointsflagssubobj(GetCurrentPointFlags(EDGE::DS_BR)), so, "right_back_points", ec);
	}
	if (dof >= 4) {
		SerialiseSubObjJson(pointsflagssubobj(GetCurrentPointFlags(EDGE::DS_FR)), so, "right_front_points", ec);
		SerialiseSubObjJson(pointsflagssubobj(GetCurrentPointFlags(EDGE::DS_BL)), so, "left_back_points", ec);
	}
}

void SerialiseRouteTargetByParentAndIndex(const route *rt, serialiser_output &so, error_collection &ec) {
	if (rt) {
		if (rt->parent) {
			SerialiseValueJson(rt->parent->GetName(), so, "route_parent");
			SerialiseValueJson(rt->index, so, "route_index");
		}
	}
}

//if after_layout_init_resolve is true, and the target piece cannot be found (does not exist yet),
//then a fixup is added to after_layout_init in world to resolve it and set output,
//such that output *must* remain in scope throughout the layout initialisation phase
void DeserialiseRouteTargetByParentAndIndex(const route *& output, const deserialiser_input &di, error_collection &ec, bool after_layout_init_resolve) {
	std::string targname;
	if (CheckTransJsonValue(targname, di, "route_parent", ec)) {
		unsigned int index;
		CheckTransJsonValueDef(index, di, "route_index", 0, ec);

		auto routetargetresolutionerror = [](error_collection &ec, const std::string &targname) {
			ec.RegisterNewError<generic_error_obj>("Cannot resolve route target: " + targname);
		};
		auto routeindexresolutionerror = [](error_collection &ec, const std::string &targname, unsigned int index) {
			ec.RegisterNewError<generic_error_obj>("Cannot resolve route target: " + targname + ", index: " + std::to_string(index));
		};

		if (di.w) {
			routing_point *rp = FastRoutingpointCast(di.w->FindTrackByName(targname));
			if (rp) {
				output = rp->GetRouteByIndex(index);
				if (!output) {
					routeindexresolutionerror(ec, targname, index);
				}
				return;
			} else if (after_layout_init_resolve) {
				world *w = di.w;
				auto resolveroutetarget = [w, targname, index, &output, routetargetresolutionerror, routeindexresolutionerror](error_collection &ec) {
					routing_point *rp = FastRoutingpointCast(w->FindTrackByName(targname));
					if (rp) {
						output = rp->GetRouteByIndex(index);
						if (!output) {
							routeindexresolutionerror(ec, targname, index);
						}
					} else {
						routetargetresolutionerror(ec, targname);
					}
				};
				di.w->layout_init_final_fixups.AddFixup(resolveroutetarget);
			} else {
				routetargetresolutionerror(ec, targname);
			}
		}
	}
	output = nullptr;
	return;
}

void speed_restriction_set::Deserialise(const deserialiser_input &di, error_collection &ec) {
	for (rapidjson::SizeType i = 0; i < di.json.Size(); i++) {
		deserialiser_input subdi(di.json[i], "speed_restriction", MkArrayRefName(i), di);
		speed_restriction sr;
		if (subdi.json.IsObject() && CheckTransJsonValueDef(sr.speed_class, subdi, "speed_class", "", ec)
				&& CheckTransJsonValueDefProc(sr.speed, subdi, "speed", 0, ec, dsconv::Speed)) {
			AddSpeedRestriction(sr);
			subdi.PostDeserialisePropCheck(ec);
		} else {
			ec.RegisterNewError<error_deserialisation>(subdi, "Invalid speed restriction definition");
		}
	}
}

void speed_restriction_set::Serialise(serialiser_output &so, error_collection &ec) const {
	return;
}

void DeserialisePointsCoupling(const deserialiser_input &di, error_collection &ec) {
	bool ok = true;
	deserialiser_input subdi(di.json["points"], "points_coupling_array", "points", di);
	if (subdi.json.IsArray() && subdi.json.Size() >= 2) {
		di.RegisterProp("points");
		auto params = std::make_shared<std::vector<std::pair<std::string, EDGE> > >();
		for (rapidjson::SizeType i = 0; i < subdi.json.Size(); i++) {
			deserialiser_input itemdi(subdi.json[i], "points_coupling", MkArrayRefName(i), subdi);
			std::string name;
			EDGE direction;
			if (itemdi.json.IsObject() && CheckTransJsonValueDef(name, itemdi, "name", "", ec)
					&& CheckTransJsonValueDef(direction, itemdi, "edge", EDGE::INVALID, ec)) {
				params->emplace_back(name, direction);
				itemdi.PostDeserialisePropCheck(ec);
			} else {
				ok = false;
			}
		}
		if (ok) {
			world *w = di.w;
			w->layout_init_final_fixups.AddFixup([params, w](error_collection &ec) {
				std::vector<vartrack_target_ptr<generic_points> > points_list;
				points_list.reserve(params->size());
				for (auto it : *params) {
					generic_points* p = dynamic_cast<generic_points*>(w->FindTrackByName(it.first));
					if (!p) {
						ec.RegisterNewError<generic_error_obj>("Points coupling: no such points: " + it.first);
						return;
					}
					if (!p->IsCoupleable(it.second)) {
						ec.RegisterNewError<generic_error_obj>("Points coupling: points cannot be coupled: " + it.first);
						return;
					}
					points_list.emplace_back(p, it.second);
				}
				for (auto it = points_list.begin(); it != points_list.end(); ++it) {
					for (auto jt = it + 1; jt != points_list.end(); ++jt) {
						std::vector<generic_points::points_coupling> pci;
						std::vector<generic_points::points_coupling> pcj;
						it->track->GetCouplingPointsFlagsByEdge(it->direction, pci);
						jt->track->GetCouplingPointsFlagsByEdge(jt->direction, pcj);
						for (auto kt : pci) {
							for (auto lt : pcj) {
								generic_points::PTF xormask = kt.xormask ^ lt.xormask;
								kt.targ->CouplePointsFlagsAtIndexTo(kt.index, generic_points::points_coupling(lt.pflags, xormask, lt.targ, lt.index));
								lt.targ->CouplePointsFlagsAtIndexTo(lt.index, generic_points::points_coupling(kt.pflags, xormask, kt.targ, kt.index));
							}
						}
					}
					for (unsigned int i = 0; i < it->track->GetPointsCount(); i++) {
						it->track->SetPointsFlagsMasked(i, it->track->GetPointsFlags(i)&generic_points::PTF::SERIALISABLE, generic_points::PTF::SERIALISABLE);
					}
				}
			});
		}
	} else {
		ok = false;
	}


	if (!ok) {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid points coupling definition");
	}
}

template<> void track_target_ptr::Deserialise(const std::string &name, const deserialiser_input &di, error_collection &ec) {
	auto parse = [&](const deserialiser_input &edi, error_collection &ec) {
		std::string piece;
		if (!edi.w || !CheckTransJsonValue(piece, edi, "piece", ec)) {
			Reset();
			return;
		}
		track = edi.w->FindTrackByName(piece);
		if (!track) {
			ec.RegisterNewError<error_deserialisation>(edi, "Invalid track target definition: no such piece");
			return;
		}
		CheckTransJsonValueDef(direction, edi, "dir", EDGE::INVALID, ec);
	};

	if (name.empty()) {
		parse(di, ec);
	} else {
		CheckTransJsonTypeFunc<json_object>(di, name.c_str(), "track_target_ptr", ec, parse);
	}
}

template<> void track_target_ptr::Serialise(const std::string &name, serialiser_output &so, error_collection &ec) const {
	if (!name.empty()) {
		so.json_out.String(name);
		so.json_out.StartObject();
	}
	if (IsValid()) {
		SerialiseValueJson(track->GetName(), so, "piece");
		SerialiseValueJson(direction, so, "dir");
	}
	if (!name.empty()) {
		so.json_out.EndObject();
	}
}

template<> void track_location::Deserialise(const std::string &name, const deserialiser_input &di, error_collection &ec) {
	auto parse = [&](const deserialiser_input &edi, error_collection &ec) {
		track_piece.Deserialise("", edi, ec);
		CheckTransJsonValueDef(offset, edi, "offset", 0, ec);
	};

	if (name.empty()) {
		parse(di, ec);
	} else {
		CheckTransJsonTypeFunc<json_object>(di, name.c_str(), "track_location", ec, parse);
	}
}

template<> void track_location::Serialise(const std::string &name, serialiser_output &so, error_collection &ec) const {
	if (!name.empty()) {
		so.json_out.String(name);
		so.json_out.StartObject();
	}
	if (IsValid()) {
		track_piece.Serialise("", so, ec);
		SerialiseValueJson(offset, so, "offset");
	}
	if (!name.empty()) {
		so.json_out.EndObject();
	}
}
