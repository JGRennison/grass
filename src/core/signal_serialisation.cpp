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
#include "signal.h"
#include "serialisable_impl.h"
#include "error.h"
#include "routetypes_serialisation.h"
#include "deserialisation_scalarconv.h"

void trackroutingpoint::Deserialise(const deserialiser_input &di, error_collection &ec) {
	routingpoint::Deserialise(di, ec);

	route_class::DeserialiseGroupProp(availableroutetypes_forward.through, di, "through", ec);
	route_class::DeserialiseGroupProp(availableroutetypes_reverse.through, di, "through_rev", ec);

	bool val;
	if(CheckTransJsonValue<bool>(val, di, "overlapend", ec)) {
		SetOrClearBitsRef(availableroutetypes_forward.end, route_class::AllOverlaps(), val);
	}
	if(CheckTransJsonValue<bool>(val, di, "overlapend_rev", ec)) {
		SetOrClearBitsRef(availableroutetypes_reverse.end, route_class::AllOverlaps(), val);
	}

	if(CheckTransJsonValue<bool>(val, di, "via", ec)) {
		SetOrClearBitsRef(availableroutetypes_forward.flags, RPRT_FLAGS::VIA, val);
		SetOrClearBitsRef(availableroutetypes_reverse.flags, RPRT_FLAGS::VIA, val);
	}
}

void trackroutingpoint::Serialise(serialiser_output &so, error_collection &ec) const {
	routingpoint::Serialise(so, ec);
}

void genericsignal::Deserialise(const deserialiser_input &di, error_collection &ec) {
	trackroutingpoint::Deserialise(di, ec);

	CheckTransJsonSubObj(start_trs, di, "start_trs", "trs", ec);
	CheckTransJsonSubObj(end_trs, di, "end_trs", "trs", ec);

	flag_conflict_checker<route_class::set> conflictcheck_start;
	flag_conflict_checker<route_class::set> conflictcheck_end;
	conflictcheck_start.Ban(route_class::AllOverlaps());
	if(di.json.HasMember("overlapend")) conflictcheck_end.RegisterFlagsMasked(availableroutetypes_forward.end, route_class::AllOverlaps(), di, "", ec);
	route_class::DeserialiseGroupProp(availableroutetypes_forward.start, di, "start", ec, conflictcheck_start);
	route_class::DeserialiseGroupProp(availableroutetypes_forward.end, di, "end", ec, conflictcheck_end);

	auto docompoundflag = [&](const char *prop, route_class::set start_flags, route_class::set end_flags) {
		bool val;
		if(CheckTransJsonValue<bool>(val, di, prop, ec)) {
			conflictcheck_start.RegisterAndProcessFlags(availableroutetypes_forward.start, val, start_flags, di, prop, ec);
			conflictcheck_end.RegisterAndProcessFlags(availableroutetypes_forward.end, val, end_flags, di, prop, ec);
		}
	};
	docompoundflag("shuntsignal", route_class::Flag(route_class::RTC_SHUNT), route_class::Flag(route_class::RTC_SHUNT));
	docompoundflag("routesignal", route_class::Flag(route_class::RTC_ROUTE), route_class::Flag(route_class::RTC_SHUNT) | route_class::Flag(route_class::RTC_ROUTE));
	docompoundflag("routeshuntsignal", route_class::Flag(route_class::RTC_SHUNT) | route_class::Flag(route_class::RTC_ROUTE), route_class::Flag(route_class::RTC_SHUNT) | route_class::Flag(route_class::RTC_ROUTE));

	CheckTransJsonValue(max_aspect, di, "maxaspect", ec);
	CheckTransJsonValueFlag(sflags, GSF::OVERLAPSWINGABLE, di, "overlapswingable", ec);
	CheckTransJsonValue(overlapswingminaspectdistance, di, "overlapswingminaspectdistance", ec);
	CheckTransJsonValueFlag(sflags, GSF::APPROACHLOCKINGMODE, di, "approachlockingmode", ec);
	CheckTransJsonValueFlag(sflags, GSF::OVERLAPTIMEOUTSTARTED, di, "overlaptimeoutstarted", ec);
	if(sflags & GSF::OVERLAPTIMEOUTSTARTED) CheckTransJsonValue(overlap_timeout_start, di, "overlap_timeout_start", ec);

	deserialiser_input sddi(di.json["approachlockingtimeout"], "approachlockingtimeout", "approachlockingtimeout", di);
	if(!sddi.json.IsNull()) {
		di.RegisterProp("approachlockingtimeout");

		auto acfunc = [&](const deserialiser_input &funcdi) {
			bool ok = false;

			if(funcdi.json.IsObject()) {
				std::string rc;
				unsigned int timeout;
				if(CheckTransJsonValue(rc, funcdi, "routeclass", ec) && CheckTransJsonValueProc(timeout, funcdi, "timeout", ec, dsconv::Time)) {
					for(int i = 0; i < route_class::LAST_RTC; i++) {
						route_class::ID type = static_cast<route_class::ID>(i);
						if(route_class::IsValidForApproachLocking(type) && route_class::GetRouteTypeName(type) == rc) {
							approachcontrol_default_timeouts[type] = timeout;
							ok = true;
							break;
						}
					}
				}
				funcdi.PostDeserialisePropCheck(ec);
			}

			if(!ok) {
				ec.RegisterNewError<error_deserialisation>(funcdi, "Invalid approach locking timeout definition");
			}
		};

		if(sddi.json.IsUint() || sddi.json.IsString()) {
			unsigned int value;
			if(TransJsonValueProc(sddi.json, value, di, "approachlockingtimeout", ec, dsconv::Time)) {
				for(int i = 0; i < route_class::LAST_RTC; i++) {
					route_class::ID type = static_cast<route_class::ID>(i);
					if(route_class::IsValidForApproachLocking(type)) approachcontrol_default_timeouts[type] = value;
				}
			}
		}
		else if(sddi.json.IsArray()) {
			for(rapidjson::SizeType i = 0; i < sddi.json.Size(); i++) {
				acfunc(deserialiser_input(sddi.json[i], MkArrayRefName(i), sddi));
			}
		}
		else acfunc(sddi);
	}
	CheckTransJsonValue(overlap_default_timeout, di, "overlaptimeout", ec);
}

void genericsignal::Serialise(serialiser_output &so, error_collection &ec) const {
	trackroutingpoint::Serialise(so, ec);

	SerialiseSubObjJson(start_trs, so, "start_trs", ec);
	SerialiseSubObjJson(end_trs, so, "end_trs", ec);
	if(sflags & GSF::APPROACHLOCKINGMODE) SerialiseValueJson<bool>(true, so, "approachlockingmode");
	if(sflags & GSF::OVERLAPTIMEOUTSTARTED) {
		SerialiseValueJson(overlap_timeout_start, so, "overlap_timeout_start");
		SerialiseValueJson<bool>(true, so, "approachlockingmode");
	}
}

void autosignal::Deserialise(const deserialiser_input &di, error_collection &ec) {
	genericsignal::Deserialise(di, ec);
}

void autosignal::Serialise(serialiser_output &so, error_collection &ec) const {
	genericsignal::Serialise(so, ec);
}

void routesignal::Deserialise(const deserialiser_input &di, error_collection &ec) {
	genericsignal::Deserialise(di, ec);
	CheckTransJsonSubArray(restrictions, di, "routerestrictions", "routerestrictions", ec);
}

void routesignal::Serialise(serialiser_output &so, error_collection &ec) const {
	genericsignal::Serialise(so, ec);
}

void route_restriction_set::Deserialise(const deserialiser_input &di, error_collection &ec) {
	restrictions.reserve(di.json.Size());
	for(rapidjson::SizeType i = 0; i < di.json.Size(); i++) {
		deserialiser_input subdi(di.json[i], "routerestriction", MkArrayRefName(i), di);
		if(subdi.json.IsObject()) {
			restrictions.emplace_back();
			route_restriction &rr = restrictions.back();
			CheckFillTypeVectorFromJsonArrayOrType<std::string>(subdi, "targets", ec, rr.targets);
			CheckFillTypeVectorFromJsonArrayOrType<std::string>(subdi, "via", ec, rr.via);
			CheckFillTypeVectorFromJsonArrayOrType<std::string>(subdi, "notvia", ec, rr.notvia);
			if(CheckTransJsonValue(rr.priority, subdi, "priority", ec)) rr.routerestrictionflags |= route_restriction::RRF::PRIORITYSET;
			if(CheckTransJsonValueProc(rr.approachlocking_timeout, subdi, "approachlockingtimeout", ec, dsconv::Time)) rr.routerestrictionflags |= route_restriction::RRF::APLOCK_TIMEOUTSET;
			if(CheckTransJsonValueProc(rr.overlap_timeout, subdi, "overlaptimeout", ec, dsconv::Time)) rr.routerestrictionflags |= route_restriction::RRF::OVERLAPTIMEOUTSET;
			bool res = CheckTransJsonValueFlag(rr.routerestrictionflags, route_restriction::RRF::APCONTROL_SET, subdi, "approachcontrol", ec);
			if(!res || rr.routerestrictionflags & route_restriction::RRF::APCONTROL_SET) {
				if(CheckTransJsonValueProc(rr.approachcontrol_triggerdelay, subdi, "approachcontroltriggerdelay", ec, dsconv::Time)) rr.routerestrictionflags |= route_restriction::RRF::APCONTROLTRIGGERDELAY_SET | route_restriction::RRF::APCONTROL_SET;
			}
			if(CheckTransJsonValueFlag(rr.routerestrictionflags, route_restriction::RRF::TORR, subdi, "torr", ec)) rr.routerestrictionflags |= route_restriction::RRF::TORR_SET;

			route_class::DeserialiseGroup(rr.allowedtypes, subdi, ec);
			subdi.PostDeserialisePropCheck(ec);
		}
		else {
			ec.RegisterNewError<error_deserialisation>(subdi, "Invalid route restriction definition");
		}
	}
}

void route_restriction_set::Serialise(serialiser_output &so, error_collection &ec) const {
}

void startofline::Deserialise(const deserialiser_input &di, error_collection &ec) {
	routingpoint::Deserialise(di, ec);

	CheckTransJsonSubObj(trs, di, "trs", "trs", ec);
	route_class::DeserialiseGroupProp(availableroutetypes.end, di, "end", ec);
}

void startofline::Serialise(serialiser_output &so, error_collection &ec) const {
	routingpoint::Serialise(so, ec);

	SerialiseSubObjJson(trs, so, "trs", ec);
}
