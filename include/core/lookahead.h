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

#ifndef INC_LOOKAHEAD_ALREADY
#define INC_LOOKAHEAD_ALREADY

#include <deque>
#include <functional>

#include "track.h"

class generic_track;
class routing_point;
class train;

class lookahead  : public serialisable_obj {
	struct lookahead_item {
		enum class LAI_FLAGS {
			ZERO                                = 0,
			HALT_UNLESS_VISIBLE                 = 1<<0,
			SCAN_BEYOND_IF_PASSABLE             = 1<<1,
			ALLOW_DIFFERENT_CONNECTION_INDEX    = 1<<2,
			TRACTION_UNSUITABLE                 = 1<<3,
			NOT_ALWAYS_PASSABLE                 = 1<<4,
		};
		uint64_t start_offset;
		uint64_t end_offset;
		uint64_t sighting_offset;
		unsigned int speed;
		track_target_ptr piece;
		unsigned int connection_index = 0;
		LAI_FLAGS flags = LAI_FLAGS::ZERO;
	};
	struct lookahead_routing_point {
		uint64_t offset;
		uint64_t sighting_offset;
		vartrack_target_ptr<routing_point> gs;
		unsigned int last_aspect;
		std::deque<lookahead_item> l2_list;
	};


	uint64_t current_offset = 0;
	std::deque<lookahead_routing_point> l1_list;

	public:
	enum class LA_ERROR {
		NONE,
		SIG_TARGET_CHANGE,
		SIG_ASPECT_LESS_THAN_EXPECTED,
		WAITING_AT_RED_SIG,
		TRACTION_UNSUITABLE,
	};

	void Init(const train *t /* optional */, const track_location &pos, const route *rt = nullptr);
	void Advance(unsigned int distance);
	void CheckLookaheads(const train *t /* optional */, const track_location &pos, std::function<void(unsigned int distance, unsigned int speed)> f,
			std::function<void(LA_ERROR err, const track_target_ptr &piece)> errfunc);
	void ScanAppend(const train *t /* optional */, const track_location &pos, unsigned int blocklimit, const route *rt);
	void Clear();

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

template<> struct enum_traits< lookahead::lookahead_item::LAI_FLAGS > { static constexpr bool flags = true; };

#endif
