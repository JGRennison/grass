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

class routingpoint : public genericzlentrack {
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
		RPRT_OVERLAPEND		= 1<<7,
		RPRT_OVERLAPTRANS	= 1<<8,
	};
	virtual unsigned int GetAvailableRouteTypes(DIRTYPE direction) const = 0;
	virtual unsigned int GetSetRouteTypes(DIRTYPE direction) const = 0;

	virtual route *GetRouteByIndex(unsigned int index) = 0;

	virtual std::string GetTypeName() const { return "Track Routing Point"; }
};

typedef std::deque<routingpoint *> via_list;

typedef enum {
	RTC_NULL,
	RTC_SHUNT,
	RTC_ROUTE,
	RTC_OVERLAP,
} ROUTE_CLASS;

struct route {
	vartrack_target_ptr<routingpoint> start;
	route_recording_list pieces;
	vartrack_target_ptr<routingpoint> end;
	via_list vias;
	ROUTE_CLASS type;
	int priority;

	routingpoint *parent;
	unsigned int index;

	route() : type(RTC_NULL), priority(0), parent(0), index(0) { }
	void FillViaList();
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
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;
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
	const track_target_ptr & GetConnectingPiece(DIRTYPE direction) const;
	unsigned int GetMaxConnectingPieces(DIRTYPE direction) const;
	const track_target_ptr & GetConnectingPieceByIndex(DIRTYPE direction, unsigned int index) const;
	DIRTYPE GetReverseDirection(DIRTYPE direction) const;
	virtual DIRTYPE GetDefaultValidDirecton() const { return TDIR_FORWARD; }

	bool HalfConnect(DIRTYPE this_entrance_direction, const track_target_ptr &target_entrance);

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;
};

class genericsignal : public trackroutingpoint {
	protected:
	unsigned int sflags;
	track_reservation_state start_trs;
	track_reservation_state end_trs;

	public:
	genericsignal(world &w_) : trackroutingpoint(w_), sflags(0) { availableroutetypes_reverse |= RPRT_SHUNTTRANS | RPRT_ROUTETRANS; }
	void TrainEnter(DIRTYPE direction, train *t);
	void TrainLeave(DIRTYPE direction, train *t);

	virtual bool Reservation(DIRTYPE direction, unsigned int index, unsigned int rr_flags, route *resroute);

	virtual std::string GetTypeName() const { return "Generic Signal"; }

	virtual unsigned int GetSignalFlags() const;
	virtual unsigned int SetSignalFlagsMasked(unsigned int set_flags, unsigned int mask_flags);

	virtual unsigned int GetAvailableRouteTypes(DIRTYPE direction) const;
	virtual unsigned int GetSetRouteTypes(DIRTYPE direction) const;

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;

	protected:
	bool PostLayoutInitTrackScan(error_collection &ec, unsigned int max_pieces, unsigned int junction_max, route_restriction_set *restrictions, std::function<route*(ROUTE_CLASS type, const track_target_ptr &piece)> mkblankroute);
};

class autosignal : public genericsignal {
	route signal_route;
	route overlap_route;

	public:
	autosignal(world &w_) : genericsignal(w_) { availableroutetypes_forward |= RPRT_ROUTESTART; }
	bool PostLayoutInit(error_collection &ec);
	unsigned int GetFlags(DIRTYPE direction) const;
	virtual std::string GetTypeName() const { return "Automatic Signal"; }

	virtual route *GetRouteByIndex(unsigned int index);

	static std::string GetTypeSerialisationNameStatic() { return "autosignal"; }
	virtual std::string GetTypeSerialisationName() const { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;
};

class routesignal : public genericsignal {
	std::list<route> signal_routes;
	route_restriction_set restrictions;

	public:
	routesignal(world &w_) : genericsignal(w_) { }
	bool PostLayoutInit(error_collection &ec);
	unsigned int GetFlags(DIRTYPE direction) const;
	virtual std::string GetTypeName() const { return "Route Signal"; }

	virtual route *GetRouteByIndex(unsigned int index);

	static std::string GetTypeSerialisationNameStatic() { return "routesignal"; }
	virtual std::string GetTypeSerialisationName() const { return GetTypeSerialisationNameStatic(); }
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const;
};

#endif
