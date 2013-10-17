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

#include "core/world_obj.h"
#include "core/world.h"
#include "core/serialisable_impl.h"

void updatable_obj::AddUpdateHook(const std::function<void(updatable_obj*, world &)> &f) {
	update_functions.push_back(f);
}

void updatable_obj::MarkUpdated(world &w) {
	w.MarkUpdated(this);
}

void updatable_obj::UpdateNotification(world &w) {
	for(auto &it : update_functions) {
		it(this, w);
	}
}

std::string world_obj::GetFriendlyName() const {
	std::string result= std::string(GetTypeName()).append(": ").append((!name.empty()) ? name : std::string("[unnamed]"));
	return result;
}

void world_obj::Deserialise(const deserialiser_input &di, error_collection &ec) {
	DeserialiseFutures(di, ec, w.future_types, w.futures);
}
