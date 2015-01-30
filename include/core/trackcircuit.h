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

#ifndef INC_TRACKCIRCUIT_ALREADY
#define INC_TRACKCIRCUIT_ALREADY

#include <string>
#include <vector>
#include "util/flags.h"
#include "core/world_obj.h"
#include "core/world.h"

class train;
class route;

struct train_ref {
	train *t;
	unsigned int count;
	train_ref(train *t_, unsigned int count_) : t(t_), count(count_) { }
};

class track_train_counter_block : public world_obj {
	unsigned int traincount = 0;
	std::vector<train_ref> occupying_trains;
	std::vector<generictrack *> owned_pieces;
	world_time last_change;

	public:
	enum class TCF {
		ZERO              = 0,
		FORCEOCCUPIED     = 1<<0,
	};

	private:
	TCF tc_flags = TCF::ZERO;

	public:

	track_train_counter_block(world &w_, const std::string &name_) : world_obj(w_), last_change(w_.GetGameTime()) { SetName(name_); }
	track_train_counter_block(world &w_) : world_obj(w_), last_change(w_.GetGameTime()) { }
	void TrainEnter(train *t);
	void TrainLeave(train *t);
	inline bool Occupied() const;
	TCF GetTCFlags() const;
	TCF SetTCFlagsMasked(TCF bits, TCF mask);
	unsigned int GetTrainOccupationCount() const { return occupying_trains.size(); }
	world_time GetLastOccupationStateChangeTime() const { return last_change; }
	void RegisterTrack(generictrack *piece) { owned_pieces.push_back(piece); }
	const std::vector<generictrack *> &GetOwnedTrackSet() const { return owned_pieces; }
	void GetSetRoutes(std::vector<const route *> &routes);

	virtual std::string GetTypeName() const override { return "Track Train Counter Block"; }
	static std::string GetTypeSerialisationClassNameStatic() { return "tracktraincounterblock"; }
	virtual std::string GetTypeSerialisationClassName() const override { return GetTypeSerialisationClassNameStatic(); }

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	protected:
	virtual void OccupationStateChangeTrigger() { };
	virtual void OccupationTrigger() { };
	virtual void DeOccupationTrigger() { };
};
template<> struct enum_traits< track_train_counter_block::TCF > { static constexpr bool flags = true; };

inline bool track_train_counter_block::Occupied() const { return traincount > 0 || tc_flags & TCF::FORCEOCCUPIED; }

class track_circuit : public track_train_counter_block {
	public:
	track_circuit(world &w_, const std::string &name_) : track_train_counter_block(w_, name_) { }
	track_circuit(world &w_) : track_train_counter_block(w_) { }

	virtual std::string GetTypeName() const override { return "Track Circuit"; }
	static std::string GetTypeSerialisationClassNameStatic() { return "trackcircuit"; }
	virtual std::string GetTypeSerialisationClassName() const override { return GetTypeSerialisationClassNameStatic(); }

	protected:
	virtual void OccupationStateChangeTrigger() override;
	virtual void OccupationTrigger() override;
	virtual void DeOccupationTrigger() override;
};

#endif
