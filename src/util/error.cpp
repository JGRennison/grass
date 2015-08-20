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

#include "util/util.h"
#include "util/error.h"

void error_collection::RegisterError(std::unique_ptr<error_obj> &&err) {
	errors.emplace_back(std::move(err));
}

void error_collection::Reset() {
	errors.clear();
}

unsigned int error_collection::GetErrorCount() const {
	return errors.size();
}

void error_collection::StreamOut(std::ostream& os) const {
	os << "Errors: " << errors.size() << "\n";
	for (auto &it : errors) {
		const error_obj& obj = *it;
		os << obj;
	}
}

void error_obj::StreamOut(std::ostream& os) const {
	os << msg.str() << "\n";
}

error_obj::error_obj() {
	millitimestamp = GetMilliTime();
	timestamp = time(nullptr);
	msg << gr_strftime(string_format("[%%F %%T.%03d %%z] Error: ", millitimestamp), localtime(&timestamp), timestamp, true);
}

std::ostream& operator<<(std::ostream& os, const error_obj& obj) {
	obj.StreamOut(os);
	return os;
}

std::ostream& operator<<(std::ostream& os, const error_collection& obj) {
	obj.StreamOut(os);
	return os;
}
