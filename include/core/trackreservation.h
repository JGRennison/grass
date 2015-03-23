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

#ifndef INC_TRACKRESERVATION_ALREADY
#define INC_TRACKRESERVATION_ALREADY

#include <string>
#include <functional>
#include <vector>
#include "util/error.h"
#include "util/flags.h"
#include "core/edgetype.h"
#include "core/serialisable.h"

class route;

enum class RRF : unsigned int {
	ZERO                    = 0,
	RESERVE                 = 1<<0,
	UNRESERVE               = 1<<1,
	TRYRESERVE              = 1<<2,
	TRYUNRESERVE            = 1<<3,
	AUTOROUTE               = 1<<4,
	STARTPIECE              = 1<<5,
	ENDPIECE                = 1<<6,

	PROVISIONAL_RESERVE     = 1<<8,       //for generictrack::RouteReservation, to prevent action/future race condition
	STOP_ON_OCCUPIED_TC     = 1<<9,       //for track dereservations, stop upon reaching an occupied track circuit
	IGNORE_OWN_OVERLAP      = 1<<10,      //for overlap swinging checks

	SAVEMASK                = AUTOROUTE | STARTPIECE | ENDPIECE | RESERVE | PROVISIONAL_RESERVE,
};
template<> struct enum_traits< RRF > { static constexpr bool flags = true; };

class track_reservation_state;

class inner_track_reservation_state {
	friend track_reservation_state;

	const route *reserved_route = nullptr;
	EDGETYPE direction = EDGE_NULL;
	unsigned int index = 0;
	RRF rr_flags = RRF::ZERO;
};

enum class GTF : unsigned int {
	ZERO             = 0,
	ROUTESET         = 1<<0,
	ROUTETHISDIR     = 1<<1,
	ROUTEFORK        = 1<<2,
	ROUTINGPOINT     = 1<<3,        //this track object **MUST** be static_castable to routingpoint
	SIGNAL           = 1<<4,        //this track object **MUST** be static_castable to genericsignal
};
template<> struct enum_traits< GTF > { static constexpr bool flags = true; };

struct reservationcountset {
	unsigned int routeset = 0;
	unsigned int routesetauto = 0;
};

class track_reservation_state : public serialisable_obj {
	std::vector<inner_track_reservation_state> itrss;

	public:
	bool Reservation(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey = nullptr);
	GTF GetGTReservationFlags(EDGETYPE direction) const;
	bool IsReserved() const;
	unsigned int GetReservationCount() const;
	bool IsReservedInDirection(EDGETYPE direction) const;

	// These are templated/inlined as they are called very frequently
	// F has the function signature: void(const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags)
	template <typename F> unsigned int ReservationEnumeration(F func, RRF checkmask = RRF::RESERVE) const {
		unsigned int counter = 0;
		for(const auto &it : itrss) {
			if(it.rr_flags & checkmask) {
				func(it.reserved_route, it.direction, it.index, it.rr_flags);
				counter++;
			}
		}
		return counter;
	}
	template <typename F> unsigned int ReservationEnumerationInDirection(EDGETYPE direction, F func, RRF checkmask = RRF::RESERVE) const {
		unsigned int counter = 0;
		for(const auto &it : itrss) {
			if(it.rr_flags & checkmask && it.direction == direction) {
				func(it.reserved_route, it.direction, it.index, it.rr_flags);
				counter++;
			}
		}
		return counter;
	}

	void ReservationTypeCount(reservationcountset &rcs) const;
	void ReservationTypeCountInDirection(reservationcountset &rcs, EDGETYPE direction) const;

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

#endif
