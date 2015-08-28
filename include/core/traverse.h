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

#ifndef INC_TRAVERSE_ALREADY
#define INC_TRAVERSE_ALREADY

#include "core/track.h"
#include <functional>
#include <deque>

enum class ADF {
	ZERO                    = 0,
	CHECK_FOR_TRAINS        = 1<<0,
};
template<> struct enum_traits<ADF> { static constexpr bool flags = true; };

enum class ADRESULTF {
	ZERO                    = 0,
	RAN_OUT_OF_TRACK        = 1<<0,
	TRACK_INVALID           = 1<<1,
	TRAIN_IN_WAY            = 1<<2,
};
template<> struct enum_traits<ADRESULTF> { static constexpr bool flags = true; };

//return displacement length that could not be fulfilled
unsigned int AdvanceDisplacement(unsigned int displacement, track_location &track, flagwrapper<ADF> ad_flags = 0,
		flagwrapper<ADRESULTF> *ad_result_flags = nullptr);
unsigned int AdvanceDisplacement(unsigned int displacement, track_location &track, int *elevation_delta /*optional, out*/,
		std::function<void(track_location & /*old*/, track_location & /*new*/)> func, flagwrapper<ADF> ad_flags = 0,
		flagwrapper<ADRESULTF> *ad_result_flags = nullptr);

enum class TSEF {
	ZERO                      = 0,
	OUT_OF_TRACK              = 1<<0,
	JUNCTION_LIMIT_REACHED    = 1<<1,
	LENGTH_LIMIT              = 1<<2,
};
template<> struct enum_traits< TSEF > { static constexpr bool flags = true; };

struct route_recording_item {
	track_target_ptr location;
	unsigned int connection_index;
	route_recording_item() : connection_index(0) { }
	route_recording_item(const track_target_ptr &loc, unsigned int index) : location(loc), connection_index(index) { }

	inline bool operator ==(const route_recording_item &other) const {
		return location == other.location && connection_index == other.connection_index;
	}

	inline bool operator !=(const route_recording_item &other) const {
		return !(*this == other);
	}
};

typedef std::deque<route_recording_item> route_recording_list;

struct generic_route_recording_state {
	virtual ~generic_route_recording_state() { };
	virtual generic_route_recording_state *Clone() const = 0;
};

void TrackScan(unsigned int max_pieces, unsigned int junction_max, track_target_ptr start_track, route_recording_list &route_pieces,
		generic_route_recording_state *grrs, TSEF &error_flags, std::function<bool(const route_recording_list &route_pieces,
		const track_target_ptr &piece, generic_route_recording_state *grrs)> step_func);

std::string GetTrackScanErrorFlagsStr(TSEF error_flags);

#endif
