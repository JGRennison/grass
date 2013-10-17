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

#ifndef INC_SIGNAL_ALREADY
#define INC_SIGNAL_ALREADY

#include <vector>

#include "core/track.h"
#include "core/trackreservation.h"
#include "core/flags.h"
#include "core/routetypes.h"
#include "core/tractiontype.h"
#include "core/route.h"

enum class GMRF : unsigned int {
	ZERO               = 0,
	DYNPRIORITY        = 1<<2,
	CHECKTRYRESERVE    = 1<<3,
	CHECKVIAS          = 1<<4,
	DONTCLEARVECTOR    = 1<<5,
	DONTSORT           = 1<<6,
};
template<> struct enum_traits< GMRF > { static constexpr bool flags = true; };

enum class ASPECT_FLAGS : unsigned int {
	ZERO            = 0,
	MAXNOTBINDING   = 1<<0,     //for banner repeaters, etc. the aspect does not override any previous longer aspect
};
template<> struct enum_traits< ASPECT_FLAGS > { static constexpr bool flags = true; };

enum class RPRT_FLAGS {
	ZERO            = 0,
	VIA             = 1<<0,
};
template<> struct enum_traits< RPRT_FLAGS > {   static constexpr bool flags = true; };

struct RPRT {
	route_class::set start;
	route_class::set through;
	route_class::set end;
	RPRT_FLAGS flags;
	inline route_class::set &GetRCSByRRF(RRF rr_flags) {
		if(rr_flags & RRF::STARTPIECE) return start;
		else if(rr_flags & RRF::ENDPIECE) return end;
		else return through;
	}
	RPRT() : start(0), through(0), end(0), flags(RPRT_FLAGS::ZERO) { }
	RPRT(route_class::set start_, route_class::set through_, route_class::set end_, RPRT_FLAGS flags_ = RPRT_FLAGS::ZERO)
		: start(start_), through(through_), end(end_), flags(flags_) { }
};
inline bool operator==(const RPRT &l, const RPRT &r) {
	return l.end == r.end && l.start == r.start && l.through == r.through && l.flags == r.flags;
}
std::ostream& operator<<(std::ostream& os, const RPRT& obj);

class routingpoint : public genericzlentrack {
	routingpoint *aspect_target = 0;
	routingpoint *aspect_route_target = 0;
	routingpoint *aspect_backwards_dependency = 0;

	route_restriction_set endrestrictions;

	protected:
	unsigned int aspect = 0;
	unsigned int reserved_aspect = 0;
	route_class::ID aspect_type  = route_class::RTC_NULL;
	void SetAspectNextTarget(routingpoint *target);
	void SetAspectRouteTarget(routingpoint *target);

	public:
	routingpoint(world &w_) : genericzlentrack(w_) { }

	inline unsigned int GetAspect() const { return aspect; }
	inline unsigned int GetReservedAspect() const { return reserved_aspect; }   //this includes approach locking aspects
	inline routingpoint *GetAspectNextTarget() const { return aspect_target; }
	inline routingpoint *GetAspectRouteTarget() const { return aspect_route_target; }
	inline routingpoint *GetAspectBackwardsDependency() const { return aspect_backwards_dependency; }
	inline route_class::ID GetAspectType() const { return aspect_type; }
	virtual ASPECT_FLAGS GetAspectFlags() const { return ASPECT_FLAGS::ZERO; }
	virtual void UpdateRoutingPoint() { }

	virtual RPRT GetAvailableRouteTypes(EDGETYPE direction) const = 0;
	virtual RPRT GetSetRouteTypes(EDGETYPE direction) const = 0;

	virtual route *GetRouteByIndex(unsigned int index) = 0;
	const route *FindBestOverlap(route_class::set types = route_class::AllOverlaps()) const;
	void EnumerateAvailableOverlaps(std::function<void(const route *rt, int score)> func, RRF extraflags = RRF::ZERO, route_class::set types = route_class::AllOverlaps()) const;
	virtual void EnumerateRoutes(std::function<void (const route *)> func) const;

	enum class GPBF {
		ZERO            = 0,
		GETNONEMPTY     = 1<<0,
	};
	virtual trackberth *GetPriorBerth(EDGETYPE direction, GPBF flags) const { return 0; }

	struct gmr_routeitem {
		const route *rt;
		int score;
	};
	unsigned int GetMatchingRoutes(std::vector<gmr_routeitem> &routes, const routingpoint *end, route_class::set rc_mask = route_class::AllNonOverlaps(), GMRF gmr_flags = GMRF::ZERO, RRF extraflags = RRF::ZERO, const via_list &vias = via_list()) const;

	const route_restriction_set &GetRouteEndRestrictions() const { return endrestrictions; }

	virtual std::string GetTypeName() const override { return "Track Routing Point"; }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
};
template<> struct enum_traits< routingpoint::GPBF > { static constexpr bool flags = true; };

class trackroutingpoint_deserialisation_extras_base {
	public:
	trackroutingpoint_deserialisation_extras_base() { }
	virtual ~trackroutingpoint_deserialisation_extras_base() { }
};

class trackroutingpoint : public routingpoint {
	protected:
	track_target_ptr prev;
	track_target_ptr next;
	RPRT availableroutetypes_forward;
	RPRT availableroutetypes_reverse;

	protected:
	std::unique_ptr<trackroutingpoint_deserialisation_extras_base> trp_de;

	public:
	trackroutingpoint(world &w_) : routingpoint(w_) { }
	virtual bool IsEdgeValid(EDGETYPE edge) const override;
	virtual const track_target_ptr & GetEdgeConnectingPiece(EDGETYPE edgeid) const override;
	virtual const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const override;
	virtual unsigned int GetMaxConnectingPieces(EDGETYPE direction) const override;
	virtual const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const override;
	virtual EDGETYPE GetReverseDirection(EDGETYPE direction) const override;
	virtual EDGETYPE GetDefaultValidDirecton() const override { return EDGE_FRONT; }
	virtual RPRT GetAvailableRouteTypes(EDGETYPE direction) const override;

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	protected:
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const override;
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const override;
};

enum class GSF : unsigned int {
	ZERO                     = 0,
	REPEATER                 = 1<<0,
	NONASPECTEDREPEATER      = 1<<1,     //true for banner repeaters, etc. not true for "standard" repeaters which show an aspect
	NOOVERLAP                = 1<<2,
	APPROACHLOCKINGMODE      = 1<<3,     //route cancelled with train approaching, hold aspect at 0
	AUTOSIGNAL               = 1<<4,
	OVERLAPSWINGABLE         = 1<<5,
	OVERLAPTIMEOUTSTARTED    = 1<<6,
};
template<> struct enum_traits< GSF > { static constexpr bool flags = true; };

class genericsignal : public trackroutingpoint {
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
	unsigned int overlapswingminaspectdistance = 1;

	std::array<unsigned int, route_class::LAST_RTC> approachlocking_default_timeouts;
	route_common route_defaults;

	route_class::set available_overlaps = 0;

	public:
	genericsignal(world &w_);
	virtual ~genericsignal();
	virtual void TrainEnter(EDGETYPE direction, train *t) override;
	virtual void TrainLeave(EDGETYPE direction, train *t) override;

	virtual std::string GetTypeName() const override { return "Generic Signal"; }

	virtual GSF GetSignalFlags() const;
	virtual GSF SetSignalFlagsMasked(GSF set_flags, GSF mask_flags);
	virtual GTF GetFlags(EDGETYPE direction) const override;

	virtual RPRT GetSetRouteTypes(EDGETYPE direction) const override;

	virtual const route *GetCurrentForwardRoute() const;                                            //this will not return the overlap, only the "real" route
	virtual const route *GetCurrentForwardOverlap() const;                                          //this will only return the overlap, not the "real" route
	virtual void EnumerateCurrentBackwardsRoutes(std::function<void (const route *)> func) const;   //this will return all routes which currently terminate here
	virtual bool RepeaterAspectMeaningfulForRouteType(route_class::ID type) const;
	virtual trackberth *GetPriorBerth(EDGETYPE direction, GPBF flags) const override;

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	virtual void UpdateSignalState();
	virtual void UpdateRoutingPoint() override { UpdateSignalState(); }
	virtual void TrackTick() override { UpdateSignalState(); }

	virtual unsigned int GetTRSList(std::vector<track_reservation_state *> &outputlist) override;

	inline unsigned int GetOverlapMinAspectDistance() const { return overlapswingminaspectdistance; }
	bool IsOverlapSwingPermitted(std::string *failreasonkey = 0) const;
	inline route_class::set GetAvailableOverlapTypes() const { return available_overlaps; }

	virtual ASPECT_FLAGS GetAspectFlags() const override;
	const route_common &GetRouteDefaults() const { return route_defaults; }

	//function parameters: return true to continue, false to stop
	void BackwardsReservedTrackScan(std::function<bool(const genericsignal*)> checksignal, std::function<bool(const track_target_ptr&)> checkpiece) const;

	protected:
	bool PostLayoutInitTrackScan(error_collection &ec, unsigned int max_pieces, unsigned int junction_max, route_restriction_set *restrictions, std::function<route*(route_class::ID type, const track_target_ptr &piece)> mkblankroute);
	virtual bool ReservationV(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey = 0) override;
};

class stdsignal : public genericsignal {
	public:
	stdsignal(world &w_);
	virtual std::string GetTypeName() const override { return "Standard Signal"; }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

class autosignal : public stdsignal {
	route signal_route;
	route overlap_route;

	public:
	autosignal(world &w_) : stdsignal(w_) { availableroutetypes_forward.start |= route_class::Flag(route_class::RTC_ROUTE); availableroutetypes_forward.end |= route_class::AllNonOverlaps(); sflags |= GSF::AUTOSIGNAL; }
	bool PostLayoutInit(error_collection &ec) override;
	virtual std::string GetTypeName() const override { return "Automatic Signal"; }

	virtual route *GetRouteByIndex(unsigned int index) override;

	static std::string GetTypeSerialisationNameStatic() { return "autosignal"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	virtual void EnumerateRoutes(std::function<void (const route *)> func) const override;
};

class routesignal : public stdsignal {
	std::list<route> signal_routes;
	route_restriction_set restrictions;

	public:
	routesignal(world &w_) : stdsignal(w_) { }
	virtual bool PostLayoutInit(error_collection &ec) override;
	virtual std::string GetTypeName() const override { return "Route Signal"; }

	virtual route *GetRouteByIndex(unsigned int index) override;

	static std::string GetTypeSerialisationNameStatic() { return "routesignal"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	const route_restriction_set &GetRouteRestrictions() const { return restrictions; }

	virtual void EnumerateRoutes(std::function<void (const route *)> func) const override;
};

class repeatersignal : public genericsignal {
	public:
	repeatersignal(world &w_) : genericsignal(w_) { sflags |= GSF::REPEATER | GSF::NOOVERLAP; }
	virtual std::string GetTypeName() const override { return "Repeater Signal"; }
	static std::string GetTypeSerialisationNameStatic() { return "repeatersignal"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
	virtual route *GetRouteByIndex(unsigned int index) override { return 0; }
};

class startofline : public routingpoint {
	protected:
	track_target_ptr connection;
	RPRT availableroutetypes;
	track_reservation_state trs;

	public:
	startofline(world &w_) : routingpoint(w_) { availableroutetypes.end |= route_class::AllNonOverlaps(); }
	virtual bool IsEdgeValid(EDGETYPE edge) const override;
	virtual const track_target_ptr & GetEdgeConnectingPiece(EDGETYPE edgeid) const override;
	virtual const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const override;
	virtual unsigned int GetMaxConnectingPieces(EDGETYPE direction) const override;
	virtual const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const override;
	virtual EDGETYPE GetReverseDirection(EDGETYPE direction) const override;
	virtual EDGETYPE GetDefaultValidDirecton() const override { return EDGE_FRONT; }
	virtual GTF GetFlags(EDGETYPE direction) const override;

	virtual void TrainEnter(EDGETYPE direction, train *t) override { }
	virtual void TrainLeave(EDGETYPE direction, train *t) override { }

	virtual RPRT GetAvailableRouteTypes(EDGETYPE direction) const override;
	virtual RPRT GetSetRouteTypes(EDGETYPE direction) const override;
	virtual bool ReservationV(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey) override;
	virtual unsigned int GetTRSList(std::vector<track_reservation_state *> &outputlist) override;

	virtual route *GetRouteByIndex(unsigned int index) override { return 0; }

	virtual std::string GetTypeName() const override { return "Start/End of Line"; }

	static std::string GetTypeSerialisationNameStatic() { return "startofline"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	protected:
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const override;
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const override;
};

class endofline : public startofline {
	public:
	endofline(world &w_) : startofline(w_) { }

	static std::string GetTypeSerialisationNameStatic() { return "endofline"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }

	protected:
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const override;
};

class routingmarker : public trackroutingpoint {
	protected:
	track_reservation_state trs;

	public:
	routingmarker(world &w_) : trackroutingpoint(w_) {
		availableroutetypes_forward.through = route_class::All();
		availableroutetypes_reverse.through = route_class::All();
	}
	virtual void TrainEnter(EDGETYPE direction, train *t) override { }
	virtual void TrainLeave(EDGETYPE direction, train *t) override { }
	virtual GTF GetFlags(EDGETYPE direction) const override;

	virtual route *GetRouteByIndex(unsigned int index) override { return 0; }

	virtual bool ReservationV(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey) override;
	virtual RPRT GetAvailableRouteTypes(EDGETYPE direction) const override;
	virtual RPRT GetSetRouteTypes(EDGETYPE direction) const override;
	virtual unsigned int GetTRSList(std::vector<track_reservation_state *> &outputlist) override;

	virtual std::string GetTypeName() const override { return "Routing Marker"; }

	static std::string GetTypeSerialisationNameStatic() { return "routingmarker"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
};

inline const genericsignal* FastSignalCast(const generictrack *gt, EDGETYPE direction) {
	if(gt && gt->GetFlags(direction) & GTF::SIGNAL) return static_cast<const genericsignal*>(gt);
	return 0;
}
inline const genericsignal* FastSignalCast(const generictrack *gt) {
	if(gt) return FastSignalCast(gt, gt->GetDefaultValidDirecton());
	else return 0;
}
inline const routingpoint* FastRoutingpointCast(const generictrack *gt, EDGETYPE direction) {
	if(gt && gt->GetFlags(direction) & GTF::ROUTINGPOINT) return static_cast<const routingpoint*>(gt);
	return 0;
}
inline const routingpoint* FastRoutingpointCast(const generictrack *gt) {
	if(gt) return FastRoutingpointCast(gt, gt->GetDefaultValidDirecton());
	else return 0;
}

inline genericsignal* FastSignalCast(generictrack *gt, EDGETYPE direction) {
	return const_cast<genericsignal*>(FastSignalCast(const_cast<const generictrack*>(gt), direction));
}
inline genericsignal* FastSignalCast(generictrack *gt) {
	return const_cast<genericsignal*>(FastSignalCast(const_cast<const generictrack*>(gt)));
}
inline routingpoint* FastRoutingpointCast(generictrack *gt, EDGETYPE direction) {
	return const_cast<routingpoint*>(FastRoutingpointCast(const_cast<const generictrack*>(gt), direction));
}
inline routingpoint* FastRoutingpointCast(generictrack *gt) {
	return const_cast<routingpoint*>(FastRoutingpointCast(const_cast<const generictrack*>(gt)));
}

#endif
