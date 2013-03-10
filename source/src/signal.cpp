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

#include <algorithm>
#include <iterator>
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

bool trackroutingpoint::HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance) {
	switch(this_entrance_direction) {
		case EDGE_FRONT:
			return TryConnectPiece(prev, target_entrance);
		case EDGE_BACK:
			return TryConnectPiece(next, target_entrance);
		default:
			assert(false);
			return false;
	}
}

const track_target_ptr & trackroutingpoint::GetConnectingPiece(EDGETYPE direction) const {
	switch(direction) {
		case EDGE_FRONT:
			return next;
		case EDGE_BACK:
			return prev;
		default:
			assert(false);
			return empty_track_target;
	}
}

EDGETYPE trackroutingpoint::GetReverseDirection(EDGETYPE direction) const {
	switch(direction) {
		case EDGE_FRONT:
			return EDGE_BACK;
		case EDGE_BACK:
			return EDGE_FRONT;
		default:
			assert(false);
			return EDGE_NULL;
	}
}

unsigned int trackroutingpoint::GetMaxConnectingPieces(EDGETYPE direction) const {
	return 1;
}

const track_target_ptr & trackroutingpoint::GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const {
	return GetConnectingPiece(direction);
}

EDGETYPE trackroutingpoint::GetAvailableAutoConnectionDirection(bool forwardconnection) const {
	if(forwardconnection && !next.IsValid()) return EDGE_BACK;
	if(!forwardconnection && !prev.IsValid()) return EDGE_FRONT;
	return EDGE_NULL;
}

void genericsignal::TrainEnter(EDGETYPE direction, train *t) { }
void genericsignal::TrainLeave(EDGETYPE direction, train *t) { }

unsigned int genericsignal::GetSignalFlags() const {
	return sflags;
}

unsigned int genericsignal::SetSignalFlagsMasked(unsigned int set_flags, unsigned int mask_flags) {
	sflags = (sflags & (~mask_flags)) | set_flags;
	return sflags;
}

bool genericsignal::Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, route *resroute) {
	if(direction != EDGE_FRONT && rr_flags & (RRF_STARTPIECE | RRF_ENDPIECE)) {
		return false;
	}
	if(rr_flags & RRF_STARTPIECE) {
		return start_trs.Reservation(direction, index, rr_flags, resroute);
	}
	else if(rr_flags & RRF_ENDPIECE) {
		return end_trs.Reservation(direction, index, rr_flags, resroute);
	}
	else {
		return start_trs.Reservation(direction, index, rr_flags, resroute) && end_trs.Reservation(direction, index, rr_flags, resroute);
	}
}

unsigned int genericsignal::GetAvailableRouteTypes(EDGETYPE direction) const {
	return (direction == EDGE_FRONT) ? availableroutetypes_forward : availableroutetypes_reverse;
}

unsigned int genericsignal::GetSetRouteTypes(EDGETYPE direction) const {
	unsigned int result = 0;
	if(direction == EDGE_FRONT) {
		if(start_trs.reserved_route && start_trs.rr_flags & RRF_RESERVE) {
			switch(start_trs.reserved_route->type) {
				case RTC_SHUNT:
					if(start_trs.rr_flags & RRF_STARTPIECE) result |= RPRT_SHUNTSTART;
					else if(start_trs.direction == EDGE_FRONT) result |= RPRT_SHUNTTRANS;
					break;
				case RTC_ROUTE:
					if(start_trs.rr_flags & RRF_STARTPIECE) result |= RPRT_ROUTESTART;
					else if(start_trs.direction == EDGE_FRONT) result |= RPRT_ROUTETRANS;
					break;
				default:
					break;
			}
		}
		if(end_trs.reserved_route && end_trs.rr_flags & RRF_RESERVE) {
			switch(end_trs.reserved_route->type) {
				case RTC_SHUNT:
					if(end_trs.rr_flags & RRF_ENDPIECE) result |= RPRT_SHUNTEND;
					else if(end_trs.direction == EDGE_FRONT) result |= RPRT_SHUNTTRANS;
					break;
				case RTC_ROUTE:
					if(end_trs.rr_flags & RRF_ENDPIECE) result |= RPRT_ROUTEEND;
					else if(end_trs.direction == EDGE_FRONT) result |= RPRT_ROUTETRANS;
					break;
				default:
					break;
			}
		}
	}
	else {
		if(start_trs.reserved_route && start_trs.rr_flags & RRF_RESERVE) {
			switch(start_trs.reserved_route->type) {
				case RTC_SHUNT:
					if(start_trs.direction != EDGE_FRONT) result |= RPRT_SHUNTTRANS;
					break;
				case RTC_ROUTE:
					if(start_trs.direction != EDGE_FRONT) result |= RPRT_ROUTETRANS;
					break;
				default:
					break;
			}
		}
	}
	return result;
}

unsigned int autosignal::GetFlags(EDGETYPE direction) const {
	return GTF_ROUTINGPOINT | start_trs.GetGTReservationFlags(direction);
}

route *autosignal::GetRouteByIndex(unsigned int index) {
	if(index == 0) return &signal_route;
	else if(index == 1) return &overlap_route;
	else return 0;
}

unsigned int routesignal::GetFlags(EDGETYPE direction) const {
	return GTF_ROUTINGPOINT | start_trs.GetGTReservationFlags(direction);
}

route *routesignal::GetRouteByIndex(unsigned int index) {
	for(auto it = signal_routes.begin(); it != signal_routes.end(); ++it) {
		if(it->index == index) return &(*it);
	}
	return 0;
}

struct signal_route_recording_state : public generic_route_recording_state {
	unsigned int rrrs_flags;

	enum {
		RRRSF_SHUNTOK	= 1<<0,
		RRRSF_ROUTEOK	= 1<<1,
		RRRSF_OVERLAPOK	= 1<<2,

		RRRSF_SCANTYPES	= RRRSF_SHUNTOK | RRRSF_ROUTEOK | RRRSF_OVERLAPOK,
	};

	virtual ~signal_route_recording_state() { }
	signal_route_recording_state() : rrrs_flags(0) { }
	virtual signal_route_recording_state *Clone() const {
		signal_route_recording_state *rrrs = new signal_route_recording_state;
		rrrs->rrrs_flags = rrrs_flags;
		return rrrs;
	}
};

bool autosignal::PostLayoutInit(error_collection &ec) {
	if(! genericsignal::PostLayoutInit(ec)) return false;

	bool ok = PostLayoutInitTrackScan(ec, 100, 0, 0, [&](ROUTE_CLASS type, const track_target_ptr &piece) -> route* {
		route *candidate = 0;
		if(type == RTC_ROUTE) {
			candidate = &this->signal_route;
			candidate->index = 0;
		}
		else if(type == RTC_OVERLAP) {
			candidate = &this->overlap_route;
			candidate->index = 1;
		}
		else {
			ec.RegisterError(std::unique_ptr<error_obj>(new error_signalinit_trackscan(*this, piece, "Autosignals support route and overlap route types only")));
			return 0;
		}

		if(candidate->type != RTC_NULL) {
			ec.RegisterError(std::unique_ptr<error_obj>(new error_signalinit_trackscan(*this, piece, "Autosignal already has a route of the corresponding type")));
			return 0;
		}
		else return candidate;
	});

	if(signal_route.type == RTC_NULL) {
		ec.RegisterError(std::unique_ptr<error_obj>(new error_signalinit(*this, "Track scan found no route")));
		return false;
	}
	else {
		if(RouteReservation(signal_route, RRF_AUTOROUTE | RRF_TRYRESERVE)) RouteReservation(signal_route, RRF_AUTOROUTE | RRF_RESERVE);
		else {
			ec.RegisterError(std::unique_ptr<error_obj>(new error_signalinit(*this, "Autosignal crosses reserved route")));
			return false;
		}
	}

	return ok;
}

bool routesignal::PostLayoutInit(error_collection &ec) {
	if(! genericsignal::PostLayoutInit(ec)) return false;

	unsigned int route_index = 0;
	return PostLayoutInitTrackScan(ec, 100, 10, &restrictions, [&](ROUTE_CLASS type, const track_target_ptr &piece) -> route* {
		this->signal_routes.emplace_back();
		route *rt = &this->signal_routes.back();
		rt->index = route_index;
		route_index++;
		return rt;
	});
}

bool genericsignal::PostLayoutInitTrackScan(error_collection &ec, unsigned int max_pieces, unsigned int junction_max, route_restriction_set *restrictions, std::function<route*(ROUTE_CLASS type, const track_target_ptr &piece)> mkblankroute) {
	bool continue_initing = true;

	auto func = [&](const route_recording_list &route_pieces, const track_target_ptr &piece, generic_route_recording_state *grrs) {
		signal_route_recording_state *rrrs = static_cast<signal_route_recording_state *>(grrs);

		unsigned int pieceflags = piece.track->GetFlags(piece.direction);
		if(pieceflags & GTF_ROUTINGPOINT) {
			routingpoint *target_routing_piece = dynamic_cast<routingpoint *>(piece.track);
			if(!target_routing_piece) {
				ec.RegisterError(std::unique_ptr<error_obj>(new error_signalinit_trackscan(*this, piece, "Track piece claims to be routingpoint but not downcastable")));
				continue_initing = false;
				return true;
			}

			unsigned int availableroutetypes = target_routing_piece->GetAvailableRouteTypes(piece.direction);
			if(availableroutetypes & (RPRT_ROUTEEND | RPRT_SHUNTEND | RPRT_OVERLAPEND)) {
				if(target_routing_piece->GetSetRouteTypes(piece.direction) & (RPRT_ROUTEEND | RPRT_SHUNTEND | RPRT_VIA)) {
					ec.RegisterError(std::unique_ptr<error_obj>(new error_signalinit_trackscan(*this, piece, "Signal route already reserved")));
					continue_initing = false;
					return true;
				}

				std::vector<const route_restriction*> matching_restrictions;
				unsigned int restriction_denyflags;
				if(restrictions) restriction_denyflags = restrictions->CheckAllRestrictions(matching_restrictions, route_pieces, piece);
				else restriction_denyflags = 0;

				auto mk_route = [&](ROUTE_CLASS type) {
					route *rt = mkblankroute(type, piece);
					if(rt) {
						rt->start = vartrack_target_ptr<routingpoint>(this, EDGE_FRONT);
						rt->pieces = route_pieces;
						rt->end = vartrack_target_ptr<routingpoint>(target_routing_piece, piece.direction);
						rt->FillViaList();
						rt->parent = this;
						for(auto it = matching_restrictions.begin(); it != matching_restrictions.end(); ++it) {
							(*it)->ApplyRestriction(*rt);
						}
					}
				};

				if(rrrs->rrrs_flags & signal_route_recording_state::RRRSF_ROUTEOK && availableroutetypes & RPRT_ROUTEEND && !(restriction_denyflags & route_restriction::RRDF_NOROUTE)) mk_route(RTC_ROUTE);
				if(rrrs->rrrs_flags & signal_route_recording_state::RRRSF_SHUNTOK && availableroutetypes & RPRT_SHUNTEND && !(restriction_denyflags & route_restriction::RRDF_NOSHUNT)) mk_route(RTC_SHUNT);
				if(rrrs->rrrs_flags & signal_route_recording_state::RRRSF_OVERLAPOK && availableroutetypes & RPRT_OVERLAPEND && !(restriction_denyflags & route_restriction::RRDF_NOOVERLAP)) mk_route(RTC_OVERLAP);
			}

			if(! (availableroutetypes & RPRT_SHUNTTRANS)) rrrs->rrrs_flags &= ~signal_route_recording_state::RRRSF_SHUNTOK;
			if(! (availableroutetypes & RPRT_ROUTETRANS)) rrrs->rrrs_flags &= ~signal_route_recording_state::RRRSF_ROUTEOK;
			if(! (availableroutetypes & RPRT_OVERLAPTRANS)) rrrs->rrrs_flags &= ~signal_route_recording_state::RRRSF_OVERLAPOK;

			if(! (rrrs->rrrs_flags & signal_route_recording_state::RRRSF_SCANTYPES)) return true;	//nothing left to scan for
		}
		if(pieceflags & GTF_ROUTESET) {
			ec.RegisterError(std::unique_ptr<error_obj>(new error_signalinit_trackscan(*this, piece, "Signal route already reserved")));
			continue_initing = false;
			return true;
		}
		return false;
	};

	unsigned int error_flags = 0;
	route_recording_list pieces;
	signal_route_recording_state rrrs;
	if(GetAvailableRouteTypes(EDGE_FRONT) & RPRT_ROUTESTART) rrrs.rrrs_flags |= signal_route_recording_state::RRRSF_ROUTEOK | signal_route_recording_state::RRRSF_OVERLAPOK;
	if(GetAvailableRouteTypes(EDGE_FRONT) & RPRT_SHUNTSTART) rrrs.rrrs_flags |= signal_route_recording_state::RRRSF_SHUNTOK;
	TrackScan(max_pieces, junction_max, GetConnectingPieceByIndex(EDGE_FRONT, 0), pieces, &rrrs, error_flags, func);

	if(error_flags != 0) {
		continue_initing = false;
		ec.RegisterError(std::unique_ptr<error_obj>(new error_signalinit(*this, std::string("Track scan failed constraints: ") + GetTrackScanErrorFlagsStr(error_flags))));
	}
	return continue_initing;
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

//return true if restriction applies
bool route_restriction::CheckRestriction(unsigned int &restriction_flags, const route_recording_list &route_pieces, const track_target_ptr &piece) const {
	if(!targets.empty() && std::find(targets.begin(), targets.end(), piece.track->GetName()) == targets.end()) return false;

	auto via_start = via.begin();
	for(auto it = route_pieces.begin(); it != route_pieces.end(); ++it) {
		if(!notvia.empty() && std::find(notvia.begin(), notvia.end(), it->location.track->GetName()) != notvia.end()) return false;
		if(!via.empty()) {
			auto found_via = std::find(via_start, via.end(), it->location.track->GetName());
			if(found_via != via.end()) {
				via_start = std::next(found_via, 1);
			}
		}
	}
	if(via_start != via.end()) return false;

	restriction_flags |= denyflags;
	return true;
}

void route_restriction::ApplyRestriction(route &rt) const {
	if(routerestrictionflags & RRF_PRIORITYSET) rt.priority = priority;
}

unsigned int route_restriction_set::CheckAllRestrictions(std::vector<const route_restriction*> &matching_restrictions, const route_recording_list &route_pieces, const track_target_ptr &piece) const {
	unsigned int restriction_flags = 0;
	for(auto it = restrictions.begin(); it != restrictions.end(); ++it) {
		if(it->CheckRestriction(restriction_flags, route_pieces, piece)) matching_restrictions.push_back(&(*it));
	}
	return restriction_flags;
}
