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

#include "track.h"
#include "traverse.h"

class routingpoint : public genericzlentrack {
	public:
	enum {
		RPRT_SHUNTSTART		= 1<<0,
		RPRT_SHUNTEND		= 1<<1,
		RPRT_ROUTESTART		= 1<<2,
		RPRT_ROUTEEND		= 1<<3,
		RPRT_VIA		= 1<<4,
	};
	virtual unsigned int GetAvailableRouteTypes(DIRTYPE direction) = 0;
	virtual unsigned int GetSetRouteTypes(DIRTYPE direction) = 0;

	virtual std::string GetTypeName() const { return "Track Routing Point"; }
};

typedef std::deque<routingpoint *> via_list;

struct route {
	vartrack_target_ptr<routingpoint> start;
	route_recording_list pieces;
	vartrack_target_ptr<routingpoint> end;
	via_list vias;
	
	void FillViaList();
};

bool RouteReservation(route &res_route, unsigned int rr_flags);

class genericsignal : public routingpoint {
	track_target_ptr prev;
	track_target_ptr next;
	unsigned int sflags;
	track_reservation_state trs;

	public:
	genericsignal() : sflags(0) { }
	const track_target_ptr & GetConnectingPiece(DIRTYPE direction) const;
	void TrainEnter(DIRTYPE direction, train *t);
	void TrainLeave(DIRTYPE direction, train *t);

	unsigned int GetMaxConnectingPieces(DIRTYPE direction) const;
	const track_target_ptr & GetConnectingPieceByIndex(DIRTYPE direction, unsigned int index) const;
	virtual bool Reservation(DIRTYPE direction, unsigned int index, unsigned int rr_flags, route *resroute);
	
	virtual std::string GetTypeName() const { return "Generic Signal"; }

	virtual unsigned int GetSignalFlags() const;
	virtual unsigned int SetSignalFlagsMasked(unsigned int set_flags, unsigned int mask_flags);

	protected:
	bool HalfConnect(DIRTYPE this_entrance_direction, const track_target_ptr &target_entrance);
};

class autosignal : public genericsignal {
	route signal_route;

	public:
	bool PostLayoutInit(error_collection &ec);
	unsigned int GetFlags(DIRTYPE direction) const;
	virtual std::string GetTypeName() const { return "Automatic Signal"; }
};

class routesignal : public genericsignal {
	std::list<route> signal_routes;

	public:
	bool PostLayoutInit(error_collection &ec);
	unsigned int GetFlags(DIRTYPE direction) const;
	virtual std::string GetTypeName() const { return "Route Signal"; }
};

#endif
