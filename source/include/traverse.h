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

#ifndef INC_TRAVERSE_ALREADY
#define INC_TRAVERSE_ALREADY

#include "track.h"
#include <functional>
#include <deque>

unsigned int AdvanceDisplacement(unsigned int displacement, track_location &track, int *elevationdelta /*optional, out*/, std::function<void(track_location & /*old*/, track_location & /*new*/)> func);

enum {
	TSEF_OUTOFTRACK			= 1<<0,
	TSEF_JUNCTIONLIMITREACHED	= 1<<1,
	TSEF_LENGTHLIMIT		= 1<<2,
};

struct route_recording_item {
	track_target_ptr location;
	unsigned int connection_index;
	route_recording_item() : connection_index(0) { }
	route_recording_item(const track_target_ptr &loc, unsigned int index) : location(loc), connection_index(index) { }
};

typedef std::deque<route_recording_item> route_recording_list;

void TrackScan(unsigned int max_pieces, unsigned int junction_max, track_target_ptr start_track, route_recording_list &route_pieces, unsigned int &error_flags, std::function<bool(const route_recording_list &route_pieces, const track_target_ptr &piece)> step_func);
std::string GetTrackScanErrorFlagsStr(unsigned int error_flags);

#endif
