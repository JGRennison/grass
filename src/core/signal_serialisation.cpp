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
#include "core/signal.h"
#include "core/serialisable_impl.h"
#include "core/routetypes_serialisation.h"
#include "core/deserialisation_scalarconv.h"
#include "core/trackcircuit.h"

void routing_point::Deserialise(const deserialiser_input &di, error_collection &ec) {
	generic_zlen_track::Deserialise(di, ec);
	CheckIterateJsonArrayOrType<json_object>(di, "routeendrestrictions", "routerestrictions", ec,
		[&](const deserialiser_input &subdi, error_collection &ec) {
			endrestrictions.DeserialiseRestriction(subdi, ec, true);
		});

	CheckTransJsonValue(aspect, di, "aspect", ec);
	CheckTransJsonValue(reserved_aspect, di, "reserved_aspect", ec);
	CheckTransJsonValue(aspect_type, di, "aspect_type", ec);

	std::string targetname;
	if (CheckTransJsonValue(targetname, di, "aspect_target", ec)) {
		aspect_target = FastRoutingpointCast(GetWorld().FindTrackByName(targetname));
	}
	if (CheckTransJsonValue(targetname, di, "aspect_route_target", ec)) {
		aspect_route_target = FastRoutingpointCast(GetWorld().FindTrackByName(targetname));
	}
	if (CheckTransJsonValue(targetname, di, "aspect_backwards_dependency", ec)) {
		aspect_backwards_dependency = FastRoutingpointCast(GetWorld().FindTrackByName(targetname));
	}
}

void routing_point::Serialise(serialiser_output &so, error_collection &ec) const {
	generic_zlen_track::Serialise(so, ec);
	if (aspect) SerialiseValueJson(aspect, so, "aspect");
	if (reserved_aspect) SerialiseValueJson(reserved_aspect, so, "reserved_aspect");
	if (aspect_type != route_class::RTC_NULL) SerialiseValueJson(aspect_type, so, "aspect_type");

	if (aspect_target) SerialiseValueJson(aspect_target->GetName(), so, "aspect_target");
	if (aspect_route_target) SerialiseValueJson(aspect_route_target->GetName(), so, "aspect_route_target");
	if (aspect_backwards_dependency) SerialiseValueJson(aspect_backwards_dependency->GetName(), so, "aspect_backwards_dependency");
}

class track_routing_point_deserialisation_extras : public track_routing_point_deserialisation_extras_base {
	public:
	flag_conflict_checker<route_class::set> conflictcheck_start;
	flag_conflict_checker<route_class::set> conflictcheck_start_rev;
	flag_conflict_checker<route_class::set> conflictcheck_end;
	flag_conflict_checker<route_class::set> conflictcheck_end_rev;
	flag_conflict_checker<route_class::set> conflictcheck_through;
	flag_conflict_checker<route_class::set> conflictcheck_through_rev;
};

void track_routing_point::Deserialise(const deserialiser_input &di, error_collection &ec) {
	routing_point::Deserialise(di, ec);

	trp_de.reset(new track_routing_point_deserialisation_extras);
	track_routing_point_deserialisation_extras *de = static_cast<track_routing_point_deserialisation_extras*>(trp_de.get());
	route_class::DeserialiseGroupProp(available_route_types_forward.through, di, "through", ec, de->conflictcheck_through);
	route_class::DeserialiseGroupProp(available_route_types_reverse.through, di, "through_rev", ec, de->conflictcheck_through_rev);

	bool val;
	if (CheckTransJsonValue<bool>(val, di, "overlapend", ec)) {
		de->conflictcheck_end.RegisterAndProcessFlags(available_route_types_forward.end, val, route_class::Flag(route_class::ID::RTC_OVERLAP), di, "", ec);
	}
	if (CheckTransJsonValue<bool>(val, di, "overlapend_rev", ec)) {
		de->conflictcheck_end_rev.RegisterAndProcessFlags(available_route_types_reverse.end, val, route_class::Flag(route_class::ID::RTC_OVERLAP), di, "", ec);
	}

	if (CheckTransJsonValue<bool>(val, di, "via", ec)) {
		SetOrClearBitsRef(available_route_types_forward.flags, RPRT_FLAGS::VIA, val);
		SetOrClearBitsRef(available_route_types_reverse.flags, RPRT_FLAGS::VIA, val);
	}
}

void track_routing_point::Serialise(serialiser_output &so, error_collection &ec) const {
	routing_point::Serialise(so, ec);
}

void generic_signal::Deserialise(const deserialiser_input &di, error_collection &ec) {
	track_routing_point::Deserialise(di, ec);

	DeserialiseReservationState(start_trs, di, "start_trs", ec);
	DeserialiseReservationState(end_trs, di, "end_trs", ec);

	CheckTransJsonValueFlag(sflags, GSF::APPROACH_LOCKING_MODE, di, "approachlockingmode", ec);
	CheckTransJsonValueFlag(sflags, GSF::OVERLAP_TIMEOUT_STARTED, di, "overlaptimeoutstarted", ec);
	if (sflags & GSF::OVERLAP_TIMEOUT_STARTED) {
		CheckTransJsonValue(overlap_timeout_start, di, "overlap_timeout_start", ec);
	}
	CheckTransJsonValue(last_route_prove_time, di, "lastrouteprovetime", ec);
	CheckTransJsonValue(last_route_clear_time, di, "lastroutecleartime", ec);
	CheckTransJsonValue(last_route_set_time, di, "lastroute_settime", ec);
}

void generic_signal::Serialise(serialiser_output &so, error_collection &ec) const {
	track_routing_point::Serialise(so, ec);

	SerialiseSubObjJson(start_trs, so, "start_trs", ec);
	SerialiseSubObjJson(end_trs, so, "end_trs", ec);
	if (sflags & GSF::APPROACH_LOCKING_MODE) {
		SerialiseValueJson<bool>(true, so, "approachlockingmode");
	}
	if (sflags & GSF::OVERLAP_TIMEOUT_STARTED) {
		SerialiseValueJson(overlap_timeout_start, so, "overlap_timeout_start");
		SerialiseValueJson<bool>(true, so, "approachlockingmode");
	}
	if (last_route_prove_time) {
		SerialiseValueJson(last_route_prove_time, so, "lastrouteprovetime");
	}
	if (last_route_clear_time) {
		SerialiseValueJson(last_route_clear_time, so, "lastroutecleartime");
	}
	if (last_route_set_time) {
		SerialiseValueJson(last_route_set_time, so, "lastroute_settime");
	}
}

void std_signal::Deserialise(const deserialiser_input &di, error_collection &ec) {
	generic_signal::Deserialise(di, ec);

	track_routing_point_deserialisation_extras *de = static_cast<track_routing_point_deserialisation_extras*>(trp_de.get());
	de->conflictcheck_start.Ban(route_class::AllOverlaps());
	de->conflictcheck_end_rev.Ban(route_class::AllNonOverlaps());
	route_class::DeserialiseGroupProp(available_route_types_forward.start, di, "start", ec, de->conflictcheck_start);
	route_class::DeserialiseGroupProp(available_route_types_forward.end, di, "end", ec, de->conflictcheck_end);
	route_class::DeserialiseGroupProp(available_route_types_reverse.end, di, "end_rev", ec, de->conflictcheck_end_rev);

	auto docompoundflag = [&](const char *prop, route_class::set start_flags, route_class::set end_flags) {
		bool val;
		if (CheckTransJsonValue<bool>(val, di, prop, ec)) {
			de->conflictcheck_start.RegisterAndProcessFlags(available_route_types_forward.start, val, start_flags, di, prop, ec);
			de->conflictcheck_end.RegisterAndProcessFlags(available_route_types_forward.end, val, end_flags, di, prop, ec);
		}
	};
	docompoundflag("shuntsignal", route_class::Flag(route_class::RTC_SHUNT), route_class::Flag(route_class::RTC_SHUNT));
	docompoundflag("route_signal", route_class::Flag(route_class::RTC_ROUTE), route_class::Flag(route_class::RTC_SHUNT) |
			route_class::Flag(route_class::RTC_ROUTE));
	docompoundflag("routeshuntsignal", route_class::Flag(route_class::RTC_SHUNT) | route_class::Flag(route_class::RTC_ROUTE),
			route_class::Flag(route_class::RTC_SHUNT) | route_class::Flag(route_class::RTC_ROUTE));

	CheckTransJsonValueFlag(sflags, GSF::OVERLAP_SWINGABLE, di, "overlapswingable", ec);
	CheckTransJsonValue(overlap_swing_min_aspect_distance, di, "overlap_swing_min_aspect_distance", ec);
	CheckTransJsonValueFlag(sflags, GSF::NO_OVERLAP, di, "nooverlap", ec);

	deserialiser_input sddi(di.json["approachlockingtimeout"], "approachlockingtimeout", "approachlockingtimeout", di);
	if (!sddi.json.IsNull()) {
		di.RegisterProp("approachlockingtimeout");

		auto acfunc = [&](const deserialiser_input &funcdi) {
			bool ok = false;

			if (funcdi.json.IsObject()) {
				std::string rc;
				unsigned int timeout;
				if (CheckTransJsonValue(rc, funcdi, "routeclass", ec) && CheckTransJsonValueProc(timeout, funcdi, "timeout", ec, dsconv::Time)) {
					for (int i = 0; i < route_class::LAST_RTC; i++) {
						route_class::ID type = static_cast<route_class::ID>(i);
						if (route_class::IsValidForApproachLocking(type) && route_class::GetRouteTypeName(type) == rc) {
							approach_locking_default_timeouts[type] = timeout;
							ok = true;
							break;
						}
					}
				}
				funcdi.PostDeserialisePropCheck(ec);
			}

			if (!ok) {
				ec.RegisterNewError<error_deserialisation>(funcdi, "Invalid approach locking timeout definition");
			}
		};

		if (sddi.json.IsUint() || sddi.json.IsString()) {
			unsigned int value;
			if (TransJsonValueProc(sddi.json, value, di, "approachlockingtimeout", ec, dsconv::Time)) {
				for (int i = 0; i < route_class::LAST_RTC; i++) {
					route_class::ID type = static_cast<route_class::ID>(i);
					if (route_class::IsValidForApproachLocking(type)) {
						approach_locking_default_timeouts[type] = value;
					}
				}
			}
		} else if (sddi.json.IsArray()) {
			for (rapidjson::SizeType i = 0; i < sddi.json.Size(); i++) {
				acfunc(deserialiser_input(sddi.json[i], "", MkArrayRefName(i), sddi));
			}
		} else {
			acfunc(sddi);
		}
	}
	route_defaults.DeserialiseRouteCommon(di, ec, route_common::DeserialisationFlags::NO_APLOCK_TIMEOUT);
}

void std_signal::Serialise(serialiser_output &so, error_collection &ec) const {
	generic_signal::Serialise(so, ec);
}

void auto_signal::Deserialise(const deserialiser_input &di, error_collection &ec) {
	std_signal::Deserialise(di, ec);
}

void auto_signal::Serialise(serialiser_output &so, error_collection &ec) const {
	std_signal::Serialise(so, ec);
}

void route_signal::Deserialise(const deserialiser_input &di, error_collection &ec) {
	std_signal::Deserialise(di, ec);
	CheckIterateJsonArrayOrType<json_object>(di, "routerestrictions", "routerestrictions", ec,
		[&](const deserialiser_input &subdi, error_collection &ec) {
			restrictions.DeserialiseRestriction(subdi, ec, false);
		});
}

void route_signal::Serialise(serialiser_output &so, error_collection &ec) const {
	std_signal::Serialise(so, ec);
}

void repeater_signal::Deserialise(const deserialiser_input &di, error_collection &ec) {
	generic_signal::Deserialise(di, ec);
	CheckTransJsonValueFlag(sflags, GSF::NON_ASPECTED_REPEATER, di, "nonaspected", ec);

	track_routing_point_deserialisation_extras *de = static_cast<track_routing_point_deserialisation_extras*>(trp_de.get());
	auto docompoundflag = [&](const char *prop, route_class::set through_flags) {
		bool val;
		if (CheckTransJsonValue<bool>(val, di, prop, ec)) {
			de->conflictcheck_through.RegisterAndProcessFlags(available_route_types_forward.through, val, through_flags, di, prop, ec);
		}
	};
	docompoundflag("route_signal", route_class::Flag(route_class::RTC_ROUTE));
	route_defaults.DeserialiseRouteCommon(di, ec, route_common::DeserialisationFlags::ASPECT_MASK_ONLY);
}

void repeater_signal::Serialise(serialiser_output &so, error_collection &ec) const {
	generic_signal::Serialise(so, ec);
}

void start_of_line::Deserialise(const deserialiser_input &di, error_collection &ec) {
	routing_point::Deserialise(di, ec);

	DeserialiseReservationState(trs, di, "trs", ec);
	route_class::DeserialiseGroupProp(available_route_types.end, di, "end", ec);
}

void start_of_line::Serialise(serialiser_output &so, error_collection &ec) const {
	routing_point::Serialise(so, ec);

	SerialiseSubObjJson(trs, so, "trs", ec);
}

void routing_marker::Deserialise(const deserialiser_input &di, error_collection &ec) {
	track_routing_point::Deserialise(di, ec);

	flag_conflict_checker<route_class::set> conflictcheck_end;
	flag_conflict_checker<route_class::set> conflictcheck_end_rev;
	conflictcheck_end.Ban(route_class::AllNonOverlaps());
	conflictcheck_end_rev.Ban(route_class::AllNonOverlaps());
	if (di.json.HasMember("overlapend")) {
		conflictcheck_end.RegisterFlagsMasked(available_route_types_forward.end, route_class::Flag(route_class::ID::RTC_OVERLAP), di, "", ec);
	}
	if (di.json.HasMember("overlapend_rev")) {
		conflictcheck_end.RegisterFlagsMasked(available_route_types_reverse.end, route_class::Flag(route_class::ID::RTC_OVERLAP), di, "", ec);
	}
	route_class::DeserialiseGroupProp(available_route_types_forward.end, di, "end", ec, conflictcheck_end);
	route_class::DeserialiseGroupProp(available_route_types_reverse.end, di, "end_rev", ec, conflictcheck_end_rev);
	DeserialiseReservationState(trs, di, "trs", ec);
}

void routing_marker::Serialise(serialiser_output &so, error_collection &ec) const {
	track_routing_point::Serialise(so, ec);

	SerialiseSubObjJson(trs, so, "trs", ec);
}
