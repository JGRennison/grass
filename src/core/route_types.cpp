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
#include "core/route_types.h"
#include "core/serialisable_impl.h"
#include "core/route_types_serialisation.h"

namespace route_class {
	std::array<name_set, static_cast<size_t>(ID::END)> route_names {{
		{ "null", "Null route type" },
		{ "shunt", "Shunt route" },
		{ "route", "Main route" },
		{ "overlap", "Overlap" },
		{ "alt_overlap_1", "Alternative Overlap 1" },
		{ "alt_overlap_2", "Alternative Overlap 2" },
		{ "alt_overlap_3", "Alternative Overlap 3" },
		{ "call_on", "Call On" },
	}};

	std::array<unsigned int, static_cast<size_t>(ID::END)> default_approach_locking_timeouts {{
		0,
		30000,
		90000,
		0,
	}};

	void DeserialiseGroupProp(set &s, const deserialiser_input &di, const char *prop, error_collection &ec, flag_conflict_checker<set> &conflictcheck) {
		const rapidjson::Value &subval=di.json[prop];

		if (subval.IsObject()) {
			di.RegisterProp(prop);
			deserialiser_input subdi(subval, "route_class_group", prop, di);
			DeserialiseGroup(s, subdi, ec, conflictcheck);
			subdi.PostDeserialisePropCheck(ec);
		} else {
			CheckJsonTypeAndReportError<json_object>(di, prop, subval, ec, false);
		}
	}

	void DeserialiseGroup(set &s, const deserialiser_input &di, error_collection &ec, flag_conflict_checker<set> &conflictcheck) {
		set val = 0;
		if (DeserialiseProp("allow", val, di, ec)) {
			s |= val;
			conflictcheck.RegisterFlags(true, val, di, "allow", ec);
		}
		if (DeserialiseProp("allow_only", val, di, ec)) {
			s = val;
			conflictcheck.RegisterFlags(true, val, di, "allow_only", ec);
			conflictcheck.RegisterFlags(false, ~val, di, "allow_only", ec);
		}
		if (DeserialiseProp("deny", val, di, ec)) {
			s &= ~val;
			conflictcheck.RegisterFlags(false, val, di, "deny", ec);
		}
	}

	bool DeserialiseProp(const char *prop, set &value, const deserialiser_input &di, error_collection &ec) {
		deserialiser_input subdi(di.json[prop], "route_class_set", prop, di);
		if (subdi.json.IsArray()) {
			di.RegisterProp(prop);
			value = Deserialise(subdi, ec);
			return true;
		} else if (subdi.json.IsString()) {
			di.RegisterProp(prop);
			value = Deserialise(subdi, ec);
			return true;
		} else if (!subdi.json.IsNull()) {
			ec.RegisterNewError<error_deserialisation>(di, "Invalid route class set definition");
		}
		return false;
	};

	set Deserialise(const deserialiser_input &di, error_collection &ec) {
		set current = 0;

		auto processitem = [&](rapidjson::SizeType index, const rapidjson::Value &item) {
			if (item.IsString()) {
				if (strcmp(item.GetString(), "all") == 0) {
					current |= route_class::All();
				} else if (strcmp(item.GetString(), "all_overlaps") == 0) {
					current |= route_class::AllOverlaps();
				} else if (strcmp(item.GetString(), "all_non_overlaps") == 0) {
					current |= route_class::AllNonOverlaps();
				} else if (strcmp(item.GetString(), "all_routes") == 0) {
					current |= route_class::AllRoutes();
				} else if (strcmp(item.GetString(), "all_shunts") == 0) {
					current |= route_class::AllShunts();
				} else if (strcmp(item.GetString(), "all_needing_overlaps") == 0) {
					current |= route_class::AllNeedingOverlap();
				} else {
					auto res = DeserialiseName(item.GetString());
					if (res.first) {
						current |= Flag(res.second);
					} else {
						ec.RegisterNewError<error_deserialisation>(di, "Invalid route class set definition: Invalid route type: " + std::string(item.GetString()));
					}
				}
			} else {
				CheckJsonTypeAndReportError<std::string>(di, MkArrayRefName(index).c_str(), item, ec, true);
			}
		};

		if (di.json.IsArray()) {
			for (rapidjson::SizeType i = 0; i < di.json.Size(); i++) {
				const rapidjson::Value &item = di.json[i];
				processitem(i, item);
			}
		} else {
			processitem(0, di.json);
		}

		return current;
	}

	std::pair<bool, ID> DeserialiseName(const std::string &name) {
		bool found = false;
		ID current = ID::NONE;
		for (unsigned char i = 0; i < static_cast<size_t>(ID::END); i++) {
			if (route_class::route_names[i].name == name) {
				found = true;
				current = static_cast<ID>(i);
				break;
			}
		}
		return std::make_pair(found, current);
	}

	void SerialiseProp(const char *prop, set value, serialiser_output &so) {
		so.json_out.String(prop);
		so.json_out.StartArray();
		for (unsigned char i = 0; i < static_cast<size_t>(ID::END); i++) {
			if (Flag(static_cast<ID>(i)) & value) {
				so.json_out.String(route_class::route_names[i].name);
			}
		}
		so.json_out.EndArray();
	}

	std::ostream& StreamOutRouteClassSet(std::ostream& os, const set& obj) {
		set found_types = obj;
		bool first = true;
		while (found_types) {
			route_class::set bit = found_types & (found_types ^ (found_types - 1));
			route_class::ID type = static_cast<route_class::ID>(__builtin_ffs(bit) - 1);
			found_types ^= bit;
			if (!first) {
				os << ", ";
			}
			os << GetRouteTypeName(type);
			first = false;
		}
		return os;
	}

	std::ostream& operator<<(std::ostream& os, set obj) {
		return StreamOutRouteClassSet(os, obj);
	}

	std::ostream& operator<<(std::ostream& os, route_class::ID obj) {
		os << GetRouteTypeName(obj);
		return os;
	}


}
