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

#ifndef INC_ROUTE_ALREADY
#define INC_ROUTE_ALREADY

#include <string>
#include <vector>

#include "util/flags.h"
#include "core/track.h"
#include "core/route_types.h"
#include "core/traverse.h"
#include "core/track_reservation.h"

class route;
class routing_point;
class track_circuit;
class generic_signal;
class track_berth;
class route_restriction_set;
typedef std::vector<routing_point *> via_list;
typedef std::vector<track_circuit *> tc_list;
typedef std::vector<generic_signal *> sig_list;
typedef std::vector<route_recording_item> passable_test_list;


typedef uint32_t aspect_mask_type; // This is a bitmask for aspects: 0 (LSB) to 31 (MSB)
const unsigned int ASPECT_MAX = 31;

inline bool aspect_in_mask(aspect_mask_type mask, unsigned int aspect) {
	if (aspect > ASPECT_MAX) {
		return false;
	} else {
		return mask & (1 << aspect);
	}
}

inline bool is_higher_aspect_in_mask(aspect_mask_type mask, unsigned int aspect) {
	if (aspect >= ASPECT_MAX) {
		return false;
	} else {
		return mask & ((~1) << aspect);
	}
}

struct berth_record {
	track_berth *berth = nullptr;
	generic_track *owner_track = nullptr;
	berth_record(track_berth *b, generic_track *o = nullptr) : berth(b), owner_track(o) {}
};
typedef std::vector<berth_record> berth_list;

bool DeserialiseAspectProps(unsigned int &aspect_mask, const deserialiser_input &di, error_collection &ec);

struct route_common {
	int priority = 0;
	unsigned int approach_locking_timeout = 0;
	unsigned int overlap_timeout = 0;
	unsigned int approach_control_triggerdelay = 0;
	route_class::ID overlap_type = route_class::ID::NONE;
	unsigned int route_prove_delay = 0;
	unsigned int route_clear_delay = 0;
	unsigned int route_set_delay = 0;
	aspect_mask_type aspect_mask = 3;

	struct conditional_aspect_mask {
		aspect_mask_type condition = 1;
		aspect_mask_type aspect_mask = 1;
	};
	std::vector<conditional_aspect_mask> conditional_aspect_masks;

	track_train_counter_block *approach_control_trigger = nullptr;
	track_train_counter_block *overlap_timeout_trigger = nullptr;

	enum class RCF {
		ZERO                         = 0,
		PRIORITY_SET                 = 1<<0,
		AP_LOCKING_TIMEOUT_SET       = 1<<1,
		OVERLAP_TIMEOUT_SET          = 1<<2,
		AP_CONTROL                   = 1<<3,
		AP_CONTROL_SET               = 1<<4,
		AP_CONTROL_TRIGGER_DELAY_SET = 1<<5,
		TORR                         = 1<<6,
		TORR_SET                     = 1<<7,
		EXIT_SIGNAL_CONTROL          = 1<<8,
		EXIT_SIGNAL_CONTROL_SET      = 1<<9,
		OVERLAP_TYPE_SET             = 1<<10,
		ROUTE_PROVE_DELAY_SET        = 1<<11,
		ROUTE_CLEAR_DELAY_SET        = 1<<12,
		ROUTE_SET_DELAY_SET          = 1<<13,
		ASPECT_MASK_SET              = 1<<14,
		AP_CONTROL_IF_NOROUTE        = 1<<15, // Conditional aspects: apply approach control if route target has no forward route set
		AP_CONTROL_IF_NOROUTE_SET    = 1<<16, // "
	};
	RCF route_common_flags = RCF::ZERO;

	enum class DeserialisationFlags {
		ZERO                        = 0,
		NO_APLOCK_TIMEOUT           = 1<<0,
		ASPECT_MASK_ONLY            = 1<<1,
	};

	void DeserialiseRouteCommon(const deserialiser_input &subdi, error_collection &ec, DeserialisationFlags flags = DeserialisationFlags::ZERO);
	void ApplyTo(route_common &target) const;
};
template<> struct enum_traits< route_common::RCF> { static constexpr bool flags = true; };
template<> struct enum_traits< route_common::DeserialisationFlags> { static constexpr bool flags = true; };

class route_restriction : public route_common {
	friend route_restriction_set;

	std::vector<std::string> targets;
	std::vector<std::string> via;
	std::vector<std::string> not_via;

	route_class::set allowed_types = route_class::All();
	route_class::set apply_to_types = route_class::All();

	public:
	bool CheckRestriction(route_class::set &allowed_routes, const route_recording_list &route_pieces, const track_target_ptr &piece) const;
	void ApplyRestriction(route &rt) const;
	route_class::set GetApplyRouteTypes() const { return apply_to_types; }
};

class route_restriction_set {
	std::vector<route_restriction> restrictions;

	public:
	route_class::set CheckAllRestrictions(std::vector<const route_restriction*> &matching_restrictions, const route_recording_list &route_pieces,
			const track_target_ptr &piece) const;
	void DeserialiseRestriction(const deserialiser_input &subdi, error_collection &ec, bool isendtype);

	unsigned int GetRestrictionCount() const { return restrictions.size(); }
};

struct route  : public route_common {
	vartrack_target_ptr<routing_point> start;
	route_recording_list pieces;
	vartrack_target_ptr<routing_point> end;
	via_list vias;
	tc_list track_circuits;
	sig_list repeater_signals;
	passable_test_list pass_test_list;
	berth_list berths;
	route_class::ID type;

	routing_point *parent = nullptr;
	unsigned int index  = 0;

	route() : type(route_class::ID::NONE) { }
	void FillLists();
	bool TestRouteForMatch(const routing_point *check_end, const via_list &check_vias) const;
	reservation_result RouteReservation(RRF reserve_flags, std::string *fail_reason_key = nullptr) const;
	reservation_result PartialRouteReservationWithActions(RRF reserve_flags, std::string *fail_reason_key, RRF action_reserve_flags,
			std::function<void(action &&reservation_act)> action_callback) const;
	void RouteReservationActions(RRF reserve_flags, std::function<void(action &&reservation_act)> action_callback) const;
	bool IsRouteSubSet(const route *subset) const;
	bool IsStartAnchored(RRF check_mask = RRF::RESERVE) const;
	bool IsRouteTractionSuitable(const train* t) const;
};

bool RouteReservation(route &res_route, RRF rr_flags);

#endif
