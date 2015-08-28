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
#include "test/deserialisation_test.h"
#include "test/test_util.h"
#include "core/track.h"
#include "core/train.h"
#include "core/traverse.h"
#include "core/world.h"
#include "core/signal.h"
#include "core/world_serialisation.h"

TEST_CASE( "deserialisation/error/invalid", "Test invalid JSON" ) {
	auto test = [&](std::string testname, std::string json, std::initializer_list<std::string> checklist) {
		INFO(testname);
		test_fixture_world env(json);

		//if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
		REQUIRE(env.ec.GetErrorCount() == 1);
		CHECK_CONTAINS(env.ec, "JSON Parsing error");
		for (auto &it : checklist) {
			CHECK_CONTAINS(env.ec, it);
		}
	};

	test("simple test",
			"{ \"content }", { "\"{ \"content }\"" });

	test("check long lines work: start",
			"{ \"foo\" : ], \"content\" : \"999999999999999999999999999999999999999999999999999999999999999"
			"999999999999999999999999999999999999999999999999999999999999999\" }",
			{ "\"{ \"foo\" : ]", "\"..." });

	test("check long lines work: middle",
			"{ \"content\" : \"99999999999999999999999999999999999999999999999999999999999999999999999999999"
			"9999999999999999999999999999999999999999999999999\", foo, "
			"\"data\" : \"99999999999999999999999999999999999999999999999999999999999999999999999999999"
			"9999999999999999999999999999999999999999999999999\" }",
			{ ", foo,", "\"...", "...\"" });

	test("check long lines work: end",
			"{ \"content\" : \"99999999999999999999999999999999999999999999999999999999999999999999999999999"
			"9999999999999999999999999999999999999999999999999\", foo : true }",
			{ " foo : true }\"" , "...\"" });

	test("check multiple lines work",
			"{ \"foo\" : true,\n\"bar\" : baz,\n\"baz\" : false }",
			{ "line: 2", "\"\"bar\" : baz,\"" });
}

TEST_CASE( "deserialisation/error/notype", "Test missing object type" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{\"name\" : \"T1\"}"
	"] }";
	test_fixture_world env(track_test_str);

	//if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 1);
}

TEST_CASE( "deserialisation/error/wrongtype", "Test wrong value type" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{ \"type\" : \"track_seg\", \"train_count\" : \"foo\"}"
	"] }";
	test_fixture_world env(track_test_str);

	//if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 1);
}

TEST_CASE( "deserialisation/error/wrongtypeobjectarray", "Test supplying a value instead of an object/array" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{ \"type\" : \"track_seg\", \"speed_limits\" : 0, \"trs\" : 0}"
	"] }";
	test_fixture_world env(track_test_str);

	//if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 2);
}

TEST_CASE( "deserialisation/error/arraytypemismatch", "Test supplying a value of the wrong type in a value array" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{ \"type\" : \"route_signal\", \"route_restrictions\" : [ { \"targets\" : 0 } ] }, "
		"{ \"type\" : \"route_signal\", \"route_restrictions\" : [ { \"targets\" : [ \"foo\", 0, \"bar\" ] } ] }"
	"] }";
	test_fixture_world env(track_test_str);

	//if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 2);
}

TEST_CASE( "deserialisation/error/extravalues", "Test unknown extra value detection" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{ \"type\" : \"track_seg\", \"length\" : 0, \"foobar\" : \"baz\" }"
	"] }";
	test_fixture_world env(track_test_str);

	//if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 1);
}

TEST_CASE( "deserialisation/error/flagcontradiction", "Test contradictory flags" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{ \"type\" : \"routing_marker\", \"through\" : { \"allow\" : \"all\", \"deny\" : \"route\" } }"
	"] }";
	test_fixture_world env(track_test_str);

	//if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 1);
}

TEST_CASE( "deserialisation/error/flagnoncontradiction", "Test non-contradictory flags" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{ \"type\" : \"routing_marker\", \"through\" : { \"allow_only\" : \"all\", \"allow\" : \"route\" } }"
	"] }";
	test_fixture_world env(track_test_str);

	if (env.ec.GetErrorCount()) {
		WARN("Error Collection: " << env.ec);
	}
	REQUIRE(env.ec.GetErrorCount() == 0);
}

TEST_CASE( "deserialisation/typedef/nested", "Test nested type declaration" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "typedef", "new_type" : "base", "base_type" : "routing_marker", "content" : { "through" : { "deny" : "all" }, "through_rev" : { "deny" : "all" } } }, )"
		R"({ "type" : "typedef", "new_type" : "derived", "base_type" : "base", "content" : { "through_rev" : { "allow" : "all" } } }, )"
		R"({ "type" : "derived", "name" : "R1", "through_rev" : { "deny" : "overlap" } } )"
	"] }";
	test_fixture_world env(track_test_str);

	if (env.ec.GetErrorCount()) {
		WARN("Error Collection: " << env.ec);
	}
	REQUIRE(env.ec.GetErrorCount() == 0);

	routing_marker *rm = dynamic_cast<routing_marker *>(env.w->FindTrackByName("R1"));
	REQUIRE(rm != nullptr);
	CHECK(rm->GetAvailableRouteTypes(EDGE_FRONT) == RPRT());
	CHECK(rm->GetAvailableRouteTypes(EDGE_BACK) == RPRT(0, route_class::All() & ~route_class::Flag(route_class::RTC_OVERLAP), 0));
}

TEST_CASE( "deserialisation/error/typedefrecursion", "Test check for typedef recursion" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "typedef", "new_type" : "T1", "base_type" : "T2" }, )"
		R"({ "type" : "typedef", "new_type" : "T2", "base_type" : "T1" }, )"
		R"({ "type" : "T1", "name" : "R1" } )"
	"] }";
	test_fixture_world env(track_test_str);

	CHECK_CONTAINS(env.ec, "Recursive expansion detected");
	REQUIRE(env.ec.GetErrorCount() == 1);
}

TEST_CASE( "deserialisation/error/templateextravalues", "Test templating extra values check" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "template", "name" : "T1", "foobar" : true, "content" : { } } )"
	"] }";
	test_fixture_world env(track_test_str);

	//if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 1);
}

TEST_CASE( "deserialisation/error/typedefextravalues", "Test typedef extra values check" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "typedef", "new_type" : "derived", "base_type" : "base", "foobar" : true, "content" : { } } )"
	"] }";
	test_fixture_world env(track_test_str);

	//if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 1);
}

TEST_CASE( "deserialisation/scalartypeconv/length", "Test scalar type conversion (length)" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "track_seg", "length" : 1000 }, )"
		R"({ "type" : "track_seg", "length" : "1000  m" }, )"
		R"({ "type" : "track_seg", "length" : "2.4km" }, )"
		R"({ "type" : "track_seg", "length" : "0.56 miles" }, )"
		R"({ "type" : "track_seg", "length" : "4699088.432 yd" } )"
	"] }";
	test_fixture_world env(track_test_str);

	if (env.ec.GetErrorCount()) {
		WARN("Error Collection: " << env.ec);
	}
	REQUIRE(env.ec.GetErrorCount() == 0);

	CHECK(PTR_CHECK(env.w->FindTrackByName("#0"))->GetLength(EDGE_FRONT) == 1000);
	CHECK(PTR_CHECK(env.w->FindTrackByName("#1"))->GetLength(EDGE_FRONT) == 1000000);
	CHECK(PTR_CHECK(env.w->FindTrackByName("#2"))->GetLength(EDGE_FRONT) == 2400000);
	CHECK(PTR_CHECK(env.w->FindTrackByName("#3"))->GetLength(EDGE_FRONT) == 901232);
	CHECK(PTR_CHECK(env.w->FindTrackByName("#4"))->GetLength(EDGE_FRONT) == 4294966826);
}

TEST_CASE( "deserialisation/scalartypeconv/errors", "Test scalar type conversion error detection" ) {
	auto check_parse_err = [&](const std::string &str) {
		test_fixture_world env_err(str);
		//if (env_err.ec.GetErrorCount()) { WARN("Error Collection: " << env_err.ec); }
		REQUIRE(env_err.ec.GetErrorCount() >= 1);
	};

	check_parse_err(R"({ "content" : [ )"
		R"({ "type" : "track_seg", "length" : "1000 ergs" } )"
	"] }");
	check_parse_err(R"({ "content" : [ )"
		R"({ "type" : "track_seg", "length" : "foobar 1000m" } )"
	"] }");
	check_parse_err(R"({ "content" : [ )"
		R"({ "type" : "track_seg", "length" : "." } )"
	"] }");
	check_parse_err(R"({ "content" : [ )"
		R"({ "type" : "track_seg", "length" : "9999999999999999999999999999999999999999999999999" } )"
	"] }");
	check_parse_err(R"({ "content" : [ )"
		R"({ "type" : "track_seg", "length" : "3000 miles" } )"
	"] }");
	check_parse_err(R"({ "content" : [ )"
		R"({ "type" : "track_seg", "length" : "0x10 mm" } )"
	"] }");
}

TEST_CASE( "deserialisation/gamestateload/typeerror", "Check that content section only types produce an error when encountered in gamestate section" ) {
	auto check_parse_err = [&](const std::string &str) {
		test_fixture_world env_err(str);
		env_err.ws->DeserialiseGameState(env_err.ec);
		INFO("Error Collection: " << env_err.ec);
		CHECK(env_err.ec.GetErrorCount() == 1);
		CHECK_CONTAINS(env_err.ec, "Unknown object type");
	};

	check_parse_err(R"({ "game_state" : [ )"
		R"({ "type" : "template" } )"
	"] }");
	check_parse_err(R"({ "game_state" : [ )"
		R"({ "type" : "typedef" } )"
	"] }");
	check_parse_err(R"({ "game_state" : [ )"
		R"({ "type" : "traction_type" } )"
	"] }");
	check_parse_err(R"({ "game_state" : [ )"
		R"({ "type" : "couple_points" } )"
	"] }");
	check_parse_err(R"({ "game_state" : [ )"
		R"({ "type" : "vehicle_class" } )"
	"] }");
}
