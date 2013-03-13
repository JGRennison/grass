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

void world_serialisation::ParseInputString(const std::string &input, error_collection &ec) {
	parsed_inputs.emplace_front();
	rapidjson::Document &dc =  parsed_inputs.front();
	if (dc.Parse<0>(input.c_str()).HasParseError()) {
		ec.RegisterError(std::unique_ptr<error_obj>(new generic_error_obj(string_format("JSON Parsing error at offset: %d, Error: %s", dc.GetErrorOffset(), dc.GetParseError()))));
	}
	else LoadGame(deserialiser_input("[root]", "", "[root]", dc, &w, this, 0), ec);
}

void world_serialisation::LoadGame(const deserialiser_input &di, error_collection &ec) {
	deserialiser_input contentdi(di.json["content"], "content", "content", di);
	if(contentdi.json.IsArray()) {
		for(rapidjson::SizeType i = 0; i < contentdi.json.Size(); i++) {
			deserialiser_input subdi("", "", MkArrayRefName(i), contentdi.json[i], &w, this, &contentdi);
			if(subdi.json.IsObject()) {
				subdi.seenprops.reserve(subdi.json.GetMemberCount());
				const rapidjson::Value &nameval = subdi.json["name"];
				if(nameval.IsString()) {
					subdi.name.assign(nameval.GetString(), nameval.GetStringLength());
					subdi.RegisterProp("name");
				}
				else subdi.name=string_format("#%d", i);

				const rapidjson::Value &typeval = subdi.json["type"];
				if(typeval.IsString()) {
					subdi.type.assign(typeval.GetString(), typeval.GetStringLength());
					subdi.RegisterProp("type");
					DeserialiseObject(subdi, ec);
				}
				else {
					ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(subdi, "LoadGame: Object has no type")));
				}
			}
			else {
				ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(subdi, "LoadGame: Expected object")));
			}
		}
	}
	else {
		ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(di, "LoadGame: Document root not array")));
	}
}

template <typename T> T* world_serialisation::MakeOrFindGenericTrack(const deserialiser_input &di, error_collection &ec) {
	std::unique_ptr<generictrack> &ptr = w.all_pieces[di.name];
	if(ptr) {
		if(typeid(*ptr) != typeid(T)) {
			ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(di, string_format("LoadGame: Track type definition conflict: %s is not a %s", ptr->GetFriendlyName().c_str(), di.type.c_str()))));
			return 0;
		}
	}
	else {
		ptr.reset(new T(w));
		ptr->SetPreviousTrackPiece(previoustrackpiece);
		previoustrackpiece = ptr.get();
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
	if(CheckTransJsonValue(name, di, "name", ec) && contentval.IsObject()) {
		template_map[name].json = &contentval;
	}
	else {
		ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(di, "Invalid template definition")));
	}
}

void world_serialisation::DeserialiseTypeDefinition(const deserialiser_input &di, error_collection &ec) {
	const rapidjson::Value &contentval=di.json["content"];
	std::string newtype;
	std::string basetype;
	if(CheckTransJsonValue(newtype, di, "newtype", ec) && CheckTransJsonValue(basetype, di, "basetype", ec)) {
		const rapidjson::Value *content;
		if(contentval.IsObject()) {
			content = &contentval;
		}
		else {
			CheckJsonTypeAndReportError<placeholder_subobject>(di, "content", contentval, ec);
			content = 0;
		}

		auto func = [=](const deserialiser_input &di, error_collection &ec) {
			deserialiser_input typedefwrapperdi(di.name, di.type, "Typedef Wrapper: " + newtype + " base: " + basetype, di.json, di);
			
			const deserialiser_input *checkparent = di.parent;
			do {
				if(checkparent->reference_name == typedefwrapperdi.reference_name) {
					ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(di, string_format("Typedef: \"%s\": Base \"%s\": Recursive expansion detected: Aborting.", newtype.c_str(), basetype.c_str()))));
					return;
				}
			} while((checkparent = checkparent->parent));
			
			typedefwrapperdi.seenprops.swap(di.seenprops);
			typedefwrapperdi.objpreparse = di.objpreparse;
			typedefwrapperdi.objpostparse = di.objpostparse;

			if(content) {
				deserialiser_input typedefcontentdi(*content, "Typedef Content: " + newtype + " base: " + basetype, typedefwrapperdi);
				deserialiser_input *targ = &typedefwrapperdi;
				while(targ->objpreparse) targ = targ->objpreparse;
				targ->objpreparse = &typedefcontentdi;
			}
			if(! this->object_types.FindAndDeserialise(basetype, typedefwrapperdi, ec))  {
				ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(typedefwrapperdi, string_format("Typedef expansion: %s: Unknown base type: %s", newtype.c_str(), basetype.c_str()))));
			}
			typedefwrapperdi.seenprops.swap(di.seenprops);
		};
		object_types.RegisterType(newtype, func);
	}
	else {
		ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(di, "Invalid typedef definition")));
	}
}

void world_serialisation::ExecuteTemplate(serialisable_obj &obj, std::string name, const deserialiser_input &di, error_collection &ec) {
	auto templ = template_map.find(name);
	if(templ != template_map.end() && templ->second.json) {
		if(templ->second.beingexpanded) {		//recursive expansion detection
			ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(di, string_format("Template: \"%s\": Recursive expansion detected: Aborting.", name.c_str()))));
			return;
		}
		else {
			templ->second.beingexpanded = true;	//recursive expansion detection
			obj.DeserialiseObject(deserialiser_input(*(templ->second.json), "Template: " + name, di), ec);
			templ->second.beingexpanded = false;
		}
	}
	else {
		ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(di, string_format("Template: \"%s\" not found", name.c_str()))));
	}
}

void world_serialisation::DeserialiseTractionType(const deserialiser_input &di, error_collection &ec) {
	std::string name;
	if(CheckTransJsonValue(name, di, "name", ec) && di.w) {
		di.w->AddTractionType(name, CheckGetJsonValueDef<bool, bool>(di, "alwaysavailable", false, ec));
	}
	else {
		ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(di, "Invalid traction type definition")));
	}
}

template <typename C> void world_serialisation::MakeGenericTrackTypeWrapper() {
	auto func = [&](const deserialiser_input &di, error_collection &ec) {
		this->DeserialiseGenericTrack<C>(di, ec);
	};
	object_types.RegisterType(C::GetTypeSerialisationNameStatic(), func);
}

void world_serialisation::InitObjectTypes() {
	MakeGenericTrackTypeWrapper<trackseg>();
	MakeGenericTrackTypeWrapper<points>();
	MakeGenericTrackTypeWrapper<autosignal>();
	MakeGenericTrackTypeWrapper<routesignal>();
	MakeGenericTrackTypeWrapper<catchpoints>();
	MakeGenericTrackTypeWrapper<springpoints>();
	MakeGenericTrackTypeWrapper<crossover>();
	MakeGenericTrackTypeWrapper<doubleslip>();
	MakeGenericTrackTypeWrapper<startofline>();
	MakeGenericTrackTypeWrapper<endofline>();
	MakeGenericTrackTypeWrapper<routingmarker>();
	object_types.RegisterType("template", [&](const deserialiser_input &di, error_collection &ec) { DeserialiseTemplate(di, ec); });
	object_types.RegisterType("typedef", [&](const deserialiser_input &di, error_collection &ec) { DeserialiseTypeDefinition(di, ec); });
	object_types.RegisterType("tractiontype", [&](const deserialiser_input &di, error_collection &ec) { DeserialiseTractionType(di, ec); });
}

void world_serialisation::DeserialiseObject(const deserialiser_input &di, error_collection &ec) {
	if(!object_types.FindAndDeserialise(di.type, di, ec)) {
		ec.RegisterError(std::unique_ptr<error_obj>(new error_deserialisation(di, string_format("LoadGame: Unknown object type: %s", di.type.c_str()))));
	}
}
