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
#include "core/traverse.h"
#include "core/trackreservation.h"

//returns displacement length that could not be fulfilled
unsigned int AdvanceDisplacement(unsigned int displacement, track_location &track, flagwrapper<ADRESULTF> *adresultflags) {
	return AdvanceDisplacement(displacement, track, 0, [&](track_location &a, track_location &b) { }, adresultflags);
}

//returns displacement length that could not be fulfilled
unsigned int AdvanceDisplacement(unsigned int displacement, track_location &track, int *elevationdelta /*optional, out*/,
		std::function<void (track_location & /*old*/, track_location & /*new*/)> func, flagwrapper<ADRESULTF> *adresultflags) {

	if(elevationdelta) *elevationdelta = 0;

	if(!track.IsValid()) {
		if(adresultflags) *adresultflags |= ADRESULTF::TRACKINVALID;
		return displacement;
	}

	while(displacement > 0) {
		unsigned int length_on_piece = track.GetTrack()->GetRemainingLength(track.GetDirection(), track.GetOffset());
		if(length_on_piece >= displacement) {
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
			else {    //run out of valid track
				track.GetOffset() = track.GetTrack()->GetNewOffset(track.GetDirection(), track.GetOffset(), length_on_piece);
				if(adresultflags) *adresultflags |= ADRESULTF::RANOUTOFTRACK;
				return displacement;
			}

			if(func) func(old_track, track);
		}
	}
	return 0;
}

void TrackScan(unsigned int max_pieces, unsigned int junction_max, track_target_ptr start_track, route_recording_list &route_pieces, generic_route_recording_state *grrs, TSEF &error_flags, std::function<bool(const route_recording_list &route_pieces, const track_target_ptr &piece, generic_route_recording_state *grrs)> step_func) {
	while(true) {
		if(!start_track.IsValid()) {
			error_flags |= TSEF::OUTOFTRACK;
			return;
		}
		if(max_pieces == 0) {
			error_flags |= TSEF::LENGTHLIMIT;
			return;
		}
		if(start_track.track->GetFlags(start_track.direction) & GTF::ROUTEFORK) {
			if(junction_max == 0) {
				error_flags |= TSEF::JUNCTIONLIMITREACHED;
				return;
			}
			else junction_max--;
		}

		if(step_func(route_pieces, start_track, grrs)) return;

		unsigned int max_exit_pieces = start_track.track->GetMaxConnectingPieces(start_track.direction);
		if(max_exit_pieces == 0) {
			error_flags |= TSEF::OUTOFTRACK;
			return;
		}

		max_pieces--;

		if(max_exit_pieces >= 2) {
			for(unsigned int i=1; i < max_exit_pieces; i++) {
				unsigned int route_pieces_size = route_pieces.size();
				route_pieces.emplace_back(start_track, i);
				generic_route_recording_state *temp_grrs = 0;
				if(grrs) temp_grrs = grrs->Clone();
				TrackScan(max_pieces, junction_max, start_track.track->GetConnectingPieceByIndex(start_track.direction, i), route_pieces, temp_grrs, error_flags, step_func);
				if(temp_grrs) delete temp_grrs;
				route_pieces.resize(route_pieces_size);
				if(error_flags != TSEF::ZERO) return;
			}
		}
		route_pieces.emplace_back(start_track, 0);
		start_track = start_track.track->GetConnectingPieceByIndex(start_track.direction, 0);
	}

}

std::string GetTrackScanErrorFlagsStr(TSEF error_flags) {
	std::string str;
	if(error_flags & TSEF::OUTOFTRACK) {
		str += "Ran out of track, ";
	}
	if(error_flags & TSEF::JUNCTIONLIMITREACHED) {
		str += "Route junction limit exceeded, ";
	}
	if(error_flags & TSEF::LENGTHLIMIT) {
		str += "Maximum route length exceeded, ";
	}
	if(str.size()) str.resize(str.size()-2);
	return str;
}
