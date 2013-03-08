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
	
	CheckTransJsonValueFlag<unsigned int>(availableroutetypes_forward, RPRT_ROUTETRANS, di, "routethrough", ec);
	CheckTransJsonValueFlag<unsigned int>(availableroutetypes_forward, RPRT_SHUNTTRANS, di, "shuntthrough", ec);
	CheckTransJsonValueFlag<unsigned int>(availableroutetypes_forward, RPRT_OVERLAPTRANS, di, "overlapthrough", ec);
	CheckTransJsonValueFlag<unsigned int>(availableroutetypes_reverse, RPRT_ROUTETRANS, di, "routethrough_rev", ec);
	CheckTransJsonValueFlag<unsigned int>(availableroutetypes_reverse, RPRT_SHUNTTRANS, di, "shuntthrough_rev", ec);
	CheckTransJsonValueFlag<unsigned int>(availableroutetypes_reverse, RPRT_OVERLAPTRANS, di, "overlapthrough_rev", ec);
}

void trackroutingpoint::Serialise(serialiser_output &so, error_collection &ec) const {
	routingpoint::Serialise(so, ec);
}

void genericsignal::Deserialise(const deserialiser_input &di, error_collection &ec) {
	trackroutingpoint::Deserialise(di, ec);

	CheckTransJsonSubObj(start_trs, di, "start_trs", "trs", ec);
	CheckTransJsonSubObj(end_trs, di, "end_trs", "trs", ec);
	CheckTransJsonValueFlag<unsigned int>(availableroutetypes_forward, RPRT_SHUNTSTART | RPRT_SHUNTEND, di, "shuntsignal", ec);
	CheckTransJsonValueFlag<unsigned int>(availableroutetypes_forward, RPRT_ROUTESTART | RPRT_ROUTEEND | RPRT_SHUNTEND, di, "routesignal", ec);
	CheckTransJsonValueFlag<unsigned int>(availableroutetypes_forward, RPRT_SHUNTSTART, di, "shuntstart", ec);
	CheckTransJsonValueFlag<unsigned int>(availableroutetypes_forward, RPRT_SHUNTEND, di, "shuntend", ec);
	CheckTransJsonValueFlag<unsigned int>(availableroutetypes_forward, RPRT_ROUTESTART, di, "routestart", ec);
	CheckTransJsonValueFlag<unsigned int>(availableroutetypes_forward, RPRT_ROUTEEND, di, "routeend", ec);
	CheckTransJsonValueFlag<unsigned int>(availableroutetypes_forward, RPRT_OVERLAPEND, di, "overlapend", ec);
}

void genericsignal::Serialise(serialiser_output &so, error_collection &ec) const {
	trackroutingpoint::Serialise(so, ec);

	SerialiseSubObjJson(start_trs, so, "start_trs", ec);
	SerialiseSubObjJson(end_trs, so, "end_trs", ec);
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
			if(CheckTransJsonValue(rr.priority, subdi, "priority", ec)) rr.routerestrictionflags |= route_restriction::RRF_PRIORITYSET;
			CheckTransJsonValueFlag<unsigned int>(rr.denyflags, route_restriction::RRDF_NOSHUNT, subdi, "denyshunt", ec);
			CheckTransJsonValueFlag<unsigned int>(rr.denyflags, route_restriction::RRDF_NOROUTE, subdi, "denyroute", ec);
			CheckTransJsonValueFlag<unsigned int>(rr.denyflags, route_restriction::RRDF_NOOVERLAP, subdi, "denyoverlap", ec);
			CheckTransJsonValueFlag<unsigned int>(rr.denyflags, route_restriction::RRDF_NOSHUNT | route_restriction::RRDF_NOROUTE | route_restriction::RRDF_NOOVERLAP, subdi, "denyall", ec);
		}
		else {
			ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(subdi, "Invalid route restriction definition")));
		}
	}
}

void route_restriction_set::Serialise(serialiser_output &so, error_collection &ec) const {
}
