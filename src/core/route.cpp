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
#include "core/route.h"
#include "core/track.h"
#include "core/util.h"
#include "core/signal.h"

#include <algorithm>

void route_common::ApplyTo(route_common &target) const {
	if(routecommonflags & RCF::PRIORITYSET) target.priority = priority;
	if(routecommonflags & RCF::APLOCK_TIMEOUTSET) target.approachlocking_timeout = approachlocking_timeout;
	if(routecommonflags & RCF::OVERLAPTIMEOUTSET) target.overlap_timeout = overlap_timeout;
	if(routecommonflags & RCF::APCONTROLTRIGGERDELAY_SET) target.approachcontrol_triggerdelay = approachcontrol_triggerdelay;
	if(routecommonflags & RCF::APCONTROL_SET) SetOrClearBitsRef(target.routecommonflags, RCF::APCONTROL, routecommonflags & RCF::APCONTROL);
	if(routecommonflags & RCF::TORR_SET) SetOrClearBitsRef(target.routecommonflags, RCF::TORR, routecommonflags & RCF::TORR);
	if(routecommonflags & RCF::EXITSIGCONTROL_SET) SetOrClearBitsRef(target.routecommonflags, RCF::EXITSIGCONTROL, routecommonflags & RCF::EXITSIGCONTROL);
	if(routecommonflags & RCF::OVERLAPTYPE_SET) target.overlap_type = overlap_type;
	if(routecommonflags & RCF::ROUTEPROVEDELAY_SET) target.routeprove_delay = routeprove_delay;
	if(routecommonflags & RCF::ROUTECLEARDELAY_SET) target.routeclear_delay = routeclear_delay;
	if(routecommonflags & RCF::ROUTESETDELAY_SET) target.routeset_delay = routeset_delay;
	if(routecommonflags & RCF::ASPECTMASK_SET) target.aspect_mask = aspect_mask;
	if(routecommonflags & RCF::APCONTROL_IF_NOROUTE_SET) SetOrClearBitsRef(target.routecommonflags, RCF::APCONTROL_IF_NOROUTE, routecommonflags & RCF::APCONTROL_IF_NOROUTE);
	if(approachcontrol_trigger) target.approachcontrol_trigger = approachcontrol_trigger;
	if(overlaptimeout_trigger) target.overlaptimeout_trigger = overlaptimeout_trigger;
}

//returns false on failure/partial completion
bool route::RouteReservation(RRF reserve_flags, std::string *failreasonkey) const {
	if(!start.track->Reservation(start.direction, 0, reserve_flags | RRF::STARTPIECE, this, failreasonkey)) return false;

	for(auto it = pieces.begin(); it != pieces.end(); ++it) {
		if(!it->location.track->Reservation(it->location.direction, it->connection_index, reserve_flags, this, failreasonkey)) return false;
	}

	if(!end.track->Reservation(end.direction, 0, reserve_flags | RRF::ENDPIECE, this, failreasonkey)) return false;
	return true;
}

//returns false on failure/partial completion
bool route::PartialRouteReservationWithActions(RRF reserve_flags, std::string *failreasonkey, RRF action_reserve_flags, std::function<void(action &&reservation_act)> actioncallback) const {
	if(!start.track->Reservation(start.direction, 0, reserve_flags | RRF::STARTPIECE, this, failreasonkey)) return false;
	start.track->ReservationActions(start.direction, 0, action_reserve_flags | RRF::STARTPIECE, this, actioncallback);

	for(auto it = pieces.begin(); it != pieces.end(); ++it) {
		if(!it->location.track->Reservation(it->location.direction, it->connection_index, reserve_flags, this, failreasonkey)) return false;
		it->location.track->ReservationActions(it->location.direction, it->connection_index, action_reserve_flags, this, actioncallback);
	}

	if(!end.track->Reservation(end.direction, 0, reserve_flags | RRF::ENDPIECE, this, failreasonkey)) return false;
	end.track->ReservationActions(end.direction, 0, action_reserve_flags | RRF::ENDPIECE, this, actioncallback);
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
		routingpoint *target_routing_piece = FastRoutingpointCast(it->location.track, it->location.direction);
		if(target_routing_piece && target_routing_piece->GetAvailableRouteTypes(it->location.direction).flags & RPRT_FLAGS::VIA) {
			vias.push_back(target_routing_piece);
		}
		genericsignal *this_signal = FastSignalCast(target_routing_piece, it->location.direction);
		if(this_signal && this_signal->RepeaterAspectMeaningfulForRouteType(type)) {
			repeatersignals.push_back(this_signal);
		}
		if(!it->location.track->IsTrackAlwaysPassable()) {
			passtestlist.push_back(*it);
		}
		if(it->location.track->HasBerth(it->location.direction)) {
			berths.emplace_back(it->location.track->GetBerth(), it->location.track);
		}
		if(it->location.track->GetFlags(it->location.track->GetDefaultValidDirecton()) & GTF::SIGNAL) berths.clear();	//if we reach a signal, remove any berths we saw beforehand
	}
}

bool route::TestRouteForMatch(const routingpoint *checkend, const via_list &checkvias) const {
	return checkend == end.track && checkvias == vias;
}

bool route::IsRouteSubSet(const route *subset) const {
	const vartrack_target_ptr<routingpoint> &substart = subset->start;

	auto this_it = pieces.begin();
	if(substart != start) {    //scan along route for start
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

bool route::IsStartAnchored(RRF checkmask) const {
	bool anchored = false;
	start.track->ReservationEnumerationInDirection(start.direction, [&](const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) {
		if(rr_flags && RRF::STARTPIECE && reserved_route == this) {
			anchored = true;
		}
	}, checkmask);
	return anchored;
}

bool route::IsRouteTractionSuitable(const train* t) const {
	for(auto it = pieces.begin(); it != pieces.end(); ++it) {
		const tractionset *ts = it->location.track->GetTractionTypes();
		if(ts && !ts->CanTrainPass(t)) return false;
	}
	return true;
}

//return true if restriction applies
bool route_restriction::CheckRestriction(route_class::set &allowed_routes, const route_recording_list &route_pieces, const track_target_ptr &piece) const {
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

	allowed_routes &= allowedtypes;
	return true;
}

void route_restriction::ApplyRestriction(route &rt) const {
	ApplyTo(rt);
}

route_class::set route_restriction_set::CheckAllRestrictions(std::vector<const route_restriction*> &matching_restrictions, const route_recording_list &route_pieces, const track_target_ptr &piece) const {
	route_class::set allowed = route_class::All();
	for(auto it = restrictions.begin(); it != restrictions.end(); ++it) {
		if(it->CheckRestriction(allowed, route_pieces, piece)) matching_restrictions.push_back(&(*it));
	}
	return allowed;
}
