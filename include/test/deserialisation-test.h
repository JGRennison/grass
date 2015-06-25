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
	test_fixture_world(std::string input)
			: test_fixture_world() {
		ws->ParseInputString(input, ec);
		orig_input = std::move(input);
	}
	test_fixture_world(std::string input, std::string input_gamestate)
			: test_fixture_world() {
		ws->ParseInputString(input, ec, world_deserialisation::WSLOADGAME_FLAGS::NOGAMESTATE);
		ws->ParseInputString(input_gamestate, ec, world_deserialisation::WSLOADGAME_FLAGS::NOCONTENT);
		orig_input = std::move(input);
		orig_input_gamstate = std::move(input_gamestate);
	}
};

struct test_fixture_world_init_checked : public test_fixture_world {
	void Init(bool pli, bool gs, bool li) {
		if(li)
			w->LayoutInit(ec);
		if(pli)
			w->PostLayoutInit(ec);
		if(gs)
			ws->DeserialiseGameState(ec);

		if(ec.GetErrorCount()) {
			FAIL("Error Collection: " << ec);
		}
	}

	//! Loads everything from one string, also combined with a call to Init
	test_fixture_world_init_checked(std::string input, bool pli = true, bool gs = false, bool li = true)
			: test_fixture_world(input) {
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
inline test_fixture_world_init_checked RoundTripCloneTestFixtureWorld(const test_fixture_world &tfw, info_rescoped_generic *msgtarg = nullptr) {
	info_rescoped_unique msgtarg_local;
	if(!msgtarg)
		msgtarg = &msgtarg_local;
	auto wflags = tfw.w->GetWFlags();

	std::string gamestate = SerialiseGameState(tfw);

	INFO_RESCOPED(*msgtarg, "gamestate:\n" + gamestate);
	test_fixture_world_init_checked rt_tfw(tfw.orig_input, gamestate);
	rt_tfw.Init(wflags & world::WFLAGS::DONE_POSTLAYOUTINIT, true, wflags & world::WFLAGS::DONE_LAYOUTINIT);
	rt_tfw.w->round_trip_actions = tfw.w->round_trip_actions;

	return std::move(rt_tfw);
}

//! Where S has the signature void()
//! Where T has the signature void(std::function<void()> RoundTrip)
//! This executes the test, both with and without serialisation round-trips
template<typename S, typename T> inline void ExecTestFixtureWorldWithRoundTrip(test_fixture_world &env, S setup_func, T test_func) {
	SECTION("No serialisation round-trip") {
		setup_func();
		test_func([]() { });
	}
	SECTION("With serialisation round-trip") {
		info_rescoped_unique roundtrip_msg;
		env.w->round_trip_actions = true;
		setup_func();
		test_func([&]() {
			env = RoundTripCloneTestFixtureWorld(env, &roundtrip_msg);
			setup_func();
		});
	}
}

#endif
