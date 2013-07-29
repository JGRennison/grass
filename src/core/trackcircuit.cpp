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

#include "trackcircuit.h"
#include "serialisable_impl.h"
#include "track.h"
#include "trackreservation.h"
#include "signal.h"

#include <algorithm>

void CheckUnreserveTrackCircuit(track_circuit *tc);

enum {
	BPFF_SOFT	= 1<<0,
};

static void BerthPushFront(const route *rt, std::string &&newvalue, unsigned int flags = 0) {
	auto it = rt->berths.begin();
	if(it == rt->berths.end()) return;	//no berths here

	if(flags & BPFF_SOFT) {			//don't insert berth value if any berths are already occupied, insert into last berth
		for(; it != rt->berths.end(); ++it) {
			if(!(*it)->contents.empty()) return;
		}
		rt->berths.back()->contents = newvalue;
	}
	else {
		std::function<void(decltype(it) &)> advance = [&](decltype(it) &start) {
			if((*start)->contents.empty()) return;
			auto next = std::next(start);
			if(next == rt->berths.end()) return;
			advance(next);
			if((*next)->contents.empty()) {
				(*next)->contents = std::move((*start)->contents);
				(*start)->contents.clear();
			}

		};
		for(; it != rt->berths.end(); ++it) {
			auto next = std::next(it);
			advance(it);
			if((*it)->contents.empty()) {
				if(next == rt->berths.end() || !(*next)->contents.empty()) {
					(*it)->contents = newvalue;
					return;
				}
			}
		}

		//berths all full, overwrite first
		rt->berths.front()->contents = newvalue;
	}
}

void track_train_counter_block::TrainEnter(train *t) {
	bool prevoccupied = Occupied();
	traincount++;
	for(auto it = occupying_trains.begin(); it != occupying_trains.end(); ++it) {
		if(it->t == t) {
			it->count++;
			break;
		}
	}
	occupying_trains.emplace_back(t, 1);
	if(prevoccupied != Occupied()) {
		last_change = GetWorld().GetGameTime();
		OccupationTrigger();
	}
}
void track_train_counter_block::TrainLeave(train *t) {
	bool prevoccupied = Occupied();
	traincount--;
	for(auto it = occupying_trains.begin(); it != occupying_trains.end(); ++it) {
		if(it->t == t) {
			it->count--;
			if(it->count == 0) {
				*it = *occupying_trains.rbegin();
				occupying_trains.pop_back();
			}
			break;
		}
	}
	if(prevoccupied != Occupied()) {
		last_change = GetWorld().GetGameTime();
		DeOccupationTrigger();
	}
}

void track_train_counter_block::Deserialise(const deserialiser_input &di, error_collection &ec) {
	world_obj::Deserialise(di, ec);

	CheckTransJsonValueFlag(tc_flags, TCF::FORCEOCCUPIED, di, "forceoccupied", ec);
	CheckTransJsonValue(last_change, di, "last_change", ec);
}

void track_train_counter_block::Serialise(serialiser_output &so, error_collection &ec) const {
	world_obj::Serialise(so, ec);

	SerialiseFlagJson(tc_flags, TCF::FORCEOCCUPIED, so, "forceoccupied");
	SerialiseValueJson(last_change, so, "last_change");
}

track_train_counter_block::TCF track_train_counter_block::GetTCFlags() const {
	return tc_flags;
}

track_train_counter_block::TCF track_train_counter_block::SetTCFlagsMasked(TCF bits, TCF mask) {
	bool prevoccupied = Occupied();
	tc_flags = (tc_flags & ~mask) | (bits & mask);
	if(prevoccupied != Occupied()) {
		last_change = GetWorld().GetGameTime();
		if(prevoccupied) DeOccupationTrigger();
		else OccupationTrigger();
	}
	return tc_flags;
}

void CheckUnreserveTrackCircuit(track_circuit *tc) {
	std::function<bool(generictrack *, generictrack *, const route *)> backtrack = [&](generictrack *bt_piece, generictrack *next_piece, const route *reserved_route) -> bool {
		if(!bt_piece) return true;
		const track_circuit *bt_tc = bt_piece->GetTrackCircuit();
		if(bt_tc && bt_tc != tc) {
			//hit different TC, unreserve if route not reserved here
			bool found = false;
			bt_piece->ReservationEnumeration([&](const route *chk_reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) {
				if(reserved_route == chk_reserved_route) found = true;
			}, RRF::RESERVE);
			return !found;
		}
		else {
			//hit same TC or no TC, unreserve if route runs out, or keep scanning
			bool success = true;
			bool unreserve = false;
			EDGETYPE unresdirection;
			unsigned int unresidex;
			RRF unresrrflags;
			bt_piece->ReservationEnumeration([&](const route *chk_reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) {
				if(reserved_route == chk_reserved_route) {
					if(rr_flags & RRF::STARTPIECE) {
						if(reserved_route->routeflags & route::RF::TORR) {
							//don't unreserve the start of the route unless TORR is enabled
							unreserve = true;
							unresdirection = direction;
							unresidex = index;
							unresrrflags = RRF::STARTPIECE | RRF::UNRESERVE;
						}
						else success = false;
					}
					else {
						bool prevres = backtrack(bt_piece->GetEdgeConnectingPiece(direction).track, bt_piece, chk_reserved_route);
						if(prevres) {
							unreserve = true;
							unresdirection = direction;
							unresidex = index;
							unresrrflags = RRF::UNRESERVE;
						}
						else success = false;
					}
				}
			}, RRF::RESERVE);
			if(unreserve) {
				bt_piece->Reservation(unresdirection, unresidex, unresrrflags, reserved_route);
			}
			return success;
		}
	};

	std::function<bool(generictrack *, const route *)> forwardtrack = [&](generictrack *ft_piece, const route *reserved_route) -> bool {
		if(!ft_piece) return true;
		const track_circuit *bt_tc = ft_piece->GetTrackCircuit();
		if(bt_tc && bt_tc->Occupied()) return true;
		else {
			bool success = false;
			bool unreserve = false;
			EDGETYPE unresdirection;
			unsigned int unresidex;
			RRF unresrrflags;
			ft_piece->ReservationEnumeration([&](const route *chk_reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) {
				if(reserved_route == chk_reserved_route) {
					if(rr_flags & RRF::ENDPIECE) {
						unreserve = true;
						unresdirection = direction;
						unresidex = index;
						success = true;
						unresrrflags = RRF::UNRESERVE | RRF::ENDPIECE;
					}
					else {
						bool nextres = forwardtrack(ft_piece->GetConnectingPieceByIndex(direction, index).track, chk_reserved_route);
						if(nextres) {
							unreserve = true;
							unresdirection = direction;
							unresidex = index;
							success = true;
							unresrrflags = RRF::UNRESERVE;
						}
					}
				}
			}, RRF::RESERVE);
			if(unreserve) {
				ft_piece->Reservation(unresdirection, unresidex, unresrrflags, reserved_route);
			}
			return success;
		}
	};

	const std::vector<generictrack *> &pieces = tc->GetOwnedTrackSet();
	for(auto piece : pieces) {
		std::vector<std::function<void()> > fixups;
		piece->ReservationEnumeration([&](const route *reserved_route, EDGETYPE r_direction, unsigned int r_index, RRF rr_flags) {
			if(!route_class::IsValid(reserved_route->type) || route_class::IsOverlap(reserved_route->type)) return;
			if(backtrack(piece->GetEdgeConnectingPiece(r_direction).track, piece, reserved_route)) {
				fixups.emplace_back([=]() {
					piece->Reservation(r_direction, r_index, RRF::UNRESERVE, reserved_route);
				});
				forwardtrack(piece->GetConnectingPieceByIndex(r_direction, r_index).track, reserved_route);
			}
		}, RRF::RESERVE);
		for(auto &fixup : fixups) fixup();
	}
}

void track_train_counter_block::GetSetRoutes(std::vector<const route *> &routes) {
	for(generictrack *it : owned_pieces) {
		it->ReservationEnumeration([&](const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) {
			if(std::find(routes.begin(), routes.end(), reserved_route) == routes.end()) {
				routes.push_back(reserved_route);
			}
		}, RRF::RESERVE);
	}
}

void track_circuit::OccupationTrigger() {
	//check berth stepping
	std::vector<const route *> routes;
	const route *found = 0;
	GetSetRoutes(routes);
	for(const route *rt : routes) {
		if(route_class::IsOverlap(rt->type)) continue;	// we don't want overlaps
		if(!rt->trackcircuits.empty() && rt->trackcircuits[0] == this) {
			found = rt;
			break;
		}
	}
	if(found) {
		//look backwards
		trackberth *prev = found->start.track->GetPriorBerth(found->start.direction, routingpoint::GPBF::GETNONEMPTY);
		if(!prev) BerthPushFront(found, "????", BPFF_SOFT);
		else {
			BerthPushFront(found, std::move(prev->contents));
			prev->contents.clear();
		}
	}
}

void track_circuit::DeOccupationTrigger() {
	CheckUnreserveTrackCircuit(this);
}
