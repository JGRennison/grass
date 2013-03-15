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
#include "traverse.h"

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

class routingpoint : public genericzlentrack {
	protected:
	unsigned int aspect = 0;
	routingpoint *aspect_target = 0;
	routingpoint *aspect_route_target = 0;
	ROUTE_CLASS aspect_type = RTC_NULL;

	public:
	routingpoint(world &w_) : genericzlentrack(w_) { }
	enum {
		RPRT_SHUNTSTART		= 1<<0,
		RPRT_SHUNTEND		= 1<<1,
		RPRT_SHUNTTRANS		= 1<<2,
		RPRT_ROUTESTART		= 1<<3,
		RPRT_ROUTEEND		= 1<<4,
		RPRT_ROUTETRANS		= 1<<5,
		RPRT_VIA		= 1<<6,
		RPRT_OVERLAPSTART	= 1<<7,
		RPRT_OVERLAPEND		= 1<<8,
		RPRT_OVERLAPTRANS	= 1<<9,

		RPRT_MASK_SHUNT		= RPRT_SHUNTSTART | RPRT_SHUNTEND | RPRT_SHUNTTRANS,
		RPRT_MASK_ROUTE		= RPRT_ROUTESTART | RPRT_ROUTEEND | RPRT_ROUTETRANS,
		RPRT_MASK_OVERLAP	= RPRT_OVERLAPSTART | RPRT_OVERLAPEND | RPRT_OVERLAPTRANS,
		RPRT_MASK_START		= RPRT_SHUNTSTART | RPRT_ROUTESTART | RPRT_OVERLAPSTART,
		RPRT_MASK_END		= RPRT_SHUNTEND | RPRT_ROUTEEND | RPRT_OVERLAPEND,
		RPRT_MASK_TRANS		= RPRT_SHUNTTRANS | RPRT_ROUTETRANS | RPRT_OVERLAPTRANS,
	};

	inline unsigned int GetAspect() const { return aspect; }
	inline routingpoint *GetNextAspectTarget() const { return aspect_target; }
	inline routingpoint *GetAspectSetRouteTarget() const { return aspect_route_target; }
	inline ROUTE_CLASS GetAspectType() const { return aspect_type; }
	virtual void UpdateRoutingPoint() { }

	static inline unsigned int GetRouteClassRPRTMask(ROUTE_CLASS rc) {
		switch(rc) {
			case RTC_NULL: return 0;
			case RTC_SHUNT: return RPRT_MASK_SHUNT;
			case RTC_ROUTE: return RPRT_MASK_ROUTE;
			case RTC_OVERLAP: return RPRT_MASK_OVERLAP;
			default: return 0;
		}
	}

	static inline unsigned int GetTRSFlagsRPRTMask(unsigned int rr_flags) {
		unsigned int result = 0;
		if(rr_flags & RRF_STARTPIECE) result |= RPRT_MASK_START;
		if(rr_flags & RRF_ENDPIECE) result |= RPRT_MASK_END;
		if(!(rr_flags & (RRF_STARTPIECE | RRF_ENDPIECE))) result |= RPRT_MASK_TRANS;
		return result;
	}

	virtual unsigned int GetAvailableRouteTypes(EDGETYPE direction) const = 0;
	virtual unsigned int GetSetRouteTypes(EDGETYPE direction) const = 0;

	virtual route *GetRouteByIndex(unsigned int index) = 0;
	const route *FindBestOverlap() const;

	virtual unsigned int GetMatchingRoutes(std::vector<const route *> &out, const routingpoint *end, const via_list &vias) const;
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

	enum {
		RF_NEEDOVERLAP		= 1<<0,
	};
	unsigned int routeflags;

	routingpoint *parent;
	unsigned int index;

	route() : type(RTC_NULL), priority(0), routeflags(0), parent(0), index(0) { }
	void FillLists();
	bool TestRouteForMatch(const routingpoint *checkend, const via_list &checkvias) const;
	bool RouteReservation(unsigned int reserve_flags) const;
	void RouteReservationActions(unsigned int reserve_flags, std::function<void(action &&reservation_act)> actioncallback) const;
};

class route_restriction_set;

class route_restriction {
	friend route_restriction_set;

	std::vector<std::string> targets;
	std::vector<std::string> via;
	std::vector<std::string> notvia;
	int priority;
	unsigned int denyflags;
	unsigned int routerestrictionflags;

	enum {
		RRF_PRIORITYSET	= 1<<0,
	};

	public:
	enum {
		RRDF_NOSHUNT	= 1<<0,
		RRDF_NOROUTE	= 1<<1,
		RRDF_NOOVERLAP	= 1<<2,
	};
	route_restriction() : priority(0), denyflags(0), routerestrictionflags(0) { }
	bool CheckRestriction(unsigned int &restriction_flags, const route_recording_list &route_pieces, const track_target_ptr &piece) const;
	void ApplyRestriction(route &rt) const;
};

class route_restriction_set : public serialisable_obj {
	std::vector<route_restriction> restrictions;

	public:
	unsigned int CheckAllRestrictions(std::vector<const route_restriction*> &matching_restrictions, const route_recording_list &route_pieces, const track_target_ptr &piece) const;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	unsigned int GetRestrictionCount() const { return restrictions.size(); }
};

bool RouteReservation(route &res_route, unsigned int rr_flags);

class trackroutingpoint : public routingpoint {
	protected:
	track_target_ptr prev;
	track_target_ptr next;
	unsigned int availableroutetypes_forward;
	unsigned int availableroutetypes_reverse;

	public:
	trackroutingpoint(world &w_) : routingpoint(w_), availableroutetypes_forward(0), availableroutetypes_reverse(0) { }
	const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const override;
	unsigned int GetMaxConnectingPieces(EDGETYPE direction) const override;
	const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const override;
	EDGETYPE GetReverseDirection(EDGETYPE direction) const override;
	virtual EDGETYPE GetDefaultValidDirecton() const override { return EDGE_FRONT; }

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	protected:
	bool HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance) override;
	virtual EDGETYPE GetAvailableAutoConnectionDirection(bool forwardconnection) const override;
	virtual void GetListOfEdges(std::vector<edgelistitem> &outputlist) const override;
};

class genericsignal : public trackroutingpoint {
	protected:
	unsigned int sflags;
	track_reservation_state start_trs;
	track_reservation_state end_trs;

	world_time last_state_update = 0;
	unsigned int max_aspect = 1;

	public:
	genericsignal(world &w_);
	virtual ~genericsignal();
	void TrainEnter(EDGETYPE direction, train *t) override;
	void TrainLeave(EDGETYPE direction, train *t) override;

	virtual bool Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute) override;

	virtual std::string GetTypeName() const override { return "Generic Signal"; }

	enum {
		GSF_REPEATER		= 1<<0,
		GSF_ASPECTEDREPEATER	= 1<<1,		//true for "standard" repeaters which show an aspect, not true for banner repeaters, etc.
		GSF_NOOVERLAP		= 1<<2,
	};
	virtual unsigned int GetSignalFlags() const;
	virtual unsigned int SetSignalFlagsMasked(unsigned int set_flags, unsigned int mask_flags);

	virtual unsigned int GetAvailableRouteTypes(EDGETYPE direction) const override;
	virtual unsigned int GetSetRouteTypes(EDGETYPE direction) const override;

	virtual const route *GetCurrentForwardRoute() const;	//this will not return the overlap, only the "real" route
	virtual bool RepeaterAspectMeaningfulForRouteType(ROUTE_CLASS type) const;

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	virtual void UpdateSignalState();
	virtual void UpdateRoutingPoint() override { UpdateSignalState(); }
	virtual void TrackTick() override { UpdateSignalState(); }

	protected:
	bool PostLayoutInitTrackScan(error_collection &ec, unsigned int max_pieces, unsigned int junction_max, route_restriction_set *restrictions, std::function<route*(ROUTE_CLASS type, const track_target_ptr &piece)> mkblankroute);
};

class autosignal : public genericsignal {
	route signal_route;
	route overlap_route;

	public:
	autosignal(world &w_) : genericsignal(w_) { availableroutetypes_forward |= RPRT_ROUTESTART | RPRT_SHUNTEND | RPRT_ROUTEEND; }
	bool PostLayoutInit(error_collection &ec) override;
	unsigned int GetFlags(EDGETYPE direction) const override;
	virtual std::string GetTypeName() const override { return "Automatic Signal"; }

	virtual route *GetRouteByIndex(unsigned int index) override;

	static std::string GetTypeSerialisationNameStatic() { return "autosignal"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	virtual unsigned int GetMatchingRoutes(std::vector<const route *> &out, const routingpoint *end, const via_list &vias) const override;
	virtual void EnumerateRoutes(std::function<void (const route *)> func) const override;
};

class routesignal : public genericsignal {
	std::list<route> signal_routes;
	route_restriction_set restrictions;

	public:
	routesignal(world &w_) : genericsignal(w_) { }
	bool PostLayoutInit(error_collection &ec) override;
	unsigned int GetFlags(EDGETYPE direction) const override;
	virtual std::string GetTypeName() const override { return "Route Signal"; }

	virtual route *GetRouteByIndex(unsigned int index) override;

	static std::string GetTypeSerialisationNameStatic() { return "routesignal"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	const route_restriction_set &GetRouteRestrictions() const { return restrictions; }

	virtual unsigned int GetMatchingRoutes(std::vector<const route *> &out, const routingpoint *end, const via_list &vias) const override;
	virtual void EnumerateRoutes(std::function<void (const route *)> func) const override;
};

class startofline : public routingpoint {
	protected:
	track_target_ptr connection;
	unsigned int availableroutetypes;
	track_reservation_state trs;

	public:
	startofline(world &w_) : routingpoint(w_), availableroutetypes(RPRT_SHUNTEND | RPRT_ROUTEEND) { }
	const track_target_ptr & GetConnectingPiece(EDGETYPE direction) const override;
	unsigned int GetMaxConnectingPieces(EDGETYPE direction) const override;
	const track_target_ptr & GetConnectingPieceByIndex(EDGETYPE direction, unsigned int index) const override;
	EDGETYPE GetReverseDirection(EDGETYPE direction) const override;
	virtual EDGETYPE GetDefaultValidDirecton() const override { return EDGE_FRONT; }
	unsigned int GetFlags(EDGETYPE direction) const override;

	void TrainEnter(EDGETYPE direction, train *t) override { }
	void TrainLeave(EDGETYPE direction, train *t) override { }

	virtual unsigned int GetAvailableRouteTypes(EDGETYPE direction) const override;
	virtual unsigned int GetSetRouteTypes(EDGETYPE direction) const override;
	virtual bool Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute) override;

	virtual route *GetRouteByIndex(unsigned int index) override { return 0; }

	virtual std::string GetTypeName() const override { return "Start/End of Line"; }

	static std::string GetTypeSerialisationNameStatic() { return "startofline"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	protected:
	bool HalfConnect(EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance) override;
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
		availableroutetypes_forward |= RPRT_SHUNTTRANS | RPRT_ROUTETRANS | RPRT_OVERLAPTRANS;
		availableroutetypes_reverse |= RPRT_SHUNTTRANS | RPRT_ROUTETRANS | RPRT_OVERLAPTRANS;
	}
	void TrainEnter(EDGETYPE direction, train *t) override { }
	void TrainLeave(EDGETYPE direction, train *t) override { }
	unsigned int GetFlags(EDGETYPE direction) const override;

	virtual route *GetRouteByIndex(unsigned int index) override { return 0; }

	virtual bool Reservation(EDGETYPE direction, unsigned int index, unsigned int rr_flags, const route *resroute) override;
	virtual unsigned int GetAvailableRouteTypes(EDGETYPE direction) const override;
	virtual unsigned int GetSetRouteTypes(EDGETYPE direction) const override;

	virtual std::string GetTypeName() const override { return "Routing Marker"; }

	static std::string GetTypeSerialisationNameStatic() { return "routingmarker"; }
	virtual std::string GetTypeSerialisationName() const override { return GetTypeSerialisationNameStatic(); }
};

const char * SerialiseRouteType(const ROUTE_CLASS& type);

#endif
