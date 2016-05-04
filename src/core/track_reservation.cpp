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
#include <algorithm>
#include "common.h"
#include "core/track_reservation.h"
#include "core/track.h"
#include "core/signal.h"
#include "core/serialisable_impl.h"

reservation_result &reservation_result::AddConflict(const route *r) {
	flags |= RSRVRF::FAILED | RSRVRF::ROUTE_CONFLICT;
	RSRVRIF conflict_flags = RSRVRIF::ZERO;
	if (route_class::IsOverlap(r->type)) {
		generic_signal *rt_start = FastSignalCast(r->start.track, r->start.direction);
		if (rt_start && rt_start->IsOverlapSwingPermitted(nullptr)) {
			conflict_flags |= RSRVRIF::SWINGABLE_OVERLAP;
		}
	}
	conflicts.push_back({ conflict_flags, r });
	return *this;
}

reservation_result &reservation_result::MergeFrom(const reservation_result &other) {
	flags |= other.flags;
	for (auto &it : other.conflicts) {
		auto cit = std::find_if(conflicts.begin(), conflicts.end(), [&](const reservation_result_conflict &c) {
			return c.conflict_route == it.conflict_route;
		});
		if (cit != conflicts.end()) {
			cit->conflict_flags |= it.conflict_flags;
		} else {
			conflicts.push_back(it);
		}
	}
	return *this;
}

track_reservation_state_backup_guard::~track_reservation_state_backup_guard() {
	for (auto &it : backups) {
		it.first->itrss = std::move(it.second);
	}
}

reservation_result track_reservation_state::Reservation(const reservation_request_res &req) {
	auto check_preserve = [&]() {
		if (!req.backup_guard) return;

		// this does nothing if the backup already contains this track_reservation_state
		req.backup_guard->backups.emplace(this, itrss);
	};

	reservation_result res;
	if (req.rr_flags & (RRF::RESERVE | RRF::TRY_RESERVE | RRF::PROVISIONAL_RESERVE)) {
		for (auto &it : itrss) {
			if (!(req.rr_flags & RRF::IGNORE_EXISTING) && it.rr_flags & (RRF::RESERVE | RRF::PROVISIONAL_RESERVE)) {    //track already reserved
				if (req.rr_flags & RRF::IGNORE_OWN_OVERLAP && it.reserved_route && it.reserved_route->start == req.res_route->start
						&& route_class::IsOverlap(it.reserved_route->type)) {
					//do nothing, own overlap is being ignored
				} else if (it.direction != req.direction || it.index != req.index) {
					if (req.fail_reason_key) {
						*(req.fail_reason_key) = "track/reservation/conflict";
					}
					res.AddConflict(it.reserved_route);    //reserved piece doesn't match
				}
			}
			if (!res.IsSuccess()) continue;
			if (it.rr_flags & RRF::PROVISIONAL_RESERVE && req.rr_flags & RRF::RESERVE) {
				if (it.reserved_route == req.res_route && (req.rr_flags & RRF::SAVEMASK & ~RRF::RESERVE)
						== (it.rr_flags & RRF::SAVEMASK & ~RRF::PROVISIONAL_RESERVE)) {
					//we are now properly reserving what we already preliminarily reserved, reuse inner_track_reservation_state
					check_preserve();
					it.rr_flags |= RRF::RESERVE;
					it.rr_flags &= ~RRF::PROVISIONAL_RESERVE;
					return res;
				}
			}
		}
		if (!res.IsSuccess()) return res;
		if (req.rr_flags & (RRF::RESERVE | RRF::PROVISIONAL_RESERVE)) {
			check_preserve();
			itrss.emplace_back();
			inner_track_reservation_state &itrs = itrss.back();

			itrs.rr_flags = req.rr_flags & RRF::SAVEMASK;
			itrs.direction = req.direction;
			itrs.index = req.index;
			itrs.reserved_route = req.res_route;
		}
		return res;
	} else if (req.rr_flags & (RRF::UNRESERVE | RRF::TRY_UNRESERVE)) {
		for (auto it = itrss.begin(); it != itrss.end(); ++it) {
			if (it->rr_flags & RRF::RESERVE && it->direction == req.direction && it->index == req.index && it->reserved_route == req.res_route) {
				if (req.rr_flags & RRF::UNRESERVE) {
					check_preserve();
					itrss.erase(it);
				}
				return res;
			}
		}
	}
	res.flags |= RSRVRF::FAILED | RSRVRF::INVALID_OP;
	return res;
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

bool track_reservation_state::IsReservedInDirection(EDGE direction) const {
	for (auto &it : itrss) {
		if (it.rr_flags & RRF::RESERVE && it.direction == direction) {
			return true;
		}
	}
	return false;
}

GTF track_reservation_state::GetGTReservationFlags(EDGE checkdirection) const {
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

void track_reservation_state::ReservationTypeCountInDirection(reservation_count_set &rcs, EDGE direction) const {
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
