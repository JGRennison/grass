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

#ifndef INC_SIGNAL_ALREADY
#define INC_SIGNAL_ALREADY

#include <vector>

#include "util/flags.h"
#include "core/track.h"
#include "core/track_reservation.h"
#include "core/route_types.h"
#include "core/traction_type.h"
#include "core/route.h"

enum class GMRF : unsigned int {
	ZERO                = 0,
	DYNAMIC_PRIORITY    = 1<<2,
	CHECK_TRY_RESERVE   = 1<<3,
	CHECK_VIAS          = 1<<4,
	DONT_CLEAR_VECTOR   = 1<<5,
	DONT_SORT           = 1<<6,
};
template<> struct enum_traits< GMRF > { static constexpr bool flags = true; };

enum class ASPECT_FLAGS : unsigned int {
	ZERO              = 0,
	MAX_NOT_BINDING   = 1<<0, ///< for banner repeaters, etc. the aspect does not override any previous longer aspect
};
template<> struct enum_traits< ASPECT_FLAGS > { static constexpr bool flags = true; };

enum class RPRT_FLAGS {
	ZERO            = 0,
	VIA             = 1<<0,
};
template<> struct enum_traits< RPRT_FLAGS > { static constexpr bool flags = true; };

struct RPRT {
	route_class::set start;
	route_class::set through;
	route_class::set end;
	RPRT_FLAGS flags;

	inline route_class::set &GetRCSByRRF(RRF rr_flags) {
		if (rr_flags & RRF::START_PIECE) {
			return start;
		} else if (rr_flags & RRF::END_PIECE) {
			return end;
		} else {
			return through;
		}
	}

	RPRT() : start(0), through(0), end(0), flags(RPRT_FLAGS::ZERO) { }
	RPRT(route_class::set start_, route_class::set through_, route_class::set end_, RPRT_FLAGS flags_ = RPRT_FLAGS::ZERO)
			: start(start_), through(through_), end(end_), flags(flags_) { }
};
inline bool operator==(const RPRT &l, const RPRT &r) {
	return l.end == r.end && l.start == r.start && l.through == r.through && l.flags == r.flags;
}
std::ostream& operator<<(std::ostream& os, const RPRT& obj);

class routing_point : public generic_zlen_track {
	routing_point *aspect_target = nullptr;
	routing_point *aspect_route_target = nullptr;
	routing_point *aspect_backwards_dependency = nullptr;

	route_restriction_set endrestrictions;

	protected:
	unsigned int aspect = 0;
	unsigned int reserved_aspect = 0;
	route_class::ID aspect_type  = route_class::RTC_NULL;
	void SetAspectNextTarget(routing_point *target);
	void SetAspectRouteTarget(routing_point *target);

	public:
	routing_point(world &w_) : generic_zlen_track(w_) { }

	inline unsigned int GetAspect() const { return aspect; }
	inline unsigned int GetReservedAspect() const { return reserved_aspect; }   //this includes approach locking aspects
	inline const routing_point *GetAspectNextTarget() const { return aspect_target; }
	inline const routing_point *GetAspectRouteTarget() const { return aspect_route_target; }
	inline const routing_point *GetAspectBackwardsDependency() const { return aspect_backwards_dependency; }
	inline route_class::ID GetAspectType() const { return aspect_type; }
	virtual ASPECT_FLAGS GetAspectFlags() const { return ASPECT_FLAGS::ZERO; }
	virtual void UpdateRoutingPoint() { }

	virtual RPRT GetAvailableRouteTypes(EDGE direction) const = 0;
	virtual RPRT GetSetRouteTypes(EDGE direction) const = 0;

	virtual route *GetRouteByIndex(unsigned int index) = 0;
	const route *FindBestOverlap(route_class::set types = route_class::AllOverlaps()) const;
	void EnumerateAvailableOverlaps(std::function<void(const route *rt, int score)> func, RRF extra_flags = RRF::ZERO,
			route_class::set types = route_class::AllOverlaps()) const;
	virtual void EnumerateRoutes(std::function<void (const route *)> func) const;

	enum class GPBF {
		ZERO            = 0,
		GET_NON_EMPTY   = 1<<0,
	};
	virtual track_berth *GetPriorBerth(EDGE direction, GPBF flags) { return nullptr; }
	inline const track_berth *GetPriorBerth(EDGE direction, GPBF flags) const {
		return const_cast<routing_point*>(this)->GetPriorBerth(direction, flags);
	}

	struct gmr_route_item {
		const route *rt;
		int score;
	};
	unsigned int GetMatchingRoutes(std::vector<gmr_route_item> &routes, const routing_point *end, route_class::set rc_mask = route_class::AllNonOverlaps(),
			GMRF gmr_flags = GMRF::ZERO, RRF extra_flags = RRF::ZERO, const via_list &vias = via_list()) const;

	const route_restriction_set &GetRouteEndRestrictions() const { return endrestrictions; }

	virtual std::string GetTypeName() const override { return "Track Routing Point"; }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};
template<> struct enum_traits< routing_point::GPBF > { static constexpr bool flags = true; };

class track_routing_point_deserialisation_extras_base {
	public:
	track_routing_point_deserialisation_extras_base() { }
	virtual ~track_routing_point_deserialisation_extras_base() { }
};

class track_routing_point : public routing_point {
	protected:
	track_target_ptr prev;
	track_target_ptr next;
	RPRT available_route_types_forward;
	RPRT available_route_types_reverse;

	protected:
	std::unique_ptr<track_routing_point_deserialisation_extras_base> trp_de;

	public:
	track_routing_point(world &w_) : routing_point(w_) { }
	virtual bool IsEdgeValid(EDGE edge) const override;
	virtual const edge_track_target GetEdgeConnectingPiece(EDGE edgeid) override;
	virtual const edge_track_target GetConnectingPiece(EDGE direction) override;
	virtual unsigned int GetMaxConnectingPieces(EDGE direction) const override;
	virtual const edge_track_target GetConnectingPieceByIndex(EDGE direction, unsigned int index) override;
	virtual EDGE GetReverseDirection(EDGE direction) const override;
	virtual EDGE GetDefaultValidDirecton() const override { return EDGE::FRONT; }
	virtual RPRT GetAvailableRouteTypes(EDGE direction) const override;

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	protected:
	virtual EDGE GetAvailableAutoConnectionDirection(bool forward_connection) const override;
	virtual void GetListOfEdges(std::vector<edgelistitem> &output_list) const override;
};

enum class GSF : unsigned int {
	ZERO                     = 0,
	REPEATER                 = 1<<0,
	NON_ASPECTED_REPEATER    = 1<<1, ///< true for banner repeaters, etc. not true for "standard" repeaters which show an aspect
	NO_OVERLAP               = 1<<2,
	APPROACH_LOCKING_MODE    = 1<<3, ///< route cancelled with train approaching, hold aspect at 0
	AUTO_SIGNAL              = 1<<4,
	OVERLAP_SWINGABLE        = 1<<5,
	OVERLAP_TIMEOUT_STARTED  = 1<<6,
};
template<> struct enum_traits< GSF > { static constexpr bool flags = true; };

class generic_signal : public track_routing_point {
	private:
	world_time overlap_timeout_start = 0;

	protected:
	GSF sflags;
	track_reservation_state start_trs;
	track_reservation_state end_trs;

	world_time last_state_update = 0;
	world_time last_route_prove_time = 0;
	world_time last_route_clear_time = 0;
	world_time last_route_set_time = 0;
	unsigned int overlap_swing_min_aspect_distance = 1;

	std::array<unsigned int, route_class::LAST_RTC> approach_locking_default_timeouts;
	route_common route_defaults;

	route_class::set available_overlaps = 0;

	public:
	generic_signal(world &w_);
	virtual ~generic_signal();

	virtual std::string GetTypeName() const override { return "Generic Signal"; }

	virtual GSF GetSignalFlags() const;
	virtual GSF SetSignalFlagsMasked(GSF set_flags, GSF mask_flags);
	virtual GTF GetFlags(EDGE direction) const override;

	virtual RPRT GetSetRouteTypes(EDGE direction) const override;

	virtual const route *GetCurrentForwardRoute() const;                                            //this will not return the overlap, only the "real" route
	virtual const route *GetCurrentForwardOverlap() const;                                          //this will only return the overlap, not the "real" route
	virtual void EnumerateCurrentBackwardsRoutes(std::function<void (const route *)> func) const;   //this will return all routes which currently terminate here
	virtual bool RepeaterAspectMeaningfulForRouteType(route_class::ID type) const;
	virtual track_berth *GetPriorBerth(EDGE direction, GPBF flags) override;

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	virtual void UpdateSignalState();
	virtual void UpdateRoutingPoint() override { UpdateSignalState(); }
	virtual void TrackTick() override { UpdateSignalState(); }

	virtual unsigned int GetTRSList(std::vector<track_reservation_state *> &output_list) override;

	inline unsigned int GetOverlapMinAspectDistance() const { return overlap_swing_min_aspect_distance; }
	bool IsOverlapSwingPermitted(std::string *fail_reason_key = nullptr) const;
	inline route_class::set GetAvailableOverlapTypes() const { return available_overlaps; }

	virtual ASPECT_FLAGS GetAspectFlags() const override;
	const route_common &GetRouteDefaults() const { return route_defaults; }

	//function parameters: return true to continue, false to stop
	void BackwardsReservedTrackScan(std::function<bool(const generic_signal*)> checksignal, std::function<bool(const track_target_ptr&)> check_piece) const;

	protected:
	bool PostLayoutInitTrackScan(error_collection &ec, unsigned int max_pieces, unsigned int junction_max, route_restriction_set *restrictions,
			std::function<route*(route_class::ID type, const track_target_ptr &piece)> make_blank_route);
	virtual bool ReservationV(EDGE direction, unsigned int index, RRF rr_flags, const route *res_route, std::string* fail_reason_key = nullptr) override;
};

class std_signal : public generic_signal {
	public:
	std_signal(world &w_);
	virtual std::string GetTypeName() const override { return "Standard Signal"; }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class auto_signal : public std_signal {
	route signal_route;
	route overlap_route;

	public:
	auto_signal(world &w_)
			: std_signal(w_) {
		available_route_types_forward.start |= route_class::Flag(route_class::RTC_ROUTE);
		available_route_types_forward.end |= route_class::AllNonOverlaps();
		sflags |= GSF::AUTO_SIGNAL;
	}

	bool PostLayoutInit(error_collection &ec) override;
	virtual std::string GetTypeName() const override { return "Automatic Signal"; }

	virtual route *GetRouteByIndex(unsigned int index) override;

	static std::string GetTypeSerialisationNameStatic() { return "auto_signal"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	virtual void EnumerateRoutes(std::function<void (const route *)> func) const override;
};

class route_signal : public std_signal {
	std::list<route> signal_routes;
	route_restriction_set restrictions;

	public:
	route_signal(world &w_) : std_signal(w_) { }
	virtual bool PostLayoutInit(error_collection &ec) override;
	virtual std::string GetTypeName() const override { return "Route Signal"; }

	virtual route *GetRouteByIndex(unsigned int index) override;

	static std::string GetTypeSerialisationNameStatic() { return "route_signal"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	const route_restriction_set &GetRouteRestrictions() const { return restrictions; }

	virtual void EnumerateRoutes(std::function<void (const route *)> func) const override;
};

class repeater_signal : public generic_signal {
	public:
	repeater_signal(world &w_) : generic_signal(w_) { sflags |= GSF::REPEATER | GSF::NO_OVERLAP; }
	virtual std::string GetTypeName() const override { return "Repeater Signal"; }
	static std::string GetTypeSerialisationNameStatic() { return "repeater_signal"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
	virtual route *GetRouteByIndex(unsigned int index) override { return nullptr; }
};

class start_of_line : public routing_point {
	protected:
	track_target_ptr connection;
	RPRT available_route_types;
	track_reservation_state trs;

	public:
	start_of_line(world &w_) : routing_point(w_) { available_route_types.end |= route_class::AllNonOverlaps(); }
	virtual bool IsEdgeValid(EDGE edge) const override;
	virtual const edge_track_target GetEdgeConnectingPiece(EDGE edgeid) override;
	virtual const edge_track_target GetConnectingPiece(EDGE direction) override;
	virtual unsigned int GetMaxConnectingPieces(EDGE direction) const override;
	virtual const edge_track_target GetConnectingPieceByIndex(EDGE direction, unsigned int index) override;
	virtual EDGE GetReverseDirection(EDGE direction) const override;
	virtual EDGE GetDefaultValidDirecton() const override { return EDGE::FRONT; }
	virtual GTF GetFlags(EDGE direction) const override;

	virtual void TrainEnter(EDGE direction, train *t) override { }
	virtual void TrainLeave(EDGE direction, train *t) override { }

	virtual RPRT GetAvailableRouteTypes(EDGE direction) const override;
	virtual RPRT GetSetRouteTypes(EDGE direction) const override;
	virtual bool ReservationV(EDGE direction, unsigned int index, RRF rr_flags, const route *res_route, std::string* fail_reason_key) override;
	virtual unsigned int GetTRSList(std::vector<track_reservation_state *> &output_list) override;

	virtual route *GetRouteByIndex(unsigned int index) override { return nullptr; }

	virtual std::string GetTypeName() const override { return "Start/End of Line"; }

	static std::string GetTypeSerialisationNameStatic() { return "start_of_line"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	protected:
	virtual EDGE GetAvailableAutoConnectionDirection(bool forward_connection) const override;
	virtual void GetListOfEdges(std::vector<edgelistitem> &output_list) const override;
};

class end_of_line : public start_of_line {
	public:
	end_of_line(world &w_) : start_of_line(w_) { }

	static std::string GetTypeSerialisationNameStatic() { return "end_of_line"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }

	protected:
	virtual EDGE GetAvailableAutoConnectionDirection(bool forward_connection) const override;
};

class routing_marker : public track_routing_point {
	protected:
	track_reservation_state trs;

	public:
	routing_marker(world &w_) : track_routing_point(w_) {
		available_route_types_forward.through = route_class::All();
		available_route_types_reverse.through = route_class::All();
	}
	virtual void TrainEnter(EDGE direction, train *t) override { }
	virtual void TrainLeave(EDGE direction, train *t) override { }
	virtual GTF GetFlags(EDGE direction) const override;

	virtual route *GetRouteByIndex(unsigned int index) override { return nullptr; }

	virtual bool ReservationV(EDGE direction, unsigned int index, RRF rr_flags, const route *res_route, std::string* fail_reason_key) override;
	virtual RPRT GetAvailableRouteTypes(EDGE direction) const override;
	virtual RPRT GetSetRouteTypes(EDGE direction) const override;
	virtual unsigned int GetTRSList(std::vector<track_reservation_state *> &output_list) override;

	virtual std::string GetTypeName() const override { return "Routing Marker"; }

	static std::string GetTypeSerialisationNameStatic() { return "routing_marker"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

inline const generic_signal* FastSignalCast(const generic_track *gt, EDGE direction) {
	if (gt && gt->GetFlags(direction) & GTF::SIGNAL) {
		return static_cast<const generic_signal*>(gt);
	}
	return nullptr;
}
inline const generic_signal* FastSignalCast(const generic_track *gt) {
	if (gt) {
		return FastSignalCast(gt, gt->GetDefaultValidDirecton());
	}
	return nullptr;
}
inline const routing_point* FastRoutingpointCast(const generic_track *gt, EDGE direction) {
	if (gt && gt->GetFlags(direction) & GTF::ROUTING_POINT) {
		return static_cast<const routing_point*>(gt);
	}
	return nullptr;
}
inline const routing_point* FastRoutingpointCast(const generic_track *gt) {
	if (gt) {
		return FastRoutingpointCast(gt, gt->GetDefaultValidDirecton());
	}
	return nullptr;
}

inline generic_signal* FastSignalCast(generic_track *gt, EDGE direction) {
	return const_cast<generic_signal*>(FastSignalCast(const_cast<const generic_track*>(gt), direction));
}
inline generic_signal* FastSignalCast(generic_track *gt) {
	return const_cast<generic_signal*>(FastSignalCast(const_cast<const generic_track*>(gt)));
}
inline routing_point* FastRoutingpointCast(generic_track *gt, EDGE direction) {
	return const_cast<routing_point*>(FastRoutingpointCast(const_cast<const generic_track*>(gt), direction));
}
inline routing_point* FastRoutingpointCast(generic_track *gt) {
	return const_cast<routing_point*>(FastRoutingpointCast(const_cast<const generic_track*>(gt)));
}

#endif
