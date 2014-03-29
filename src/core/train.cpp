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
#include "core/util.h"
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
	return drag_const + (((uint64_t) v) * ((uint64_t) drag_v)) / ((uint64_t) 1000) + (((uint64_t) v) * ((uint64_t) v) * ((uint64_t) drag_v_2)) / ((uint64_t) 1000000);
}

//brake_deceleration should be <<8
//return true if need for speed control
bool CheckCalculateProjectedBrakingSpeed(unsigned int brake_deceleration, unsigned int target_speed, unsigned int current_speed, unsigned int brake_threshold_speed, unsigned int target_distance, unsigned int &brake_speed) {
	uint64_t vsq = ((((uint64_t) 2) * ((uint64_t) brake_deceleration) * ((uint64_t) target_distance)) >> 8);
	if(target_speed) vsq += (((uint64_t) target_speed) * ((uint64_t) target_speed));

	if(vsq >= ((uint64_t) brake_threshold_speed) * ((uint64_t) brake_threshold_speed)) {
		return false;    //no need for any braking at all, don't bother with the isqrt
	}
	else {
		brake_speed = (unsigned int) fast_isqrt(vsq);
		return true;
	}
}

void train::TrainTimeStep(unsigned int ms) {
	if(!train_segments.empty()) TrainMoveStep(ms);
}

void train::TrainMoveStep(unsigned int ms) {
	unsigned int displacement=(ms*current_speed)/1000;

	int head_elevationdelta, tail_elevationdelta;

	AdvanceDisplacement(displacement, head_pos, &head_elevationdelta, [this](track_location &old_track, track_location &new_track) { new_track.GetTrack()->TrainEnter(new_track.GetDirection(), this); });
	AdvanceDisplacement(displacement, tail_pos, &tail_elevationdelta, [this](track_location &old_track, track_location &new_track) { old_track.GetTrack()->TrainLeave(old_track.GetDirection(), this); });
	head_relative_height += head_elevationdelta;
	tail_relative_height += tail_elevationdelta;
	la.Advance(displacement);

	int slopeforce = CalcGravityForce(total_mass, tail_relative_height-head_relative_height, total_length);
	int dragforce = CalcDrag(total_drag_const, total_drag_v, total_drag_v2, current_speed);

	unsigned int current_max_tractive_force = total_tractive_force;
	if(current_speed) {    //don't do this check if stationary, divide by zero issue...
		unsigned int speed_limited_tractive_force = total_tractive_power / current_speed;
		if(speed_limited_tractive_force < current_max_tractive_force) current_max_tractive_force = speed_limited_tractive_force;
	}

	unsigned int current_max_braking_force = total_braking_force;

	int max_total_force = current_max_tractive_force + slopeforce - dragforce;
	int min_total_force = -current_max_braking_force + slopeforce - dragforce;

	int max_acceleration = (max_total_force << 8) / total_mass;
	int min_acceleration = (min_total_force << 8) / total_mass;

	if(max_acceleration > ACCEL_BRAKE_CAP) max_acceleration = ACCEL_BRAKE_CAP;    //cap acceleration/braking
	else if(min_acceleration < -ACCEL_BRAKE_CAP) min_acceleration = -ACCEL_BRAKE_CAP;

	int max_new_speed = (int) current_speed + ((max_acceleration * ms) >> 8);
	int min_new_speed = (int) current_speed + ((min_acceleration * ms) >> 8);

	int target_speed = std::min((int) current_max_speed, max_new_speed);

	auto lookaheadfunc = [&](unsigned int distance, unsigned int speed) {
		unsigned int brake_speed;
		if(CheckCalculateProjectedBrakingSpeed(-min_acceleration, speed, current_speed, CREEP_SPEED, distance, brake_speed)) {
			if((int) brake_speed < target_speed) target_speed = brake_speed;
		}
	};

	bool waitingatredsig = false;

	auto lookaheaderrorfunc = [&](lookahead::LA_ERROR err, const track_target_ptr &piece) {
		switch(err) {
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

	if(waitingatredsig && current_speed == 0) {
		if(!(tflags & TF::WAITINGREDSIG)) {
			tflags |= TF::WAITINGREDSIG;
			redsigwaitstarttime = GetWorld().GetGameTime();
		}

		//TODO: do something if we've been waiting for a while
	}
	else tflags &= ~TF::WAITINGREDSIG;

	current_speed = std::max(std::min(max_new_speed, target_speed), min_new_speed);
}

void train::CalculateTrainMotionProperties(unsigned int weatherfactor_shl8) {
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

	if(train_segments.begin() == train_segments.end()) return;    //train with no carriages

	for(auto it = train_segments.begin(); it != train_segments.end(); ++it) {
		total_length += it->vehtype->length * it->veh_multiplier;
		total_drag_const += it->vehtype->cumul_drag_const * it->veh_multiplier;
		total_drag_v += it->vehtype->cumul_drag_v * it->veh_multiplier;
		total_drag_v2 += it->vehtype->cumul_drag_v2 * it->veh_multiplier;
		total_mass += it->segment_total_mass;

		unsigned int unit_tractive_force = it->vehtype->tractive_force;
		unsigned int unit_braking_force = it->vehtype->braking_force;
		unsigned int unit_traction_limit = (it->vehtype->nominal_rail_traction_limit * weatherfactor_shl8) >> 8;

		total_braking_force += std::min(unit_braking_force, unit_traction_limit) * it->veh_multiplier;
		if(active_tractions.IsIntersecting(it->vehtype->tractiontypes))	{
			total_tractive_force += std::min(unit_tractive_force, unit_traction_limit) * it->veh_multiplier;
			total_tractive_power += it->vehtype->tractive_power * it->veh_multiplier;
		}

		if(it == train_segments.begin() || veh_max_speed > it->vehtype->max_speed) veh_max_speed = it->vehtype->max_speed;

	}
	total_drag_v2 += train_segments.front().vehtype->face_drag_v2;
	total_drag_v2 += train_segments.back().vehtype->face_drag_v2;
}

void train::AddCoveredTrackSpeedLimit(unsigned int speed) {
	if(speed >= veh_max_speed) return;
	for(auto it = covered_track_speed_limits.begin(); it != covered_track_speed_limits.end(); ++it) {
		if(it->speed == speed) {
			it->count++;
			return;
		}
	}
	covered_track_speed_limits.emplace_front(speed, 1);
	CalculateCoveredTrackSpeedLimit();
}
void train::RemoveCoveredTrackSpeedLimit(unsigned int speed) {
	if(speed >= veh_max_speed) return;
	auto prev_it = covered_track_speed_limits.before_begin();
	auto it = prev_it;
	++it;
	for(; it != covered_track_speed_limits.end(); prev_it = it, ++it) {
		if(it->speed == speed) {
			it->count--;
			if(it->count == 0) {
				covered_track_speed_limits.erase_after(prev_it);    //this invalidates iterator it
				CalculateCoveredTrackSpeedLimit();
			}
			return;
		}
	}
}

void train::CalculateCoveredTrackSpeedLimit() {
	current_max_speed = veh_max_speed;
	for(auto it = covered_track_speed_limits.begin(); it != covered_track_speed_limits.begin(); ++it) {
		if(it->speed < current_max_speed) current_max_speed = it->speed;
	}
}

void train::ReverseDirection() {
	tflags ^= TF::CONSISTREVDIR;

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

	track_location local_backtrack_pos = head_pos;
	local_backtrack_pos.ReverseDirection();

	auto func = [this](track_location &old_track, track_location &new_track) {
		const speedrestrictionset *speeds = new_track.GetTrack()->GetSpeedRestrictions();
		if(speeds) this->AddCoveredTrackSpeedLimit(speeds->GetTrainTrackSpeedLimit(this));
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

	auto func = [this](track_location &old_track, track_location &new_track) {
		new_track.GetTrack()->TrainEnter(new_track.GetTrack()->GetReverseDirection(new_track.GetDirection()), this);
	};

	track_location temp;
	func(temp, tail_pos);    //include the first track piece

	flagwrapper<ADRESULTF> adresultflags;
	unsigned int leftover = AdvanceDisplacement(total_length, tail_pos, &tail_relative_height, func, &adresultflags);

	tail_pos.ReverseDirection();
	if(leftover) {
		ec.RegisterNewError<error_droptrainintoposition>(this, position,
				string_format("Insufficient track to drop train: tail at: (%s), leftover: %umm",
				stringify(tail_pos).c_str(), leftover));
	}

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

tractionset train::GetAllTractionTypes() const {
	tractionset ts_union;
	for(auto &it : train_segments) {
		ts_union.UnionWith(it.vehtype->tractiontypes);
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
	} while(GetWorld().FindTrainByName(name));
	SetName(name);
}

void train_unit::CalculateSegmentMass() {
	if(vehtype) {
		unsigned int unit_mass = (stflags & STF::FULL) ? vehtype->fullmass : vehtype->emptymass;
		segment_total_mass = unit_mass * veh_multiplier;
	}
	else segment_total_mass = 0;
}
