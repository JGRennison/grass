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
#include "core/route.h"
#include "core/track.h"
#include "util/util.h"
#include "core/signal.h"

#include <algorithm>

void route_common::ApplyTo(route_common &target) const {
	if (route_common_flags & RCF::PRIORITY_SET) {
		target.priority = priority;
	}
	if (route_common_flags & RCF::AP_LOCKING_TIMEOUT_SET) {
		target.approach_locking_timeout = approach_locking_timeout;
	}
	if (route_common_flags & RCF::OVERLAP_TIMEOUT_SET) {
		target.overlap_timeout = overlap_timeout;
	}
	if (route_common_flags & RCF::AP_CONTROL_TRIGGER_DELAY_SET) {
		target.approach_control_triggerdelay = approach_control_triggerdelay;
	}
	if (route_common_flags & RCF::AP_CONTROL_SET) {
		SetOrClearBitsRef(target.route_common_flags, RCF::AP_CONTROL, route_common_flags & RCF::AP_CONTROL);
	}
	if (route_common_flags & RCF::TORR_SET) {
		SetOrClearBitsRef(target.route_common_flags, RCF::TORR, route_common_flags & RCF::TORR);
	}
	if (route_common_flags & RCF::EXIT_SIGNAL_CONTROL_SET) {
		SetOrClearBitsRef(target.route_common_flags, RCF::EXIT_SIGNAL_CONTROL, route_common_flags & RCF::EXIT_SIGNAL_CONTROL);
	}
	if (route_common_flags & RCF::OVERLAP_TYPE_SET) {
		target.overlap_type = overlap_type;
	}
	if (route_common_flags & RCF::ROUTE_PROVE_DELAY_SET) {
		target.route_prove_delay = route_prove_delay;
	}
	if (route_common_flags & RCF::ROUTE_CLEAR_DELAY_SET) {
		target.route_clear_delay = route_clear_delay;
	}
	if (route_common_flags & RCF::ROUTE_SET_DELAY_SET) {
		target.route_set_delay = route_set_delay;
	}
	if (route_common_flags & RCF::ASPECT_MASK_SET) {
		target.aspect_mask = aspect_mask;
	}
	if (route_common_flags & RCF::AP_CONTROL_IF_NOROUTE_SET) {
		SetOrClearBitsRef(target.route_common_flags, RCF::AP_CONTROL_IF_NOROUTE, route_common_flags & RCF::AP_CONTROL_IF_NOROUTE);
	}
	if (approach_control_trigger) {
		target.approach_control_trigger = approach_control_trigger;
	}
	if (overlap_timeout_trigger) {
		target.overlap_timeout_trigger = overlap_timeout_trigger;
	}
	target.conditional_aspect_masks.insert(target.conditional_aspect_masks.end(), conditional_aspect_masks.begin(), conditional_aspect_masks.end());
}

//returns false on failure/partial completion
bool route::RouteReservation(RRF reserve_flags, std::string *fail_reason_key) const {
	if (!start.track->Reservation(start.direction, 0, reserve_flags | RRF::START_PIECE, this, fail_reason_key)) {
		return false;
	}

	for (auto &it : pieces) {
		if (!it.location.track->Reservation(it.location.direction, it.connection_index, reserve_flags, this, fail_reason_key)) {
			return false;
		}
	}

	if (!end.track->Reservation(end.direction, 0, reserve_flags | RRF::END_PIECE, this, fail_reason_key)) {
		return false;
	}
	return true;
}

//returns false on failure/partial completion
bool route::PartialRouteReservationWithActions(RRF reserve_flags, std::string *fail_reason_key, RRF action_reserve_flags, std::function<void(action &&reservation_act)> action_callback) const {
	if (!start.track->Reservation(start.direction, 0, reserve_flags | RRF::START_PIECE, this, fail_reason_key)) {
		return false;
	}
	start.track->ReservationActions(start.direction, 0, action_reserve_flags | RRF::START_PIECE, this, action_callback);

	for (auto &it : pieces) {
		if (!it.location.track->Reservation(it.location.direction, it.connection_index, reserve_flags, this, fail_reason_key)) {
			return false;
		}
		it.location.track->ReservationActions(it.location.direction, it.connection_index, action_reserve_flags, this, action_callback);
	}

	if (!end.track->Reservation(end.direction, 0, reserve_flags | RRF::END_PIECE, this, fail_reason_key))
		return false;
	end.track->ReservationActions(end.direction, 0, action_reserve_flags | RRF::END_PIECE, this, action_callback);
	return true;
}

void route::RouteReservationActions(RRF reserve_flags, std::function<void(action &&reservation_act)> action_callback) const {
	start.track->ReservationActions(start.direction, 0, reserve_flags | RRF::START_PIECE, this, action_callback);

	for (auto &it : pieces) {
		it.location.track->ReservationActions(it.location.direction, it.connection_index, reserve_flags, this, action_callback);
	}

	end.track->ReservationActions(end.direction, 0, reserve_flags | RRF::END_PIECE, this, action_callback);
}

void route::FillLists() {
	track_circuit *last_tc = nullptr;
	for (auto &it : pieces) {
		track_circuit *this_tc = it.location.track->GetTrackCircuit();
		if (this_tc && this_tc != last_tc) {
			last_tc = this_tc;
			track_circuits.push_back(this_tc);
		}
		routing_point *target_routing_piece = FastRoutingpointCast(it.location.track, it.location.direction);
		if (target_routing_piece && target_routing_piece->GetAvailableRouteTypes(it.location.direction).flags & RPRT_FLAGS::VIA) {
			vias.push_back(target_routing_piece);
		}
		generic_signal *this_signal = FastSignalCast(target_routing_piece, it.location.direction);
		if (this_signal && this_signal->RepeaterAspectMeaningfulForRouteType(type)) {
			repeater_signals.push_back(this_signal);
		}
		if (!it.location.track->IsTrackAlwaysPassable()) {
			pass_test_list.push_back(it);
		}
		if (it.location.track->HasBerth(it.location.direction)) {
			berths.emplace_back(it.location.track->GetBerth(), it.location.track);
		}
		if (it.location.track->GetFlags(it.location.track->GetDefaultValidDirecton()) & GTF::SIGNAL) {
			berths.clear();	//if we reach a signal, remove any berths we saw beforehand
		}
	}
}

bool route::TestRouteForMatch(const routing_point *check_end, const via_list &check_vias) const {
	return check_end == end.track && check_vias == vias;
}

bool route::IsRouteSubSet(const route *subset) const {
	const vartrack_target_ptr<routing_point> &substart = subset->start;

	auto this_it = pieces.begin();
	if (substart != start) {    //scan along route for start
		while (true) {
			if (this_it->location == substart) {
				break;
			}
			++this_it;
			if (this_it == pieces.end()) {
				return false;
			}
		}
		++this_it;
	}

	auto sub_it = subset->pieces.begin();

	//sub_it and this_it should now point to same track piece if route is a subset

	for (; sub_it != subset->pieces.end() && this_it != pieces.end(); ++sub_it, ++this_it) {
		if (*this_it != *sub_it) {
			return false;
		}
	}

	if (sub_it == subset->pieces.end()) {
		if (this_it == pieces.end()) {
			return subset->end == end;
		} else {
			return subset->end == this_it->location;
		}
	} else {
		return false;
	}
}

bool route::IsStartAnchored(RRF check_mask) const {
	bool anchored = false;
	start.track->ReservationEnumerationInDirection(start.direction, [&](const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) {
		if (rr_flags && RRF::START_PIECE && reserved_route == this) {
			anchored = true;
		}
	}, check_mask);
	return anchored;
}

bool route::IsRouteTractionSuitable(const train* t) const {
	for (auto &it : pieces) {
		const traction_set *ts = it.location.track->GetTractionTypes();
		if (ts && !ts->CanTrainPass(t)) {
			return false;
		}
	}
	return true;
}

//return true if restriction applies
bool route_restriction::CheckRestriction(route_class::set &allowed_routes, const route_recording_list &route_pieces, const track_target_ptr &piece) const {
	if (!targets.empty() && std::find(targets.begin(), targets.end(), piece.track->GetName()) == targets.end()) {
		return false;
	}

	auto via_start = via.begin();
	for (auto &it : route_pieces) {
		if (!not_via.empty() && std::find(not_via.begin(), not_via.end(), it.location.track->GetName()) != not_via.end()) {
			return false;
		}
		if (!via.empty()) {
			auto found_via = std::find(via_start, via.end(), it.location.track->GetName());
			if (found_via != via.end()) {
				via_start = std::next(found_via, 1);
			}
		}
	}
	if (via_start != via.end()) {
		return false;
	}

	allowed_routes &= allowed_types;
	return true;
}

void route_restriction::ApplyRestriction(route &rt) const {
	ApplyTo(rt);
}

route_class::set route_restriction_set::CheckAllRestrictions(std::vector<const route_restriction*> &matching_restrictions,
		const route_recording_list &route_pieces, const track_target_ptr &piece) const {
	route_class::set allowed = route_class::All();
	for (auto &it : restrictions) {
		if (it.CheckRestriction(allowed, route_pieces, piece)) {
			matching_restrictions.push_back(&it);
		}
	}
	return allowed;
}
