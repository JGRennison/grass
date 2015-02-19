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
#include "util/util.h"
#include "core/serialisable_impl.h"
#include "core/route.h"
#include "core/routetypes_serialisation.h"
#include "core/deserialisation_scalarconv.h"

void ParseAspectMaskString(aspect_mask_type &aspect_mask, const std::string &aspect_string, const deserialiser_input &di,
		error_collection &ec, const std::string &value_name) {

	auto invalid_format = [&](const char *str, size_t len) {
		ec.RegisterNewError<error_deserialisation>(di,
				string_format("Invalid format for %s: '%s'", value_name.c_str(), std::string(str, len).c_str()));
	};
	auto getnumber = [&](const char *str, size_t len) -> unsigned int {
		unsigned int num = 0;
		bool error = false;
		for(size_t i = 0; i < len; i++) {
			char c = str[i];
			if(c >= '0' && c <= '9') num = (num * 10) + c - '0';
			else {
				error = true;
				break;
			}
			if(num > ASPECT_MAX) {
				error = true;
				break;
			}
		}
		if(error) ec.RegisterNewError<error_deserialisation>(di,
				string_format("Invalid numeric format %s: '%s' is not an integer between 0 and %u",
						value_name.c_str(), std::string(str, len).c_str(), ASPECT_MAX));
		return num;
	};

	aspect_mask = 0;
	std::vector<std::pair<const char*, size_t>> splitresult;
	std::vector<std::pair<const char*, size_t>> innersplitresult;
	SplitString(aspect_string.c_str(), aspect_string.size(), ',', splitresult);
	for(auto &it : splitresult) {
		const char *str = it.first;
		size_t len = it.second;
		SplitString(str, len, '-', innersplitresult);
		if(innersplitresult.size() > 2) {
			invalid_format(str, len);
		}
		else {
			unsigned int vallow;
			unsigned int valhigh;

			vallow = getnumber(innersplitresult[0].first, innersplitresult[0].second);
			if(innersplitresult.size() == 2) {
				valhigh = getnumber(innersplitresult[1].first, innersplitresult[1].second);
				if(vallow > valhigh) std::swap(vallow, valhigh);
			}
			else valhigh = vallow;

			unsigned int highmask = (valhigh == 31) ? 0xFFFFFFFF : ((1 << (valhigh + 1)) - 1);
			unsigned int lowmask = (vallow == 31) ? 0x7FFFFFFF : (((1 << (vallow + 1)) - 1) >> 1);
			aspect_mask |= highmask ^ lowmask;
		}
	}
}

//returns true if value set
bool DeserialiseAspectProps(aspect_mask_type &aspect_mask, const deserialiser_input &di, error_collection &ec) {
	unsigned int max_aspect;
	std::string aspect_string;
	if(CheckTransJsonValue(max_aspect, di, "maxaspect", ec)) {
		if(max_aspect > ASPECT_MAX) {
			ec.RegisterNewError<error_deserialisation>(di, string_format("Maximum signal aspect cannot exceed %u. %u given.", ASPECT_MAX, max_aspect));
		}
		else aspect_mask = (max_aspect == 31) ? 0xFFFFFFFF : (1 << (max_aspect + 1)) - 1;
		return true;
	}
	else if(CheckTransJsonValue(aspect_string, di, "allowedaspects", ec)) {
		ParseAspectMaskString(aspect_mask, aspect_string, di, ec, "allowed aspects");
		return true;
	}
	return false;
}

void DeserialiseConditionalAspectProps(std::vector<route_common::conditional_aspect_mask> &cams, const deserialiser_input &di, error_collection &ec) {
	CheckIterateJsonArrayOrType<json_object>(di, "conditionalallowedaspects", "conditionalallowedaspect", ec, [&](const deserialiser_input &innerdi, error_collection &ec) {
		cams.emplace_back();

		auto parse_aspect = [&](aspect_mask_type &aspect_mask, const char *prop, const std::string &name) {
			std::string aspect_string;
			if(CheckTransJsonValue(aspect_string, innerdi, prop, ec)) {
				ParseAspectMaskString(aspect_mask, aspect_string, innerdi, ec, name);
			}
		};
		parse_aspect(cams.back().condition, "condition", "aspect mask condition");
		parse_aspect(cams.back().aspect_mask, "allowedaspects", "conditional allowed aspects");
		innerdi.PostDeserialisePropCheck(ec);
	});
}

void route_common::DeserialiseRouteCommon(const deserialiser_input &subdi, error_collection &ec, DeserialisationFlags flags) {
	if(DeserialiseAspectProps(aspect_mask, subdi, ec)) routecommonflags |= route_restriction::RCF::ASPECTMASK_SET;
	DeserialiseConditionalAspectProps(conditional_aspect_masks, subdi, ec);
	if(flags & DeserialisationFlags::ASPECTMASK_ONLY) return;

	//f's references must not go out of scope until after the layout init final fixups phase
	auto deserialise_ttcb = [&](std::string prop, std::function<void(track_train_counter_block *)> f) {
		std::string ttcb_name;
		if(CheckTransJsonValue(ttcb_name, subdi, prop.c_str(), ec)) {
			world *w = subdi.w;
			if(w) {
				auto func = [this, ttcb_name, w, prop, f](error_collection &ec) {
					track_train_counter_block *ttcb = w->FindTrackTrainBlockOrTrackCircuitByName(ttcb_name);
					if(ttcb) {
						f(ttcb);
					}
					else {
						ec.RegisterNewError<generic_error_obj>(prop + ": No such track circuit or track trigger: " + ttcb_name);
					}
				};
				w->layout_init_final_fixups.AddFixup(func);
			}
			else {
				ec.RegisterNewError<error_deserialisation>(subdi, prop + " not valid in this context (no world)");
			}
		}
	};

	if(CheckTransJsonValue(priority, subdi, "priority", ec)) routecommonflags |= route_restriction::RCF::PRIORITYSET;
	if(!(flags & DeserialisationFlags::NO_APLOCK_TIMEOUT)) {
		if(CheckTransJsonValueProc(approachlocking_timeout, subdi, "approachlockingtimeout", ec, dsconv::Time)) routecommonflags |= route_restriction::RCF::APLOCK_TIMEOUTSET;
	}
	if(CheckTransJsonValueProc(overlap_timeout, subdi, "overlaptimeout", ec, dsconv::Time)) routecommonflags |= route_restriction::RCF::OVERLAPTIMEOUTSET;
	if(CheckTransJsonValueProc(routeprove_delay, subdi, "routeprovedelay", ec, dsconv::Time)) routecommonflags |= route_restriction::RCF::ROUTEPROVEDELAY_SET;
	if(CheckTransJsonValueProc(routeclear_delay, subdi, "routecleardelay", ec, dsconv::Time)) routecommonflags |= route_restriction::RCF::ROUTECLEARDELAY_SET;
	if(CheckTransJsonValueProc(routeset_delay, subdi, "routesetdelay", ec, dsconv::Time)) routecommonflags |= route_restriction::RCF::ROUTESETDELAY_SET;
	deserialise_ttcb("overlaptimeouttrigger", [this](track_train_counter_block *ttcb) {
		this->overlaptimeout_trigger = ttcb;
	});
	bool res = CheckTransJsonValueFlag(routecommonflags, route_restriction::RCF::APCONTROL, subdi, "approachcontrol", ec);
	if(res) routecommonflags |= route_restriction::RCF::APCONTROL_SET;
	if(!res || routecommonflags & route_restriction::RCF::APCONTROL) {
		if(CheckTransJsonValueProc(approachcontrol_triggerdelay, subdi, "approachcontroltriggerdelay", ec, dsconv::Time)) routecommonflags |= route_restriction::RCF::APCONTROLTRIGGERDELAY_SET | route_restriction::RCF::APCONTROL_SET | route_restriction::RCF::APCONTROL;
		deserialise_ttcb("approachcontroltrigger", [this](track_train_counter_block *ttcb) {
			this->approachcontrol_trigger = ttcb;
			this->routecommonflags |= route_restriction::RCF::APCONTROL_SET | route_restriction::RCF::APCONTROL;
		});

		// Approach control if no forward route
		bool ap_control_if_route_res = CheckTransJsonValueFlag(routecommonflags, route_restriction::RCF::APCONTROL_IF_NOROUTE, subdi, "approachcontrolifnoforwardroute", ec);
		if(ap_control_if_route_res) routecommonflags |= route_restriction::RCF::APCONTROL_IF_NOROUTE_SET;
		// If user enabled it here, also enable approach control
		if(ap_control_if_route_res && routecommonflags & route_restriction::RCF::APCONTROL_IF_NOROUTE) routecommonflags |= route_restriction::RCF::APCONTROL_SET | route_restriction::RCF::APCONTROL;
	}
	if(CheckTransJsonValueFlag(routecommonflags, route_restriction::RCF::TORR, subdi, "torr", ec)) routecommonflags |= route_restriction::RCF::TORR_SET;
	if(CheckTransJsonValueFlag(routecommonflags, route_restriction::RCF::EXITSIGCONTROL, subdi, "exitsignalcontrol", ec)) routecommonflags |= route_restriction::RCF::EXITSIGCONTROL_SET;

	deserialiser_input odi(subdi.json["overlap"], "overlap", "overlap", subdi);
	if(!odi.json.IsNull()) {
		subdi.RegisterProp("overlap");

		if(odi.json.IsBool()) overlap_type = odi.json.GetBool() ? route_class::ID::RTC_OVERLAP : route_class::ID::RTC_NULL;
		else if(odi.json.IsString()) {
			auto res = route_class::DeserialiseName(odi.json.GetString());
			if(res.first) overlap_type = res.second;
			else ec.RegisterNewError<error_deserialisation>(odi, "Invalid overlap type definition: " + std::string(odi.json.GetString()));
		}
		else ec.RegisterNewError<error_deserialisation>(odi, "Invalid overlap type definition: wrong type");

		routecommonflags |= RCF::OVERLAPTYPE_SET;
	}
}

void route_restriction_set::DeserialiseRestriction(const deserialiser_input &subdi, error_collection &ec, bool isendtype) {
	restrictions.emplace_back();
	route_restriction &rr = restrictions.back();

	if(isendtype) CheckFillTypeVectorFromJsonArrayOrType<std::string>(subdi, "routestart", ec, rr.targets);
	else {
		CheckFillTypeVectorFromJsonArrayOrType<std::string>(subdi, "targets", ec, rr.targets) ||
			CheckFillTypeVectorFromJsonArrayOrType<std::string>(subdi, "routeend", ec, rr.targets);
	}
	CheckFillTypeVectorFromJsonArrayOrType<std::string>(subdi, "via", ec, rr.via);
	CheckFillTypeVectorFromJsonArrayOrType<std::string>(subdi, "notvia", ec, rr.notvia);

	rr.DeserialiseRouteCommon(subdi, ec);

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

	subdi.PostDeserialisePropCheck(ec);
}
