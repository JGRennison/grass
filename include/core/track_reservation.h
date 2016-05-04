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
struct action;
class track_reservation_state_backup_guard;
class track_reservation_state;

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
	IGNORE_EXISTING         = 1<<10,      //for overlap swinging

	SAVEMASK                = AUTO_ROUTE | START_PIECE | END_PIECE | RESERVE | PROVISIONAL_RESERVE,
	RESERVE_MASK            = RESERVE | PROVISIONAL_RESERVE | TRY_RESERVE,
	UNRESERVE_MASK          = UNRESERVE | TRY_UNRESERVE,
};
template<> struct enum_traits< RRF > { static constexpr bool flags = true; };

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

enum class RSRVRF : unsigned char {
	ZERO                        = 0,
	FAILED                      = 1<<0,
	INVALID_OP                  = 1<<1,
	ROUTE_CONFLICT              = 1<<2,
	STOP_ON_OCCUPIED_TC         = 1<<3,
	POINTS_LOCKED               = 1<<4,
};
template<> struct enum_traits< RSRVRF > { static constexpr bool flags = true; };

enum class RSRVRIF : unsigned char {
	ZERO               = 0,
	SWINGABLE_OVERLAP  = 1<<0,
};
template<> struct enum_traits< RSRVRIF > { static constexpr bool flags = true; };

struct reservation_request_base {
	EDGE direction;
	unsigned int index;
	RRF rr_flags;
	const route *res_route;

	reservation_request_base(EDGE direction_, unsigned int index_, RRF rr_flags_, const route *res_route_)
			: direction(direction_), index(index_), rr_flags(rr_flags_), res_route(res_route_) { }
};

struct reservation_request_res : public reservation_request_base {
	std::string* fail_reason_key;
	track_reservation_state_backup_guard *backup_guard = nullptr;

	reservation_request_res(EDGE direction_, unsigned int index_, RRF rr_flags_, const route *res_route_, std::string* fail_reason_key_ = nullptr)
			: reservation_request_base(direction_, index_, rr_flags_, res_route_), fail_reason_key(fail_reason_key_) { }
	reservation_request_res(const reservation_request_base &base, std::string* fail_reason_key_)
			: reservation_request_base(base), fail_reason_key(fail_reason_key_) { }

	reservation_request_res &SetBackupGuard(track_reservation_state_backup_guard *guard) {
		backup_guard = guard;
		return *this;
	}
};

struct reservation_request_action : public reservation_request_base {
	std::function<void(action &&reservation_act)> submit_action;

	reservation_request_action(EDGE direction_, unsigned int index_, RRF rr_flags_, const route *res_route_, std::function<void(action &&reservation_act)> submit_action_)
			: reservation_request_base(direction_, index_, rr_flags_, res_route_), submit_action(std::move(submit_action_)) { }
	reservation_request_action(const reservation_request_base &base, std::function<void(action &&reservation_act)> submit_action_)
			: reservation_request_base(base), submit_action(std::move(submit_action_)) { }
};

class track_reservation_state_backup_guard {
	friend track_reservation_state;

	std::map<track_reservation_state *, std::vector<inner_track_reservation_state>> backups;

	public:
	~track_reservation_state_backup_guard();
};

struct reservation_result {
	struct reservation_result_conflict {
		RSRVRIF conflict_flags;
		const route *conflict_route;
	};

	std::vector<reservation_result_conflict> conflicts;
	RSRVRF flags;

	reservation_result() : flags(RSRVRF::ZERO) { }
	reservation_result(RSRVRF flags_) : flags(flags_) { }
	reservation_result(flagwrapper<RSRVRF> flags_) : flags(flags_) { }
	bool IsSuccess() const { return !(flags & RSRVRF::FAILED); }
	reservation_result &MergeFrom(const reservation_result &other);
	reservation_result &AddConflict(const route *r);
};

class track_reservation_state : public serialisable_obj {
	friend track_reservation_state_backup_guard;

	std::vector<inner_track_reservation_state> itrss;

	public:
	reservation_result Reservation(const reservation_request_res &req);
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
