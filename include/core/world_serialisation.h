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
//  2012 - j.g.rennison@gmail.com
//==========================================================================

#ifndef INC_WORLD_SERIALISATION_ALREADY
#define INC_WORLD_SERIALISATION_ALREADY

#include <map>
#include <string>
#include <deque>

#include "util/flags.h"
#include "core/world.h"
#include "core/serialisable.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wtype-limits"
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include "rapidjson/document.h"
#pragma GCC diagnostic pop

class generic_track;
class track_berth;

struct template_def {
	const rapidjson::Value *json = nullptr;
	bool being_expanded = false;
};

class world_deserialisation {
	world &w;
	std::forward_list<rapidjson::Document> parsed_inputs;
	generic_track *previous_track_piece;
	unsigned int current_content_index;

	public:
	struct ws_dtf_params {
		enum class WSDTFP_FLAGS {
			ZERO            = 0,
			NO_NEW_TRACK    = 1<<0,
		};
		WSDTFP_FLAGS flags;
		ws_dtf_params(WSDTFP_FLAGS flags_ = WSDTFP_FLAGS::ZERO) : flags(flags_) { }
	};
	typedef deserialisation_type_factory<const ws_dtf_params &> ws_deserialisation_type_factory;
	ws_deserialisation_type_factory content_object_types;
	ws_deserialisation_type_factory game_state_object_types;

	fixup_list game_state_init;

	std::function<void(const generic_track *, const deserialiser_input &, error_collection &)> gui_layout_generic_track;
	std::function<void(const track_berth *, const generic_track *, const deserialiser_input &, error_collection &)> gui_layout_track_berth;
	std::function<void(const deserialiser_input &, error_collection &)> gui_layout_gui_object;
	std::function<void(const deserialiser_input &, error_collection &)> gui_layout_text_object;

	enum class WS_LOAD_GAME_FLAGS {
		ZERO                    = 0,
		NO_CONTENT              = 1<<0,
		NO_GAME_STATE           = 1<<1,
		TRY_REPLACE_GAME_STATE  = 1<<2,
	};

	void InitObjectTypes();

	world_deserialisation(world &w_)
			: w(w_), previous_track_piece(nullptr) {
		InitObjectTypes();
	}

	void ParseInputString(const std::string &input, error_collection &ec, WS_LOAD_GAME_FLAGS flags = WS_LOAD_GAME_FLAGS::ZERO);
	void LoadGame(const deserialiser_input &di, error_collection &ec, WS_LOAD_GAME_FLAGS flags = WS_LOAD_GAME_FLAGS::ZERO);
	void DeserialiseRootObjArray(const ws_deserialisation_type_factory &wdtf, const ws_dtf_params &wdtf_params, const deserialiser_input &contentdi, error_collection &ec);
	void DeserialiseObject(const ws_deserialisation_type_factory &wdtf, const ws_dtf_params &wdtf_params, const deserialiser_input &di, error_collection &ec);
	void DeserialiseTypeDefinition(const deserialiser_input &di, error_collection &ec);
	void DeserialiseTractionType(const deserialiser_input &di, error_collection &ec);
	template <typename T> T* MakeOrFindGenericTrack(const deserialiser_input &di, error_collection &ec, bool findonly);
	template <typename T> T* DeserialiseGenericTrack(const deserialiser_input &di, error_collection &ec, const ws_dtf_params &wdp);
	void DeserialiseVehicleClass(const deserialiser_input &di, error_collection &ec);
	void DeserialiseTrain(const deserialiser_input &di, error_collection &ec);
	template <typename C> void MakeGenericTrackTypeWrapper();
	inline unsigned int GetCurrentContentIndex() const { return current_content_index; }
	void DeserialiseGameState(error_collection &ec);

	void LoadGameFromStrings(const std::string &base, const std::string &save, error_collection &ec);
	void LoadGameFromFiles(const std::string &basefile, const std::string &savefile, error_collection &ec);
};
template<> struct enum_traits< world_deserialisation::ws_dtf_params::WSDTFP_FLAGS > { static constexpr bool flags = true; };
template<> struct enum_traits< world_deserialisation::WS_LOAD_GAME_FLAGS > { static constexpr bool flags = true; };


class world_serialisation {
	const world &w;

	public:
	world_serialisation(const world &w_) : w(w_) { }

	enum class WS_SAVE_GAME_FLAGS {
		PRETTY_MODE           = 1<<0,
	};
	std::string SaveGameToString(error_collection &ec, flagwrapper<WS_SAVE_GAME_FLAGS> ws_flags = 0);
};
template<> struct enum_traits< world_serialisation::WS_SAVE_GAME_FLAGS > { static constexpr bool flags = true; };

#endif
