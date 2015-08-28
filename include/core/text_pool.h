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

#ifndef INC_TEXT_POOL_ALREADY
#define INC_TEXT_POOL_ALREADY

#include "core/serialisable.h"
#include <string>
#include <map>

class text_pool : public serialisable_obj {
	std::map<std::string, std::string> textmap;

	public:
	text_pool();
	void RegisterNewText(const std::string &key, const std::string &text);
	std::string GetTextByName(const std::string &key) const;
	virtual void Deserialise(const deserialiser_input &di, error_collection &ec);
	virtual void Serialise(serialiser_output &so, error_collection &ec) const { }
};

class default_user_message_pool : public text_pool {
	public:
	default_user_message_pool();
};

#endif
