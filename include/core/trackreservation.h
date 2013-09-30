//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
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

#ifndef INC_TRACKRESERVATION_ALREADY
#define INC_TRACKRESERVATION_ALREADY

#include <string>
#include <functional>
#include <vector>
#include "core/flags.h"
#include "core/edgetype.h"
#include "core/serialisable.h"
#include "core/error.h"

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

	const route *reserved_route = 0;
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

class track_reservation_state : public serialisable_obj {
	std::vector<inner_track_reservation_state> itrss;

	public:
	bool Reservation(EDGETYPE direction, unsigned int index, RRF rr_flags, const route *resroute, std::string* failreasonkey = 0);
	GTF GetGTReservationFlags(EDGETYPE direction) const;
	bool IsReserved() const;
	unsigned int GetReservationCount() const;
	bool IsReservedInDirection(EDGETYPE direction) const;
	unsigned int ReservationEnumeration(std::function<void(const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags)> func, RRF checkmask = RRF::RESERVE) const;
	unsigned int ReservationEnumerationInDirection(EDGETYPE direction, std::function<void(const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags)> func, RRF checkmask = RRF::RESERVE) const;

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};

#endif
