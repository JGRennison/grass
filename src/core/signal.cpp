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
#include "world.h"
#include "trackcircuit.h"
#include "track_ops.h"

#include <algorithm>
#include <iterator>
#include <cassert>
#include <initializer_list>

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

void routingpoint::EnumerateRoutes(std::function<void (const route *)> func) const { }

const route *routingpoint::FindBestOverlap() const {
	int best_score = INT_MIN;
	const route *best_overlap = 0;
	EnumerateAvailableOverlaps([&](const route *r, int score){
		if(score>best_score) {
			best_score = score;
			best_overlap = r;
		}
	});
	return best_overlap;
}

void routingpoint::EnumerateAvailableOverlaps(std::function<void(const route *rt, int score)> func, RRF extraflags) const {

	auto overlap_finder = [&](const route *r) {
		if(r->type != RTC_OVERLAP) return;
		if(!r->RouteReservation(RRF::TRYRESERVE | extraflags)) return;

		int score = r->priority;
		auto actioncounter = [&](action &&reservation_act) {
			score -= 10;
		};
		r->RouteReservationActions(RRF::RESERVE | extraflags, actioncounter);
		func(r, score);
	};
	EnumerateRoutes(overlap_finder);
}

unsigned int routingpoint::GetMatchingRoutes(std::vector<gmr_routeitem> &routes, const routingpoint *end, GMRF gmr_flags, RRF extraflags, const via_list &vias) const {
	unsigned int count = 0;
	if(!(gmr_flags & GMRF::DONTCLEARVECTOR)) routes.clear();
	auto route_finder = [&](const route *r) {
		if(r->type == RTC_SHUNT && !(gmr_flags & GMRF::SHUNTOK)) return;
		if(r->type == RTC_ROUTE && !(gmr_flags & GMRF::ROUTEOK)) return;
		if(r->type == RTC_OVERLAP && !(gmr_flags & GMRF::OVERLAPOK)) return;
		if(gmr_flags & GMRF::CHECKVIAS && r->vias != vias) return;
		if(end && r->end.track != end) return;

		int score = r->priority;
		if(gmr_flags & GMRF::TRACKTEST) {
			if(!r->RouteReservation(RRF::TRYRESERVE | extraflags)) return;
			auto actioncounter = [&](action &&reservation_act) {
				score -= 10;
			};
			r->RouteReservationActions(RRF::RESERVE | extraflags, actioncounter);
		}
		gmr_routeitem gmr;
		gmr.rt = r;
		gmr.score = score;
		routes.push_back(gmr);
		count++;

	};
	EnumerateRoutes(route_finder);
	auto sortfunc = [&](const gmr_routeitem &a, const gmr_routeitem &b) -> bool {
		if(a.score > b.score) return true;
		if(a.score < b.score) return false;
		if(a.rt->type == RTC_ROUTE && b.rt->type == RTC_SHUNT) return true;
		if(a.rt->type == RTC_SHUNT && b.rt->type == RTC_ROUTE) return false;
		if(a.rt->pieces.size() < b.rt->pieces.size()) return true;
		if(a.rt->pieces.size() > b.rt->pieces.size()) return false;
		if(a.rt->index < b.rt->index) return true;
		return false;
	};
	if(!(gmr_flags & GMRF::DONTSORT)) std::sort(routes.begin(), routes.end(), sortfunc);
	return count;
}

void routingpoint::SetAspectNextTarget(routingpoint *target) {
	if(target == aspect_target) return;
	if(aspect_target && aspect_target->aspect_backwards_dependency == this) aspect_target->aspect_backwards_dependency = 0;
	if(target) target->aspect_backwards_dependency = this;
	aspect_target = target;
}

void routingpoint::SetAspectRouteTarget(routingpoint *target) {
	aspect_route_target = target;
}

const track_target_ptr& trackroutingpoint::GetEdgeConnectingPiece(EDGETYPE edgeid) const {
	switch(edgeid) {
		case EDGE_FRONT:
			return prev;
		case EDGE_BACK:
			return next;
		default:
			assert(false);
			return empty_track_target;
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

bool trackroutingpoint::IsEdgeValid(EDGETYPE edge) const {
	switch(edge) {
		case EDGE_FRONT:
		case EDGE_BACK:
			return true;
		default:
			return false;
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

void trackroutingpoint::GetListOfEdges(std::vector<edgelistitem> &outputlist) const {
	outputlist.insert(outputlist.end(), { edgelistitem(EDGE_BACK, next), edgelistitem(EDGE_FRONT, prev) });
}


genericsignal::genericsignal(world &w_) : trackroutingpoint(w_), sflags(GSF::ZERO) {
	availableroutetypes_reverse |= RPRT::SHUNTTRANS | RPRT::ROUTETRANS;
	w_.RegisterTickUpdate(this);
}

genericsignal::~genericsignal() {
	GetWorld().UnregisterTickUpdate(this);
}

void genericsignal::TrainEnter(EDGETYPE direction, train *t) { }
void genericsignal::TrainLeave(EDGETYPE direction, train *t) { }

genericsignal::GSF genericsignal::GetSignalFlags() const {
	return sflags;
}

genericsignal::GSF genericsignal::SetSignalFlagsMasked(genericsignal::GSF set_flags, genericsignal::GSF mask_flags) {
	sflags = (sflags & (~mask_flags)) | set_flags;
	return sflags;
}

bool genericsignal::Reservation(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey) {
	if(direction != EDGE_FRONT && rr_flags & (RRF::STARTPIECE | RRF::ENDPIECE)) {
		return false;
	}
	if(rr_flags & RRF::STARTPIECE) {
		return start_trs.Reservation(direction, index, rr_flags, resroute);
	}
	else if(rr_flags & RRF::ENDPIECE) {
		bool result = end_trs.Reservation(direction, index, rr_flags, resroute);
		if(rr_flags & RRF::UNRESERVE) {
			GetWorld().ExecuteIfActionScope([&]() {
				const route* ovlp = GetCurrentForwardOverlap();
				if(ovlp) GetWorld().SubmitAction(action_unreservetrackroute(GetWorld(), *ovlp));
			});
		}
		return result;
	}
	else {
		return start_trs.Reservation(direction, index, rr_flags, resroute) && end_trs.Reservation(direction, index, rr_flags, resroute);
	}
}

RPRT genericsignal::GetAvailableRouteTypes(EDGETYPE direction) const {
	return (direction == EDGE_FRONT) ? availableroutetypes_forward : availableroutetypes_reverse;
}

RPRT genericsignal::GetSetRouteTypes(EDGETYPE direction) const {
	RPRT result = RPRT::ZERO;

	auto result_flag_adds = [&](const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) {
		if(reserved_route) result = result | (GetRouteClassRPRTMask(reserved_route->type) & GetTRSFlagsRPRTMask(rr_flags));
	};

	if(direction == EDGE_FRONT) {
		start_trs.ReservationEnumerationInDirection(EDGE_FRONT, result_flag_adds);
		end_trs.ReservationEnumerationInDirection(EDGE_FRONT, result_flag_adds);
	}
	else {
		start_trs.ReservationEnumerationInDirection(EDGE_BACK, result_flag_adds);
	}
	return result;
}

void genericsignal::UpdateSignalState() {
	if(last_state_update == GetWorld().GetGameTime()) return;

	last_state_update = GetWorld().GetGameTime();

	auto clear_route = [&]() {
		aspect = 0;
		SetAspectNextTarget(0);
		SetAspectRouteTarget(0);
		aspect_type = RTC_NULL;
	};

	const route *set_route = GetCurrentForwardRoute();
	if(!set_route) {
		clear_route();
		return;
	}

	if(!(GetSignalFlags() & GSF::REPEATER)) {
		for(auto it = set_route->trackcircuits.begin(); it != set_route->trackcircuits.end(); ++it) {
			if((*it)->Occupied()) {
				clear_route();
				return;
			}
		}
		for(auto it = set_route->passtestlist.begin(); it != set_route->passtestlist.end(); ++it) {
			if(!it->location.track->IsTrackPassable(it->location.direction, it->connection_index)) {
				clear_route();
				return;
			}
		}
		if(set_route->end.track->GetFlags(set_route->end.direction) & GTF::SIGNAL) {
			genericsignal *gs = dynamic_cast<genericsignal*>(set_route->end.track);
			if(gs) {
				const route *rt = gs->GetCurrentForwardOverlap();
				if(rt) {
					for(auto it = rt->trackcircuits.begin(); it != rt->trackcircuits.end(); ++it) {
						if((*it)->Occupied()) {
							clear_route();
							return;
						}
					}
				}
			}
		}
	}

	aspect_type = set_route->type;
	routingpoint *aspect_target = set_route->end.track;
	routingpoint *aspect_route_target = set_route->end.track;

	sig_list::const_iterator next_repeater;
	if(set_route->start.track == this) {
		next_repeater = set_route->repeatersignals.begin();
	}
	else {
		sig_list::const_iterator check_current_repeater = std::find(set_route->repeatersignals.begin(), set_route->repeatersignals.end(), this);
		if(check_current_repeater != set_route->repeatersignals.end()) {
			next_repeater = std::next(check_current_repeater, 1);
		}
		else next_repeater = set_route->repeatersignals.end();
	}
	if(next_repeater != set_route->repeatersignals.end()) {
		aspect_target = *next_repeater;
	}

	if(set_route->type == RTC_SHUNT) aspect = std::min((unsigned int) 1, max_aspect);
	else {
		if(max_aspect <= 1) aspect = max_aspect;
		else {
			aspect_target->UpdateRoutingPoint();
			aspect = std::min(max_aspect, aspect_target->GetAspect() + 1);
		}
	}
	SetAspectNextTarget(aspect_target);
	SetAspectRouteTarget(aspect_route_target);
}

//this will not return the overlap, only the "real" route
const route *genericsignal::GetCurrentForwardRoute() const {
	const route *output = 0;

	auto route_fetch = [&](const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) {
		if(reserved_route && reserved_route->type != RTC_OVERLAP && reserved_route->type != RTC_NULL) output = reserved_route;
	};

	start_trs.ReservationEnumerationInDirection(EDGE_FRONT, route_fetch);
	return output;
}

//this will only return the overlap, not the "real" route
const route *genericsignal::GetCurrentForwardOverlap() const {
	const route *output = 0;

	auto route_fetch = [&](const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) {
		if(reserved_route && reserved_route->type == RTC_OVERLAP) output = reserved_route;
	};

	start_trs.ReservationEnumerationInDirection(EDGE_FRONT, route_fetch);
	return output;
}

void genericsignal::EnumerateCurrentBackwardsRoutes(std::function<void (const route *)> func) const {
	auto enumfunc = [&](const route *rt, EDGETYPE direction, unsigned int index, RRF rr_flags) {
		if(rr_flags & RRF::ENDPIECE) func(rt);
	};
	end_trs.ReservationEnumerationInDirection(EDGE_FRONT, enumfunc);
}

bool genericsignal::RepeaterAspectMeaningfulForRouteType(ROUTE_CLASS type) const {
	if(type == RTC_SHUNT) return true;
	else if(type == RTC_ROUTE) {
		if(GetSignalFlags() & GSF::REPEATER && GetSignalFlags() & GSF::ASPECTEDREPEATER) return true;
	}
	return false;
}

//function parameters: return true to continue, false to stop
void genericsignal::BackwardsReservedTrackScan(std::function<bool(const genericsignal*)> checksignal, std::function<bool(const track_target_ptr&)> checkpiece) const {
	const genericsignal *current = this;
	const genericsignal *next = 0;
	bool stop_tracing  = false;

	auto routecheckfunc = [&](const route *rt) {
		for(auto it = rt->pieces.rbegin(); it != rt->pieces.rend(); ++it) {
			if(!(it->location.track->GetFlags(it->location.direction) & GTF::ROUTETHISDIR)) {
				stop_tracing = true;
				return;	//route not reserved, stop tracing
			}
			if(it->location.track == next) {
				//found a repeater signal
				current = next;
				next = dynamic_cast<genericsignal*>(current->GetAspectBackwardsDependency());
				if(!checksignal(current)) {
					stop_tracing = true;
					return;
				}
			}
			else if(!checkpiece(it->location)) return;
		}
	};
	do {
		if(!checksignal(current)) return;
		next = dynamic_cast<genericsignal*>(current->GetAspectBackwardsDependency());

		current->EnumerateCurrentBackwardsRoutes(routecheckfunc);

		current = next;
	} while(current && !stop_tracing);
}

bool genericsignal::IsOverlapSwingPermitted(std::string *failreasonkey) const {
	if(!(GetSignalFlags() & genericsignal::GSF::OVERLAPSWINGABLE)) {
		if(failreasonkey) *failreasonkey = "track/reservation/overlapcantswing/notpermitted";
		return false;
	}

	bool occupied = false;
	unsigned int signalcount = 0;

	auto checksignal = [&](const genericsignal *targ) -> bool {
		signalcount++;
		return signalcount <= GetOverlapMinAspectDistance();
	};

	auto checkpiece = [&](const track_target_ptr &piece) -> bool {
		track_circuit *tc = piece.track->GetTrackCircuit();
		if(tc && tc->Occupied()) {
			occupied = true;		//found an occupied piece, train is on approach
			return false;
		}
		else return true;
	};
	BackwardsReservedTrackScan(checksignal, checkpiece);

	if(occupied) {
		if(failreasonkey) *failreasonkey = "track/reservation/overlapcantswing/trainapproaching";
		return 0;
	}

	return true;
}

unsigned int genericsignal::GetTRSList(std::vector<track_reservation_state *> &outputlist) {
	outputlist.push_back(&start_trs);
	outputlist.push_back(&end_trs);
	return 2;
}

GTF autosignal::GetFlags(EDGETYPE direction) const {
	GTF result = GTF::ROUTINGPOINT | start_trs.GetGTReservationFlags(direction);
	if(direction == EDGE_FRONT) result |= GTF::SIGNAL;
	return result;
}

route *autosignal::GetRouteByIndex(unsigned int index) {
	if(index == 0) return &signal_route;
	else if(index == 1) return &overlap_route;
	else return 0;
}

void autosignal::EnumerateRoutes(std::function<void (const route *)> func) const {
	func(&signal_route);
	if(overlap_route.type == RTC_OVERLAP) func(&overlap_route);
};

GTF routesignal::GetFlags(EDGETYPE direction) const {
	GTF result = GTF::ROUTINGPOINT | start_trs.GetGTReservationFlags(direction);
	if(direction == EDGE_FRONT) result |= GTF::SIGNAL;
	return result;
}

route *routesignal::GetRouteByIndex(unsigned int index) {
	for(auto it = signal_routes.begin(); it != signal_routes.end(); ++it) {
		if(it->index == index) return &(*it);
	}
	return 0;
}

void routesignal::EnumerateRoutes(std::function<void (const route *)> func) const {
	for(auto it = signal_routes.begin(); it != signal_routes.end(); ++it) {
		func(&*it);
	}
};

struct signal_route_recording_state : public generic_route_recording_state {

	enum class RRRSF {
		ZERO		= 0,
		SHUNTOK		= 1<<0,
		ROUTEOK		= 1<<1,
		OVERLAPOK	= 1<<2,

		SCANTYPES	= SHUNTOK | ROUTEOK | OVERLAPOK,
	};
	RRRSF rrrs_flags;

	virtual ~signal_route_recording_state() { }
	signal_route_recording_state() : rrrs_flags(RRRSF::ZERO) { }
	virtual signal_route_recording_state *Clone() const {
		signal_route_recording_state *rrrs = new signal_route_recording_state;
		rrrs->rrrs_flags = rrrs_flags;
		return rrrs;
	}
};
template<> struct enum_traits< signal_route_recording_state::RRRSF > {	static constexpr bool flags = true; };

bool autosignal::PostLayoutInit(error_collection &ec) {
	if(! genericsignal::PostLayoutInit(ec)) return false;

	auto mkroutefunc = [&](ROUTE_CLASS type, const track_target_ptr &piece) -> route* {
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
			ec.RegisterNewError<error_signalinit_trackscan>(*this, piece, "Autosignals support route and overlap route types only");
			return 0;
		}

		if(candidate->type != RTC_NULL) {
			ec.RegisterNewError<error_signalinit_trackscan>(*this, piece, "Autosignal already has a route of the corresponding type");
			return 0;
		}
		else {
			candidate->type = type;
			return candidate;
		}
	};

	bool scanresult = PostLayoutInitTrackScan(ec, 100, 0, 0, mkroutefunc);

	if(scanresult && signal_route.type == RTC_ROUTE) {
		if(signal_route.RouteReservation(RRF::AUTOROUTE | RRF::TRYRESERVE)) {
			signal_route.RouteReservation(RRF::AUTOROUTE | RRF::RESERVE);

			genericsignal *end_signal = dynamic_cast<genericsignal *>(signal_route.end.track);
			if(end_signal && ! (end_signal->GetSignalFlags() & GSF::NOOVERLAP)) {	//reserve an overlap beyond the end signal too if needed
				end_signal->PostLayoutInit(ec);					//make sure that the end piece is inited
				const route *best_overlap = end_signal->FindBestOverlap();
				if(best_overlap && best_overlap->RouteReservation(RRF::AUTOROUTE | RRF::TRYRESERVE)) {
					best_overlap->RouteReservation(RRF::AUTOROUTE | RRF::RESERVE);
				}
				else {
					ec.RegisterNewError<error_signalinit>(*this, "Autosignal route cannot reserve overlap");
					return false;
				}
			}
		}
		else {
			ec.RegisterNewError<error_signalinit>(*this, "Autosignal crosses reserved route");
			return false;
		}
	}
	else {
		ec.RegisterNewError<error_signalinit>(*this, "Track scan found no route");
		return false;
	}
	return true;
}

bool routesignal::PostLayoutInit(error_collection &ec) {
	if(! genericsignal::PostLayoutInit(ec)) return false;

	unsigned int route_index = 0;
	return PostLayoutInitTrackScan(ec, 100, 10, &restrictions, [&](ROUTE_CLASS type, const track_target_ptr &piece) -> route* {
		this->signal_routes.emplace_back();
		route *rt = &this->signal_routes.back();
		rt->index = route_index;
		rt->type = type;
		route_index++;
		return rt;
	});
}

bool genericsignal::PostLayoutInitTrackScan(error_collection &ec, unsigned int max_pieces, unsigned int junction_max, route_restriction_set *restrictions, std::function<route*(ROUTE_CLASS type, const track_target_ptr &piece)> mkblankroute) {
	bool continue_initing = true;

	bool foundoverlap = false;

	auto func = [&](const route_recording_list &route_pieces, const track_target_ptr &piece, generic_route_recording_state *grrs) {
		signal_route_recording_state *rrrs = static_cast<signal_route_recording_state *>(grrs);

		GTF pieceflags = piece.track->GetFlags(piece.direction);
		if(pieceflags & GTF::ROUTINGPOINT) {
			routingpoint *target_routing_piece = dynamic_cast<routingpoint *>(piece.track);
			if(!target_routing_piece) {
				ec.RegisterNewError<error_signalinit_trackscan>(*this, piece, "Track piece claims to be routingpoint but not downcastable");
				continue_initing = false;
				return true;
			}

			RPRT availableroutetypes = target_routing_piece->GetAvailableRouteTypes(piece.direction);
			if(availableroutetypes & (RPRT::ROUTEEND | RPRT::SHUNTEND | RPRT::OVERLAPEND)) {
				if(target_routing_piece->GetSetRouteTypes(piece.direction) & (RPRT::ROUTEEND | RPRT::SHUNTEND | RPRT::VIA)) {
					ec.RegisterNewError<error_signalinit_trackscan>(*this, piece, "Signal route already reserved");
					continue_initing = false;
					return true;
				}

				std::vector<const route_restriction*> matching_restrictions;
				route_restriction::RRDF restriction_denyflags;
				if(restrictions) restriction_denyflags = restrictions->CheckAllRestrictions(matching_restrictions, route_pieces, piece);
				else restriction_denyflags = route_restriction::RRDF::ZERO;

				auto mk_route = [&](ROUTE_CLASS type) {
					route *rt = mkblankroute(type, piece);
					if(rt) {
						rt->start = vartrack_target_ptr<routingpoint>(this, EDGE_FRONT);
						rt->pieces = route_pieces;
						rt->end = vartrack_target_ptr<routingpoint>(target_routing_piece, piece.direction);
						rt->FillLists();
						rt->parent = this;

						if(type == RTC_ROUTE) {
							genericsignal *rt_sig = dynamic_cast<genericsignal *>(target_routing_piece);
							if(rt_sig && !(rt_sig->GetSignalFlags()&GSF::NOOVERLAP)) rt->routeflags |= route::RF::NEEDOVERLAP;
						}

						for(auto it = matching_restrictions.begin(); it != matching_restrictions.end(); ++it) {
							(*it)->ApplyRestriction(*rt);
						}
					}
					else {
						ec.RegisterNewError<error_signalinit_trackscan>(*this, piece, "Unable to make new route of type: " + std::string(SerialiseRouteType(type)));
					}
				};

				if(rrrs->rrrs_flags & signal_route_recording_state::RRRSF::ROUTEOK && availableroutetypes & RPRT::ROUTEEND && !(restriction_denyflags & route_restriction::RRDF::NOROUTE)) mk_route(RTC_ROUTE);
				if(rrrs->rrrs_flags & signal_route_recording_state::RRRSF::SHUNTOK && availableroutetypes & RPRT::SHUNTEND && !(restriction_denyflags & route_restriction::RRDF::NOSHUNT)) mk_route(RTC_SHUNT);
				if(rrrs->rrrs_flags & signal_route_recording_state::RRRSF::OVERLAPOK && availableroutetypes & RPRT::OVERLAPEND && !(restriction_denyflags & route_restriction::RRDF::NOOVERLAP)) {
					mk_route(RTC_OVERLAP);
					rrrs->rrrs_flags &= ~signal_route_recording_state::RRRSF::OVERLAPOK;	//don't look for more overlap ends beyond the end of the first
					foundoverlap = true;
				}
			}

			if(! (availableroutetypes & RPRT::SHUNTTRANS)) rrrs->rrrs_flags &= ~signal_route_recording_state::RRRSF::SHUNTOK;
			if(! (availableroutetypes & RPRT::ROUTETRANS)) rrrs->rrrs_flags &= ~signal_route_recording_state::RRRSF::ROUTEOK;
			if(! (availableroutetypes & RPRT::OVERLAPTRANS)) rrrs->rrrs_flags &= ~signal_route_recording_state::RRRSF::OVERLAPOK;

			if(! (rrrs->rrrs_flags & signal_route_recording_state::RRRSF::SCANTYPES)) return true;	//nothing left to scan for
		}
		if(pieceflags & GTF::ROUTESET) {
			ec.RegisterNewError<error_signalinit_trackscan>(*this, piece, "Piece already reserved");
			continue_initing = false;
			return true;
		}
		return false;
	};

	TSEF error_flags = TSEF::ZERO;
	route_recording_list pieces;
	signal_route_recording_state rrrs;
	bool needoverlap = false;
	if(GetAvailableRouteTypes(EDGE_FRONT) & RPRT::ROUTESTART) rrrs.rrrs_flags |= signal_route_recording_state::RRRSF::ROUTEOK;
	if(GetAvailableRouteTypes(EDGE_FRONT) & RPRT::ROUTEEND && ! (GetSignalFlags() & GSF::NOOVERLAP)) {
		rrrs.rrrs_flags |= signal_route_recording_state::RRRSF::OVERLAPOK;
		needoverlap = true;
	}
	if(GetAvailableRouteTypes(EDGE_FRONT) & RPRT::SHUNTSTART) rrrs.rrrs_flags |= signal_route_recording_state::RRRSF::SHUNTOK;
	TrackScan(max_pieces, junction_max, GetConnectingPieceByIndex(EDGE_FRONT, 0), pieces, &rrrs, error_flags, func);

	if(error_flags != TSEF::ZERO) {
		continue_initing = false;
		ec.RegisterNewError<error_signalinit>(*this, std::string("Track scan failed constraints: ") + GetTrackScanErrorFlagsStr(error_flags));
	}
	if(needoverlap && !foundoverlap) {
		continue_initing = false;
		ec.RegisterNewError<error_signalinit>(*this, "No overlap found for signal");
	}
	return continue_initing;
}

//returns false on failure
bool route::RouteReservation(RRF reserve_flags, std::string *failreasonkey) const {
	if(!start.track->Reservation(start.direction, 0, reserve_flags | RRF::STARTPIECE, this, failreasonkey)) return false;

	for(auto it = pieces.begin(); it != pieces.end(); ++it) {
		if(!it->location.track->Reservation(it->location.direction, it->connection_index, reserve_flags, this, failreasonkey)) return false;
	}

	if(!end.track->Reservation(end.direction, 0, reserve_flags | RRF::ENDPIECE, this, failreasonkey)) return false;
	return true;
}

void route::RouteReservationActions(RRF reserve_flags, std::function<void(action &&reservation_act)> actioncallback) const {
	start.track->ReservationActions(start.direction, 0, reserve_flags | RRF::STARTPIECE, this, actioncallback);

	for(auto it = pieces.begin(); it != pieces.end(); ++it) {
		it->location.track->ReservationActions(it->location.direction, it->connection_index, reserve_flags, this, actioncallback);
	}

	end.track->ReservationActions(end.direction, 0, reserve_flags | RRF::ENDPIECE, this, actioncallback);
}

void route::FillLists() {
	track_circuit *last_tc = 0;
	for(auto it = pieces.begin(); it != pieces.end(); ++it) {
		track_circuit *this_tc = it->location.track->GetTrackCircuit();
		if(this_tc && this_tc != last_tc) {
			last_tc = this_tc;
			trackcircuits.push_back(this_tc);
		}
		routingpoint *target_routing_piece = dynamic_cast<routingpoint *>(it->location.track);
		if(target_routing_piece && target_routing_piece->GetAvailableRouteTypes(it->location.direction) & RPRT::VIA) {
			vias.push_back(target_routing_piece);
		}
		if(target_routing_piece && target_routing_piece->GetFlags(it->location.direction) & GTF::SIGNAL) {
			genericsignal *this_signal = dynamic_cast<genericsignal *>(target_routing_piece);
			if(this_signal && this_signal->RepeaterAspectMeaningfulForRouteType(type)) repeatersignals.push_back(this_signal);
		}
		if(!it->location.track->IsTrackAlwaysPassable()) {
			passtestlist.push_back(*it);
		}
	}
}

bool route::TestRouteForMatch(const routingpoint *checkend, const via_list &checkvias) const {
	return checkend == end.track && checkvias == vias;
}

bool route::IsRouteSubSet(const route *subset) const {
	const vartrack_target_ptr<routingpoint> &substart = subset->start;

	auto this_it = pieces.begin();
	if(substart != start) {		//scan along route for start
		while(true) {
			if(this_it->location == substart) break;
			++this_it;
			if(this_it == pieces.end()) return false;
		}
		++this_it;
	}

	auto sub_it = subset->pieces.begin();

	//sub_it and this_it should now point to same track piece if route is a subset

	for(; sub_it != subset->pieces.end() && this_it != pieces.end(); ++sub_it, ++this_it) {
		if(*this_it != *sub_it) return false;
	}

	if(sub_it == subset->pieces.end()) {
		if(this_it == pieces.end()) return subset->end == end;
		else return subset->end == this_it->location;
	}
	else {
		return false;
	}
}

//return true if restriction applies
bool route_restriction::CheckRestriction(RRDF &restriction_flags, const route_recording_list &route_pieces, const track_target_ptr &piece) const {
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
	if(routerestrictionflags & RRF::PRIORITYSET) rt.priority = priority;
}

route_restriction::RRDF route_restriction_set::CheckAllRestrictions(std::vector<const route_restriction*> &matching_restrictions, const route_recording_list &route_pieces, const track_target_ptr &piece) const {
	route_restriction::RRDF restriction_flags = route_restriction::RRDF::ZERO;
	for(auto it = restrictions.begin(); it != restrictions.end(); ++it) {
		if(it->CheckRestriction(restriction_flags, route_pieces, piece)) matching_restrictions.push_back(&(*it));
	}
	return restriction_flags;
}

const track_target_ptr& startofline::GetEdgeConnectingPiece(EDGETYPE edgeid) const {
	switch(edgeid) {
		case EDGE_FRONT:
			return connection;
		default:
			assert(false);
			return empty_track_target;
	}
}

const track_target_ptr & startofline::GetConnectingPiece(EDGETYPE direction) const {
	switch(direction) {
		case EDGE_FRONT:
			return empty_track_target;
		case EDGE_BACK:
			return connection;
		default:
			assert(false);
			return empty_track_target;
	}
}

EDGETYPE startofline::GetReverseDirection(EDGETYPE direction) const {
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

unsigned int startofline::GetMaxConnectingPieces(EDGETYPE direction) const {
	return 1;
}

const track_target_ptr & startofline::GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const {
	return GetConnectingPiece(direction);
}

bool startofline::IsEdgeValid(EDGETYPE edge) const {
	switch(edge) {
		case EDGE_FRONT:
			return true;
		default:
			return false;
	}
}

EDGETYPE startofline::GetAvailableAutoConnectionDirection(bool forwardconnection) const {
	if(forwardconnection && !connection.IsValid()) return EDGE_FRONT;
	return EDGE_NULL;
}

void startofline::GetListOfEdges(std::vector<edgelistitem> &outputlist) const {
	outputlist.insert(outputlist.end(), { edgelistitem(EDGE_FRONT, connection) });
}

bool startofline::Reservation(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey) {
	if(rr_flags & RRF::STARTPIECE && direction == EDGE_BACK) {
		return trs.Reservation(direction, index, rr_flags, resroute);
	}
	else if(rr_flags & RRF::ENDPIECE && direction == EDGE_FRONT) {
		return trs.Reservation(direction, index, rr_flags, resroute);
	}
	else {
		return false;
	}
}

GTF startofline::GetFlags(EDGETYPE direction) const {
	return GTF::ROUTINGPOINT | trs.GetGTReservationFlags(direction);
}

RPRT startofline::GetAvailableRouteTypes(EDGETYPE direction) const {
	return (direction == EDGE_FRONT) ? availableroutetypes : RPRT::ZERO;
}

RPRT startofline::GetSetRouteTypes(EDGETYPE direction) const {
	RPRT result = RPRT::ZERO;

	auto result_flag_adds = [&](const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) {
		if(reserved_route) result = result | (GetRouteClassRPRTMask(reserved_route->type) & GetTRSFlagsRPRTMask(rr_flags) & RPRT::MASK_END);
	};

	if(direction == EDGE_FRONT) {
		trs.ReservationEnumerationInDirection(EDGE_FRONT, result_flag_adds);
	}
	return result;
}

EDGETYPE endofline::GetAvailableAutoConnectionDirection(bool forwardconnection) const {
	return startofline::GetAvailableAutoConnectionDirection(!forwardconnection);
}

GTF routingmarker::GetFlags(EDGETYPE direction) const {
	return GTF::ROUTINGPOINT | trs.GetGTReservationFlags(direction);
}

bool routingmarker::Reservation(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey) {
	return trs.Reservation(direction, index, rr_flags, resroute);
}

RPRT routingmarker::GetAvailableRouteTypes(EDGETYPE direction) const {
	return (direction == EDGE_FRONT) ? availableroutetypes_forward : availableroutetypes_reverse;
}

RPRT routingmarker::GetSetRouteTypes(EDGETYPE direction) const {
	RPRT result = RPRT::ZERO;

	auto result_flag_adds = [&](const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) {
		if(reserved_route) result = result | (GetRouteClassRPRTMask(reserved_route->type) & GetTRSFlagsRPRTMask(rr_flags));
	};

	trs.ReservationEnumerationInDirection(direction, result_flag_adds);
	return result;
}

const char * SerialiseRouteType(const ROUTE_CLASS& type) {
	switch(type) {
		case RTC_NULL: return "Null Route Type";
		case RTC_SHUNT: return "Shunt";
		case RTC_ROUTE: return "Route";
		case RTC_OVERLAP: return "Overlap";
		default: return "Unknown Route Type";
	}
}