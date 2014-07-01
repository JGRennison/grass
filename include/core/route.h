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

#ifndef INC_ROUTE_ALREADY
#define INC_ROUTE_ALREADY

#include <string>
#include <vector>

#include "core/track.h"
#include "core/flags.h"
#include "core/routetypes.h"
#include "core/traverse.h"
#include "core/trackreservation.h"

class route;
class routingpoint;
class track_circuit;
class genericsignal;
class trackberth;
class route_restriction_set;
typedef std::vector<routingpoint *> via_list;
typedef std::vector<track_circuit *> tc_list;
typedef std::vector<genericsignal *> sig_list;
typedef std::vector<route_recording_item> passable_test_list;

struct berth_record {
	trackberth *berth = 0;
	generictrack *ownertrack = 0;
	berth_record(trackberth *b, generictrack *o = 0) : berth(b), ownertrack(o) {}
};
typedef std::vector<berth_record> berth_list;

bool DeserialiseAspectProps(unsigned int &aspect_mask, const deserialiser_input &di, error_collection &ec);

struct route_common {
	int priority = 0;
	unsigned int approachlocking_timeout = 0;
	unsigned int overlap_timeout = 0;
	unsigned int approachcontrol_triggerdelay = 0;
	route_class::ID overlap_type = route_class::ID::RTC_NULL;
	unsigned int routeprove_delay = 0;
	unsigned int routeclear_delay = 0;
	unsigned int routeset_delay = 0;
	unsigned int aspect_mask = 1;

	track_train_counter_block *approachcontrol_trigger = 0;
	track_train_counter_block *overlaptimeout_trigger = 0;

	enum class RCF {
		ZERO                        = 0,
		PRIORITYSET                 = 1<<0,
		APLOCK_TIMEOUTSET           = 1<<1,
		OVERLAPTIMEOUTSET           = 1<<2,
		APCONTROL                   = 1<<3,
		APCONTROL_SET               = 1<<4,
		APCONTROLTRIGGERDELAY_SET   = 1<<5,
		TORR                        = 1<<6,
		TORR_SET                    = 1<<7,
		EXITSIGCONTROL              = 1<<8,
		EXITSIGCONTROL_SET          = 1<<9,
		OVERLAPTYPE_SET             = 1<<10,
		ROUTEPROVEDELAY_SET         = 1<<11,
		ROUTECLEARDELAY_SET         = 1<<12,
		ROUTESETDELAY_SET           = 1<<13,
		ASPECTMASK_SET              = 1<<14,
		APCONTROL_IF_NOROUTE        = 1<<15, // Conditional aspects: apply approach control if route target has no forward route set
		APCONTROL_IF_NOROUTE_SET    = 1<<16, // "
	};
	RCF routecommonflags = RCF::ZERO;

	enum class DeserialisationFlags {
		ZERO                        = 0,
		NO_APLOCK_TIMEOUT           = 1<<0,
		ASPECTMASK_ONLY             = 1<<1,
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
	std::vector<std::string> notvia;

	route_class::set allowedtypes = route_class::All();
	route_class::set applytotypes = route_class::All();

	public:
	bool CheckRestriction(route_class::set &allowed_routes, const route_recording_list &route_pieces, const track_target_ptr &piece) const;
	void ApplyRestriction(route &rt) const;
	route_class::set GetApplyRouteTypes() const { return applytotypes; }
};

class route_restriction_set {
	std::vector<route_restriction> restrictions;

	public:
	route_class::set CheckAllRestrictions(std::vector<const route_restriction*> &matching_restrictions, const route_recording_list &route_pieces, const track_target_ptr &piece) const;
	void DeserialiseRestriction(const deserialiser_input &subdi, error_collection &ec, bool isendtype);

	unsigned int GetRestrictionCount() const { return restrictions.size(); }
};

struct route  : public route_common {
	vartrack_target_ptr<routingpoint> start;
	route_recording_list pieces;
	vartrack_target_ptr<routingpoint> end;
	via_list vias;
	tc_list trackcircuits;
	sig_list repeatersignals;
	passable_test_list passtestlist;
	berth_list berths;
	route_class::ID type;

	routingpoint *parent = 0;
	unsigned int index  = 0;

	route() : type(route_class::RTC_NULL) { }
	void FillLists();
	bool TestRouteForMatch(const routingpoint *checkend, const via_list &checkvias) const;
	bool RouteReservation(RRF reserve_flags, std::string *failreasonkey = 0) const;
	bool PartialRouteReservationWithActions(RRF reserve_flags, std::string *failreasonkey, RRF action_reserve_flags, std::function<void(action &&reservation_act)> actioncallback) const;
	void RouteReservationActions(RRF reserve_flags, std::function<void(action &&reservation_act)> actioncallback) const;
	bool IsRouteSubSet(const route *subset) const;
	bool IsStartAnchored(RRF checkmask = RRF::RESERVE) const;
	bool IsRouteTractionSuitable(const train* t) const;
};

bool RouteReservation(route &res_route, RRF rr_flags);

#endif
