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

#include "common.h"
#include "signal.h"

#include <cassert>

bool genericsignal::HalfConnect(DIRTYPE this_entrance_direction, const track_target_ptr &target_entrance) {
	switch(this_entrance_direction) {
		case TDIR_FORWARD:
			return TryConnectPiece(prev, target_entrance);
		case TDIR_REVERSE:
			return TryConnectPiece(next, target_entrance);
		default:
			assert(false);
			return false;
	}
}

const track_target_ptr & genericsignal::GetConnectingPiece(DIRTYPE direction) const {
	switch(direction) {
		case TDIR_FORWARD:
			return next;
		case TDIR_REVERSE:
			return prev;
		default:
			assert(false);
			return empty_track_target;
	}
}

void genericsignal::TrainEnter(DIRTYPE direction, train *t) { }
void genericsignal::TrainLeave(DIRTYPE direction, train *t) { }

unsigned int genericsignal::GetMaxConnectingPieces(DIRTYPE direction) const {
	return 1;
}

const track_target_ptr & genericsignal::GetConnectingPieceByIndex(DIRTYPE direction, unsigned int index) const {
	return GetConnectingPiece(direction);
}

unsigned int genericsignal::GetSignalFlags() const {
	return sflags;
}

unsigned int genericsignal::SetSignalFlagsMasked(unsigned int set_flags, unsigned int mask_flags) {
	sflags = (sflags & (~mask_flags)) | set_flags;
	return sflags;
}

bool autosignal::PostLayoutInit(error_collection &ec) {
	bool continue_initing = genericsignal::PostLayoutInit(ec);
	if(continue_initing) {
		signal_route.start = vartrack_target_ptr<routingpoint>(this, TDIR_FORWARD);

		auto func = [&](const route_recording_list &route_pieces, const track_target_ptr &piece) {
			unsigned int pieceflags = piece.track->GetFlags(piece.direction);
			if(pieceflags & GTF_ROUTINGPOINT) {
				routingpoint *target_routing_piece = dynamic_cast<routingpoint *>(piece.track);
				if(target_routing_piece && target_routing_piece->GetAvailableRouteTypes(piece.direction) & RPRT_ROUTEEND) {
					if(! (target_routing_piece->GetSetRouteTypes(piece.direction) & (RPRT_ROUTEEND | RPRT_SHUNTEND | RPRT_VIA))) {
						signal_route.pieces = route_pieces;
						signal_route.end = vartrack_target_ptr<routingpoint>(target_routing_piece, piece.direction);
						return true;
					}
				}
				//err = GTL_SIGNAL_ROUTING;
				continue_initing = false;
				return true;
			}
			if(pieceflags & GTF_ROUTESET) {
				//err = GTL_SIGNAL_ROUTING;
				continue_initing = false;
				return true;
			}
			return false;
		};

		signal_route.pieces.clear();
		unsigned int error_flags = 0;
		TrackScan(100, 0, GetConnectingPieceByIndex(TDIR_FORWARD, 0), signal_route.pieces, error_flags, func);
		
		if(error_flags != 0) {
			continue_initing = false;
		}
	}
	return continue_initing;
}

unsigned int autosignal::GetFlags(DIRTYPE direction) const {
	return GTF_ROUTINGPOINT;
}
