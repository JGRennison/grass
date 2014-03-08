//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
//  WEBSITE: http://grss.sourceforge.net
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

#ifndef INC_ROUTETYPES_SERIALISATION_ALREADY
#define INC_ROUTETYPES_SERIALISATION_ALREADY

#include "core/routetypes.h"
#include "core/serialisable_impl.h"

namespace route_class {
	set Deserialise(const deserialiser_input &di, error_collection &ec);
	bool DeserialiseProp(const char *prop, set &value, const deserialiser_input &di, error_collection &ec);
	void DeserialiseGroup(set &s, const deserialiser_input &di, error_collection &ec, flag_conflict_checker<set> &conflictcheck);
	inline void DeserialiseGroup(set &s, const deserialiser_input &di, error_collection &ec) {
		flag_conflict_checker<set> conflictcheck;
		DeserialiseGroup(s, di, ec, conflictcheck);
	}
	void DeserialiseGroupProp(set &s, const deserialiser_input &di, const char *prop, error_collection &ec, flag_conflict_checker<set> &conflictcheck);
	inline void DeserialiseGroupProp(set &s, const deserialiser_input &di, const char *prop, error_collection &ec) {
		flag_conflict_checker<set> conflictcheck;
		DeserialiseGroupProp(s, di, prop, ec, conflictcheck);
	}
	std::pair<bool, ID> DeserialiseName(const std::string &name);
}

template <> inline bool IsType<route_class::ID>(const rapidjson::Value& val) {
	if(val.IsString()) {
		auto pair = route_class::DeserialiseName(val.GetString());
		return pair.first;
	}
	else return false;
}

template <> inline route_class::ID GetType<route_class::ID>(const rapidjson::Value& val) {
	auto pair = route_class::DeserialiseName(val.GetString());
	return pair.second;
}

template <> inline void SetType<route_class::ID>(Handler &out, route_class::ID val) {
	out.String(route_class::route_names[val].name);
}

template <> inline const char *GetTypeFriendlyName<route_class::ID>() { return "route class ID"; }

template <> struct flagtyperemover<route_class::ID> {
	typedef route_class::ID type;
};

#endif
