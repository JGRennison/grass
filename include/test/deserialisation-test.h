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

#ifndef INC_DESERIALISATION_TEST_ALREADY
#define INC_DESERIALISATION_TEST_ALREADY

#include "test/catch.hpp"
#include "test/world-test.h"
#include "test/testutil.h"
#include "util/error.h"
#include "core/world.h"
#include "core/world_serialisation.h"

struct test_fixture_world {
	//These are pointers to make test_fixture_world movable/move assignable
	std::unique_ptr<world_test> w;
	std::unique_ptr<world_deserialisation> ws;

	error_collection ec;
	std::string orig_input;
	std::string orig_input_gamstate;

	private:
	//common constructor
	test_fixture_world()
			: w(new world_test), ws(new world_deserialisation(*w)) { }

	public:
	test_fixture_world(std::string input) : test_fixture_world() {
		ws->ParseInputString(input, ec);
		orig_input = std::move(input);
	}
	test_fixture_world(std::string input, std::string input_gamestate) : test_fixture_world() {
		ws->ParseInputString(input, ec, world_deserialisation::WSLOADGAME_FLAGS::NOGAMESTATE);
		ws->ParseInputString(input_gamestate, ec, world_deserialisation::WSLOADGAME_FLAGS::NOCONTENT);
		orig_input = std::move(input);
		orig_input_gamstate = std::move(input_gamestate);
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

	//! Loads content and gamestate from two strings, doesn't call Init
	test_fixture_world_init_checked(std::string input, std::string input_gamestate)
			: test_fixture_world(input, input_gamestate) {
	}
};

inline std::string SerialiseGameState(const test_fixture_world &tfw) {
	error_collection ec;
	world_serialisation ws(*(tfw.w));
	std::string gamestate = ws.SaveGameToString(ec, world_serialisation::WSSAVEGAME_FLAGS::PRETTYMODE);
	if(ec.GetErrorCount()) {
		FAIL("Error Collection: " << ec);
	}
	return std::move(gamestate);
}

//! This clones a test_fixture_world using a gamestate serialisation round-trip, and the original content json
//! This uses the same layout/post layout init settings as the original
inline test_fixture_world_init_checked RoundTripCloneTestFixtureWorld(const test_fixture_world &tfw, info_rescoped_generic *msgtarg = 0) {
	info_rescoped_unique msgtarg_local;
	if(!msgtarg) msgtarg = &msgtarg_local;
	auto wflags = tfw.w->GetWFlags();

	std::string gamestate = SerialiseGameState(tfw);

	INFO_RESCOPED(*msgtarg, "gamestate:\n" + gamestate);
	test_fixture_world_init_checked rt_tfw(tfw.orig_input, gamestate);
	rt_tfw.Init(wflags & world::WFLAGS::DONE_POSTLAYOUTINIT, true, wflags & world::WFLAGS::DONE_LAYOUTINIT);

	return std::move(rt_tfw);
}

//! Where F has the signature void(test_fixture_world &)
//! This executes the tests, both with and without a serialisation round-trip
template<typename F> inline void ExecTestFixtureWorldWithRoundTrip(test_fixture_world &tfw, F func) {
	test_fixture_world_init_checked rt_tfw = RoundTripCloneTestFixtureWorld(tfw);

	{
		INFO("Testing without serialisation round-trip");
		func(tfw);
	}
	{
		INFO("Testing with serialisation round-trip");
		INFO("Gamestate: " + rt_tfw.orig_input_gamstate);
		func(rt_tfw);
	}
}

#endif
