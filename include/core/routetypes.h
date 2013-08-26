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

#ifndef INC_ROUTETYPES_ALREADY
#define INC_ROUTETYPES_ALREADY

#include <string>
#include <array>
#include "serialisable.h"

namespace route_class {
	typedef enum {
		RTC_NULL = 0,
		RTC_SHUNT,
		RTC_ROUTE,
		RTC_OVERLAP,
		RTC_ALTOVERLAP1,
		RTC_ALTOVERLAP2,
		RTC_ALTOVERLAP3,
		RTC_CALLON,
		LAST_RTC,
	} ID;

	struct name_set {
		std::string name;
		std::string friendlyname;
	};

	extern std::array<name_set, LAST_RTC> route_names;
	extern std::array<unsigned int, LAST_RTC> default_approach_locking_timeouts;

	inline const std::string &GetRouteTypeName(ID id) { return route_names[id].name; }
	inline const std::string &GetRouteTypeFriendlyName(ID id) { return route_names[id].friendlyname; }

	inline bool IsValid(ID id) { return id != RTC_NULL; }
	inline bool IsShunt(ID id) { return id == RTC_SHUNT || id == RTC_CALLON; }
	inline bool IsRoute(ID id) { return id == RTC_ROUTE; }
	inline bool IsOverlap(ID id) { return id == RTC_OVERLAP || id == RTC_ALTOVERLAP1 || id == RTC_ALTOVERLAP2 || id == RTC_ALTOVERLAP3; }
	inline bool IsCallOn(ID id) { return id == RTC_CALLON; }
	inline bool IsValidForApproachLocking(ID id) { return IsValid(id) && !IsOverlap(id); }
	inline bool IsAspectLimitedToUnity(ID id) { return IsShunt(id); }
	inline bool IsAspectDirectlyPropagatable(ID dep, ID targ) { return dep == RTC_ROUTE && targ == RTC_ROUTE; }
	inline bool NeedsOverlap(ID id) { return IsRoute(id); }
	inline bool IsNotEndExtendable(ID id) { return IsOverlap(id); }
	inline bool AllowEntryWhilstOccupied(ID id) { return IsCallOn(id); }
	inline bool DefaultApproachControlOn(ID id) { return IsCallOn(id); }
	inline bool PreferWhenReservingIfAlreadyOccupied(ID id) { return IsCallOn(id); }

	typedef unsigned char set;

	enum {
		RTCB_ALL		= ((1 << route_class::LAST_RTC) - 1) & ~(1 << RTC_NULL),
		RTCB_SHUNTS		= (1 << RTC_SHUNT) | (1 << RTC_CALLON),
		RTCB_ROUTES		= 1 << RTC_ROUTE,
		RTCB_OVERLAPS		= (1 << RTC_OVERLAP) | (1 << RTC_ALTOVERLAP1) | (1 << RTC_ALTOVERLAP2) | (1 << RTC_ALTOVERLAP3),
	};

	inline void Set(set &s, ID r) { s |= (1<<r); }
	inline void Unset(set &s, ID r) { s &= ~(1<<r); }
	inline set Flag(ID r) { return (1<<r); }
	inline set All() { return RTCB_ALL; }
	inline set AllNonOverlaps() { return (RTCB_ALL & ~RTCB_OVERLAPS); }
	inline set AllOverlaps() { return RTCB_OVERLAPS; }
	inline set AllRoutes() { return RTCB_ROUTES; }
	inline set AllShunts() { return RTCB_SHUNTS; }
	inline set AllNeedingOverlap() { return AllRoutes(); }

	std::ostream& StreamOutRouteClassSet(std::ostream& os, const set& obj);
}

#endif
