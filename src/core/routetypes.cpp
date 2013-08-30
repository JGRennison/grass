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
#include "routetypes.h"
#include "serialisable_impl.h"
#include "routetypes_serialisation.h"

namespace route_class {
	std::array<name_set, LAST_RTC> route_names {{
		{ "null", "Null route type" },
		{ "shunt", "Shunt route" },
		{ "route", "Main route" },
		{ "overlap", "Overlap" },
		{ "altoverlap1", "Alternative Overlap 1" },
		{ "altoverlap2", "Alternative Overlap 2" },
		{ "altoverlap3", "Alternative Overlap 3" },
		{ "callon", "Call On" },
	}};

	std::array<unsigned int, LAST_RTC> default_approach_locking_timeouts {{
		0,
		30000,
		90000,
		0,
	}};

	void DeserialiseGroupProp(set &s, const deserialiser_input &di, const char *prop, error_collection &ec, flag_conflict_checker<set> &conflictcheck) {
		const rapidjson::Value &subval=di.json[prop];

		if(subval.IsObject()) {
			di.RegisterProp(prop);
			deserialiser_input subdi(subval, "routeclassgroup", prop, di);
			DeserialiseGroup(s, subdi, ec, conflictcheck);
			subdi.PostDeserialisePropCheck(ec);
		}
		else CheckJsonTypeAndReportError<json_object>(di, prop, subval, ec, false);
	}

	void DeserialiseGroup(set &s, const deserialiser_input &di, error_collection &ec, flag_conflict_checker<set> &conflictcheck) {
		set val = 0;
		if(DeserialiseProp("allow", val, di, ec)) {
			s |= val;
			conflictcheck.RegisterFlags(true, val, di, "allow", ec);
		}
		if(DeserialiseProp("allowonly", val, di, ec)) {
			s = val;
			conflictcheck.RegisterFlags(true, val, di, "allowonly", ec);
			conflictcheck.RegisterFlags(false, ~val, di, "allowonly", ec);
		}
		if(DeserialiseProp("deny", val, di, ec)) {
			s &= ~val;
			conflictcheck.RegisterFlags(false, val, di, "deny", ec);
		}
	}

	bool DeserialiseProp(const char *prop, set &value, const deserialiser_input &di, error_collection &ec) {
		deserialiser_input subdi(di.json[prop], "routeclassset", prop, di);
		if(subdi.json.IsArray()) {
			di.RegisterProp(prop);
			value = Deserialise(subdi, ec);
			return true;
		}
		else if(subdi.json.IsString()) {
			di.RegisterProp(prop);
			value = Deserialise(subdi, ec);
			return true;
		}
		else if(!subdi.json.IsNull()) ec.RegisterNewError<error_deserialisation>(di, "Invalid route class set definition");
		return false;
	};

	set Deserialise(const deserialiser_input &di, error_collection &ec) {
		set current = 0;

		auto processitem = [&](rapidjson::SizeType index, const rapidjson::Value &item) {
			if(item.IsString()) {
				if(strcmp(item.GetString(), "all") == 0) current |= route_class::All();
				else if(strcmp(item.GetString(), "alloverlaps") == 0) current |= route_class::AllOverlaps();
				else if(strcmp(item.GetString(), "allnonoverlaps") == 0) current |= route_class::AllNonOverlaps();
				else if(strcmp(item.GetString(), "allroutes") == 0) current |= route_class::AllRoutes();
				else if(strcmp(item.GetString(), "allshunts") == 0) current |= route_class::AllShunts();
				else if(strcmp(item.GetString(), "allneedingoverlaps") == 0) current |= route_class::AllNeedingOverlap();
				else {
					auto res = DeserialiseName(item.GetString(), ec);
					if(res.first) current |= Flag(res.second);
					else ec.RegisterNewError<error_deserialisation>(di, "Invalid route class set definition: Invalid route type: " + std::string(item.GetString()));
				}
			}
			else CheckJsonTypeAndReportError<std::string>(di, MkArrayRefName(index).c_str(), item, ec, true);
		};

		if(di.json.IsArray()) {
			for(rapidjson::SizeType i = 0; i < di.json.Size(); i++) {
				const rapidjson::Value &item = di.json[i];
				processitem(i, item);
			}
		}
		else processitem(0, di.json);

		return current;
	}

	std::pair<bool, ID> DeserialiseName(const std::string &name, error_collection &ec) {
		bool found = false;
		ID current = ID::RTC_NULL;
		for(unsigned char i = 1; i < route_class::LAST_RTC; i++ ) {
			if(route_class::route_names[i].name == name) {
				found = true;
				current = static_cast<ID>(i);
				break;
			}
		}
		return std::make_pair(found, current);
	}

	std::ostream& StreamOutRouteClassSet(std::ostream& os, const set& obj) {
		set found_types = obj;
		bool first = true;
		while(found_types) {
			route_class::set bit = found_types & (found_types ^ (found_types - 1));
			route_class::ID type = static_cast<route_class::ID>(__builtin_ffs(bit) - 1);
			found_types ^= bit;
			if(!first) os << ", ";
			os << GetRouteTypeName(type);
			first = false;
		}
		return os;
	}
}
