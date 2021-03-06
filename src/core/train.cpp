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
#include "util/util.h"
#include "core/param.h"
#include "core/track.h"
#include "core/train.h"
#include "core/traverse.h"
#include "core/world.h"

#include <cassert>

train::train(world &w_) : world_obj(w_) {

}

inline int CalcGravityForce(unsigned int total_mass, int elevationdecrease, unsigned int length) {
	//slope angle = arctan(elevationdecrease/length)
	//vertical force = g * mass
	//horizontal and vertical force add to vector along slope angle from vertical
	//horizontal force = tan(slope angle) * vertical force
	//horizontal force = g * mass * elevationdecrease / length
	return (int) ((((int64_t) (9810 * elevationdecrease)) * ((int64_t) total_mass)) / ((int64_t) length));
}

inline int CalcDrag(unsigned int drag_const, unsigned int drag_v, unsigned int drag_v_2, unsigned int v) {
	return drag_const + (((uint64_t) v) * ((uint64_t) drag_v)) / ((uint64_t) 1000)
			+ (((uint64_t) v) * ((uint64_t) v) * ((uint64_t) drag_v_2)) / ((uint64_t) 1000000);
}

// returns true if need for speed control
// Parameters:
// IN     brake_deceleration    m/(s^2) << 8            Maximum positive deceleration
// IN     target_speed          mm/s                    Target speed that we are aiming to decelerate to
// IN     current_max_speed     mm/s                    Current maximum speed that we could go at
// IN     brake_threshold_speed mm/s                    Speed below which the stopping distance is 0 (to prevent asymptotic braking)
// IN     target_distance       mm                      Distance to start of target speed
// OUT    brake_speed           mm/s                    The ideal speed to brake to (if return true)
// OUT    displacement_limit    mm                      Limit the displacement in this step (if return true)
bool CheckCalculateProjectedBrakingSpeed(unsigned int brake_deceleration, unsigned int target_speed, unsigned int current_max_speed,
		unsigned int brake_threshold_speed, unsigned int target_distance, unsigned int &brake_speed, unsigned int &displacement_limit) {

	// v^2 = u^2 + 2as
	// (2000 * (m/(s^2) << 8) * mm) >> 8 --> 2000 * m/(s^2) * mm --> 2 * mm/(s^2) * mm --> 2 * (mm/s)^2
	uint64_t vsq = ((((uint64_t) 2000) * ((uint64_t) brake_deceleration) * ((uint64_t) target_distance)) >> 8);
	if (target_speed) {
		vsq += (((uint64_t) target_speed) * ((uint64_t) target_speed));
	}

	displacement_limit = UINT_MAX;

	if (vsq >= ((uint64_t) current_max_speed) * ((uint64_t) current_max_speed)) {
		return false;    //no need for any braking at all, don't bother with the isqrt
	} else if (vsq <= ((uint64_t) brake_threshold_speed) * ((uint64_t) brake_threshold_speed)) {
		brake_speed = brake_threshold_speed;    // Below the brake threshold speed, assume stopping distance is zero
		if (target_speed == 0) {
			displacement_limit = target_distance;
			if (target_distance == 0) {
				brake_speed = 0;
			}
		}
		return true;
	} else {
		brake_speed = (unsigned int) fast_isqrt(vsq);
		return true;
	}
}

void train::TrainTimeStep(unsigned int ms) {
	if (!train_segments.empty()) {
		TrainMoveStep(ms);
	}
}

void train::TrainMoveStep(unsigned int ms) {
	int slopeforce = CalcGravityForce(total_mass, tail_relative_height - head_relative_height, total_length);
	int dragforce = CalcDrag(total_drag_const, total_drag_v, total_drag_v2, current_speed);

	int current_max_tractive_force = total_tractive_force;
	if (current_speed) {    //don't do this check if stationary, divide by zero issue...
		int speed_limited_tractive_force = 1000 * total_tractive_power / current_speed; // 1000 * W / (mm/s) -> N
		if (speed_limited_tractive_force < current_max_tractive_force) {
			current_max_tractive_force = speed_limited_tractive_force;
		}
	}

	int current_max_braking_force = total_braking_force;

	int max_total_force = current_max_tractive_force + slopeforce - dragforce;
	int min_total_force = -current_max_braking_force + slopeforce - dragforce;

	int max_acceleration = (max_total_force << 8) / ((int) total_mass);
	int min_acceleration = (min_total_force << 8) / ((int) total_mass);

	if (max_acceleration > ACCEL_BRAKE_CAP) max_acceleration = ACCEL_BRAKE_CAP;    //cap acceleration/braking
	if (min_acceleration < -ACCEL_BRAKE_CAP) min_acceleration = -ACCEL_BRAKE_CAP;

	// ((m/(s^2) << 8) * ms) >> 8 --> mm/(ms*s) * ms --> mm/s
	int max_new_speed = ((int) current_speed) + ((max_acceleration * ((int) ms)) >> 8);
	int min_new_speed = ((int) current_speed) + ((min_acceleration * ((int) ms)) >> 8);

	if (min_new_speed < 0) min_new_speed = 0;
	if (max_new_speed < 0) max_new_speed = 0;

	int target_speed = std::min((int) current_max_speed, max_new_speed);
	unsigned int displacement_limit = UINT_MAX;

	auto lookaheadfunc = [&](unsigned int distance, unsigned int speed) {
		unsigned int brake_speed;
		unsigned int local_displacement_limit;
		if (CheckCalculateProjectedBrakingSpeed(-min_acceleration, speed, target_speed, CREEP_SPEED, distance, brake_speed, local_displacement_limit)) {
			if ((int) brake_speed < target_speed) {
				target_speed = brake_speed;
			}
			if (displacement_limit > local_displacement_limit) {
				displacement_limit = local_displacement_limit;
			}
		}
	};

	bool waitingatredsig = false;

	auto lookaheaderrorfunc = [&](lookahead::LA_ERROR err, const track_target_ptr &piece) {
		switch (err) {
			case lookahead::LA_ERROR::NONE:
				break;

			case lookahead::LA_ERROR::SIG_TARGET_CHANGE:
				//TODO: fill this in
				break;

			case lookahead::LA_ERROR::SIG_ASPECT_LESS_THAN_EXPECTED:
				//TODO: fill this in
				break;

			case lookahead::LA_ERROR::WAITING_AT_RED_SIG:
				waitingatredsig = true;
				break;

			case lookahead::LA_ERROR::TRACTION_UNSUITABLE:
				//TODO: fill this in
				break;
		}
	};

	la.CheckLookaheads(this, head_pos, lookaheadfunc, lookaheaderrorfunc);

	if (waitingatredsig && current_speed == 0) {
		if (!(tflags & TF::WAITING_AT_RED_SIG)) {
			tflags |= TF::WAITING_AT_RED_SIG;
			red_sig_wait_start_time = GetWorld().GetGameTime();
		}

		//TODO: do something if we've been waiting for a while
	}
	else tflags &= ~TF::WAITING_AT_RED_SIG;

	unsigned int prev_speed = current_speed;
	current_speed = std::max(std::min(max_new_speed, target_speed), min_new_speed);

	unsigned int displacement = (ms * (current_speed + prev_speed)) / 2000;
	if (displacement > displacement_limit)
		displacement = displacement_limit;

	int head_elevation_delta, tail_elevation_delta;

	AdvanceDisplacement(displacement, head_pos, &head_elevation_delta, [this](track_location &old_track, track_location &new_track) {
		new_track.GetTrack()->TrainEnter(new_track.GetDirection(), this);
	});
	AdvanceDisplacement(displacement, tail_pos, &tail_elevation_delta, [this](track_location &old_track, track_location &new_track) {
		old_track.GetTrack()->TrainLeave(old_track.GetDirection(), this);
	});
	head_relative_height += head_elevation_delta;
	tail_relative_height += tail_elevation_delta;
	la.Advance(displacement);
}

void train::CalculateTrainMotionProperties(unsigned int weather_factor_shl8) {
	total_length = 0;
	total_drag_const = 0;
	total_drag_v = 0;
	total_drag_v2 = 0;
	total_mass = 0;
	total_tractive_force = 0;
	total_tractive_power = 0;
	total_braking_force = 0;
	veh_max_speed = 0;
	current_max_speed = 0;

	if (train_segments.begin() == train_segments.end()) return;    //train with no carriages

	for (auto it = train_segments.begin(); it != train_segments.end(); ++it) {
		total_length += it->veh_type->length * it->veh_multiplier;
		total_drag_const += it->veh_type->cumul_drag_const * it->veh_multiplier;
		total_drag_v += it->veh_type->cumul_drag_v * it->veh_multiplier;
		total_drag_v2 += it->veh_type->cumul_drag_v2 * it->veh_multiplier;
		total_mass += it->segment_total_mass;

		unsigned int unit_tractive_force = it->veh_type->tractive_force;
		unsigned int unit_braking_force = it->veh_type->braking_force;
		unsigned int unit_traction_limit = (it->veh_type->nominal_rail_traction_limit * weather_factor_shl8) >> 8;

		total_braking_force += std::min(unit_braking_force, unit_traction_limit) * it->veh_multiplier;
		if (active_tractions.IsIntersecting(it->veh_type->traction_types))	{
			total_tractive_force += std::min(unit_tractive_force, unit_traction_limit) * it->veh_multiplier;
			total_tractive_power += it->veh_type->tractive_power * it->veh_multiplier;
		}

		if (it == train_segments.begin() || veh_max_speed > it->veh_type->max_speed) {
			veh_max_speed = it->veh_type->max_speed;
		}

	}
	total_drag_v2 += train_segments.front().veh_type->face_drag_v2;
	total_drag_v2 += train_segments.back().veh_type->face_drag_v2;
}

void train::AddCoveredTrackSpeedLimit(unsigned int speed) {
	if (speed >= veh_max_speed) {
		return;
	}
	for (auto &it : covered_track_speed_limits) {
		if (it.speed == speed) {
			it.count++;
			return;
		}
	}
	covered_track_speed_limits.emplace_front(speed, 1);
	CalculateCoveredTrackSpeedLimit();
}
void train::RemoveCoveredTrackSpeedLimit(unsigned int speed) {
	if (speed >= veh_max_speed) {
		return;
	}
	auto prev_it = covered_track_speed_limits.before_begin();
	auto it = prev_it;
	++it;
	for (; it != covered_track_speed_limits.end(); prev_it = it, ++it) {
		if (it->speed == speed) {
			it->count--;
			if (it->count == 0) {
				covered_track_speed_limits.erase_after(prev_it);    //this invalidates iterator it
				CalculateCoveredTrackSpeedLimit();
			}
			return;
		}
	}
}

void train::CalculateCoveredTrackSpeedLimit() {
	current_max_speed = veh_max_speed;
	for (auto it = covered_track_speed_limits.begin(); it != covered_track_speed_limits.begin(); ++it) {
		if (it->speed < current_max_speed) {
			current_max_speed = it->speed;
		}
	}
}

void train::ReverseDirection() {
	tflags ^= TF::CONSIST_REV_DIR;

	track_location new_head = tail_pos;
	tail_pos = head_pos;
	head_pos = new_head;

	tail_pos.ReverseDirection();
	head_pos.ReverseDirection();

	unsigned int new_head_height = tail_relative_height;
	tail_relative_height = head_relative_height;
	head_relative_height = new_head_height;

	la.Init(this, head_pos);
}

void train::RefreshCoveredTrackSpeedLimits() {
	covered_track_speed_limits.clear();
	CalculateCoveredTrackSpeedLimit();

	track_location local_backtrack_pos = head_pos;
	local_backtrack_pos.ReverseDirection();

	auto func = [this](track_location &old_track, track_location &new_track) {
		const speed_restriction_set *speeds = new_track.GetTrack()->GetSpeedRestrictions();
		if (speeds) {
			this->AddCoveredTrackSpeedLimit(speeds->GetTrainTrackSpeedLimit(this));
		}
	};

	track_location temp;
	func(temp, local_backtrack_pos);    //include the first track piece
	AdvanceDisplacement(total_length, local_backtrack_pos, 0, func);
	local_backtrack_pos.ReverseDirection();
	assert(local_backtrack_pos == tail_pos);
}

class error_droptrainintoposition : public error_obj {
	public:
	error_droptrainintoposition(const train *t, const track_location &position, const std::string &str = "") {
		msg << "Failed to drop train: " << t->GetName() << " into position: (" << position << ") failed: " << str;
	}
};

void train::DropTrainIntoPosition(const track_location &position, error_collection &ec) {
	tail_relative_height = head_relative_height = 0;

	head_pos = position;
	tail_pos = position;

	tail_pos.ReverseDirection();

	//enter old_track instead of new_track, to avoid interfering with collision checks on new_track which happen immediately after this
	auto func = [this](track_location &old_track, track_location &new_track) {
		old_track.GetTrack()->TrainEnter(old_track.GetTrack()->GetReverseDirection(old_track.GetDirection()), this);
	};

	flagwrapper<ADRESULTF> ad_result_flags;
	unsigned int leftover = AdvanceDisplacement(total_length, tail_pos, &tail_relative_height, func, ADF::CHECK_FOR_TRAINS, &ad_result_flags);

	track_location temp;
	func(tail_pos, temp);    //include the last track piece

	tail_pos.ReverseDirection();
	if (leftover) {
		ec.RegisterNewError<error_droptrainintoposition>(this, position,
				string_format("Insufficient track to drop train: tail at: (%s), leftover: %umm",
				stringify(tail_pos).c_str(), leftover));
		return;
	}

	RefreshCoveredTrackSpeedLimits();

	la.Init(this, head_pos);
}

void train::UprootTrain(error_collection &ec) {

	auto func = [this](track_location &old_track, track_location &new_track) {
		new_track.GetTrack()->TrainLeave(new_track.GetTrack()->GetReverseDirection(new_track.GetDirection()), this);
	};

	track_location temp;
	func(temp, tail_pos);    //include the first track piece
	AdvanceDisplacement(total_length, tail_pos, 0, func);

	tail_relative_height = head_relative_height = 0;
	tail_pos = head_pos = track_location();
	la.Clear();
}

traction_set train::GetAllTractionTypes() const {
	traction_set ts_union;
	for (auto &it : train_segments) {
		ts_union.UnionWith(it.veh_type->traction_types);
	}
	return ts_union;
}

void train::ValidateActiveTractionSet() {
	active_tractions.IntersectWith(GetAllTractionTypes());
}

void train::GenerateName() {
	size_t id = reinterpret_cast<size_t>(this);
	std::string name;
	do {
		name = "train_" + std::to_string(GetWorld().GetLoadCount()) + "_" + std::to_string(GetWorld().GetGameTime()) + "_" + std::to_string(id);
		id++;
	} while (GetWorld().FindTrainByName(name));
	SetName(name);
}

void train_unit::CalculateSegmentMass() {
	if (veh_type) {
		unsigned int unit_mass = (stflags & STF::FULL) ? veh_type->full_mass : veh_type->empty_mass;
		segment_total_mass = unit_mass * veh_multiplier;
	} else {
		segment_total_mass = 0;
	}
}
