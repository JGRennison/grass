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

#ifndef INC_DESERIALISATION_TEST_ALREADY
#define INC_DESERIALISATION_TEST_ALREADY

#include "test/catch.hpp"
#include "test/world-test.h"
#include "core/error.h"
#include "core/world.h"
#include "core/world_serialisation.h"

struct test_fixture_world {
	//These are pointers to make test_fixture_world movable/move assignable
	std::unique_ptr<world_test> w;
	std::unique_ptr<world_deserialisation> ws;

	error_collection ec;
	std::string orig_input;

	private:
	//common constructor
	test_fixture_world()
			: w(new world_test), ws(new world_deserialisation(*w)) { }

	public:
	test_fixture_world(std::string input) : test_fixture_world() {
		ws->ParseInputString(input, ec);
		orig_input = std::move(input);
	}
};

struct test_fixture_world_init_checked : public test_fixture_world {
	void Init(bool pli, bool gs, bool li) {
		if(li) w->LayoutInit(ec);
		if(pli) w->PostLayoutInit(ec);
		if(gs) ws->DeserialiseGameState(ec);

		if(ec.GetErrorCount()) {
			FAIL("Error Collection: " << ec);
		}
	}

	//! Loads everything from one string, also combined with a call to Init
	test_fixture_world_init_checked(std::string input, bool pli = true, bool gs = false, bool li = true) : test_fixture_world(input) {
		Init(pli, gs, li);
	}
};

#endif
