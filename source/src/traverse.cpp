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

#include "traverse.h"

unsigned int AdvanceDisplacement(unsigned int displacement, track_location &track, int *elevationdelta /*optional, out*/, std::function<void (track_location & /*old*/, track_location & /*new*/)> func) {

	if(elevationdelta) *elevationdelta = 0;

	if(!track.IsValid()) return displacement;

	while(displacement > 0) {
		unsigned int length_on_piece = track.GetTrack()->GetRemainingLength(track.GetDirection(), track.GetOffset());
		if(length_on_piece > displacement) {
			track.GetOffset() = track.GetTrack()->GetNewOffset(track.GetDirection(), track.GetOffset(), displacement);
			if(elevationdelta) *elevationdelta += track.GetTrack()->GetPartialElevationDelta(track.GetDirection(), displacement);
			break;
		}
		else {
			displacement -= length_on_piece;

			if(elevationdelta) *elevationdelta += track.GetTrack()->GetElevationDelta(track.GetDirection());

			track_location old_track = track;
			const track_target_ptr &targ = old_track.GetTrack()->GetConnectingPiece(old_track.GetDirection());
			if(targ.IsValid()) {
				track.SetTargetStartLocation(targ);
			}
			else {	//run out of valid track
				track.GetOffset() = track.GetTrack()->GetNewOffset(track.GetDirection(), track.GetOffset(), length_on_piece);
				return displacement;
			}

			if(func) func(old_track, track);
		}
	}
	return 0;
}

void TrackScan(unsigned int max_pieces, unsigned int junction_max, track_target_ptr start_track, route_recording_list &route_pieces, unsigned int &error_flags, std::function<bool(const route_recording_list &route_pieces, const track_target_ptr &piece)> step_func) {
	while(true) {
		if(!start_track.IsValid()) {
			error_flags |= TSEF_OUTOFTRACK;
			return;
		}
		if(max_pieces == 0) {
			error_flags |= TSEF_LENGTHLIMIT;
			return;
		}
		if(start_track.track->GetFlags(start_track.direction) & generictrack::GTF_ROUTEFORK) {
			if(junction_max == 0) {
				error_flags |= TSEF_JUNCTIONLIMITREACHED;
				return;
			}
			else junction_max--;
		}

		if(step_func(route_pieces, start_track)) return;

		unsigned int max_exit_pieces = start_track.track->GetMaxConnectingPieces(start_track.direction);
		if(max_exit_pieces == 0) {
			error_flags |= TSEF_OUTOFTRACK;
			return;
		}

		max_pieces--;

		if(max_exit_pieces >= 2) {
			for(unsigned int i=1; i < max_exit_pieces; i++) {
				unsigned int route_pieces_size = route_pieces.size();
				route_pieces.emplace_back(start_track, i);
				TrackScan(max_pieces, junction_max, start_track.track->GetConnectingPieceByIndex(start_track.direction, i), route_pieces, error_flags, step_func);
				route_pieces.resize(route_pieces_size);
			}
		}
		route_pieces.emplace_back(start_track, 0);
		start_track = start_track.track->GetConnectingPieceByIndex(start_track.direction, 0);
	}

}
