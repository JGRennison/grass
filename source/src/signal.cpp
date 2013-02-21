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

class error_signalinit : public layout_initialisation_error_obj {
	public:
	error_signalinit(const genericsignal &sig, const std::string &str = "") {
		msg << "Signal Initialisation Error for " << sig << ": " << str;
	}
};

class error_signalinit_trackscan : public error_signalinit {
	public:
	error_signalinit_trackscan(const genericsignal &sig, const track_target_ptr &piece, const std::string &str = "") : error_signalinit(sig) {
		msg << "Track Scan: Piece: " << piece << ": " << str;
	}
};

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

bool genericsignal::Reservation(DIRTYPE direction, unsigned int index, unsigned int rr_flags, route *resroute) {
	if(direction != TDIR_FORWARD && rr_flags & (RRF_STARTPIECE | RRF_ENDPIECE)) {
		return false;
	}
	if(rr_flags & RRF_ENDPIECE) return true;
	return trs.Reservation(direction, index, rr_flags, resroute);
}

bool autosignal::PostLayoutInit(error_collection &ec) {
	bool continue_initing = genericsignal::PostLayoutInit(ec);
	if(continue_initing) {
		signal_route.start = vartrack_target_ptr<routingpoint>(this, TDIR_FORWARD);
		bool route_success = false;

		auto func = [&](const route_recording_list &route_pieces, const track_target_ptr &piece) {
			unsigned int pieceflags = piece.track->GetFlags(piece.direction);
			if(pieceflags & GTF_ROUTINGPOINT) {
				routingpoint *target_routing_piece = dynamic_cast<routingpoint *>(piece.track);
				if(target_routing_piece && target_routing_piece->GetAvailableRouteTypes(piece.direction) & RPRT_ROUTEEND) {
					if(! (target_routing_piece->GetSetRouteTypes(piece.direction) & (RPRT_ROUTEEND | RPRT_SHUNTEND | RPRT_VIA))) {
						signal_route.pieces = route_pieces;
						signal_route.end = vartrack_target_ptr<routingpoint>(target_routing_piece, piece.direction);
						route_success = true;
						return true;
					}
				}
				ec.RegisterError(std::unique_ptr<error_obj>(new error_signalinit_trackscan(*this, piece, "Routing piece not a valid autosignal route end")));
				continue_initing = false;
				return true;
			}
			if(pieceflags & GTF_ROUTESET) {
				ec.RegisterError(std::unique_ptr<error_obj>(new error_signalinit_trackscan(*this, piece, "Autosignal route already reserved")));
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
			ec.RegisterError(std::unique_ptr<error_obj>(new error_signalinit(*this, std::string("Track scan failed constraints: ") + GetTrackScanErrorFlagsStr(error_flags))));
		}
		else if(!route_success) {
			continue_initing = false;
			ec.RegisterError(std::unique_ptr<error_obj>(new error_signalinit(*this, "Track scan found no route")));
		}
		else {
			if(RouteReservation(signal_route, RRF_AUTOROUTE | RRF_TRYRESERVE)) RouteReservation(signal_route, RRF_AUTOROUTE | RRF_RESERVE);
			else {
				ec.RegisterError(std::unique_ptr<error_obj>(new error_signalinit(*this, "Autosignal crosses reserved route")));
				continue_initing = false;
			}
		}
	}
	return continue_initing;
}

unsigned int autosignal::GetFlags(DIRTYPE direction) const {
	return GTF_ROUTINGPOINT;
}

bool routesignal::PostLayoutInit(error_collection &ec) {
	bool continue_initing = genericsignal::PostLayoutInit(ec);
	if(continue_initing) {
		auto func = [&](const route_recording_list &route_pieces, const track_target_ptr &piece) {
			unsigned int pieceflags = piece.track->GetFlags(piece.direction);
			if(pieceflags & GTF_ROUTINGPOINT) {
				routingpoint *target_routing_piece = dynamic_cast<routingpoint *>(piece.track);
				if(target_routing_piece && target_routing_piece->GetAvailableRouteTypes(piece.direction) & RPRT_ROUTEEND) {
					if(! (target_routing_piece->GetSetRouteTypes(piece.direction) & (RPRT_ROUTEEND | RPRT_SHUNTEND | RPRT_VIA))) {
						signal_routes.emplace_back();
						route &rt = signal_routes.back();
						rt.start = vartrack_target_ptr<routingpoint>(this, TDIR_FORWARD);
						rt.pieces = route_pieces;
						rt.end = vartrack_target_ptr<routingpoint>(target_routing_piece, piece.direction);
						rt.FillViaList();
						return true;
					}
				}
				else if(target_routing_piece && target_routing_piece->GetAvailableRouteTypes(piece.direction) & RPRT_VIA) {
					//via point, ignore, handled by route::FillViaList
					return true;
				}
				ec.RegisterError(std::unique_ptr<error_obj>(new error_signalinit_trackscan(*this, piece, "Routing piece not a valid route signal route end")));
				continue_initing = false;
				return true;
			}
			if(pieceflags & GTF_ROUTESET) {
				ec.RegisterError(std::unique_ptr<error_obj>(new error_signalinit_trackscan(*this, piece, "Route signal route already reserved")));
				continue_initing = false;
				return true;
			}
			return false;
		};
		
		unsigned int error_flags = 0;
		route_recording_list pieces;
		TrackScan(100, 10, GetConnectingPieceByIndex(TDIR_FORWARD, 0), pieces, error_flags, func);
		
		if(error_flags != 0) {
			continue_initing = false;
			ec.RegisterError(std::unique_ptr<error_obj>(new error_signalinit(*this, std::string("Track scan failed constraints: ") + GetTrackScanErrorFlagsStr(error_flags))));
		}
		else if(signal_routes.empty()) {
			continue_initing = false;
			ec.RegisterError(std::unique_ptr<error_obj>(new error_signalinit(*this, "Track scan found no route")));
		}
	}
	return continue_initing;
}

unsigned int routesignal::GetFlags(DIRTYPE direction) const {
	return GTF_ROUTINGPOINT;
}

//returns false on failure
bool RouteReservation(route &res_route, unsigned int rr_flags) {
	if(!res_route.start.track->Reservation(res_route.start.direction, 0, rr_flags | RRF_STARTPIECE, &res_route)) return false;

	for(auto it = res_route.pieces.begin(); it != res_route.pieces.end(); ++it) {
		if(!it->location.track->Reservation(it->location.direction, it->connection_index, rr_flags, &res_route)) return false;
	}

	if(!res_route.end.track->Reservation(res_route.end.direction, 0, rr_flags | RRF_ENDPIECE, &res_route)) return false;
	return true;
}

void route::FillViaList() {
	for(auto it = pieces.begin(); it != pieces.end(); ++it) {
		routingpoint *target_routing_piece = dynamic_cast<routingpoint *>(it->location.track);
		if(target_routing_piece && target_routing_piece->GetAvailableRouteTypes(it->location.direction) & routingpoint::RPRT_VIA) {
			vias.push_back(target_routing_piece);
		}
	}
}
