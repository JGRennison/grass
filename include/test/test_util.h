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

#ifndef INC_TEST_UTIL_ALREADY
#define INC_TEST_UTIL_ALREADY

#include "test/catch.hpp"
#include <memory>
#include <string>
#include <sstream>

template <typename C> C* CheckPtr(C* in, const char *file, unsigned int line) {
	if (!in) {
		FAIL("CheckPtr failed. File: " << file << ", line: " << line);
	}
	return in;
}

#define PTR_CHECK(p) CheckPtr(p, __FILE__, __LINE__)

class info_rescoped_generic {
	public:
	virtual void AddMsg(Catch::ScopedMessage *msg) = 0;
};

class info_rescoped_unique : public info_rescoped_generic {
	std::unique_ptr<Catch::ScopedMessage> ptr;

	public:
	virtual void AddMsg(Catch::ScopedMessage *msg) override {
		ptr.reset(msg);
	}
};

#define INFO_RESCOPED(container, msg) \
	(container).AddMsg(new Catch::ScopedMessage(Catch::MessageBuilder( "INFO", CATCH_INTERNAL_LINEINFO, Catch::ResultWas::Info ) << msg));

inline void CheckContains(const char *file, unsigned int line, std::string haystack, std::string needle) {
	INFO("CheckContains: File: " << file << ", line: " << line);
	INFO("Looking for: " << needle);
	INFO("In: " << haystack);
	CHECK(haystack.find(needle) != std::string::npos);
}

struct error_collection;
inline void CheckContains(const char *file, unsigned int line, const error_collection &ec, std::string needle) {
	std::stringstream s;
	s << ec;
	CheckContains(file, line, s.str(), needle);
}

#define CHECK_CONTAINS(e, n) CheckContains(__FILE__, __LINE__, e, n)

#endif
