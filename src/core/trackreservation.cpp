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

#include <vector>
#include "common.h"
#include "core/track_reservation.h"
#include "core/track.h"
#include "core/signal.h"
#include "core/serialisable_impl.h"

bool track_reservation_state::Reservation(EDGETYPE in_dir, unsigned int in_index, RRF in_rr_flags, const route *res_route, std::string* fail_reason_key) {
	if (in_rr_flags & (RRF::RESERVE | RRF::TRY_RESERVE | RRF::PROVISIONAL_RESERVE)) {
		for (auto &it : itrss) {
			if (it.rr_flags & (RRF::RESERVE | RRF::PROVISIONAL_RESERVE)) {    //track already reserved
				if (in_rr_flags & RRF::IGNORE_OWN_OVERLAP && it.reserved_route && it.reserved_route->start == res_route->start
						&& route_class::IsOverlap(it.reserved_route->type)) {
					//do nothing, own overlap is being ignored
				} else if (it.direction != in_dir || it.index != in_index) {
					if (fail_reason_key) {
						*fail_reason_key = "track/reservation/conflict";
					}
					return false;    //reserved piece doesn't match
				}
			}
			if (it.rr_flags & RRF::PROVISIONAL_RESERVE && in_rr_flags & RRF::RESERVE) {
				if (it.reserved_route == res_route && (in_rr_flags & RRF::SAVEMASK & ~RRF::RESERVE)
						== (it.rr_flags & RRF::SAVEMASK & ~RRF::PROVISIONAL_RESERVE)) {
					//we are now properly reserving what we already preliminarily reserved, reuse inner_track_reservation_state
					it.rr_flags |= RRF::RESERVE;
					it.rr_flags &= ~RRF::PROVISIONAL_RESERVE;
					return true;
				}
			}
		}
		if (in_rr_flags & (RRF::RESERVE | RRF::PROVISIONAL_RESERVE)) {
			itrss.emplace_back();
			inner_track_reservation_state &itrs = itrss.back();

			itrs.rr_flags = in_rr_flags & RRF::SAVEMASK;
			itrs.direction = in_dir;
			itrs.index = in_index;
			itrs.reserved_route = res_route;
		}
		return true;
	} else if (in_rr_flags & (RRF::UNRESERVE | RRF::TRY_UNRESERVE)) {
		for (auto it = itrss.begin(); it != itrss.end(); ++it) {
			if (it->rr_flags & RRF::RESERVE && it->direction == in_dir && it->index == in_index && it->reserved_route == res_route) {
				if (in_rr_flags & RRF::UNRESERVE) {
					itrss.erase(it);
				}
				return true;
			}
		}
	}
	return false;
}

bool track_reservation_state::IsReserved() const {
	for (auto &it : itrss) {
		if (it.rr_flags & RRF::RESERVE) {
			return true;
		}
	}
	return false;
}

unsigned int track_reservation_state::GetReservationCount() const {
	unsigned int count = 0;
	for (auto &it : itrss) {
		if (it.rr_flags & RRF::RESERVE) {
			count++;
		}
	}
	return count;
}

bool track_reservation_state::IsReservedInDirection(EDGETYPE direction) const {
	for (auto &it : itrss) {
		if (it.rr_flags & RRF::RESERVE && it.direction == direction) {
			return true;
		}
	}
	return false;
}

GTF track_reservation_state::GetGTReservationFlags(EDGETYPE checkdirection) const {
	GTF outputflags = GTF::ZERO;
	if (IsReserved()) {
		outputflags |= GTF::ROUTE_SET;
		if (IsReservedInDirection(checkdirection)) {
			outputflags |= GTF::ROUTE_THIS_DIR;
		}
	}
	return outputflags;
}

void track_reservation_state::ReservationTypeCount(reservation_count_set &rcs) const {
	for (auto &it : itrss) {
		if (it.rr_flags & RRF::RESERVE) {
			rcs.route_set++;
			if (it.rr_flags & RRF::AUTO_ROUTE) {
				rcs.route_set_auto++;
			}
		}
	}
}

void track_reservation_state::ReservationTypeCountInDirection(reservation_count_set &rcs, EDGETYPE direction) const {
	for (auto &it : itrss) {
		if (it.rr_flags & RRF::RESERVE && it.direction == direction) {
			rcs.route_set++;
			if (it.rr_flags & RRF::AUTO_ROUTE) {
				rcs.route_set_auto++;
			}
		}
	}
}

void track_reservation_state::Deserialise(const deserialiser_input &di, error_collection &ec) {
	itrss.clear();
	CheckIterateJsonArrayOrType<json_object>(di, "reservations", "track_reservation", ec, [&](const deserialiser_input &innerdi, error_collection &ec) {
		itrss.emplace_back();
		inner_track_reservation_state &itrs = itrss.back();

		DeserialiseRouteTargetByParentAndIndex(itrs.reserved_route, innerdi, ec, true);
		CheckTransJsonValue(itrs.direction, innerdi, "direction", ec);
		CheckTransJsonValue(itrs.index, innerdi, "index", ec);
		CheckTransJsonValue(itrs.rr_flags, innerdi, "rr_flags", ec);
		innerdi.PostDeserialisePropCheck(ec);
	});
}

void track_reservation_state::Serialise(serialiser_output &so, error_collection &ec) const {
	if (!itrss.empty()) {
		so.json_out.String("reservations");
		so.json_out.StartArray();
		for (auto it = itrss.begin(); it != itrss.end(); ++it) {
			so.json_out.StartObject();
			SerialiseRouteTargetByParentAndIndex(it->reserved_route, so, ec);
			SerialiseValueJson(it->direction, so, "direction");
			SerialiseValueJson(it->index, so, "index");
			SerialiseValueJson(it->rr_flags, so, "rr_flags");
			so.json_out.EndObject();
		}
		so.json_out.EndArray();
	}
}
