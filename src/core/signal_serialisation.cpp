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

void trackroutingpoint::Deserialise(const deserialiser_input &di, error_collection &ec) {
	routingpoint::Deserialise(di, ec);

	flag_conflict_checker<RPRT> conflictcheck_forward;
	flag_conflict_checker<RPRT> conflictcheck_reverse;
	CheckTransJsonValueFlag(availableroutetypes_forward, RPRT::MASK_TRANS, di, "through", ec, &conflictcheck_forward);
	CheckTransJsonValueFlag(availableroutetypes_reverse, RPRT::MASK_TRANS, di, "through_rev", ec, &conflictcheck_reverse);
	CheckTransJsonValueFlag(availableroutetypes_forward, RPRT::ROUTETRANS, di, "routethrough", ec, &conflictcheck_forward);
	CheckTransJsonValueFlag(availableroutetypes_forward, RPRT::SHUNTTRANS, di, "shuntthrough", ec, &conflictcheck_forward);
	CheckTransJsonValueFlag(availableroutetypes_forward, RPRT::OVERLAPTRANS, di, "overlapthrough", ec, &conflictcheck_forward);
	CheckTransJsonValueFlag(availableroutetypes_reverse, RPRT::ROUTETRANS, di, "routethrough_rev", ec, &conflictcheck_reverse);
	CheckTransJsonValueFlag(availableroutetypes_reverse, RPRT::SHUNTTRANS, di, "shuntthrough_rev", ec, &conflictcheck_reverse);
	CheckTransJsonValueFlag(availableroutetypes_reverse, RPRT::OVERLAPTRANS, di, "overlapthrough_rev", ec, &conflictcheck_reverse);
	CheckTransJsonValueFlag(availableroutetypes_forward, RPRT::OVERLAPEND, di, "overlapend", ec, &conflictcheck_forward);
	CheckTransJsonValueFlag(availableroutetypes_reverse, RPRT::OVERLAPEND, di, "overlapend_rev", ec, &conflictcheck_reverse);
}

void trackroutingpoint::Serialise(serialiser_output &so, error_collection &ec) const {
	routingpoint::Serialise(so, ec);
}

void genericsignal::Deserialise(const deserialiser_input &di, error_collection &ec) {
	trackroutingpoint::Deserialise(di, ec);

	CheckTransJsonSubObj(start_trs, di, "start_trs", "trs", ec);
	CheckTransJsonSubObj(end_trs, di, "end_trs", "trs", ec);
	flag_conflict_checker<RPRT> conflictcheck;
	CheckTransJsonValueFlag(availableroutetypes_forward, RPRT::SHUNTSTART | RPRT::SHUNTEND, di, "shuntsignal", ec, &conflictcheck);
	CheckTransJsonValueFlag(availableroutetypes_forward, RPRT::ROUTESTART | RPRT::ROUTEEND | RPRT::SHUNTEND, di, "routesignal", ec, &conflictcheck);
	CheckTransJsonValueFlag(availableroutetypes_forward, RPRT::ROUTESTART | RPRT::SHUNTSTART | RPRT::ROUTEEND | RPRT::SHUNTEND, di, "routeshuntsignal", ec, &conflictcheck);
	CheckTransJsonValueFlag(availableroutetypes_forward, RPRT::SHUNTSTART, di, "shuntstart", ec, &conflictcheck);
	CheckTransJsonValueFlag(availableroutetypes_forward, RPRT::SHUNTEND, di, "shuntend", ec, &conflictcheck);
	CheckTransJsonValueFlag(availableroutetypes_forward, RPRT::ROUTESTART, di, "routestart", ec, &conflictcheck);
	CheckTransJsonValueFlag(availableroutetypes_forward, RPRT::ROUTEEND, di, "routeend", ec, &conflictcheck);
	CheckTransJsonValue(max_aspect, di, "maxaspect", ec);
	CheckTransJsonValueFlag(sflags, GSF::OVERLAPSWINGABLE, di, "overlapswingable", ec);
	CheckTransJsonValue(overlapswingminaspectdistance, di, "overlapswingminaspectdistance", ec);
	CheckTransJsonValueFlag(sflags, GSF::APPROACHLOCKINGMODE, di, "approachlockingmode", ec);
	CheckTransJsonValueFlag(sflags, GSF::OVERLAPTIMEOUTSTARTED, di, "overlaptimeoutstarted", ec);
	if(sflags & GSF::OVERLAPTIMEOUTSTARTED) CheckTransJsonValue(overlap_timeout_start, di, "overlap_timeout_start", ec);

	unsigned int approachcontrol_timeout;
	if(CheckTransJsonValue(approachcontrol_timeout, di, "approachcontroltimeout", ec)) {
		approachcontrol_default_shunt_timeout = approachcontrol_default_route_timeout = approachcontrol_timeout;
	}
	else {
		CheckTransJsonValue(approachcontrol_default_shunt_timeout, di, "approachcontroltimeout_shunt", ec);
		CheckTransJsonValue(approachcontrol_default_route_timeout, di, "approachcontroltimeout_route", ec);
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
			if(CheckTransJsonValue(rr.approachcontrol_timeout, subdi, "approachcontroltimeout", ec)) rr.routerestrictionflags |= route_restriction::RRF::ACTIMEOUTSET;
			if(CheckTransJsonValue(rr.overlap_timeout, subdi, "overlaptimeout", ec)) rr.routerestrictionflags |= route_restriction::RRF::OVERLAPTIMEOUTSET;
			flag_conflict_checker<route_restriction::RRDF> conflictcheck;
			CheckTransJsonValueFlag(rr.denyflags, route_restriction::RRDF::NOSHUNT, subdi, "denyshunt", ec, &conflictcheck);
			CheckTransJsonValueFlag(rr.denyflags, route_restriction::RRDF::NOROUTE, subdi, "denyroute", ec, &conflictcheck);
			CheckTransJsonValueFlag(rr.denyflags, route_restriction::RRDF::NOOVERLAP, subdi, "denyoverlap", ec, &conflictcheck);
			CheckTransJsonValueFlag(rr.denyflags, route_restriction::RRDF::NOSHUNT | route_restriction::RRDF::NOROUTE | route_restriction::RRDF::NOOVERLAP, subdi, "denyall", ec, &conflictcheck);
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
	CheckTransJsonValueFlag(availableroutetypes, RPRT::SHUNTEND, di, "shuntend", ec);
	CheckTransJsonValueFlag(availableroutetypes, RPRT::ROUTEEND, di, "routeend", ec);
	CheckTransJsonValueFlag(availableroutetypes, RPRT::OVERLAPEND, di, "overlapend", ec);
}

void startofline::Serialise(serialiser_output &so, error_collection &ec) const {
	routingpoint::Serialise(so, ec);

	SerialiseSubObjJson(trs, so, "trs", ec);
}
