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

#ifndef INC_TRAIN_ALREADY
#define INC_TRAIN_ALREADY

#include "core/world_obj.h"
#include "core/track.h"
#include "core/timetable.h"
#include "core/lookahead.h"
#include "core/serialisable.h"
#include "core/tractiontype.h"
#include <list>

struct vehicle_class : public serialisable_obj {
	const std::string name;
	unsigned int length = 0;                        // mm
	unsigned int max_speed = 0;                     // mm/s (μm/ms)
	unsigned int tractive_force = 0;                // kg μm/(ms^2) --> N
	unsigned int tractive_power = 0;                // 1000 * N mm/s --> W
	unsigned int braking_force = 0;                 // N
	unsigned int nominal_rail_traction_limit = 0;   // N
	unsigned int cumul_drag_const = 0;              // N
	unsigned int cumul_drag_v = 0;                  // N/(m/s)
	unsigned int cumul_drag_v2 = 0;                 // N/(m/s)^2
	unsigned int face_drag_v2 = 0;                  // N/(m/s)^2
	unsigned int fullmass = 0;                      // kg
	unsigned int emptymass = 0;                     // kg
	tractionset tractiontypes;

	vehicle_class(const std::string &name_) : name(name_) { }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

/* sample values:
 * v -> 50 m/s --> 50000 mm/s
 * A -> 8000 N
 * B -> 100 N/(m/s) --> 100 mN/(mm/s)
 * C -> 10 N/(m/s)^2 ->- 0.00001 N/(mm/s)^2 --> 10 μN/(mm/s)^2
 */

struct train_unit {
	vehicle_class *vehtype = 0;
	unsigned int veh_multiplier = 0;
	unsigned int segment_total_mass = 0;
	enum class STF {
		ZERO        = 0,
		REV         = 1<<0,
		FULL        = 1<<1,
	};
	STF stflags = STF::ZERO;

	void CalculateSegmentMass();
};
template<> struct enum_traits< train_unit::STF > { static constexpr bool flags = true; };

struct train_track_speed_limit_item {
	unsigned int speed;
	unsigned int count;
	train_track_speed_limit_item(unsigned int speed_, unsigned int count_) : speed(speed_), count(count_) { }
};

struct train_dynamics {
	unsigned int total_length = 0;
	unsigned int veh_max_speed = 0;
	unsigned int total_tractive_force = 0;
	unsigned int total_tractive_power = 0;
	unsigned int total_braking_force = 0;
	unsigned int total_drag_const = 0;
	unsigned int total_drag_v = 0;
	unsigned int total_drag_v2 = 0;
	unsigned int total_mass = 0;
};

struct train_motion_state {
	unsigned int current_speed = 0;
	unsigned int current_max_speed = 0;
	track_location head_pos;
	track_location tail_pos;
	int head_relative_height = 0;
	int tail_relative_height = 0;
};

class train : public world_obj, protected train_dynamics, protected train_motion_state {
	enum class TF {
		ZERO            = 0,
		CONSISTREVDIR   = 1<<0,
		WAITINGREDSIG   = 1<<1,
	};
	TF tflags = TF::ZERO;

	std::list<train_unit> train_segments;
	tractionset active_tractions;
	std::string vehspeedclass;
	std::forward_list<train_track_speed_limit_item> covered_track_speed_limits;

	lookahead la;
	world_time redsigwaitstarttime = 0;

	std::string headcode;
	timetable currenttimetable;

	void TrainMoveStep(unsigned int ms);

	public:
	train(world &w_);
	void TrainTimeStep(unsigned int ms);
	void CalculateTrainMotionProperties(unsigned int weatherfactor_shl8);
	void AddCoveredTrackSpeedLimit(unsigned int speed);
	void RemoveCoveredTrackSpeedLimit(unsigned int speed);
	void CalculateCoveredTrackSpeedLimit();
	void ReverseDirection();
	void RefreshCoveredTrackSpeedLimits();
	void DropTrainIntoPosition(const track_location &position, error_collection &ec);
	void UprootTrain(error_collection &ec);
	inline const train_dynamics &GetTrainDynamics() const { return *this; }
	inline const train_motion_state &GetTrainMotionState() const { return *this; }
	inline unsigned int GetMaxVehSpeed() const { return veh_max_speed; }
	inline std::string GetVehSpeedClass() const { return vehspeedclass; }
	const tractionset &GetActiveTractionTypes() const { return active_tractions; }
	tractionset GetAllTractionTypes() const;
	void SetActiveTractionSet(tractionset ts) { active_tractions = std::move(ts); }
	void ValidateActiveTractionSet();

	virtual std::string GetTypeName() const { return "Train"; }
	static std::string GetTypeSerialisationClassNameStatic() { return "train"; }
	virtual std::string GetTypeSerialisationClassName() const override { return GetTypeSerialisationClassNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	void GenerateName();
};
template<> struct enum_traits< train::TF > { static constexpr bool flags = true; };

#endif
