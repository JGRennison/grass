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

#ifndef INC_WORLD_ALREADY
#define INC_WORLD_ALREADY

#include <unordered_map>
#include <deque>
#include <forward_list>
#include <set>
#include "util/flags.h"
#include "core/tractiontype.h"
#include "core/serialisable.h"
#include "core/future.h"
#include "core/edgetype.h"

class world_deserialisation;
class world_serialisation;
class action;
class generictrack;
class textpool;
class track_circuit;
class track_train_counter_block;
class vehicle_class;
class world;
class updatable_obj;

struct connection_forward_declaration {
	generictrack *track1;
	EDGETYPE dir1;
	std::string name2;
	EDGETYPE dir2;
	connection_forward_declaration(generictrack *t1, EDGETYPE d1, const std::string &n2, EDGETYPE d2) : track1(t1), dir1(d1), name2(n2), dir2(d2) { }
};

enum class GAMEMODE {
	SINGLE,
	SERVER,
	CLIENT,
};

typedef enum {
	LOG_NULL,
	LOG_DENIED,
	LOG_MESSAGE,
	LOG_FAILED,
} LOGCATEGORY;

class fixup_list {
	std::deque<std::function<void(error_collection &ec)> > fixups;

	public:
	void AddFixup(std::function<void(error_collection &ec)> fixup) {
		fixups.push_back(fixup);
	}
	void Execute(error_collection &ec) {
		for(auto it = fixups.begin(); it != fixups.end(); ++it) {
			(*it)(ec);
		}
		fixups.clear();
	}
	void Clear() {
		fixups.clear();
	}
	size_t Count() const {
		return fixups.size();
	}
};

template <typename C>
class track_train_counter_block_container {
	std::unordered_map<std::string, std::unique_ptr<C> > all_items;
	world &w;

	public:
	track_train_counter_block_container(world &w_) : w(w_) { }
	C *FindByName(const std::string &name) {
		auto it = all_items.find(name);
		if(it != all_items.end()) {
			if(it->second)
				return it->second.get();
		}
		return nullptr;
	}
	C *FindOrMakeByName(const std::string &name) {
		std::unique_ptr<C> &tc = all_items[name];
		if(!tc.get())
			tc.reset(new C(w, name));
		return tc.get();
	}
	template <typename F> void Enumerate(F func) const {
		for(auto &it : all_items) {
			func(*(it.second));
		}
	}
	template <typename F> void Enumerate(F func) {
		const_cast<const track_train_counter_block_container<C>*>(this)->EnumerateTrains([&](const C& t) { f(const_cast<C&>(t)); });
	}
};

class world : public serialisable_futurable_obj {
	friend world_deserialisation;
	friend world_serialisation;
	std::unordered_map<std::string, std::unique_ptr<generictrack> > all_pieces;
	std::unordered_map<std::string, std::unique_ptr<vehicle_class> > all_vehicle_classes;
	std::forward_list<train> all_trains;
	std::deque<connection_forward_declaration> connection_forward_declarations;
	std::unordered_map<std::string, traction_type> traction_types;
	std::deque<generictrack *> tick_update_list;
	world_time gametime = 0;
	GAMEMODE mode = GAMEMODE::SINGLE;
	error_collection ec;
	unsigned int auto_seq_item = 0;
	uint64_t load_count = 0;    // incremented on each save/load cycle
	uint64_t last_future_id = 0;

	public:
	enum class WFLAGS {
		DONE_LAYOUTINIT      = 1<<0,
		DONE_POSTLAYOUTINIT  = 1<<1,
	};

	private:
	flagwrapper<WFLAGS> wflags;

	public:
	future_deserialisation_type_factory future_types;
	future_set futures;
	fixup_list layout_init_final_fixups;
	fixup_list post_layout_init_final_fixups;
	track_train_counter_block_container<track_circuit> track_circuits;
	track_train_counter_block_container<track_train_counter_block> track_triggers;
	std::set<updatable_obj *> update_set;

	world();
	virtual ~world();
	void AddTrack(std::unique_ptr<generictrack> &&piece, error_collection &ec);
	void AddTractionType(std::string name, bool alwaysavailable);
	traction_type *GetTractionTypeByName(std::string name) const;
	void ConnectTrack(generictrack *track1, EDGETYPE dir1, std::string name2, EDGETYPE dir2, error_collection &ec);
	void LayoutInit(error_collection &ec);
	void PostLayoutInit(error_collection &ec);
	generictrack *FindTrackByName(const std::string &name) const;
	template <typename C> C *FindTrackByNameCast(const std::string &name) const {
		return dynamic_cast<C*>(FindTrackByName(name));
	}
	track_train_counter_block *FindTrackTrainBlockOrTrackCircuitByName(const std::string &name);
	void InitFutureTypes();
	world_time GetGameTime() const { return gametime; }
	std::string FormatGameTime(world_time wt) const;
	void SubmitAction(const action &request);
	void ExecuteIfActionScope(std::function<void()> func);
	named_futurable_obj *FindFuturableByName(const std::string &name);
	virtual std::string GetTypeSerialisationClassName() const override { return "world"; }
	virtual std::string GetSerialisationName() const override { return ""; }
	virtual textpool &GetUserMessageTextpool();
	virtual void LogUserMessageLocal(LOGCATEGORY lc, const std::string &message);
	virtual void GameStep(world_time delta);
	void RegisterTickUpdate(generictrack *targ);
	void UnregisterTickUpdate(generictrack *targ);
	inline bool IsAuthoritative() const {
		return mode == GAMEMODE::SINGLE || mode == GAMEMODE::SERVER;
	}
	error_collection &GetEC() { return ec; }
	inline uint64_t GetLoadCount() const { return load_count; }
	void CapAllTrackPieceUnconnectedEdges();
	train *CreateEmptyTrain();
	train *FindTrainByName(const std::string &name) const;
	void DeleteTrain(train *t);
	unsigned int EnumerateTrains(std::function<void(const train &)> f) const;
	inline unsigned int EnumerateTrains(std::function<void(train &)> f) {
		return const_cast<const world*>(this)->EnumerateTrains([&](const train &t) { f(const_cast<train&>(t)); });
	}
	vehicle_class *FindOrMakeVehicleClassByName(const std::string &name);
	vehicle_class *FindVehicleClassByName(const std::string &name);
	void MarkUpdated(updatable_obj *wo);
	const std::set<updatable_obj *> &GetLastUpdateSet() const { return update_set; }

	flagwrapper<WFLAGS> GetWFlags() const { return wflags; }

	uint64_t MakeNewFutureID() { return ++last_future_id; }

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};
template<> struct enum_traits< world::WFLAGS > { static constexpr bool flags = true; };

#endif
