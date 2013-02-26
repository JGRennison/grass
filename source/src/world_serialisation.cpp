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
#include "world.h"
#include "serialisable_impl.h"
#include "error.h"
#include "util.h"
#include "world_serialisation.h"
#include "track.h"
#include "signal.h"
#include <typeinfo>

void world_serialisation::LoadGame(const rapidjson::Value &json, error_collection &ec) {
	if(json.IsArray()) {
		for(rapidjson::SizeType i = 0; i < json.Size(); i++) {
			const rapidjson::Value &cur = json[i];
			if(cur.IsObject()) {
				const rapidjson::Value &typeval = cur["type"];
				if(typeval.IsString()) {
					std::string type(typeval.GetString(), typeval.GetStringLength());
					const rapidjson::Value &nameval = cur["name"];
					std::string name;
					if(nameval.IsString()) {
						name.assign(nameval.GetString(), nameval.GetStringLength());
					}
					else name=string_format("#%d", i);

					DeserialiseObject(deserialiser_input(name, type, cur, &w, this), ec);
				}
				else {
					ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation("LoadGame: Object has no type")));
				}
			}
			else {
				ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation("LoadGame: Expected object")));
			}
		}
	}
	else {
		ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation("LoadGame: Document root not array")));
	}
}

template <typename T> T* world_serialisation::MakeOrFindGenericTrack(const deserialiser_input &di, error_collection &ec) {
	std::unique_ptr<generictrack> &ptr = w.all_pieces[di.name];
	if(ptr) {
		if(typeid(*ptr) != typeid(T)) {
			ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(string_format("LoadGame: Track type definition conflict: %s is not a %s", ptr->GetFriendlyName().c_str(), di.type.c_str()))));
			return 0;
		}
	}
	else {
		ptr.reset(new T(w));
	}
	return static_cast<T*>(ptr.get());
}

template <typename T> T* world_serialisation::DeserialiseGenericTrack(const deserialiser_input &di, error_collection &ec) {
	T* target = MakeOrFindGenericTrack<T>(di, ec);
	if(target) {
		target->DeserialiseObject(di, ec);
	}
	return target;
}

void world_serialisation::DeserialiseTemplate(const deserialiser_input &di, error_collection &ec) {
	const rapidjson::Value &contentval=di.json["content"];
	std::string name;
	if(CheckTransJsonValue(name, di.json, "name", ec) && contentval.IsObject()) {
		template_map[name] = &contentval;
	}
	else {
		ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation("Invalid template definition")));
	}
}

void world_serialisation::ExecuteTemplate(serialisable_obj &obj, std::string name, const deserialiser_input &di, error_collection &ec) {
	auto templ = template_map.find(name);
	if(templ != template_map.end() && templ->second) {
		obj.Deserialise(deserialiser_input(*(templ->second), di), ec);
	}
	else {
		ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(string_format("Template: \"%s\" not found", name.c_str()))));
	}
}

void world_serialisation::DeserialiseTractionType(const deserialiser_input &di, error_collection &ec) {
	std::string name;
	if(CheckTransJsonValue(name, di.json, "name", ec) && di.w) {
		di.w->AddTractionType(name, CheckGetJsonValueDef<bool, bool>(di.json, "alwaysavailable", false, ec));
	}
	else {
		ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation("Invalid traction type definition")));
	}
}

void world_serialisation::DeserialiseObject(const deserialiser_input &di, error_collection &ec) {
	if(di.type == "trackseg") {
		DeserialiseGenericTrack<trackseg>(di, ec);
	}
	else if(di.type == "points") {
		DeserialiseGenericTrack<points>(di, ec);
	}
	else if(di.type == "autosignal") {
		DeserialiseGenericTrack<autosignal>(di, ec);
	}
	else if(di.type == "routesignal") {
		DeserialiseGenericTrack<routesignal>(di, ec);
	}
	else if(di.type == "template") {
		DeserialiseTemplate(di, ec);
	}
	else if(di.type == "tractiontype") {
		DeserialiseTractionType(di, ec);
	}
	else {
		ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(string_format("LoadGame: Unknown object type: %s", di.type.c_str()))));
	}
}
