//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
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
#include "core/signal.h"
#include "core/serialisable_impl.h"
#include "core/error.h"
#include "core/routetypes_serialisation.h"
#include "core/deserialisation_scalarconv.h"
#include "core/trackcircuit.h"

void routingpoint::Deserialise(const deserialiser_input &di, error_collection &ec) {
	genericzlentrack::Deserialise(di, ec);
	CheckIterateJsonArrayOrType<json_object>(di, "routeendrestrictions", "routerestrictions", ec,
		[&](const deserialiser_input &subdi, error_collection &ec) {
			endrestrictions.DeserialiseRestriction(subdi, ec, true);
		});
}

class trackroutingpoint_deserialisation_extras : public trackroutingpoint_deserialisation_extras_base {
	public:
	flag_conflict_checker<route_class::set> conflictcheck_start;
	flag_conflict_checker<route_class::set> conflictcheck_start_rev;
	flag_conflict_checker<route_class::set> conflictcheck_end;
	flag_conflict_checker<route_class::set> conflictcheck_end_rev;
	flag_conflict_checker<route_class::set> conflictcheck_through;
	flag_conflict_checker<route_class::set> conflictcheck_through_rev;
};

void trackroutingpoint::Deserialise(const deserialiser_input &di, error_collection &ec) {
	routingpoint::Deserialise(di, ec);

	trp_de.reset(new trackroutingpoint_deserialisation_extras);
	trackroutingpoint_deserialisation_extras *de = static_cast<trackroutingpoint_deserialisation_extras*>(trp_de.get());
	route_class::DeserialiseGroupProp(availableroutetypes_forward.through, di, "through", ec, de->conflictcheck_through);
	route_class::DeserialiseGroupProp(availableroutetypes_reverse.through, di, "through_rev", ec, de->conflictcheck_through_rev);

	bool val;
	if(CheckTransJsonValue<bool>(val, di, "overlapend", ec)) {
		de->conflictcheck_end.RegisterAndProcessFlags(availableroutetypes_forward.end, val, route_class::Flag(route_class::ID::RTC_OVERLAP), di, "", ec);
	}
	if(CheckTransJsonValue<bool>(val, di, "overlapend_rev", ec)) {
		de->conflictcheck_end_rev.RegisterAndProcessFlags(availableroutetypes_reverse.end, val, route_class::Flag(route_class::ID::RTC_OVERLAP), di, "", ec);
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

	CheckTransJsonValue(max_aspect, di, "maxaspect", ec);
	CheckTransJsonValueFlag(sflags, GSF::APPROACHLOCKINGMODE, di, "approachlockingmode", ec);
	CheckTransJsonValueFlag(sflags, GSF::OVERLAPTIMEOUTSTARTED, di, "overlaptimeoutstarted", ec);
	if(sflags & GSF::OVERLAPTIMEOUTSTARTED) CheckTransJsonValue(overlap_timeout_start, di, "overlap_timeout_start", ec);
	CheckTransJsonValue(last_route_prove_time, di, "lastrouteprovetime", ec);
	CheckTransJsonValue(last_route_clear_time, di, "lastroutecleartime", ec);
	CheckTransJsonValue(last_route_set_time, di, "lastroutesettime", ec);
	CheckTransJsonValue(routeprove_default_delay, di, "routeprovedelay", ec);
	CheckTransJsonValue(routeclear_default_delay, di, "routecleardelay", ec);
	CheckTransJsonValue(routeset_default_delay, di, "routesetdelay", ec);
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
	if(last_route_prove_time) SerialiseValueJson(last_route_prove_time, so, "lastrouteprovetime");
	if(last_route_clear_time) SerialiseValueJson(last_route_clear_time, so, "lastroutecleartime");
	if(last_route_set_time) SerialiseValueJson(last_route_set_time, so, "lastroutesettime");
}

void stdsignal::Deserialise(const deserialiser_input &di, error_collection &ec) {
	genericsignal::Deserialise(di, ec);

	trackroutingpoint_deserialisation_extras *de = static_cast<trackroutingpoint_deserialisation_extras*>(trp_de.get());
	de->conflictcheck_start.Ban(route_class::AllOverlaps());
	de->conflictcheck_end_rev.Ban(route_class::AllNonOverlaps());
	route_class::DeserialiseGroupProp(availableroutetypes_forward.start, di, "start", ec, de->conflictcheck_start);
	route_class::DeserialiseGroupProp(availableroutetypes_forward.end, di, "end", ec, de->conflictcheck_end);
	route_class::DeserialiseGroupProp(availableroutetypes_reverse.end, di, "end_rev", ec, de->conflictcheck_end_rev);

	auto docompoundflag = [&](const char *prop, route_class::set start_flags, route_class::set end_flags) {
		bool val;
		if(CheckTransJsonValue<bool>(val, di, prop, ec)) {
			de->conflictcheck_start.RegisterAndProcessFlags(availableroutetypes_forward.start, val, start_flags, di, prop, ec);
			de->conflictcheck_end.RegisterAndProcessFlags(availableroutetypes_forward.end, val, end_flags, di, prop, ec);
		}
	};
	docompoundflag("shuntsignal", route_class::Flag(route_class::RTC_SHUNT), route_class::Flag(route_class::RTC_SHUNT));
	docompoundflag("routesignal", route_class::Flag(route_class::RTC_ROUTE), route_class::Flag(route_class::RTC_SHUNT) | route_class::Flag(route_class::RTC_ROUTE));
	docompoundflag("routeshuntsignal", route_class::Flag(route_class::RTC_SHUNT) | route_class::Flag(route_class::RTC_ROUTE), route_class::Flag(route_class::RTC_SHUNT) | route_class::Flag(route_class::RTC_ROUTE));

	CheckTransJsonValueFlag(sflags, GSF::OVERLAPSWINGABLE, di, "overlapswingable", ec);
	CheckTransJsonValue(overlapswingminaspectdistance, di, "overlapswingminaspectdistance", ec);
	CheckTransJsonValueFlag(sflags, GSF::NOOVERLAP, di, "nooverlap", ec);

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
							approachlocking_default_timeouts[type] = timeout;
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
					if(route_class::IsValidForApproachLocking(type)) approachlocking_default_timeouts[type] = value;
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

void stdsignal::Serialise(serialiser_output &so, error_collection &ec) const {
	genericsignal::Serialise(so, ec);
}

void autosignal::Deserialise(const deserialiser_input &di, error_collection &ec) {
	stdsignal::Deserialise(di, ec);
}

void autosignal::Serialise(serialiser_output &so, error_collection &ec) const {
	stdsignal::Serialise(so, ec);
}

void routesignal::Deserialise(const deserialiser_input &di, error_collection &ec) {
	stdsignal::Deserialise(di, ec);
	CheckIterateJsonArrayOrType<json_object>(di, "routerestrictions", "routerestrictions", ec,
		[&](const deserialiser_input &subdi, error_collection &ec) {
			restrictions.DeserialiseRestriction(subdi, ec, false);
		});
}

void routesignal::Serialise(serialiser_output &so, error_collection &ec) const {
	stdsignal::Serialise(so, ec);
}

void repeatersignal::Deserialise(const deserialiser_input &di, error_collection &ec) {
	genericsignal::Deserialise(di, ec);
	CheckTransJsonValueFlag(sflags, GSF::NONASPECTEDREPEATER, di, "nonaspected", ec);

	trackroutingpoint_deserialisation_extras *de = static_cast<trackroutingpoint_deserialisation_extras*>(trp_de.get());
	auto docompoundflag = [&](const char *prop, route_class::set through_flags) {
		bool val;
		if(CheckTransJsonValue<bool>(val, di, prop, ec)) {
			de->conflictcheck_through.RegisterAndProcessFlags(availableroutetypes_forward.through, val, through_flags, di, prop, ec);
		}
	};
	docompoundflag("routesignal", route_class::Flag(route_class::RTC_ROUTE));
}

void repeatersignal::Serialise(serialiser_output &so, error_collection &ec) const {
	genericsignal::Serialise(so, ec);
}

void route_restriction_set::DeserialiseRestriction(const deserialiser_input &subdi, error_collection &ec, bool isendtype) {
	restrictions.emplace_back();
	route_restriction &rr = restrictions.back();

	auto deserialise_ttcb = [&](std::string prop, std::function<void(route_restriction &, track_train_counter_block *)> f) {
		std::string approachcontroltrigger_ttcb;
		if(CheckTransJsonValue(approachcontroltrigger_ttcb, subdi, prop.c_str(), ec)) {
			world *w = subdi.w;
			if(w) {
				size_t rr_index = restrictions.size() - 1;
				auto func = [this, approachcontroltrigger_ttcb, w, rr_index, prop, f](error_collection &ec) {
					track_train_counter_block *ttcb = w->FindTrackTrainBlockOrTrackCircuitByName(approachcontroltrigger_ttcb);
					if(ttcb) {
						f(this->restrictions[rr_index], ttcb);
					}
					else {
						ec.RegisterNewError<generic_error_obj>(prop + ": No such track circuit or track trigger: " + approachcontroltrigger_ttcb);
					}
				};
				w->layout_init_final_fixups.AddFixup(func);
			}
			else {
				ec.RegisterNewError<error_deserialisation>(subdi, prop + " not valid in this context (no world)");
			}
		}
	};

	if(isendtype) CheckFillTypeVectorFromJsonArrayOrType<std::string>(subdi, "routestart", ec, rr.targets);
	else {
		CheckFillTypeVectorFromJsonArrayOrType<std::string>(subdi, "targets", ec, rr.targets) ||
			CheckFillTypeVectorFromJsonArrayOrType<std::string>(subdi, "routeend", ec, rr.targets);
	}
	CheckFillTypeVectorFromJsonArrayOrType<std::string>(subdi, "via", ec, rr.via);
	CheckFillTypeVectorFromJsonArrayOrType<std::string>(subdi, "notvia", ec, rr.notvia);
	if(CheckTransJsonValue(rr.priority, subdi, "priority", ec)) rr.routerestrictionflags |= route_restriction::RRF::PRIORITYSET;
	if(CheckTransJsonValueProc(rr.approachlocking_timeout, subdi, "approachlockingtimeout", ec, dsconv::Time)) rr.routerestrictionflags |= route_restriction::RRF::APLOCK_TIMEOUTSET;
	if(CheckTransJsonValueProc(rr.overlap_timeout, subdi, "overlaptimeout", ec, dsconv::Time)) rr.routerestrictionflags |= route_restriction::RRF::OVERLAPTIMEOUTSET;
	if(CheckTransJsonValueProc(rr.routeprove_delay, subdi, "routeprovedelay", ec, dsconv::Time)) rr.routerestrictionflags |= route_restriction::RRF::ROUTEPROVEDELAY_SET;
	if(CheckTransJsonValueProc(rr.routeclear_delay, subdi, "routecleardelay", ec, dsconv::Time)) rr.routerestrictionflags |= route_restriction::RRF::ROUTECLEARDELAY_SET;
	if(CheckTransJsonValueProc(rr.routeset_delay, subdi, "routesetdelay", ec, dsconv::Time)) rr.routerestrictionflags |= route_restriction::RRF::ROUTESETDELAY_SET;
	deserialise_ttcb("overlaptimeouttrigger", [&](route_restriction &rr, track_train_counter_block *ttcb) {
		rr.overlaptimeout_trigger = ttcb;
	});
	bool res = CheckTransJsonValueFlag(rr.routerestrictionflags, route_restriction::RRF::APCONTROL, subdi, "approachcontrol", ec);
	if(res) rr.routerestrictionflags |= route_restriction::RRF::APCONTROL_SET;
	if(!res || rr.routerestrictionflags & route_restriction::RRF::APCONTROL) {
		if(CheckTransJsonValueProc(rr.approachcontrol_triggerdelay, subdi, "approachcontroltriggerdelay", ec, dsconv::Time)) rr.routerestrictionflags |= route_restriction::RRF::APCONTROLTRIGGERDELAY_SET | route_restriction::RRF::APCONTROL_SET | route_restriction::RRF::APCONTROL;
		deserialise_ttcb("approachcontroltrigger", [&](route_restriction &rr, track_train_counter_block *ttcb) {
			rr.approachcontrol_trigger = ttcb;
			rr.routerestrictionflags |= route_restriction::RRF::APCONTROL_SET | route_restriction::RRF::APCONTROL;
		});
	}
	if(CheckTransJsonValueFlag(rr.routerestrictionflags, route_restriction::RRF::TORR, subdi, "torr", ec)) rr.routerestrictionflags |= route_restriction::RRF::TORR_SET;
	if(CheckTransJsonValueFlag(rr.routerestrictionflags, route_restriction::RRF::EXITSIGCONTROL, subdi, "exitsignalcontrol", ec)) rr.routerestrictionflags |= route_restriction::RRF::EXITSIGCONTROL_SET;

	route_class::DeserialiseGroup(rr.allowedtypes, subdi, ec);

	flag_conflict_checker<route_class::set> conflictnegcheck;
	route_class::set val = 0;
	if(route_class::DeserialiseProp("applyonly", val, subdi, ec)) {
		rr.applytotypes = val;
		conflictnegcheck.RegisterFlags(true, val, subdi, "applyonly", ec);
		conflictnegcheck.RegisterFlags(false, ~val, subdi, "applyonly", ec);
	}
	if(route_class::DeserialiseProp("applydeny", val, subdi, ec)) {
		rr.applytotypes &= ~val;
		conflictnegcheck.RegisterFlags(false, val, subdi, "applydeny", ec);
	}

	deserialiser_input odi(subdi.json["overlap"], "overlap", subdi);
	if(!odi.json.IsNull()) {
		subdi.RegisterProp("overlap");

		if(odi.json.IsBool()) rr.overlap_type = odi.json.GetBool() ? route_class::ID::RTC_OVERLAP : route_class::ID::RTC_NULL;
		else if(odi.json.IsString()) {
			auto res = route_class::DeserialiseName(odi.json.GetString(), ec);
			if(res.first) rr.overlap_type = res.second;
			else ec.RegisterNewError<error_deserialisation>(odi, "Invalid overlap type definition: " + std::string(odi.json.GetString()));
		}
		else ec.RegisterNewError<error_deserialisation>(odi, "Invalid overlap type definition: wrong type");

		rr.routerestrictionflags |= route_restriction::RRF::OVERLAPTYPE_SET;
	}

	subdi.PostDeserialisePropCheck(ec);
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

void routingmarker::Deserialise(const deserialiser_input &di, error_collection &ec) {
	trackroutingpoint::Deserialise(di, ec);

	flag_conflict_checker<route_class::set> conflictcheck_end;
	flag_conflict_checker<route_class::set> conflictcheck_end_rev;
	conflictcheck_end.Ban(route_class::AllNonOverlaps());
	conflictcheck_end_rev.Ban(route_class::AllNonOverlaps());
	if(di.json.HasMember("overlapend")) conflictcheck_end.RegisterFlagsMasked(availableroutetypes_forward.end, route_class::Flag(route_class::ID::RTC_OVERLAP), di, "", ec);
	if(di.json.HasMember("overlapend_rev")) conflictcheck_end.RegisterFlagsMasked(availableroutetypes_reverse.end, route_class::Flag(route_class::ID::RTC_OVERLAP), di, "", ec);
	route_class::DeserialiseGroupProp(availableroutetypes_forward.end, di, "end", ec, conflictcheck_end);
	route_class::DeserialiseGroupProp(availableroutetypes_reverse.end, di, "end_rev", ec, conflictcheck_end_rev);
}
