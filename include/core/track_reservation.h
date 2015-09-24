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

#ifndef INC_TRACK_RESERVATION_ALREADY
#define INC_TRACK_RESERVATION_ALREADY

#include <string>
#include <functional>
#include <vector>
#include "util/error.h"
#include "util/flags.h"
#include "core/edge_type.h"
#include "core/serialisable.h"

class route;

enum class RRF : unsigned int {
	ZERO                    = 0,
	RESERVE                 = 1<<0,
	UNRESERVE               = 1<<1,
	TRY_RESERVE             = 1<<2,
	TRY_UNRESERVE           = 1<<3,
	AUTO_ROUTE              = 1<<4,
	START_PIECE             = 1<<5,
	END_PIECE               = 1<<6,

	PROVISIONAL_RESERVE     = 1<<8,       //for generic_track::RouteReservation, to prevent action/future race condition
	STOP_ON_OCCUPIED_TC     = 1<<9,       //for track dereservations, stop upon reaching an occupied track circuit
	IGNORE_OWN_OVERLAP      = 1<<10,      //for overlap swinging checks

	SAVEMASK                = AUTO_ROUTE | START_PIECE | END_PIECE | RESERVE | PROVISIONAL_RESERVE,
};
template<> struct enum_traits< RRF > { static constexpr bool flags = true; };

class track_reservation_state;

class inner_track_reservation_state {
	friend track_reservation_state;

	const route *reserved_route = nullptr;
	EDGE direction = EDGE::INVALID;
	unsigned int index = 0;
	RRF rr_flags = RRF::ZERO;
};

enum class GTF : unsigned int {
	ZERO             = 0,
	ROUTE_SET        = 1<<0,
	ROUTE_THIS_DIR   = 1<<1,
	ROUTE_FORK       = 1<<2,
	ROUTING_POINT    = 1<<3, ///< this track object **MUST** be static_castable to routing_point
	SIGNAL           = 1<<4, ///< this track object **MUST** be static_castable to generic_signal
};
template<> struct enum_traits< GTF > { static constexpr bool flags = true; };

struct reservation_count_set {
	unsigned int route_set = 0;
	unsigned int route_set_auto = 0;
};

class track_reservation_state : public serialisable_obj {
	std::vector<inner_track_reservation_state> itrss;

	public:
	bool Reservation(EDGE direction, unsigned int index, RRF rr_flags, const route *res_route, std::string* fail_reason_key = nullptr);
	GTF GetGTReservationFlags(EDGE direction) const;
	bool IsReserved() const;
	unsigned int GetReservationCount() const;
	bool IsReservedInDirection(EDGE direction) const;

	// These are templated/inlined as they are called very frequently
	// F has the function signature: void(const route *reserved_route, EDGE direction, unsigned int index, RRF rr_flags)
	template <typename F> unsigned int ReservationEnumeration(F func, RRF check_mask = RRF::RESERVE) const {
		unsigned int counter = 0;
		for (const auto &it : itrss) {
			if (it.rr_flags & check_mask) {
				func(it.reserved_route, it.direction, it.index, it.rr_flags);
				counter++;
			}
		}
		return counter;
	}
	template <typename F> unsigned int ReservationEnumerationInDirection(EDGE direction, F func, RRF check_mask = RRF::RESERVE) const {
		unsigned int counter = 0;
		for (const auto &it : itrss) {
			if (it.rr_flags & check_mask && it.direction == direction) {
				func(it.reserved_route, it.direction, it.index, it.rr_flags);
				counter++;
			}
		}
		return counter;
	}

	void ReservationTypeCount(reservation_count_set &rcs) const;
	void ReservationTypeCountInDirection(reservation_count_set &rcs, EDGE direction) const;

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

#endif