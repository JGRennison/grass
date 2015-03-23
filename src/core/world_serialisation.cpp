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
#include "core/world.h"
#include "core/serialisable_impl.h"
#include "core/world_serialisation.h"
#include "core/track.h"
#include "core/trackpiece.h"
#include "core/points.h"
#include "core/signal.h"
#include "core/trackcircuit.h"
#include "core/train.h"
#include "core/pretty_serialiser.h"
#include <typeinfo>

void world_deserialisation::ParseInputString(const std::string &input, error_collection &ec, world_deserialisation::WSLOADGAME_FLAGS flags) {
	parsed_inputs.emplace_front();
	rapidjson::Document &dc = parsed_inputs.front();
	if (dc.Parse<0>(input.c_str()).HasParseError()) {
		ec.RegisterNewError<error_jsonparse>(input, dc.GetErrorOffset(), dc.GetParseError());
	}
	else {
		LoadGame(deserialiser_input(dc, "", "[root]", &w, this, nullptr), ec, flags);
	}
}

void world_deserialisation::LoadGame(const deserialiser_input &di, error_collection &ec, world_deserialisation::WSLOADGAME_FLAGS flags) {
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

void world_deserialisation::DeserialiseGameState(error_collection &ec) {
	gamestate_init.Execute(ec);
}

void world_deserialisation::DeserialiseRootObjArray(const ws_deserialisation_type_factory &wdtf, const ws_dtf_params &wdtf_params,
		const deserialiser_input &contentdi, error_collection &ec) {
	if(contentdi.json.IsArray()) {
		for(rapidjson::SizeType i = 0; i < contentdi.json.Size(); i++) {
			deserialiser_input subdi(contentdi.json[i], "", MkArrayRefName(i), &w, this, &contentdi);
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

template <typename T> T* world_deserialisation::MakeOrFindGenericTrack(const deserialiser_input &di, error_collection &ec, bool findonly) {
	std::string trackname;
	bool isautoname = false;
	if(!CheckTransJsonValue(trackname, di, "name", ec)) {
		trackname = string_format("#%d", current_content_index);
		isautoname = true;
	}

	std::unique_ptr<generictrack> &ptr = w.all_pieces[trackname];
	if(ptr) {
		if(typeid(*ptr) != typeid(T)) {
			ec.RegisterNewError<error_deserialisation>(di,
					string_format("LoadGame: Track type definition conflict: %s is not a %s", ptr->GetFriendlyName().c_str(), di.type.c_str()));
			return nullptr;
		}
	}
	else if(!findonly) {
		ptr.reset(new T(w));
		ptr->SetName(trackname);
		ptr->SetAutoName(isautoname);
		ptr->SetPreviousTrackPiece(previoustrackpiece);
		if(previoustrackpiece)
			previoustrackpiece->SetNextTrackPiece(ptr.get());
		previoustrackpiece = ptr.get();
	}
	else {
		ec.RegisterNewError<error_deserialisation>(di, string_format("LoadGame: Cannot make a new track piece at this point: %s", trackname.c_str()));
		return nullptr;
	}
	return static_cast<T*>(ptr.get());
}

template <typename T> T* world_deserialisation::DeserialiseGenericTrack(const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) {
	T* target = MakeOrFindGenericTrack<T>(di, ec, wdp.flags & ws_dtf_params::WSDTFP_FLAGS::NONEWTRACK);
	if(target) {
		target->DeserialiseObject(di, ec);
	}
	return target;
}

void world_deserialisation::DeserialiseTypeDefinition(const deserialiser_input &di, error_collection &ec) {
	std::string newtype;
	std::string basetype;
	if(CheckTransJsonValue(newtype, di, "newtype", ec) && CheckTransJsonValue(basetype, di, "basetype", ec)) {
		const rapidjson::Value *content;
		CheckTransRapidjsonValueDef<json_object>(content, di, "content", ec);

		auto func = [=](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) {
			//This is effectively a re-named clone of di
			deserialiser_input typedefwrapperdi(di.json, di.type, "Typedef Wrapper: " + newtype + ", base: " + basetype, di);

			const deserialiser_input *checkparent = di.parent;
			do {
				if(checkparent->reference_name == typedefwrapperdi.reference_name) {
					ec.RegisterNewError<error_deserialisation>(di,
							string_format("Typedef: \"%s\": Base \"%s\": Recursive expansion detected: Aborting.", newtype.c_str(), basetype.c_str()));
					return;
				}
			} while((checkparent = checkparent->parent));

			typedefwrapperdi.seenprops.swap(di.seenprops); //this is so that seen properties in di are propagated to the base handler
			typedefwrapperdi.objpreparse = di.objpreparse;
			typedefwrapperdi.objpostparse = di.objpostparse;

			auto exec = [&](const deserialiser_input &typedef_di) {
				if(!this->content_object_types.FindAndDeserialise(basetype, typedef_di, ec, wdp))  {
					ec.RegisterNewError<error_deserialisation>(typedef_di,
							string_format("Typedef expansion: %s: Unknown base type: %s", newtype.c_str(), basetype.c_str()));
				}
			};

			if(content) {
				deserialiser_input typedefcontentdi(*content, di.type, "Typedef Content: " + newtype + " base: " + basetype, typedefwrapperdi);
				/* 1st typedefcontentdi: typedefwrapperdi.objpreparse
				 * 2nd di preparse:  typedefwrapperdi.objpreparse.objpostparse
				 * 3rd di: typedefwrapperdi
				 */
				typedefcontentdi.objpostparse = typedefwrapperdi.objpreparse;
				typedefwrapperdi.objpreparse = &typedefcontentdi;

				// Need to use typedefwrapperdi instead of typedefcontentdi as the base, as typedefwrapperdi has the name which is needed first.
				exec(typedefwrapperdi);
			}
			else {
				exec(typedefwrapperdi);
			}

			typedefwrapperdi.seenprops.swap(di.seenprops); //this is so that seen properties in the base handler are propagated to di
		};
		content_object_types.RegisterType(newtype, func);
		di.PostDeserialisePropCheck(ec);
	}
	else {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid typedef definition");
	}
}

void world_deserialisation::DeserialiseTractionType(const deserialiser_input &di, error_collection &ec) {
	std::string name;
	if(CheckTransJsonValue(name, di, "name", ec) && di.w) {
		di.w->AddTractionType(name, CheckGetJsonValueDef<bool, bool>(di, "alwaysavailable", false, ec));
		di.PostDeserialisePropCheck(ec);
	}
	else {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid traction type definition");
	}
}

template<typename T> void DeserialiseTrackTrainCounterBlockCommon(const deserialiser_input &di, error_collection &ec, const world_deserialisation::ws_dtf_params &wdp, track_train_counter_block_container<T> &ttcb_container) {
	std::string name;
	if(CheckTransJsonValue(name, di, "name", ec) && di.w) {
		T *ttcb = nullptr;
		if(wdp.flags & world_deserialisation::ws_dtf_params::WSDTFP_FLAGS::NONEWTRACK) {
			ttcb = ttcb_container.FindByName(name);
			if(!ttcb) {
				ec.RegisterNewError<error_deserialisation>(di, "No such track circuit/track train counter block: " + name);
				return;
			}
		}
		else {
			ttcb = ttcb_container.FindOrMakeByName(name);
		}
		ttcb->DeserialiseObject(di, ec);
	}
	else {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid track circuit/track train counter definition");
	}
}

void world_deserialisation::DeserialiseVehicleClass(const deserialiser_input &di, error_collection &ec) {
	std::string name;
	if(CheckTransJsonValue(name, di, "name", ec) && di.w) {
		vehicle_class *vc = di.w->FindOrMakeVehicleClassByName(name);
		vc->DeserialiseObject(di, ec);
	}
	else {
		ec.RegisterNewError<error_deserialisation>(di, "Invalid vehicle class definition");
	}
}

void world_deserialisation::DeserialiseTrain(const deserialiser_input &di, error_collection &ec) {
	if(di.w) {
		std::string name;
		train *t = nullptr;
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

template <typename... Args> void wd_RegisterBothTypes(world_deserialisation &wd, Args... args) {
	wd.content_object_types.RegisterType(args...);
	wd.gamestate_object_types.RegisterType(args...);
}

template <typename C> void world_deserialisation::MakeGenericTrackTypeWrapper() {
	auto func = [&](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) {
		this->DeserialiseGenericTrack<C>(di, ec, wdp);
	};
	wd_RegisterBothTypes(*this, C::GetTypeSerialisationNameStatic(), func);
}

void world_deserialisation::InitObjectTypes() {
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
	content_object_types.RegisterType("typedef", [&](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) {
		DeserialiseTypeDefinition(di, ec);
	});
	content_object_types.RegisterType("tractiontype", [&](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) {
		DeserialiseTractionType(di, ec);
	});
	wd_RegisterBothTypes(*this, "trackcircuit", [&](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) {
		DeserialiseTrackTrainCounterBlockCommon<track_circuit>(di, ec, wdp, w.track_circuits);
	});
	wd_RegisterBothTypes(*this, "tracktraincounterblock", [&](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) {
		DeserialiseTrackTrainCounterBlockCommon<track_train_counter_block>(di, ec, wdp, w.track_triggers);
	});
	content_object_types.RegisterType("couplepoints", [&](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) {
		DeserialisePointsCoupling(di, ec);
	});
	content_object_types.RegisterType("vehicleclass", [&](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) {
		DeserialiseVehicleClass(di, ec);
	});
	gamestate_object_types.RegisterType("train", [&](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) {
		DeserialiseTrain(di, ec);
	});
	wd_RegisterBothTypes(*this, "world", [&](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) {
		w.DeserialiseObject(di, ec);
	});

	gui_layout_generictrack = [](const generictrack *t, const deserialiser_input &di, error_collection &ec) { };
	gui_layout_trackberth = [](const trackberth *b, const generictrack *t, const deserialiser_input &di, error_collection &ec) { };
	gui_layout_guiobject = [](const deserialiser_input &di, error_collection &ec) { };
	content_object_types.RegisterType("layoutobj", [this](const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp) {
		this->gui_layout_guiobject(di, ec);
	});
}

void world_deserialisation::DeserialiseObject(const ws_deserialisation_type_factory &wdtf, const ws_dtf_params &wdtf_params,
		const deserialiser_input &di, error_collection &ec) {
	if(!wdtf.FindAndDeserialise(di.type, di, ec, wdtf_params)) {
		ec.RegisterNewError<error_deserialisation>(di, string_format("LoadGame: Unknown object type: %s", di.type.c_str()));
	}
}

void world_deserialisation::LoadGameFromStrings(const std::string &base, const std::string &save, error_collection &ec) {
	if(!base.empty()) {
		//load everything from base
		ParseInputString(base, ec);
	}

	if(!save.empty()) {
		//load gamestate from save, override any initial gamestate in base
		ParseInputString(base, ec, WSLOADGAME_FLAGS::NOCONTENT | WSLOADGAME_FLAGS::TRYREPLACEGAMESTATE);
	}

	if(ec.GetErrorCount())
		return;

	w.LayoutInit(ec);
	if(ec.GetErrorCount())
		return;

	w.PostLayoutInit(ec);
	if(ec.GetErrorCount())
		return;

	DeserialiseGameState(ec);
}

void world_deserialisation::LoadGameFromFiles(const std::string &basefile, const std::string &savefile, error_collection &ec) {
	std::string base, save;
	bool result = true;
	if(result && !basefile.empty())
		result = slurp_file(basefile, base, ec);
	if(result && !savefile.empty())
		result = slurp_file(savefile, save, ec);

	if(result)
		LoadGameFromStrings(base, save, ec);
}

std::string world_serialisation::SaveGameToString(error_collection &ec, flagwrapper<world_serialisation::WSSAVEGAME_FLAGS> ws_flags) {
	auto exec = [&](Handler &hndl) {
		serialiser_output so(hndl);
		so.flags |= SOUTPUT_FLAGS::OUTPUT_ALLNAMES;

		hndl.StartObject();
		hndl.String("gamestate");
		hndl.StartArray();

		hndl.StartObject();
		w.Serialise(so, ec);
		hndl.EndObject();
		for(auto &it : w.all_pieces) {
			generictrack &gt = *(it.second);
			hndl.StartObject();
			gt.Serialise(so, ec);
			hndl.EndObject();
		}
		w.track_circuits.Enumerate([&](track_circuit &tc) {
			hndl.StartObject();
			tc.Serialise(so, ec);
			hndl.EndObject();
		});
		w.track_triggers.Enumerate([&](track_train_counter_block &ttcb) {
			hndl.StartObject();
			ttcb.Serialise(so, ec);
			hndl.EndObject();
		});

		hndl.EndArray();
		hndl.EndObject();
	};

	std::string output;
	writestream wr(output);
	if(ws_flags & WSSAVEGAME_FLAGS::PRETTYMODE) {
		PrettyWriterHandler hndl(wr);
		exec(hndl);
	}
	else {
		WriterHandler hndl(wr);
		exec(hndl);
	}
	return std::move(output);
}
