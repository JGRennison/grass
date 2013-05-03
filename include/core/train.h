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

#ifndef INC_TRAIN_ALREADY
#define INC_TRAIN_ALREADY

#include "world_obj.h"
#include "track.h"
#include "timetable.h"
#include "serialisable.h"
#include "tractiontype.h"
#include <list>

class lookahead_item {
	unsigned int speed;
	unsigned int start_offset;
};

class lookahead_set {

	std::list<lookahead_item> items;
};

struct vehicle_class : public serialisable_obj {
	const std::string name;
	unsigned int length = 0;			// mm
	unsigned int max_speed = 0;			// mm/s (μm/ms)
	unsigned int tractive_force = 0;		// kg μm/(ms^2) --> N
	unsigned int tractive_power = 0;		// 1000 * N mm/s --> W
	unsigned int braking_force = 0;			// N
	unsigned int nominal_rail_traction_limit = 0;	// N
	unsigned int cumul_drag_const = 0;		// N
	unsigned int cumul_drag_v = 0;			// N/(m/s)
	unsigned int cumul_drag_v2 = 0;			// N/(m/s)^2
	unsigned int face_drag_v2 = 0;			// N/(m/s)^2
	unsigned int fullmass = 0;			// kg
	unsigned int emptymass = 0;			// kg
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
	vehicle_class *vehtype;
	unsigned int veh_multiplier = 0;
	unsigned int segment_total_mass = 0;
	enum class STF {
		ZERO		= 0,
		REV		= 1<<0,
	};
	STF stflags = STF::REV;
};
template<> struct enum_traits< train_unit::STF > {	static constexpr bool flags = true; };

struct train_track_speed_limit_item {
	unsigned int speed;
	unsigned int count;
	train_track_speed_limit_item(unsigned int speed_, unsigned int count_) : speed(speed_), count(count_) { }
};

class train : public world_obj  {
	enum class TF {
		ZERO		= 0,
		CONSISTREVDIR	= 1<<0,
	};
	TF tflags = TF::ZERO;

	std::list<train_unit> train_segments;
	tractionset active_tractions;
	std::string vehspeedclass;
	std::forward_list<train_track_speed_limit_item> covered_track_speed_limits;

	lookahead_set lookahead;

	unsigned int total_length = 0;
	unsigned int veh_max_speed = 0;
	unsigned int current_max_speed = 0;
	unsigned int total_tractive_force = 0;
	unsigned int total_tractive_power = 0;
	unsigned int total_braking_force = 0;
	//unsigned int total_rail_traction_limit = 0;
	unsigned int total_drag_const = 0;
	unsigned int total_drag_v = 0;
	unsigned int total_drag_v2 = 0;
	unsigned int total_mass = 0;
	unsigned int target_braking_deceleration = 0; 	// m/s^2 << 8

	track_location head_pos;
	track_location tail_pos;
	int head_relative_height = 0;
	int tail_relative_height = 0;

	unsigned int current_speed = 0;
	std::string headcode;
	timetable currenttimetable;

	public:
	train(world &w_) : world_obj(w_) { }
	void TrainMoveStep(unsigned int ms);
	void CalculateTrainMotionProperties(unsigned int weatherfactor_shl8);
	void AddCoveredTrackSpeedLimit(unsigned int speed);
	void RemoveCoveredTrackSpeedLimit(unsigned int speed);
	void CalculateCoveredTrackSpeedLimit();
	void ReverseDirection();
	inline void RefreshLookahead() { };
	void RefreshCoveredTrackSpeedLimits();
	void DropTrainIntoPosition(const track_location &position);
	void UprootTrain();
	inline unsigned int GetMaxVehSpeed() const { return veh_max_speed; }
	inline std::string GetVehSpeedClass() const { return vehspeedclass; }
	const tractionset &GetTractionTypes() const { return active_tractions; }
};
template<> struct enum_traits< train::TF > {	static constexpr bool flags = true; };

#endif