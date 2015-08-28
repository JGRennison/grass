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

#include "test/catch.hpp"
#include "test/world_test.h"
#include "core/world_ops.h"
#include "core/track.h"
#include "core/points.h"
#include "core/text_pool.h"
#include "core/var.h"

struct test_fixture_world_ops_1 {
	world_test w;
	points p1;
	test_fixture_world_ops_1() : p1(w) {
		p1.SetName("P1");
	}
};

TEST_CASE( "logging/basic", "Test basic text logging" ) {
	test_fixture_world_ops_1 env;

	env.w.LogUserMessageLocal(LOG_DENIED, "test test");
	env.w.LogUserMessageLocal(LOG_MESSAGE, "foo bar");

	REQUIRE(env.w.GetLogText() == "test test\nfoo bar\n");
	REQUIRE(env.w.GetLastLogCategory() == LOG_MESSAGE);
}

TEST_CASE( "text_pool/lookup", "Test basic text lookup" ) {
	test_fixture_world_ops_1 env;

	env.w.GetUserMessageTextpool().RegisterNewText("test", "foobar");
	env.w.LogUserMessageLocal(LOG_MESSAGE, env.w.GetUserMessageTextpool().GetTextByName("test"));

	REQUIRE(env.w.GetLogText() == "foobar\n");
}

TEST_CASE( "text_pool/missing", "Test handling of nonexistent key text lookup" ) {
	test_fixture_world_ops_1 env;

	env.w.GetUserMessageTextpool().RegisterNewText("test", "foobar");
	env.w.LogUserMessageLocal(LOG_MESSAGE, env.w.GetUserMessageTextpool().GetTextByName("test1"));

	REQUIRE(env.w.GetLogText() == "text_pool: No such key: test1\n");
}

TEST_CASE( "text_pool/variables", "Test variable expansion" ) {
	test_fixture_world_ops_1 env;

	env.w.GetUserMessageTextpool().RegisterNewText("test", "foobar $bar $foo() $bar(test) \\$bar");
	message_formatter mf;
	mf.RegisterVariable("bar", [&](const std::string &in) { return "[baz](" + in + ")"; });
	mf.RegisterVariable("foo", [&](const std::string &in) { return "[zab](" + in + ")"; });
	env.w.LogUserMessageLocal(LOG_MESSAGE, mf.FormatMessage(env.w.GetUserMessageTextpool().GetTextByName("test")));

	REQUIRE(env.w.GetLogText() == "foobar [baz]() [zab]() [baz](test) $bar\n");
}

TEST_CASE( "text_pool/variables/badinput", "Test variable expansion input error" ) {
	test_fixture_world_ops_1 env;

	env.w.GetUserMessageTextpool().RegisterNewText("test", "foobar $bar $foo() $bar(test) $() $@ $");
	message_formatter mf;
	env.w.LogUserMessageLocal(LOG_MESSAGE, mf.FormatMessage(env.w.GetUserMessageTextpool().GetTextByName("test")));

	REQUIRE(env.w.GetLogText() == "foobar [No Such Variable] [No Such Variable] [No Such Variable] () @ \n");
}

TEST_CASE( "logging/future/generic", "Test generic future log messages" ) {
	test_fixture_world_ops_1 env;

	env.w.GetUserMessageTextpool().RegisterNewText("test", "foo $gametime bar");
	env.w.futures.RegisterFuture(std::make_shared<future_generic_user_message>(env.w, 150, &env.w, "test"));
	env.w.futures.RegisterFuture(std::make_shared<future_generic_user_message>(env.w, 300, &env.w, "test"));
	env.w.futures.RegisterFuture(std::make_shared<future_generic_user_message>(env.w, 500, &env.w, "test"));

	std::string expected1 = "foo 00:00:00.200 bar\n";
	std::string expected2 = "foo 00:00:00.300 bar\n";

	REQUIRE(env.w.GetLogText() == "");
	env.w.GameStep(100);
	REQUIRE(env.w.GetLogText() == "");
	env.w.GameStep(100);
	REQUIRE(env.w.GetLogText() == expected1);
	env.w.GameStep(100);
	REQUIRE(env.w.GetLogText() == expected1 + expected2);
}
