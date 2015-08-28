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

#include "core/track_circuit.h"
#include "core/serialisable_impl.h"
#include "core/track.h"
#include "core/track_reservation.h"
#include "core/signal.h"

#include <algorithm>

void CheckUnreserveTrackCircuit(track_circuit *tc);

enum {
	BPFF_SOFT      = 1<<0,
};

static void BerthPushFront(const route *rt, std::string newvalue, unsigned int flags = 0) {
	auto it = rt->berths.begin();
	if (it == rt->berths.end()) {
		return;    //no berths here
	}

	if (flags & BPFF_SOFT) {    //don't insert berth value if any berths are already occupied, insert into last berth
		for (; it != rt->berths.end(); ++it) {
			if (!(*it).berth->contents.empty()) {
				return;
			}
		}
		rt->berths.back().berth->contents = std::move(newvalue);
		if (rt->berths.back().owner_track) rt->berths.back().owner_track->MarkUpdated();
	} else {
		std::function<void(decltype(it) &)> advance = [&](decltype(it) &start) {
			if ((*start).berth->contents.empty()) {
				return;
			}
			auto next = std::next(start);
			if (next == rt->berths.end()) {
				return;
			}
			advance(next);
			if ((*next).berth->contents.empty()) {
				(*next).berth->contents = std::move((*start).berth->contents);
				(*start).berth->contents.clear();
				if ((*next).owner_track) {
					(*next).owner_track->MarkUpdated();
				}
				if ((*start).owner_track) {
					(*start).owner_track->MarkUpdated();
				}
			}

		};
		for (; it != rt->berths.end(); ++it) {
			auto next = std::next(it);
			advance(it);
			if ((*it).berth->contents.empty()) {
				if (next == rt->berths.end() || !(*next).berth->contents.empty()) {
					(*it).berth->contents = std::move(newvalue);
					if ((*it).owner_track) {
						(*it).owner_track->MarkUpdated();
					}
					return;
				}
			}
		}

		//berths all full, overwrite first
		rt->berths.front().berth->contents = std::move(newvalue);
		if (rt->berths.front().owner_track) {
			rt->berths.front().owner_track->MarkUpdated();
		}
	}
}

void track_train_counter_block::TrainEnter(train *t) {
	bool prevoccupied = Occupied();
	train_count++;

	bool added = false;
	for (auto &it : occupying_trains) {
		if (it.t == t) {
			it.count++;
			added = true;
			break;
		}
	}
	if (!added) {
		occupying_trains.emplace_back(t, 1);
	}

	if (prevoccupied != Occupied()) {
		last_change = GetWorld().GetGameTime();
		OccupationStateChangeTrigger();
		OccupationTrigger();
	}
}
void track_train_counter_block::TrainLeave(train *t) {
	bool prevoccupied = Occupied();
	train_count--;
	for (auto it = occupying_trains.begin(); it != occupying_trains.end(); ++it) {
		if (it->t == t) {
			it->count--;
			if (it->count == 0) {
				*it = *occupying_trains.rbegin();
				occupying_trains.pop_back();
			}
			break;
		}
	}
	if (prevoccupied != Occupied()) {
		last_change = GetWorld().GetGameTime();
		OccupationStateChangeTrigger();
		DeOccupationTrigger();
	}
}

void track_train_counter_block::Deserialise(const deserialiser_input &di, error_collection &ec) {
	world_obj::Deserialise(di, ec);

	CheckTransJsonValueFlag(tc_flags, TCF::FORCE_OCCUPIED, di, "forceoccupied", ec);
	CheckTransJsonValue(last_change, di, "last_change", ec);
}

void track_train_counter_block::Serialise(serialiser_output &so, error_collection &ec) const {
	world_obj::Serialise(so, ec);

	SerialiseFlagJson(tc_flags, TCF::FORCE_OCCUPIED, so, "forceoccupied");
	SerialiseValueJson(last_change, so, "last_change");
}

track_train_counter_block::TCF track_train_counter_block::GetTCFlags() const {
	return tc_flags;
}

track_train_counter_block::TCF track_train_counter_block::SetTCFlagsMasked(TCF bits, TCF mask) {
	bool prevoccupied = Occupied();
	tc_flags = (tc_flags & ~mask) | (bits & mask);
	if (prevoccupied != Occupied()) {
		last_change = GetWorld().GetGameTime();
		OccupationStateChangeTrigger();
		if (prevoccupied) {
			DeOccupationTrigger();
		} else {
			OccupationTrigger();
		}
	}
	return tc_flags;
}

void CheckUnreserveTrackCircuit(track_circuit *tc) {
	std::function<bool(generic_track *, generic_track *, const route *)> backtrack =
			[&](generic_track *bt_piece, generic_track *next_piece, const route *reserved_route) -> bool {
		if (!bt_piece) {
			return true;
		}
		const track_circuit *bt_tc = bt_piece->GetTrackCircuit();
		if (bt_tc && bt_tc != tc) {
			//hit different TC, unreserve if route not reserved here
			bool found = false;
			bt_piece->ReservationEnumeration([&](const route *chk_reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) {
				if (reserved_route == chk_reserved_route) {
					found = true;
				}
			}, RRF::RESERVE);
			return !found;
		} else {
			//hit same TC or no TC, unreserve if route runs out, or keep scanning
			bool success = true;
			bool unreserve = false;
			EDGETYPE unresdirection;
			unsigned int unresidex;
			RRF unresrrflags;
			bt_piece->ReservationEnumeration([&](const route *chk_reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) {
				if (reserved_route == chk_reserved_route) {
					if (rr_flags & RRF::START_PIECE) {
						if (reserved_route->route_common_flags & route::RCF::TORR) {
							//don't unreserve the start of the route unless TORR is enabled
							unreserve = true;
							unresdirection = direction;
							unresidex = index;
							unresrrflags = RRF::START_PIECE | RRF::UNRESERVE;
						} else {
							success = false;
						}
					} else {
						bool prevres = backtrack(bt_piece->GetEdgeConnectingPiece(direction).track, bt_piece, chk_reserved_route);
						if (prevres) {
							unreserve = true;
							unresdirection = direction;
							unresidex = index;
							unresrrflags = RRF::UNRESERVE;
						} else {
							success = false;
						}
					}
				}
			}, RRF::RESERVE);
			if (unreserve) {
				bt_piece->Reservation(unresdirection, unresidex, unresrrflags, reserved_route);
			}
			return success;
		}
	};

	std::function<bool(generic_track *, const route *)> forwardtrack = [&](generic_track *ft_piece, const route *reserved_route) -> bool {
		if (!ft_piece) {
			return true;
		}
		const track_circuit *bt_tc = ft_piece->GetTrackCircuit();
		if (bt_tc && bt_tc->Occupied()) {
			return true;
		} else {
			bool success = false;
			bool unreserve = false;
			EDGETYPE unresdirection;
			unsigned int unresidex;
			RRF unresrrflags;
			ft_piece->ReservationEnumeration([&](const route *chk_reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) {
				if (reserved_route == chk_reserved_route) {
					if (rr_flags & RRF::END_PIECE) {
						unreserve = true;
						unresdirection = direction;
						unresidex = index;
						success = true;
						unresrrflags = RRF::UNRESERVE | RRF::END_PIECE;
					} else {
						bool nextres = forwardtrack(ft_piece->GetConnectingPieceByIndex(direction, index).track, chk_reserved_route);
						if (nextres) {
							unreserve = true;
							unresdirection = direction;
							unresidex = index;
							success = true;
							unresrrflags = RRF::UNRESERVE;
						}
					}
				}
			}, RRF::RESERVE);
			if (unreserve) {
				ft_piece->Reservation(unresdirection, unresidex, unresrrflags, reserved_route);
			}
			return success;
		}
	};

	const std::vector<generic_track *> &pieces = tc->GetOwnedTrackSet();
	for (auto piece : pieces) {
		std::vector<std::function<void()> > fixups;
		piece->ReservationEnumeration([&](const route *reserved_route, EDGETYPE r_direction, unsigned int r_index, RRF rr_flags) {
			if (!route_class::IsValid(reserved_route->type) || route_class::IsOverlap(reserved_route->type)) {
				return;
			}
			if (backtrack(piece->GetEdgeConnectingPiece(r_direction).track, piece, reserved_route)) {
				fixups.emplace_back([=]() {
					piece->Reservation(r_direction, r_index, RRF::UNRESERVE, reserved_route);
				});
				forwardtrack(piece->GetConnectingPieceByIndex(r_direction, r_index).track, reserved_route);
			}
		}, RRF::RESERVE);
		for (auto &fixup : fixups) {
			fixup();
		}
	}
}

void track_train_counter_block::GetSetRoutes(std::vector<const route *> &routes) {
	for (generic_track *it : owned_pieces) {
		it->ReservationEnumeration([&](const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) {
			if (std::find(routes.begin(), routes.end(), reserved_route) == routes.end()) {
				routes.push_back(reserved_route);
			}
		}, RRF::RESERVE);
	}
}

void track_circuit::TrackReserved(generic_track *track) {
	reserved_track_pieces++;
	if (reserved_track_pieces == 1) {
		OccupationStateChangeTrigger();
	}
}

void track_circuit::TrackUnreserved(generic_track *track) {
	reserved_track_pieces--;
	if (reserved_track_pieces == 0) {
		OccupationStateChangeTrigger();
	}
}

void track_circuit::OccupationStateChangeTrigger() {
	GetWorld().MarkUpdated(this);
	for (auto &it : GetOwnedTrackSet()) {
		it->MarkUpdated();
	}
}

void track_circuit::OccupationTrigger() {
	//check berth stepping
	std::vector<const route *> routes;
	const route *found = nullptr;
	GetSetRoutes(routes);
	for (const route *rt : routes) {
		if (route_class::IsOverlap(rt->type)) {
			continue;    // we don't want overlaps
		}
		if (!rt->track_circuits.empty() && rt->track_circuits[0] == this) {
			found = rt;
			break;
		}
	}
	if (found) {
		//look backwards
		track_berth *prev = found->start.track->GetPriorBerth(found->start.direction, routing_point::GPBF::GET_NON_EMPTY);
		if (!prev) {
			BerthPushFront(found, "????", BPFF_SOFT);
		} else {
			BerthPushFront(found, std::move(prev->contents));
			prev->contents.clear();
		}
	}
}

void track_circuit::DeOccupationTrigger() {
	CheckUnreserveTrackCircuit(this);
}
