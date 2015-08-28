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

#ifndef INC_ROUTE_TYPES_ALREADY
#define INC_ROUTE_TYPES_ALREADY

#include <string>
#include <array>
#include "core/serialisable.h"

namespace route_class {
	enum class ID {
		NONE = 0,
		SHUNT,
		ROUTE,
		OVERLAP,
		ALTOVERLAP1,
		ALTOVERLAP2,
		ALTOVERLAP3,
		CALLON,

		END,
	};

	struct name_set {
		std::string name;
		std::string friendlyname;
	};

	extern std::array<name_set, static_cast<size_t>(ID::END)> route_names;
	extern std::array<unsigned int, static_cast<size_t>(ID::END)> default_approach_locking_timeouts;

	inline const std::string &GetRouteTypeName(ID id) { return route_names[static_cast<size_t>(id)].name; }
	inline const std::string &GetRouteTypeFriendlyName(ID id) { return route_names[static_cast<size_t>(id)].friendlyname; }

	inline bool IsValid(ID id) { return id != ID::NONE; }
	inline bool IsShunt(ID id) { return id == ID::SHUNT || id == ID::CALLON; }
	inline bool IsRoute(ID id) { return id == ID::ROUTE; }
	inline bool IsOverlap(ID id) { return id == ID::OVERLAP || id == ID::ALTOVERLAP1 || id == ID::ALTOVERLAP2 || id == ID::ALTOVERLAP3; }
	inline bool IsCallOn(ID id) { return id == ID::CALLON; }
	inline bool IsValidForApproachLocking(ID id) { return IsValid(id) && !IsOverlap(id); }
	inline bool IsAspectLimitedToUnity(ID id) { return IsShunt(id); }
	inline bool IsAspectDirectlyPropagatable(ID dep, ID targ) { return dep == ID::ROUTE && targ == ID::ROUTE; }
	inline bool NeedsOverlap(ID id) { return IsRoute(id); }
	inline bool IsNotEndExtendable(ID id) { return IsOverlap(id); }
	inline bool AllowEntryWhilstOccupied(ID id) { return IsCallOn(id); }
	inline bool DefaultApproachControlOn(ID id) { return IsCallOn(id); }
	inline bool PreferWhenReservingIfAlreadyOccupied(ID id) { return IsCallOn(id); }

	typedef unsigned char set;

	inline constexpr set Flag(ID r) { return (1 << static_cast<unsigned char>(r)); }

	enum {
		RTCB_ALL         = (Flag(ID::END) - 1) & ~Flag(ID::NONE),
		RTCB_SHUNTS      = Flag(ID::SHUNT) | Flag(ID::CALLON),
		RTCB_ROUTES      = Flag(ID::ROUTE),
		RTCB_OVERLAPS    = Flag(ID::OVERLAP) | Flag(ID::ALTOVERLAP1) | Flag(ID::ALTOVERLAP2) | Flag(ID::ALTOVERLAP3),
	};

	inline void Set(set &s, ID r) { s |= Flag(r); }
	inline void Unset(set &s, ID r) { s &= ~Flag(r); }
	inline set All() { return RTCB_ALL; }
	inline set AllNonOverlaps() { return (RTCB_ALL & ~RTCB_OVERLAPS); }
	inline set AllOverlaps() { return RTCB_OVERLAPS; }
	inline set AllRoutes() { return RTCB_ROUTES; }
	inline set AllShunts() { return RTCB_SHUNTS; }
	inline set AllNeedingOverlap() { return AllRoutes(); }

	std::ostream& StreamOutRouteClassSet(std::ostream& os, const set& obj);
	std::ostream& operator<<(std::ostream& os, ID obj);
	std::ostream& operator<<(std::ostream& os, set obj);
}

#endif
