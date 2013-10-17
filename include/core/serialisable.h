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

#ifndef INC_SERIALISABLE_ALREADY
#define INC_SERIALISABLE_ALREADY

#include "core/error.h"
#include <map>

struct deserialiser_input;
struct serialiser_output;
class world_serialisation;

template <typename... Args> class deserialisation_type_factory {
	std::map<std::string, std::function<void(const deserialiser_input &di, error_collection &ec, Args...)> > typemapping;

	public:
	bool FindAndDeserialise(const std::string &name, const deserialiser_input &di, error_collection &ec, const Args & ...extras) const {
		auto targ = typemapping.find(name);
		if(targ != typemapping.end()) {
			targ->second(di, ec, extras...);
			return true;
		}
		else return false;
	}

	void RegisterType(const std::string &name, std::function<void(const deserialiser_input &di, error_collection &ec, Args...)> func) {
		typemapping[name] = func;
	}
};

class serialisable_obj {
	friend world_serialisation;

	void DeserialisePrePost(const char *name, const deserialiser_input &di, error_collection &ec);

	public:
	void DeserialiseObject(const deserialiser_input &di, error_collection &ec);
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec) = 0;
	virtual void Serialise(serialiser_output &so, error_collection &ec) const = 0;
};

#endif
