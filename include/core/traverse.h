//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
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

#ifndef INC_TRAVERSE_ALREADY
#define INC_TRAVERSE_ALREADY

#include "track.h"
#include <functional>
#include <deque>

unsigned int AdvanceDisplacement(unsigned int displacement, track_location &track);
unsigned int AdvanceDisplacement(unsigned int displacement, track_location &track, int *elevationdelta /*optional, out*/, std::function<void(track_location & /*old*/, track_location & /*new*/)> func);

enum class TSEF {
	ZERO			= 0,
	OUTOFTRACK		= 1<<0,
	JUNCTIONLIMITREACHED	= 1<<1,
	LENGTHLIMIT		= 1<<2,
};
template<> struct enum_traits< TSEF > {	static constexpr bool flags = true; };

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

void TrackScan(unsigned int max_pieces, unsigned int junction_max, track_target_ptr start_track, route_recording_list &route_pieces, generic_route_recording_state *grrs, TSEF &error_flags, std::function<bool(const route_recording_list &route_pieces, const track_target_ptr &piece, generic_route_recording_state *grrs)> step_func);
std::string GetTrackScanErrorFlagsStr(TSEF error_flags);

#endif
