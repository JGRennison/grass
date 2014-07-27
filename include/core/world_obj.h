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

#ifndef INC_WORLDOBJ_ALREADY
#define INC_WORLDOBJ_ALREADY

#include <string>
#include <vector>
#include <functional>
#include "core/future.h"
#include "core/serialisable.h"
#include "core/util.h"
#include "core/flags.h"

class world;

class updatable_obj {
	std::vector<std::function<void(updatable_obj*, world &)> > update_functions;

	public:
	void AddUpdateHook(const std::function<void(updatable_obj*, world &)> &f);
	virtual void MarkUpdated(world &w);
	virtual void UpdateNotification(world &w);
};

class world_obj : public serialisable_futurable_obj, public updatable_obj {
	std::string name;
	world &w;

	enum class WOPRIVF {
		AUTONAME     = 1<<0,
	};
	flagwrapper<WOPRIVF> wo_privflags;

	public:
	world_obj(world &w_) : w(w_) { }
	virtual std::string GetTypeName() const = 0;
	std::string GetName() const { return name; }
	virtual std::string GetSerialisationName() const override { return GetName(); }
	void SetName(std::string newname) { name = newname; }
	virtual std::string GetFriendlyName() const;
	world &GetWorld() const { return w; }
	void MarkUpdated() { updatable_obj::MarkUpdated(w); }
	bool IsAutoNamed() const { return wo_privflags & WOPRIVF::AUTONAME; }
	void SetAutoName(bool autoname) { SetOrClearBitsRef(wo_privflags.getref(), WOPRIVF::AUTONAME, autoname); }

	virtual std::string GetTypeSerialisationName() const {
		return GetTypeSerialisationClassName();
	}

	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) override;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const override;
};
template<> struct enum_traits< world_obj::WOPRIVF > { static constexpr bool flags = true; };

#endif
