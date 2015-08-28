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

#include "common.h"
#include "core/track_reservation.h"
#include "core/signal.h"
#include "core/world.h"
#include "core/track_circuit.h"
#include "core/track_ops.h"
#include "core/param.h"
#include "core/route_types.h"
#include "util/util.h"

#include <algorithm>
#include <iterator>
#include <cassert>
#include <initializer_list>

class error_signalinit : public layout_initialisation_error_obj {
	public:
	error_signalinit(const generic_signal &sig, const std::string &str = "") {
		msg << "Signal Initialisation Error for " << sig << ": " << str;
	}
};

class error_signalinit_trackscan : public error_signalinit {
	public:
	error_signalinit_trackscan(const generic_signal &sig, const track_target_ptr &piece, const std::string &str = "")
			: error_signalinit(sig) {
		msg << "Track Scan: Piece: " << piece << ": " << str;
	}
};

void routing_point::EnumerateRoutes(std::function<void (const route *)> func) const { }

const route *routing_point::FindBestOverlap(route_class::set types) const {
	int best_score = INT_MIN;
	const route *best_overlap = nullptr;
	EnumerateAvailableOverlaps([&](const route *r, int score) {
		if (score > best_score) {
			best_score = score;
			best_overlap = r;
		}
	}, RRF::ZERO, types);
	return best_overlap;
}

void routing_point::EnumerateAvailableOverlaps(std::function<void(const route *rt, int score)> func, RRF extra_flags, route_class::set types) const {

	auto overlap_finder = [&](const route *r) {
		if (!route_class::IsOverlap(r->type)) {
			return;
		}
		if (!(types & route_class::Flag(r->type))) {
			return;
		}
		if (!r->RouteReservation(RRF::TRY_RESERVE | extra_flags)) {
			return;
		}

		int score = r->priority;
		auto action_counter = [&](action &&reservation_act) {
			score -= 10;
		};
		r->RouteReservationActions(RRF::RESERVE | extra_flags, action_counter);
		func(r, score);
	};
	EnumerateRoutes(overlap_finder);
}

unsigned int routing_point::GetMatchingRoutes(std::vector<gmr_route_item> &routes, const routing_point *end, route_class::set rc_mask, GMRF gmr_flags, RRF extra_flags, const via_list &vias) const {
	unsigned int count = 0;
	if (!(gmr_flags & GMRF::DONT_CLEAR_VECTOR)) {
		routes.clear();
	}
	auto route_finder = [&](const route *r) {
		if (!(route_class::Flag(r->type) & rc_mask)) {
			return;
		}
		if (gmr_flags & GMRF::CHECK_VIAS && r->vias != vias) {
			return;
		}
		if (end && r->end.track != end) {
			return;
		}

		int score = r->priority;
		if (gmr_flags & GMRF::CHECK_TRY_RESERVE) {
			if (!r->RouteReservation(RRF::TRY_RESERVE | extra_flags)) {
				return;
			}
		}
		if (gmr_flags & GMRF::DYNAMIC_PRIORITY) {
			auto action_counter = [&](action &&reservation_act) {
				score -= 10;
			};
			r->RouteReservationActions(RRF::RESERVE | extra_flags, action_counter);

			if (route_class::PreferWhenReservingIfAlreadyOccupied(r->type)) {
				for (auto &it : r->track_circuits) {
					if (it->Occupied()) {
						score += 10;
						break;
					}
				}
			}
		}
		gmr_route_item gmr;
		gmr.rt = r;
		gmr.score = score;
		routes.push_back(gmr);
		count++;

	};
	EnumerateRoutes(route_finder);
	auto sortfunc = [&](const gmr_route_item &a, const gmr_route_item &b) -> bool {
		if (a.score > b.score) return true;
		if (a.score < b.score) return false;
		if (route_class::IsRoute(a.rt->type) && route_class::IsShunt(b.rt->type)) return true;
		if (route_class::IsShunt(a.rt->type) && route_class::IsRoute(b.rt->type)) return false;
		if (a.rt->pieces.size() < b.rt->pieces.size()) return true;
		if (a.rt->pieces.size() > b.rt->pieces.size()) return false;
		if (a.rt->index < b.rt->index) return true;
		return false;
	};
	if (!(gmr_flags & GMRF::DONT_SORT)) {
		std::sort(routes.begin(), routes.end(), sortfunc);
	}
	return count;
}

void routing_point::SetAspectNextTarget(routing_point *target) {
	if (target == aspect_target) {
		return;
	}
	if (aspect_target && aspect_target->aspect_backwards_dependency == this) {
		aspect_target->aspect_backwards_dependency = 0;
	}
	if (target) {
		target->aspect_backwards_dependency = this;
	}
	aspect_target = target;
}

void routing_point::SetAspectRouteTarget(routing_point *target) {
	aspect_route_target = target;
}

generic_track::edge_track_target track_routing_point::GetEdgeConnectingPiece(EDGE edgeid) {
	switch (edgeid) {
		case EDGE::FRONT:
			return prev;

		case EDGE::BACK:
			return next;

		default:
			assert(false);
			return empty_track_target;
	}
}

generic_track::edge_track_target track_routing_point::GetConnectingPiece(EDGE direction) {
	switch (direction) {
		case EDGE::FRONT:
			return next;

		case EDGE::BACK:
			return prev;

		default:
			assert(false);
			return empty_track_target;
	}
}

EDGE track_routing_point::GetReverseDirection(EDGE direction) const {
	switch (direction) {
		case EDGE::FRONT:
			return EDGE::BACK;

		case EDGE::BACK:
			return EDGE::FRONT;

		default:
			assert(false);
			return EDGE::INVALID;
	}
}

bool track_routing_point::IsEdgeValid(EDGE edge) const {
	switch (edge) {
		case EDGE::FRONT:
		case EDGE::BACK:
			return true;

		default:
			return false;
	}
}

unsigned int track_routing_point::GetMaxConnectingPieces(EDGE direction) const {
	return 1;
}

generic_track::edge_track_target track_routing_point::GetConnectingPieceByIndex(EDGE direction, unsigned int index) {
	return GetConnectingPiece(direction);
}

EDGE track_routing_point::GetAvailableAutoConnectionDirection(bool forward_connection) const {
	if (forward_connection && !next.IsValid()) {
		return EDGE::BACK;
	}
	if (!forward_connection && !prev.IsValid()) {
		return EDGE::FRONT;
	}
	return EDGE::INVALID;
}

void track_routing_point::GetListOfEdges(std::vector<edgelistitem> &output_list) const {
	output_list.insert(output_list.end(), { edgelistitem(EDGE::BACK, next), edgelistitem(EDGE::FRONT, prev) });
}

RPRT track_routing_point::GetAvailableRouteTypes(EDGE direction) const {
	return (direction == EDGE::FRONT) ? available_route_types_forward : available_route_types_reverse;
}

generic_signal::generic_signal(world &w_) : track_routing_point(w_), sflags(GSF::ZERO) {
	available_route_types_reverse.through |= route_class::AllNonOverlaps();
	w_.RegisterTickUpdate(this);
	sighting_distances.emplace_back(EDGE::FRONT, SIGHTING_DISTANCE_SIG);
	std::copy(route_class::default_approach_locking_timeouts.begin(), route_class::default_approach_locking_timeouts.end(),
			approach_locking_default_timeouts.begin());
}

generic_signal::~generic_signal() {
	GetWorld().UnregisterTickUpdate(this);
}

GSF generic_signal::GetSignalFlags() const {
	return sflags;
}

GSF generic_signal::SetSignalFlagsMasked(GSF set_flags, GSF mask_flags) {
	sflags = (sflags & (~mask_flags)) | set_flags;
	return sflags;
}

bool generic_signal::ReservationV(EDGE direction, unsigned int index, RRF rr_flags, const route *res_route, std::string* fail_reason_key) {
	if (direction != EDGE::FRONT && rr_flags & (RRF::START_PIECE | RRF::END_PIECE)) {
		return false;
	}
	if (rr_flags & RRF::START_PIECE) {
		return start_trs.Reservation(direction, index, rr_flags, res_route);
	} else if (rr_flags & RRF::END_PIECE) {
		bool result = end_trs.Reservation(direction, index, rr_flags, res_route);
		if (rr_flags & RRF::UNRESERVE) {
			GetWorld().ExecuteIfActionScope([&]() {
				const route* ovlp = GetCurrentForwardOverlap();
				if (ovlp) {
					GetWorld().SubmitAction(action_unreserve_track_route(GetWorld(), *ovlp));
				}
			});
		}
		return result;
	} else {
		return start_trs.Reservation(direction, index, rr_flags, res_route) && end_trs.Reservation(direction, index, rr_flags, res_route);
	}
}

RPRT generic_signal::GetSetRouteTypes(EDGE direction) const {
	RPRT result;

	auto result_flag_adds = [&](const route *reserved_route, EDGE direction, unsigned int index, RRF rr_flags) {
		if (reserved_route) {
			result.GetRCSByRRF(rr_flags) |= route_class::Flag(reserved_route->type);
		}
	};

	if (direction == EDGE::FRONT) {
		start_trs.ReservationEnumerationInDirection(EDGE::FRONT, result_flag_adds);
		end_trs.ReservationEnumerationInDirection(EDGE::FRONT, result_flag_adds);
	} else {
		start_trs.ReservationEnumerationInDirection(EDGE::BACK, result_flag_adds);
	}
	return result;
}

void generic_signal::UpdateSignalState() {
	if (last_state_update == GetWorld().GetGameTime()) {
		return;
	}

	last_state_update = GetWorld().GetGameTime();

	unsigned int previous_aspect = aspect;
	const routing_point *previous_aspect_target = GetAspectNextTarget();
	const routing_point *previous_aspect_route_target = GetAspectRouteTarget();

	auto check_aspect_change = [&]() {
		if (aspect != previous_aspect ||
				previous_aspect_target != GetAspectNextTarget() ||
				previous_aspect_route_target != GetAspectRouteTarget()) {
			MarkUpdated();
		}
	};

	auto clear_route_base = [&]() {
		aspect = 0;
		reserved_aspect = 0;
		SetAspectNextTarget(nullptr);
		SetAspectRouteTarget(nullptr);
		aspect_type = route_class::RTC_NULL;
		check_aspect_change();
	};

	auto set_clear_time = [&]() {
		if (last_route_clear_time == 0) {
			last_route_clear_time = last_state_update;
		}
	};

	auto set_prove_time = [&]() {
		if (last_route_prove_time == 0) {
			last_route_prove_time = last_state_update;
		}
	};

	auto clear_route = [&]() {
		last_route_prove_time = 0;
		last_route_clear_time = 0;
		clear_route_base();
	};

	auto clear_route_occ = [&]() {
		set_prove_time();
		last_route_clear_time = 0;
		clear_route_base();
	};

	auto clear_route_noprove = [&]() {
		last_route_prove_time = 0;
		last_route_clear_time = 0;
		clear_route_base();
	};

	auto clear_route_notrigger = [&]() {
		set_prove_time();
		set_clear_time();
		clear_route_base();
	};

	const route *own_overlap = GetCurrentForwardOverlap();
	bool overlap_timeout_set = false;
	if (own_overlap && own_overlap->overlap_timeout) {
		bool can_timeout_overlap = false;
		bool start_anchored = false;
		if (own_overlap->overlap_timeout_trigger) {
			if (own_overlap->overlap_timeout_trigger->Occupied()) {
				can_timeout_overlap = true;
			}
		}
		EnumerateCurrentBackwardsRoutes([&](const route *r) {
			if (r->IsStartAnchored()) {
				start_anchored = true;
			}
			if (!own_overlap->overlap_timeout_trigger && !r->track_circuits.empty()) {
				if (r->track_circuits.back()->Occupied()) {
					can_timeout_overlap = true;
				}
			}
		});
		if (can_timeout_overlap && !start_anchored) {
			if (GetSignalFlags() & GSF::OVERLAP_TIMEOUT_STARTED) {
				if (overlap_timeout_start + own_overlap->overlap_timeout <= GetWorld().GetGameTime()) {
					//overlap has timed out
					if (GetWorld().IsAuthoritative()) {
						GetWorld().SubmitAction(action_unreserve_track_route(GetWorld(), *own_overlap));
					}
				}
			} else {
				overlap_timeout_start = GetWorld().GetGameTime();
				SetSignalFlagsMasked(GSF::OVERLAP_TIMEOUT_STARTED, GSF::OVERLAP_TIMEOUT_STARTED);
			}
			overlap_timeout_set = true;
		}
	}
	if (!overlap_timeout_set) {
		SetSignalFlagsMasked(GSF::ZERO, GSF::OVERLAP_TIMEOUT_STARTED);
		overlap_timeout_start = 0;
	}

	const route *set_route = GetCurrentForwardRoute();
	if (!set_route) {
		last_route_set_time = 0;
		clear_route();
		return;
	}
	if (last_route_set_time == 0) {
		last_route_set_time = last_state_update;
	}

	if (!(GetSignalFlags() & GSF::REPEATER) && !(sflags & GSF::APPROACH_LOCKING_MODE)) {
		for (auto &it : set_route->pass_test_list) {
			if (!it.location.track->IsTrackPassable(it.location.direction, it.connection_index)) {
				clear_route_noprove();
				return;
			}
		}
		if (!route_class::AllowEntryWhilstOccupied(set_route->type)) {
			for (auto &it : set_route->track_circuits) {
				if (it->Occupied()) {
					clear_route_occ();
					return;
				}
			}
			if (set_route->end.track->GetFlags(set_route->end.direction) & GTF::SIGNAL) {
				generic_signal *gs = static_cast<generic_signal*>(set_route->end.track);
				const route *rt = gs->GetCurrentForwardOverlap();
				if (rt) {
					for (auto &it : rt->track_circuits) {
						if (it->Occupied()) {
							clear_route_occ();
							return;
						}
					}
				}
			}
		}
	}

	if (set_route->route_common_flags & route::RCF::AP_CONTROL) {
		bool do_ap_control = false;

		if (set_route->route_common_flags & route::RCF::AP_CONTROL_IF_NOROUTE) {
			// Approach control if route target has no route set
			generic_signal *targ_sig = FastSignalCast(set_route->end.track, set_route->end.direction);
			if (targ_sig) {
				if (!targ_sig->GetCurrentForwardRoute()) {
					do_ap_control = true; // This is a signal with no route set
				}
			} else {
				do_ap_control = true; // This is not a signal, treat it as a signal with no route set
			}
		} else if (aspect == 0) {
			// Normal approach control, and signal is red
			do_ap_control = true;
		}

		if (do_ap_control) {
			bool can_trigger = false;

			auto test_ttcb = [&](track_train_counter_block *ttcb) {
				if (ttcb && ttcb->Occupied() && ttcb->GetLastOccupationStateChangeTime() +
						set_route->approach_control_triggerdelay <= GetWorld().GetGameTime()) {
					can_trigger = true;
				}
			};

			if (set_route->approach_control_trigger) {
				test_ttcb(set_route->approach_control_trigger);
			} else {
				EnumerateCurrentBackwardsRoutes([&](const route *rt) {
					if (rt->track_circuits.empty()) {
						return;
					}
					test_ttcb(rt->track_circuits.back());
				});
			}

			if (!can_trigger) {
				clear_route_notrigger();
				return;
			}
		}
	}

	aspect_type = set_route->type;
	routing_point *aspect_target = set_route->end.track;
	routing_point *aspect_route_target = set_route->end.track;

	sig_list::const_iterator next_repeater;
	if (set_route->start.track == this) {
		next_repeater = set_route->repeater_signals.begin();
	} else {
		sig_list::const_iterator check_current_repeater = std::find(set_route->repeater_signals.begin(), set_route->repeater_signals.end(), this);
		if (check_current_repeater != set_route->repeater_signals.end()) {
			next_repeater = std::next(check_current_repeater, 1);
		} else {
			next_repeater = set_route->repeater_signals.end();
		}
	}
	if (next_repeater != set_route->repeater_signals.end()) {
		aspect_target = *next_repeater;
	}

	unsigned int aspect_mask;
	const std::vector<route_common::conditional_aspect_mask> *cams;
	if (GetSignalFlags() & GSF::REPEATER) {
		aspect_mask = route_defaults.aspect_mask;
		cams = &(route_defaults.conditional_aspect_masks);
	} else {
		aspect_mask = set_route->aspect_mask;
		cams = &(set_route->conditional_aspect_masks);
	}

	if (route_class::IsAspectLimitedToUnity(set_route->type))
		aspect_mask &= 3; // This limits the aspect to no more than 1

	if (aspect_mask & ~3) {
		// Aspect allowed to go higher than 1, check what the route points to
		aspect_target->UpdateRoutingPoint();
		if (route_class::IsAspectDirectlyPropagatable(set_route->type, aspect_target->GetAspectType())) {
			aspect = aspect_target->GetAspect() + 1;
			reserved_aspect = aspect_target->GetReservedAspect() + 1;
		} else {
			reserved_aspect = aspect = 1;
		}
	} else {
		reserved_aspect = aspect = 1;
	}

	SetAspectNextTarget(aspect_target);
	SetAspectRouteTarget(aspect_route_target);
	set_clear_time();
	set_prove_time();

	if (sflags & GSF::APPROACH_LOCKING_MODE) {
		aspect = 0;
	}
	if (last_state_update - last_route_prove_time < set_route->route_prove_delay) {
		aspect = 0;
	}
	if (last_state_update - last_route_clear_time < set_route->route_clear_delay) {
		aspect = 0;
	}
	if (last_state_update - last_route_set_time < set_route->route_set_delay) {
		aspect = 0;
	}

	auto check_aspect_mask = [&](unsigned int &aspect, aspect_mask_type mask) {
		while (aspect) {
			if (aspect_in_mask(mask, aspect)) {
				break;                                       //aspect is in mask: OK
			}
			aspect--;                                        //aspect not in mask, try one lower
		}
	};
	check_aspect_mask(aspect, aspect_mask);
	check_aspect_mask(reserved_aspect, aspect_mask);
	for (auto &it : *cams) {
		aspect_target->UpdateRoutingPoint();
		if (aspect_in_mask(it.condition, aspect_target->GetAspect())) {
			check_aspect_mask(aspect, it.aspect_mask);
			check_aspect_mask(reserved_aspect, it.aspect_mask);
		}
	}

	check_aspect_change();
}

//this will not return the overlap, only the "real" route
const route *generic_signal::GetCurrentForwardRoute() const {
	const route *output = nullptr;

	auto route_fetch = [&](const route *reserved_route, EDGE direction, unsigned int index, RRF rr_flags) {
		if (reserved_route && !route_class::IsOverlap(reserved_route->type) && route_class::IsValid(reserved_route->type)) {
			output = reserved_route;
		}
	};

	start_trs.ReservationEnumerationInDirection(EDGE::FRONT, route_fetch);
	return output;
}

//this will only return the overlap, not the "real" route
const route *generic_signal::GetCurrentForwardOverlap() const {
	const route *output = nullptr;

	auto route_fetch = [&](const route *reserved_route, EDGE direction, unsigned int index, RRF rr_flags) {
		if (reserved_route && route_class::IsOverlap(reserved_route->type)) {
			output = reserved_route;
		}
	};

	start_trs.ReservationEnumerationInDirection(EDGE::FRONT, route_fetch);
	return output;
}

void generic_signal::EnumerateCurrentBackwardsRoutes(std::function<void (const route *)> func) const {
	auto enumfunc = [&](const route *rt, EDGE direction, unsigned int index, RRF rr_flags) {
		if (rr_flags & RRF::END_PIECE) {
			func(rt);
		}
	};
	end_trs.ReservationEnumerationInDirection(EDGE::FRONT, enumfunc);
}

bool generic_signal::RepeaterAspectMeaningfulForRouteType(route_class::ID type) const {
	if (route_class::IsShunt(type)) {
		return true;
	}
	else if (route_class::IsRoute(type)) {
		if (GetSignalFlags() & GSF::REPEATER && !(GetSignalFlags() & GSF::NON_ASPECTED_REPEATER)) {
			return true;
		}
	}
	return false;
}

//function parameters: return true to continue, false to stop
void generic_signal::BackwardsReservedTrackScan(std::function<bool(const generic_signal*)> checksignal, std::function<bool(const track_target_ptr&)> check_piece) const {
	const generic_signal *current = this;
	const generic_signal *next = 0;
	bool stop_tracing  = false;

	auto routecheckfunc = [&](const route *rt) {
		for (auto it = rt->pieces.rbegin(); it != rt->pieces.rend(); ++it) {
			if (!(it->location.track->GetFlags(it->location.direction) & GTF::ROUTE_THIS_DIR)) {
				stop_tracing = true;
				return;    //route not reserved, stop tracing
			}
			if (it->location.track == next) {
				//found a repeater signal
				current = next;
				next = FastSignalCast(current->GetAspectBackwardsDependency());
				if (!checksignal(current)) {
					stop_tracing = true;
					return;
				}
			} else if (!check_piece(it->location)) {
				return;
			}
		}
	};
	do {
		if (!checksignal(current)) {
			return;
		}
		next = FastSignalCast(current->GetAspectBackwardsDependency());

		current->EnumerateCurrentBackwardsRoutes(routecheckfunc);

		current = next;
	} while (current && !stop_tracing);
}

bool generic_signal::IsOverlapSwingPermitted(std::string *fail_reason_key) const {
	if (!(GetSignalFlags() & GSF::OVERLAP_SWINGABLE)) {
		if (fail_reason_key) {
			*fail_reason_key = "track/reservation/overlapcantswing/notpermitted";
		}
		return false;
	}

	bool occupied = false;
	unsigned int signalcount = 0;

	auto checksignal = [&](const generic_signal *targ) -> bool {
		signalcount++;
		return signalcount <= GetOverlapMinAspectDistance();
	};

	auto check_piece = [&](const track_target_ptr &piece) -> bool {
		track_circuit *tc = piece.track->GetTrackCircuit();
		if (tc && tc->Occupied()) {
			occupied = true;    //found an occupied piece, train is on approach
			return false;
		} else {
			return true;
		}
	};
	BackwardsReservedTrackScan(checksignal, check_piece);

	if (occupied) {
		if (fail_reason_key) {
			*fail_reason_key = "track/reservation/overlapcantswing/trainapproaching";
		}
		return false;
	}

	return true;
}

unsigned int generic_signal::GetTRSList(std::vector<track_reservation_state *> &output_list) {
	output_list.push_back(&start_trs);
	output_list.push_back(&end_trs);
	return 2;
}

ASPECT_FLAGS generic_signal::GetAspectFlags() const {
	return (sflags & GSF::NON_ASPECTED_REPEATER) ? ASPECT_FLAGS::MAX_NOT_BINDING : ASPECT_FLAGS::ZERO;
}

track_berth *generic_signal::GetPriorBerth(EDGE direction, routing_point::GPBF flags) {
	if (direction != EDGE::FRONT)
		return nullptr;
	track_berth *berth = nullptr;
	EnumerateCurrentBackwardsRoutes([&](const route *rt) {
		if (route_class::IsOverlap(rt->type)) {
			return;    // we don't want overlaps
		}
		if (!rt->berths.empty()) {
			if (flags & routing_point::GPBF::GET_NON_EMPTY) {
				for (auto &it : rt->berths) {
					if (!it.berth->contents.empty()) {
						berth = it.berth;    //find last non-empty berth on route
					}
				}
			} else {
				berth = rt->berths.back().berth;
			}
		}
	});
	if (berth) {
		return berth;
	}

	//not found anything, try previous track pieces if there
	route_recording_list pieces;
	TSEF errflags;
	TrackScan(100, 0, GetEdgeConnectingPiece(EDGE::FRONT), pieces, 0, errflags,
			[&](const route_recording_list &route_pieces, const track_target_ptr &piece, generic_route_recording_state *grrs) -> bool {
		if (FastSignalCast(piece.track)) {
			return true;    //this is a signal, stop here
		}
		if (piece.track->HasBerth(piece.track->GetReverseDirection(piece.direction))) {
			track_berth *b = piece.track->GetBerth();
			if (!(flags & routing_point::GPBF::GET_NON_EMPTY) || !b->contents.empty()) {
				berth = b;
				return true;
			}
		}
		return false;
	});
	return berth;
}

GTF generic_signal::GetFlags(EDGE direction) const {
	GTF result = GTF::ROUTING_POINT | start_trs.GetGTReservationFlags(direction);
	if (direction == EDGE::FRONT) {
		result |= GTF::SIGNAL;
	}
	return result;
}

std_signal::std_signal(world &w_) : generic_signal(w_) {
	available_route_types_forward.start |= route_class::AllOverlaps();
}

route *auto_signal::GetRouteByIndex(unsigned int index) {
	if (index == 0) {
		return &signal_route;
	} else if (index == 1) {
		return &overlap_route;
	} else {
		return nullptr;
	}
}

void auto_signal::EnumerateRoutes(std::function<void (const route *)> func) const {
	func(&signal_route);
	if (overlap_route.type == route_class::RTC_OVERLAP) {
		func(&overlap_route);
	}
};

route *route_signal::GetRouteByIndex(unsigned int index) {
	for (auto &it : signal_routes) {
		if (it.index == index) {
			return &it;
		}
	}
	return nullptr;
}

void route_signal::EnumerateRoutes(std::function<void (const route *)> func) const {
	for (auto &it : signal_routes) {
		func(&it);
	}
};

struct signal_route_recording_state : public generic_route_recording_state {
	route_class::set allowed_routeclasses;

	virtual ~signal_route_recording_state() { }
	signal_route_recording_state() : allowed_routeclasses(0) { }
	virtual signal_route_recording_state *Clone() const {
		signal_route_recording_state *rrrs = new signal_route_recording_state;
		rrrs->allowed_routeclasses = allowed_routeclasses;
		return rrrs;
	}
};

bool auto_signal::PostLayoutInit(error_collection &ec) {
	if (!(std_signal::PostLayoutInit(ec))) return false;

	auto mkroutefunc = [&](route_class::ID type, const track_target_ptr &piece) -> route* {
		route *candidate = nullptr;
		if (type == route_class::RTC_ROUTE) {
			candidate = &this->signal_route;
			candidate->index = 0;
		} else if (type == route_class::RTC_OVERLAP) {
			candidate = &this->overlap_route;
			candidate->index = 1;
		} else {
			ec.RegisterNewError<error_signalinit_trackscan>(*this, piece, "Autosignals support route and overlap route types only");
			return nullptr;
		}

		if (candidate->type != route_class::RTC_NULL) {
			ec.RegisterNewError<error_signalinit_trackscan>(*this, piece, "Autosignal already has a route of the corresponding type");
			return nullptr;
		} else {
			candidate->type = type;
			return candidate;
		}
	};

	bool scanresult = PostLayoutInitTrackScan(ec, 100, 0, 0, mkroutefunc);

	if (scanresult && signal_route.type == route_class::RTC_ROUTE) {
		if (signal_route.RouteReservation(RRF::AUTO_ROUTE | RRF::TRY_RESERVE)) {
			signal_route.RouteReservation(RRF::AUTO_ROUTE | RRF::RESERVE);

			generic_signal *end_signal = FastSignalCast(signal_route.end.track, signal_route.end.direction);
			if (end_signal && ! (end_signal->GetSignalFlags() & GSF::NO_OVERLAP)) {    //reserve an overlap beyond the end signal too if needed
				end_signal->PostLayoutInit(ec);    //make sure that the end piece is inited
				const route *best_overlap = end_signal->FindBestOverlap(route_class::Flag(route_class::RTC_OVERLAP));
				if (best_overlap && best_overlap->RouteReservation(RRF::AUTO_ROUTE | RRF::TRY_RESERVE)) {
					best_overlap->RouteReservation(RRF::AUTO_ROUTE | RRF::RESERVE);
					signal_route.overlap_type = route_class::RTC_OVERLAP;
				} else {
					ec.RegisterNewError<error_signalinit>(*this, "Autosignal route cannot reserve overlap");
					return false;
				}
			}
		} else {
			ec.RegisterNewError<error_signalinit>(*this, "Autosignal crosses reserved route");
			return false;
		}
	} else {
		ec.RegisterNewError<error_signalinit>(*this, "Track scan found no route");
		return false;
	}
	return true;
}

bool route_signal::PostLayoutInit(error_collection &ec) {
	if (!(std_signal::PostLayoutInit(ec))) return false;

	unsigned int route_index = 0;
	return PostLayoutInitTrackScan(ec, 100, 10, &restrictions, [&](route_class::ID type, const track_target_ptr &piece) -> route* {
		this->signal_routes.emplace_back();
		route *rt = &this->signal_routes.back();
		rt->index = route_index;
		rt->type = type;
		if (route_class::DefaultApproachControlOn(type)) {
			rt->route_common_flags |= route::RCF::AP_CONTROL;
		}
		route_index++;
		return rt;
	});
}

bool generic_signal::PostLayoutInitTrackScan(error_collection &ec, unsigned int max_pieces, unsigned int junction_max,
		route_restriction_set *restrictions, std::function<route*(route_class::ID type, const track_target_ptr &piece)> make_blank_route) {
	bool continue_initing = true;

	route_class::set foundoverlaps = 0;

	auto func = [&](const route_recording_list &route_pieces, const track_target_ptr &piece, generic_route_recording_state *grrs) {
		signal_route_recording_state *rrrs = static_cast<signal_route_recording_state *>(grrs);

		GTF pieceflags = piece.track->GetFlags(piece.direction);
		if (pieceflags & GTF::ROUTING_POINT) {
			routing_point *target_routing_piece = static_cast<routing_point *>(piece.track);

			RPRT available_route_types = target_routing_piece->GetAvailableRouteTypes(piece.direction);
			if (available_route_types.end) {
				RPRT current = target_routing_piece->GetSetRouteTypes(piece.direction);
				if (current.end || current.through) {
					ec.RegisterNewError<error_signalinit_trackscan>(*this, piece, "Signal route already reserved");
					continue_initing = false;
					return true;
				}

				std::vector<const route_restriction*> matching_restrictions;
				route_class::set restriction_permitted_types;
				if (restrictions) {
					restriction_permitted_types = restrictions->CheckAllRestrictions(matching_restrictions, route_pieces, piece);
				} else {
					restriction_permitted_types = route_class::All();
				}

				restriction_permitted_types &= target_routing_piece->GetRouteEndRestrictions().CheckAllRestrictions(matching_restrictions,
						route_pieces, track_target_ptr(this, EDGE::FRONT));

				auto mk_route = [&](route_class::ID type) {
					route *rt = make_blank_route(type, piece);
					if (rt) {
						rt->start = vartrack_target_ptr<routing_point>(this, EDGE::FRONT);
						rt->pieces = route_pieces;
						rt->end = vartrack_target_ptr<routing_point>(target_routing_piece, piece.direction);
						route_defaults.ApplyTo(*rt);
						rt->approach_locking_timeout = approach_locking_default_timeouts[type];
						rt->FillLists();
						rt->parent = this;

						generic_signal *rt_sig = FastSignalCast(target_routing_piece, piece.direction);

						if (route_class::NeedsOverlap(type)) {
							if (rt_sig && !(rt_sig->GetSignalFlags()&GSF::NO_OVERLAP)) {
								rt->overlap_type = route_class::ID::RTC_OVERLAP;
							}
						}

						for (auto &it : matching_restrictions) {
							if (it->GetApplyRouteTypes() & route_class::Flag(type)) {
								it->ApplyRestriction(*rt);
							}
						}

						if (rt->overlap_type != route_class::ID::RTC_NULL) {    //check whether the target overlap exists
							if (rt_sig) {
								auto checktarg = [this, rt_sig, rt](error_collection &ec) {
									if (!(rt_sig->GetAvailableOverlapTypes() & route_class::Flag(rt->overlap_type))) {
										//target signal does not have required type of overlap...
										ec.RegisterNewError<error_signalinit>(*this, "No overlap of type: '" +
												route_class::GetRouteTypeName(rt->overlap_type) + "', found for signal: " + rt_sig->GetFriendlyName());
									}
								};
								if (rt_sig->IsPostLayoutInitDone()) {
									checktarg(ec);
								} else {
									GetWorld().post_layout_init_final_fixups.AddFixup(checktarg);
								}
							} else {
								ec.RegisterNewError<error_signalinit>(*this, "No overlap of type: '" + route_class::GetRouteTypeName(rt->overlap_type) + "', found. " + rt->end.track->GetFriendlyName() + " is not a signal.");
							}
						}
					} else {
						ec.RegisterNewError<error_signalinit_trackscan>(*this, piece, "Unable to make new route of type: " + route_class::GetRouteTypeFriendlyName(type));
					}
				};
				route_class::set found_types = rrrs->allowed_routeclasses & available_route_types.end & restriction_permitted_types;
				while (found_types) {
					route_class::set bit = found_types & (found_types ^ (found_types - 1));
					route_class::ID type = static_cast<route_class::ID>(__builtin_ffs(bit) - 1);
					found_types ^= bit;
					mk_route(type);
					if (route_class::IsNotEndExtendable(type)) {
						rrrs->allowed_routeclasses &= ~bit;    //don't look for more overlap ends beyond the end of the first
					}
					if (route_class::IsOverlap(type)) {
						foundoverlaps |= bit;
					}
				}
			}
			rrrs->allowed_routeclasses &= available_route_types.through;

			if (!rrrs->allowed_routeclasses) {
				return true;    //nothing left to scan for
			}
		}
		if (pieceflags & GTF::ROUTE_SET) {
			ec.RegisterNewError<error_signalinit_trackscan>(*this, piece, "Piece already reserved");
			continue_initing = false;
			return true;
		}
		return false;
	};

	TSEF error_flags = TSEF::ZERO;
	route_recording_list pieces;
	signal_route_recording_state rrrs;

	RPRT allowed = GetAvailableRouteTypes(EDGE::FRONT);
	rrrs.allowed_routeclasses |= allowed.start;
	TrackScan(max_pieces, junction_max, GetConnectingPieceByIndex(EDGE::FRONT, 0), pieces, &rrrs, error_flags, func);

	available_overlaps = foundoverlaps;

	if (error_flags != TSEF::ZERO) {
		continue_initing = false;
		ec.RegisterNewError<error_signalinit>(*this, std::string("Track scan failed constraints: ") + GetTrackScanErrorFlagsStr(error_flags));
	}
	return continue_initing;
}

generic_track::edge_track_target start_of_line::GetEdgeConnectingPiece(EDGE edgeid) {
	switch (edgeid) {
		case EDGE::FRONT:
			return connection;

		default:
			assert(false);
			return empty_track_target;
	}
}

generic_track::edge_track_target start_of_line::GetConnectingPiece(EDGE direction) {
	switch (direction) {
		case EDGE::FRONT:
			return empty_track_target;

		case EDGE::BACK:
			return connection;

		default:
			assert(false);
			return empty_track_target;
	}
}

EDGE start_of_line::GetReverseDirection(EDGE direction) const {
	switch (direction) {
		case EDGE::FRONT:
			return EDGE::BACK;

		case EDGE::BACK:
			return EDGE::FRONT;

		default:
			assert(false);
			return EDGE::INVALID;
	}
}

unsigned int start_of_line::GetMaxConnectingPieces(EDGE direction) const {
	return 1;
}

generic_track::edge_track_target start_of_line::GetConnectingPieceByIndex(EDGE direction, unsigned int index) {
	return GetConnectingPiece(direction);
}

bool start_of_line::IsEdgeValid(EDGE edge) const {
	switch (edge) {
		case EDGE::FRONT:
			return true;

		default:
			return false;
	}
}

EDGE start_of_line::GetAvailableAutoConnectionDirection(bool forward_connection) const {
	if (forward_connection && !connection.IsValid()) {
		return EDGE::FRONT;
	}
	return EDGE::INVALID;
}

void start_of_line::GetListOfEdges(std::vector<edgelistitem> &output_list) const {
	output_list.insert(output_list.end(), { edgelistitem(EDGE::FRONT, connection) });
}

bool start_of_line::ReservationV(EDGE direction, unsigned int index, RRF rr_flags, const route *res_route, std::string* fail_reason_key) {
	if (rr_flags & RRF::START_PIECE && direction == EDGE::BACK) {
		return trs.Reservation(direction, index, rr_flags, res_route);
	} else if (rr_flags & RRF::END_PIECE && direction == EDGE::FRONT) {
		return trs.Reservation(direction, index, rr_flags, res_route);
	} else {
		return false;
	}
}

GTF start_of_line::GetFlags(EDGE direction) const {
	return GTF::ROUTING_POINT | trs.GetGTReservationFlags(direction);
}

RPRT start_of_line::GetAvailableRouteTypes(EDGE direction) const {
	return (direction == EDGE::FRONT) ? available_route_types : RPRT();
}

RPRT start_of_line::GetSetRouteTypes(EDGE direction) const {
	RPRT result;

	auto result_flag_adds = [&](const route *reserved_route, EDGE direction, unsigned int index, RRF rr_flags) {
		if (reserved_route) {
			result.GetRCSByRRF(rr_flags) |= route_class::Flag(reserved_route->type);
		}
	};

	if (direction == EDGE::FRONT) {
		trs.ReservationEnumerationInDirection(EDGE::FRONT, result_flag_adds);
	}
	return result;
}

unsigned int start_of_line::GetTRSList(std::vector<track_reservation_state *> &output_list) {
	output_list.push_back(&trs);
	return 1;
}

EDGE end_of_line::GetAvailableAutoConnectionDirection(bool forward_connection) const {
	return start_of_line::GetAvailableAutoConnectionDirection(!forward_connection);
}

GTF routing_marker::GetFlags(EDGE direction) const {
	return GTF::ROUTING_POINT | trs.GetGTReservationFlags(direction);
}

bool routing_marker::ReservationV(EDGE direction, unsigned int index, RRF rr_flags, const route *res_route, std::string* fail_reason_key) {
	return trs.Reservation(direction, index, rr_flags, res_route);
}

RPRT routing_marker::GetAvailableRouteTypes(EDGE direction) const {
	return (direction == EDGE::FRONT) ? available_route_types_forward : available_route_types_reverse;
}

RPRT routing_marker::GetSetRouteTypes(EDGE direction) const {
	RPRT result;

	auto result_flag_adds = [&](const route *reserved_route, EDGE direction, unsigned int index, RRF rr_flags) {
		if (reserved_route) {
			result.GetRCSByRRF(rr_flags) |= route_class::Flag(reserved_route->type);
		}
	};

	trs.ReservationEnumerationInDirection(direction, result_flag_adds);
	return result;
}

unsigned int routing_marker::GetTRSList(std::vector<track_reservation_state *> &output_list) {
	output_list.push_back(&trs);
	return 1;
}

std::ostream& operator<<(std::ostream& os, const RPRT& obj) {
	os << "RPRT(start: [";
	route_class::StreamOutRouteClassSet(os, obj.start);
	os << "], through: [";
	route_class::StreamOutRouteClassSet(os, obj.through);
	os << "], end: [";
	route_class::StreamOutRouteClassSet(os, obj.end);
	os << "], flags: [" << obj.flags << "])";
	return os;
}
