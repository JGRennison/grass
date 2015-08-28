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

#ifndef INC_TRACTION_TYPE_ALREADY
#define INC_TRACTION_TYPE_ALREADY

#include <string>
#include <vector>
#include "core/serialisable.h"

class train;

struct traction_type {
	std::string name;
	bool always_available;
	traction_type(std::string n, bool a) : name(n), always_available(a) { }
	traction_type() : always_available(false) { }
};

class traction_set : public serialisable_obj {
	std::vector<traction_type *> tractions;

	public:
	inline void AddTractionType(traction_type *type) {
		if (!HasTraction(type)) {
			tractions.emplace_back(type);
		}
	}

	bool CanTrainPass(const train *t) const;
	bool HasTraction(const traction_type *tt) const;
	bool IsIntersecting(const traction_set &ts) const;
	void IntersectWith(const traction_set &ts);
	void UnionWith(const traction_set &ts);
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;

	std::string DumpString() const;
};

#endif
