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

#ifndef INC_SIGNAL_ALREADY
#define INC_SIGNAL_ALREADY

#include <vector>

#include "track.h"
#include "trackreservation.h"
#include "traverse.h"
#include "flags.h"

typedef enum {
	RTC_NULL,
	RTC_SHUNT,
	RTC_ROUTE,
	RTC_OVERLAP,
} ROUTE_CLASS;

class route;
class routingpoint;
class track_circuit;
class genericsignal;
typedef std::deque<routingpoint *> via_list;
typedef std::deque<track_circuit *> tc_list;
typedef std::deque<genericsignal *> sig_list;

enum class RPRT {
	ZERO			= 0,
	SHUNTSTART		= 1<<0,
	SHUNTEND		= 1<<1,
	SHUNTTRANS		= 1<<2,
	ROUTESTART		= 1<<3,
	ROUTEEND		= 1<<4,
	ROUTETRANS		= 1<<5,
	VIA			= 1<<6,
	OVERLAPSTART		= 1<<7,
	OVERLAPEND		= 1<<8,
	OVERLAPTRANS		= 1<<9,

	MASK_SHUNT		= SHUNTSTART | SHUNTEND | SHUNTTRANS,
	MASK_ROUTE		= ROUTESTART | ROUTEEND | ROUTETRANS,
	MASK_OVERLAP		= OVERLAPSTART | OVERLAPEND | OVERLAPTRANS,
	MASK_START		= SHUNTSTART | ROUTESTART | OVERLAPSTART,
	MASK_END		= SHUNTEND | ROUTEEND | OVERLAPEND,
	MASK_TRANS		= SHUNTTRANS | ROUTETRANS | OVERLAPTRANS,
};
template<> struct enum_traits< RPRT > {	static constexpr bool flags = true; };

enum class GMRF {
	ROUTEOK		= 1<<0,
	SHUNTOK		= 1<<1,
	OVERLAPOK	= 1<<2,
	ALL		= ROUTEOK | SHUNTOK | OVERLAPOK,
};
template<> struct enum_traits< GMRF > {	static constexpr bool flags = true; };

class routingpoint : public genericzlentrack {
	routingpoint *aspect_target = 0;
	routingpoint *aspect_route_target = 0;
	routingpoint *aspect_backwards_dependency = 0;

	protected:
	unsigned int aspect = 0;
	ROUTE_CLASS aspect_type = RTC_NULL;
	void SetAspectNextTarget(routingpoint *target);
	void SetAspectRouteTarget(routingpoint *target);

	public:
	routingpoint(world &w_) : genericzlentrack(w_) { }

	inline unsigned int GetAspect() const { return aspect; }
	inline routingpoint *GetAspectNextTarget() const { return aspect_target; }
	inline routingpoint *GetAspectRouteTarget() const { return aspect_route_target; }
	inline routingpoint *GetAspectBackwardsDependency() const { return aspect_backwards_dependency; }
	inline ROUTE_CLASS GetAspectType() const { return aspect_type; }
	virtual void UpdateRoutingPoint() { }

	static inline RPRT GetRouteClassRPRTMask(ROUTE_CLASS rc) {
		switch(rc) {
			case RTC_NULL: return RPRT::ZERO;
			case RTC_SHUNT: return RPRT::MASK_SHUNT;
			case RTC_ROUTE: return RPRT::MASK_ROUTE;
			case RTC_OVERLAP: return RPRT::MASK_OVERLAP;
			default: return RPRT::ZERO;
		}
	}

	static inline RPRT GetTRSFlagsRPRTMask(RRF rr_flags) {
		RPRT result = RPRT::ZERO;
		if(rr_flags & RRF::STARTPIECE) result |= RPRT::MASK_START;
		if(rr_flags & RRF::ENDPIECE) result |= RPRT::MASK_END;
		if(!(rr_flags & (RRF::STARTPIECE | RRF::ENDPIECE))) result |= RPRT::MASK_TRANS;
		return result;
	}

	virtual RPRT GetAvailableRouteTypes(EDGETYPE direction) const = 0;
	virtual RPRT GetSetRouteTypes(EDGETYPE direction) const = 0;

	virtual route *GetRouteByIndex(unsigned int index) = 0;
	const route *FindBestOverlap() const;
	void EnumerateAvailableOverlaps(std::function<void(const route *rt, int score)> func) const;
	const route *FindBestRoute(const routingpoint *end, GMRF gmr_flags = GMRF::ROUTEOK | GMRF::SHUNTOK) const;

	virtual unsigned int GetMatchingRoutes(std::vector<const route *> &out, const routingpoint *end, const via_list &vias, GMRF gmr_flags = GMRF::ROUTEOK | GMRF::SHUNTOK) const;
	virtual void EnumerateRoutes(std::function<void (const route *)> func) const;

	virtual std::string GetTypeName() const override { return "Track Routing Point"; }
};

struct route {
	vartrack_target_ptr<routingpoint> start;
	route_recording_list pieces;
	vartrack_target_ptr<routingpoint> end;
	via_list vias;
	tc_list trackcircuits;
	sig_list repeatersignals;
	ROUTE_CLASS type;
	int priority;

	enum class RF {
		ZERO			= 0,
		NEEDOVERLAP		= 1<<0,
	};
	RF routeflags;

	routingpoint *parent;
	unsigned int index;

	route() : type(RTC_NULL), priority(0), routeflags(RF::ZERO), parent(0), index(0) { }
	void FillLists();
	bool TestRouteForMatch(const routingpoint *checkend, const via_list &checkvias) const;
	bool RouteReservation(RRF reserve_flags, std::string *failreasonkey = 0) const;
	void RouteReservationActions(RRF reserve_flags, std::function<void(action &&reservation_act)> actioncallback) const;
	bool IsRouteSubSet(const route *subset) const;
};
template<> struct enum_traits< route::RF > {	static constexpr bool flags = true; };

class route_restriction_set;

class route_restriction {
	friend route_restriction_set;

	std::vector<std::string> targets;
	std::vector<std::string> via;
	std::vector<std::string> notvia;
	int priority = 0;
	enum class RRF {
		ZERO		= 0,
		PRIORITYSET	= 1<<0,
	};
	RRF routerestrictionflags = RRF::ZERO;

	public:
	enum class RRDF {
		ZERO		= 0,
		NOSHUNT		= 1<<0,
		NOROUTE		= 1<<1,
		NOOVERLAP	= 1<<2,
	};
	private:
	RRDF denyflags = RRDF::ZERO;

	public:
	bool CheckRestriction(RRDF &restriction_flags, const route_recording_list &route_pieces, const track_target_ptr &piece) const;
	void ApplyRestriction(route &rt) const;
};
template<> struct enum_traits< route_restriction::RRF> {	static constexpr bool flags = true; };
template<> struct enum_traits< route_restriction::RRDF > {	static constexpr bool flags = true; };

class route_restriction_set : public serialisable_obj {
	std::vector<route_restriction> restrictions;

	public:
	route_restriction::RRDF CheckAllRestrictions(std::vector<const route_restriction*> &matching_restrictions, const route_recording_list &route_pieces, const track_target_ptr &piece) const;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	unsigned int GetRestrictionCount() const { return restrictions.size(); }
};

bool RouteReservation(route &res_route, RRF rr_flags);

class trackroutingpoint : public routingpoint {
	protected:
	track_target_ptr prev;
	track_target_ptr next;
	RPRT availableroutetypes_forward;
	RPRT availableroutetypes_reverse;

	public:
	trackroutingpoint(world &w_) : routingpoint(w_), availableroutetypes_forward(RPRT::ZERO), availableroutetypes_reverse(RPRT::ZERO) { }
	virtual bool IsEdgeValid(EDGETYPE edge) const override;
	virtual const track_target_ptr & GetEdgeConnectingPiece(EDGETYPE edgeid) const override;
	virtual const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const override;
	virtual unsigned int GetMaxConnectingPieces(EDGETYPE direction) const override;
	virtual const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const override;
	virtual EDGETYPE GetReverseDirection(EDGETYPE direction) const override;
	virtual EDGETYPE GetDefaultValidDirecton() const override { return EDGE_FRONT; }

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	protected:
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const override;
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const override;
};

class genericsignal : public trackroutingpoint {
	public:
	enum class GSF {
		ZERO			= 0,
		REPEATER		= 1<<0,
		ASPECTEDREPEATER	= 1<<1,		//true for "standard" repeaters which show an aspect, not true for banner repeaters, etc.
		NOOVERLAP		= 1<<2,
		APPROACHCONTROLMODE	= 1<<3,		//route cancelled with train approaching, hold aspect at 0
		AUTOSIGNAL		= 1<<4,
		OVERLAPSWINGABLE	= 1<<5,
	};

	protected:
	genericsignal::GSF sflags;
	track_reservation_state start_trs;
	track_reservation_state end_trs;

	world_time last_state_update = 0;
	unsigned int max_aspect = 1;

	public:
	genericsignal(world &w_);
	virtual ~genericsignal();
	virtual void TrainEnter(EDGETYPE direction, train *t) override;
	virtual void TrainLeave(EDGETYPE direction, train *t) override;

	virtual bool Reservation(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey = 0) override;

	virtual std::string GetTypeName() const override { return "Generic Signal"; }

	virtual GSF GetSignalFlags() const;
	virtual GSF SetSignalFlagsMasked(GSF set_flags, GSF mask_flags);

	virtual RPRT GetAvailableRouteTypes(EDGETYPE direction) const override;
	virtual RPRT GetSetRouteTypes(EDGETYPE direction) const override;

	virtual const route *GetCurrentForwardRoute() const;	//this will not return the overlap, only the "real" route
	virtual const route *GetCurrentForwardOverlap() const;	//this will only return the overlap, not the "real" route
	virtual void EnumerateCurrentBackwardsRoutes(std::function<void (const route *)> func) const;	//this will return all routes which currently terminate here
	virtual bool RepeaterAspectMeaningfulForRouteType(ROUTE_CLASS type) const;

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	virtual void UpdateSignalState();
	virtual void UpdateRoutingPoint() override { UpdateSignalState(); }
	virtual void TrackTick() override { UpdateSignalState(); }

	//function parameters: return true to continue, false to stop
	void BackwardsReservedTrackScan(std::function<bool(const genericsignal*)> checksignal, std::function<bool(const track_target_ptr&)> checkpiece) const;

	protected:
	bool PostLayoutInitTrackScan(error_collection &ec, unsigned int max_pieces, unsigned int junction_max, route_restriction_set *restrictions, std::function<route*(ROUTE_CLASS type, const track_target_ptr &piece)> mkblankroute);
};
template<> struct enum_traits< genericsignal::GSF > {	static constexpr bool flags = true; };

class autosignal : public genericsignal {
	route signal_route;
	route overlap_route;

	public:
	autosignal(world &w_) : genericsignal(w_) { availableroutetypes_forward |= RPRT::ROUTESTART | RPRT::SHUNTEND | RPRT::ROUTEEND; sflags |= GSF::AUTOSIGNAL; }
	bool PostLayoutInit(error_collection &ec) override;
	virtual GTF GetFlags(EDGETYPE direction) const override;
	virtual std::string GetTypeName() const override { return "Automatic Signal"; }

	virtual route *GetRouteByIndex(unsigned int index) override;

	static std::string GetTypeSerialisationNameStatic() { return "autosignal"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	virtual unsigned int GetMatchingRoutes(std::vector<const route *> &out, const routingpoint *end, const via_list &vias, GMRF gmr_flags = GMRF::ROUTEOK | GMRF::SHUNTOK) const override;
	virtual void EnumerateRoutes(std::function<void (const route *)> func) const override;
};

class routesignal : public genericsignal {
	std::list<route> signal_routes;
	route_restriction_set restrictions;

	public:
	routesignal(world &w_) : genericsignal(w_) { }
	virtual bool PostLayoutInit(error_collection &ec) override;
	virtual GTF GetFlags(EDGETYPE direction) const override;
	virtual std::string GetTypeName() const override { return "Route Signal"; }

	virtual route *GetRouteByIndex(unsigned int index) override;

	static std::string GetTypeSerialisationNameStatic() { return "routesignal"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	const route_restriction_set &GetRouteRestrictions() const { return restrictions; }

	virtual unsigned int GetMatchingRoutes(std::vector<const route *> &out, const routingpoint *end, const via_list &vias, GMRF gmr_flags = GMRF::ROUTEOK | GMRF::SHUNTOK) const override;
	virtual void EnumerateRoutes(std::function<void (const route *)> func) const override;
};

class startofline : public routingpoint {
	protected:
	track_target_ptr connection;
	RPRT availableroutetypes;
	track_reservation_state trs;

	public:
	startofline(world &w_) : routingpoint(w_), availableroutetypes(RPRT::SHUNTEND | RPRT::ROUTEEND) { }
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
	virtual bool Reservation(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey) override;

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
		availableroutetypes_forward |= RPRT::SHUNTTRANS | RPRT::ROUTETRANS | RPRT::OVERLAPTRANS;
		availableroutetypes_reverse |= RPRT::SHUNTTRANS | RPRT::ROUTETRANS | RPRT::OVERLAPTRANS;
	}
	virtual void TrainEnter(EDGETYPE direction, train *t) override { }
	virtual void TrainLeave(EDGETYPE direction, train *t) override { }
	virtual GTF GetFlags(EDGETYPE direction) const override;

	virtual route *GetRouteByIndex(unsigned int index) override { return 0; }

	virtual bool Reservation(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey) override;
	virtual RPRT GetAvailableRouteTypes(EDGETYPE direction) const override;
	virtual RPRT GetSetRouteTypes(EDGETYPE direction) const override;

	virtual std::string GetTypeName() const override { return "Routing Marker"; }

	static std::string GetTypeSerialisationNameStatic() { return "routingmarker"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
};

const char * SerialiseRouteType(const ROUTE_CLASS& type);

#endif
