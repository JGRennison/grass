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

#include "common.h"
#include "core/world.h"
#include "core/serialisable_impl.h"
#include "core/error.h"
#include "core/util.h"
#include "core/world_serialisation.h"
#include "core/track.h"
#include "core/trackpiece.h"
#include "core/points.h"
#include "core/signal.h"
#include "core/trackcircuit.h"
#include "core/train.h"
#include <typeinfo>

void world_serialisation::ParseInputString(const std::string &input, error_collection &ec, world_serialisation::WSLOADGAME_FLAGS flags) {
	parsed_inputs.emplace_front();
	rapidjson::Document &dc =  parsed_inputs.front();
	if (dc.Parse<0>(input.c_str()).HasParseError()) {
		ec.RegisterNewError<error_jsonparse>(input, dc.GetErrorOffset(), dc.GetParseError());
	}
	else LoadGame(deserialiser_input("", "[root]", dc, &w, this, 0), ec, flags);
}

void world_serialisation::LoadGame(const deserialiser_input &di, error_collection &ec, world_serialisation::WSLOADGAME_FLAGS flags) {
	if(!(flags & WSLOADGAME_FLAGS::NOCONTENT)) {
		deserialiser_input contentdi(di.json["content"], "content", "content", di);
		if(!contentdi.json.IsNull()) {
			DeserialiseRootObjArray(content_object_types, ws_dtf_params(), contentdi, ec);
		}
	}

	if(!(flags & WSLOADGAME_FLAGS::NOGAMESTATE)) {
		deserialiser_input gamestatetdi(di.json["gamestate"], "gamestate", "gamestate", di);
		if(!gamestatetdi.json.IsNull()) {
			if(flags & WSLOADGAME_FLAGS::TRYREPLACEGAMESTATE) {
				gamestate_init.Clear();
			}
			auto gsclone = gamestatetdi.CloneWithAncestors();
			gamestate_init.AddFixup([gsclone, this](error_collection &ec) {
				DeserialiseRootObjArray(gamestate_object_types, ws_dtf_params(ws_dtf_params::WSDTFP_FLAGS::NONEWTRACK), *gsclone->GetTop(), ec);
			});
		}
	}
}

void world_serialisation::DeserialiseGameState(error_collection &ec) {
	gamestate_init.Execute(ec);
}

void world_serialisation::DeserialiseRootObjArray(const ws_deserialisation_type_factory &wdtf, const ws_dtf_params &wdtf_params, const deserialiser_input &contentdi, error_collection &ec) {
	if(contentdi.json.IsArray()) {
		for(rapidjson::SizeType i = 0; i < contentdi.json.Size(); i++) {
			deserialiser_input subdi("", MkArrayRefName(i), contentdi.json[i], &w, this, &contentdi);
			if(subdi.json.IsObject()) {
				subdi.seenprops.reserve(subdi.json.GetMemberCount());

				current_content_index = i;

				const rapidjson::Value &typeval = subdi.json["type"];
				if(typeval.IsString()) {
					subdi.type.assign(typeval.GetString(), typeval.GetStringLength());
					subdi.RegisterProp("type");
					DeserialiseObject(wdtf, wdtf_params, subdi, ec);
				}
				else {
					ec.RegisterNewError<error_deserialisation>(subdi, "LoadGame: Object has no type");
				}
			}
			else {
				ec.RegisterNewError<error_deserialisation>(subdi, "LoadGame: Expected object");
			}
		}
	}
	else if(!contentdi.json.IsNull()) {
		ec.RegisterNewError<error_deserialisation>(contentdi, "LoadGame: Top level section not array");
	}
}

template <typename T> T* world_serialisation::MakeOrFindGenericTrack(const deserialiser_input &di, error_collection &ec, bool findonly) {
	std::string trackname;
	if(!CheckTransJsonValue(trackname, di, "name", ec)) {
		trackname=string_format("#%d", current_content_index);
	}

	std::unique_ptr<generictrack> &ptr = w.all_pieces[trackname];
	if(ptr) {
		if(typeid(*ptr) != typeid(T)) {
			ec.RegisterNewError<error_deserialisation>(di, string_format("LoadGame: Track type definition conflict: %s is not a %s", ptr->GetFriendlyName().c_str(), di.type.c_str()));
			return 0;
		}
	}
	else if(!findonly) {
		ptr.reset(new T(w));
		ptr->SetName(trackname);
		ptr->SetPreviousTrackPiece(previoustrackpiece);
		if(previoustrackpiece) previoustrackpiece->SetNextTrackPiece(ptr.get());
		previoustrackpiece = ptr.get();
	}
	else {
		ec.RegisterNewError<error_deserialisation>(di, string_format("LoadGame: Cannot make a new track piece at this point: %s", trackname.c_str()));
		return 0;
	}
	return static_cast<T*>(ptr.get());
}

template <typename T> T* world_serialisation::DeserialiseGenericTrack(const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) {
	T* target = MakeOrFindGenericTrack<T>(di, ec, wdp.flags & ws_dtf_params::WSDTFP_FLAGS::NONEWTRACK);
	if(target) {
		target->DeserialiseObject(di, ec);
	}
	return target;
}

void world_serialisation::DeserialiseTemplate(const deserialiser_input &di, error_collection &ec) {
	const rapidjson::Value *contentval;
	std::string name;
	if(CheckTransJsonValue(name, di, "name", ec) && CheckTransRapidjsonValue<json_object>(contentval, di, "content", ec)) {
		template_map[name].json = contentval;
		di.PostDeserialisePropCheck(ec);
	}
	else {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid template definition");
	}
}

void world_serialisation::DeserialiseTypeDefinition(const deserialiser_input &di, error_collection &ec) {
	std::string newtype;
	std::string basetype;
	if(CheckTransJsonValue(newtype, di, "newtype", ec) && CheckTransJsonValue(basetype, di, "basetype", ec)) {
		const rapidjson::Value *content;
		CheckTransRapidjsonValueDef<json_object>(content, di, "content", ec);

		auto func = [=](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) {
			deserialiser_input typedefwrapperdi(di.type, "Typedef Wrapper: " + newtype + " base: " + basetype, di.json, di);

			const deserialiser_input *checkparent = di.parent;
			do {
				if(checkparent->reference_name == typedefwrapperdi.reference_name) {
					ec.RegisterNewError<error_deserialisation>(di, string_format("Typedef: \"%s\": Base \"%s\": Recursive expansion detected: Aborting.", newtype.c_str(), basetype.c_str()));
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
			if(! this->content_object_types.FindAndDeserialise(basetype, typedefwrapperdi, ec, wdp))  {
				ec.RegisterNewError<error_deserialisation>(typedefwrapperdi, string_format("Typedef expansion: %s: Unknown base type: %s", newtype.c_str(), basetype.c_str()));
			}
			typedefwrapperdi.seenprops.swap(di.seenprops);
		};
		content_object_types.RegisterType(newtype, func);
		di.PostDeserialisePropCheck(ec);
	}
	else {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid typedef definition");
	}
}

void world_serialisation::ExecuteTemplate(serialisable_obj &obj, std::string name, const deserialiser_input &di, error_collection &ec) {
	auto templ = template_map.find(name);
	if(templ != template_map.end() && templ->second.json) {
		if(templ->second.beingexpanded) {    //recursive expansion detection
			ec.RegisterNewError<error_deserialisation>(di, string_format("Template: \"%s\": Recursive expansion detected: Aborting.", name.c_str()));
			return;
		}
		else {
			templ->second.beingexpanded = true;    //recursive expansion detection
			obj.DeserialiseObject(deserialiser_input(*(templ->second.json), "Template: " + name, di), ec);
			templ->second.beingexpanded = false;
		}
	}
	else {
		ec.RegisterNewError<error_deserialisation>(di, string_format("Template: \"%s\" not found", name.c_str()));
	}
}

void world_serialisation::DeserialiseTractionType(const deserialiser_input &di, error_collection &ec) {
	std::string name;
	if(CheckTransJsonValue(name, di, "name", ec) && di.w) {
		di.w->AddTractionType(name, CheckGetJsonValueDef<bool, bool>(di, "alwaysavailable", false, ec));
		di.PostDeserialisePropCheck(ec);
	}
	else {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid traction type definition");
	}
}

void world_serialisation::DeserialiseTrackCircuit(const deserialiser_input &di, error_collection &ec) {
	std::string name;
	if(CheckTransJsonValue(name, di, "name", ec) && di.w) {
		track_circuit *tc = di.w->track_circuits.FindOrMakeByName(name);
		tc->DeserialiseObject(di, ec);
	}
	else {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid track circuit definition");
	}
}

void world_serialisation::DeserialiseTrackTrainBlock(const deserialiser_input &di, error_collection &ec) {
	std::string name;
	if(CheckTransJsonValue(name, di, "name", ec) && di.w) {
		track_train_counter_block *tc = di.w->track_triggers.FindOrMakeByName(name);
		tc->DeserialiseObject(di, ec);
	}
	else {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid track train counter block definition");
	}
}

void world_serialisation::DeserialiseVehicleClass(const deserialiser_input &di, error_collection &ec) {
	std::string name;
	if(CheckTransJsonValue(name, di, "name", ec) && di.w) {
		vehicle_class *vc = di.w->FindOrMakeVehicleClassByName(name);
		vc->DeserialiseObject(di, ec);
	}
	else {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid vehicle class definition");
	}
}

void world_serialisation::DeserialiseTrain(const deserialiser_input &di, error_collection &ec) {
	if(di.w) {
		std::string name;
		train *t = 0;
		if(CheckTransJsonValue(name, di, "name", ec)) {
			t = di.w->FindTrainByName(name);
			if(!t) {
				t = di.w->CreateEmptyTrain();
				t->SetName(name);
			}

		}
		else {
			t = di.w->CreateEmptyTrain();
			t->GenerateName();
		}
		t->DeserialiseObject(di, ec);
	}
	else {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid train definition");
	}
}

template <typename C> void world_serialisation::MakeGenericTrackTypeWrapper() {
	auto func = [&](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) {
		this->DeserialiseGenericTrack<C>(di, ec, wdp);
	};
	content_object_types.RegisterType(C::GetTypeSerialisationNameStatic(), func);
	gamestate_object_types.RegisterType(C::GetTypeSerialisationNameStatic(), func);
}

void world_serialisation::InitObjectTypes() {
	MakeGenericTrackTypeWrapper<trackseg>();
	MakeGenericTrackTypeWrapper<points>();
	MakeGenericTrackTypeWrapper<autosignal>();
	MakeGenericTrackTypeWrapper<routesignal>();
	MakeGenericTrackTypeWrapper<repeatersignal>();
	MakeGenericTrackTypeWrapper<catchpoints>();
	MakeGenericTrackTypeWrapper<springpoints>();
	MakeGenericTrackTypeWrapper<crossover>();
	MakeGenericTrackTypeWrapper<doubleslip>();
	MakeGenericTrackTypeWrapper<startofline>();
	MakeGenericTrackTypeWrapper<endofline>();
	MakeGenericTrackTypeWrapper<routingmarker>();
	content_object_types.RegisterType("template", [&](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) { DeserialiseTemplate(di, ec); });
	content_object_types.RegisterType("typedef", [&](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) { DeserialiseTypeDefinition(di, ec); });
	content_object_types.RegisterType("tractiontype", [&](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) { DeserialiseTractionType(di, ec); });
	content_object_types.RegisterType("trackcircuit", [&](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) { DeserialiseTrackCircuit(di, ec); });
	content_object_types.RegisterType("tracktraincounterblock", [&](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) { DeserialiseTrackTrainBlock(di, ec); });
	content_object_types.RegisterType("couplepoints", [&](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) { DeserialisePointsCoupling(di, ec); });
	content_object_types.RegisterType("vehicleclass", [&](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) { DeserialiseVehicleClass(di, ec); });
	gamestate_object_types.RegisterType("train", [&](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) { DeserialiseTrain(di, ec); });

	gui_layout_generictrack = [](const generictrack *t, const deserialiser_input &di, error_collection &ec) { };
	gui_layout_trackberth = [](const trackberth *b, const generictrack *t, const deserialiser_input &di, error_collection &ec) { };
	gui_layout_guiobject = [](const deserialiser_input &di, error_collection &ec) { };
	content_object_types.RegisterType("layoutobj", [this](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) {
		this->gui_layout_guiobject(di, ec);
	});
}

void world_serialisation::DeserialiseObject(const ws_deserialisation_type_factory &wdtf, const ws_dtf_params &wdtf_params, const deserialiser_input &di, error_collection &ec) {
	if(!wdtf.FindAndDeserialise(di.type, di, ec, wdtf_params)) {
		ec.RegisterNewError<error_deserialisation>(di, string_format("LoadGame: Unknown object type: %s", di.type.c_str()));
	}
}

void world_serialisation::LoadGameFromStrings(const std::string &base, const std::string &save, error_collection &ec) {
	if(!base.empty()) {
		//load everything from base
		ParseInputString(base, ec);
	}

	if(!save.empty()) {
		//load gamestate from save, override any initial gamestate in base
		ParseInputString(base, ec, WSLOADGAME_FLAGS::NOCONTENT | WSLOADGAME_FLAGS::TRYREPLACEGAMESTATE);
	}

	if(ec.GetErrorCount()) return;

	w.LayoutInit(ec);
	if(ec.GetErrorCount()) return;

	w.PostLayoutInit(ec);
	if(ec.GetErrorCount()) return;

	DeserialiseGameState(ec);
}

void world_serialisation::LoadGameFromFiles(const std::string &basefile, const std::string &savefile, error_collection &ec) {
	std::string base, save;
	bool result = true;
	if(result && !basefile.empty()) result = slurp_file(basefile, base, ec);
	if(result && !savefile.empty()) result = slurp_file(savefile, save, ec);

	if(result) LoadGameFromStrings(base, save, ec);
}
