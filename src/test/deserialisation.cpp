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

#include "test/catch.hpp"
#include "test/deserialisation-test.h"
#include "test/testutil.h"
#include "core/track.h"
#include "core/train.h"
#include "core/traverse.h"
#include "core/world.h"
#include "core/signal.h"
#include "core/world_serialisation.h"

TEST_CASE( "deserialisation/error/notype", "Test missing object type" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{\"name\" : \"T1\"}"
	"] }";
	test_fixture_world env(track_test_str);

	//if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 1);
}

TEST_CASE( "deserialisation/error/wrongtype", "Test wrong value type" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{ \"type\" : \"trackseg\", \"traincount\" : \"foo\"}"
	"] }";
	test_fixture_world env(track_test_str);

	//if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 1);
}

TEST_CASE( "deserialisation/error/wrongtypeobjectarray", "Test supplying a value instead of an object/array" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{ \"type\" : \"trackseg\", \"speedlimits\" : 0, \"trs\" : 0}"
	"] }";
	test_fixture_world env(track_test_str);

	//if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 2);
}

TEST_CASE( "deserialisation/error/arraytypemismatch", "Test supplying a value of the wrong type in a value array" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{ \"type\" : \"routesignal\", \"routerestrictions\" : [ { \"targets\" : 0 } ] }, "
		"{ \"type\" : \"routesignal\", \"routerestrictions\" : [ { \"targets\" : [ \"foo\", 0, \"bar\" ] } ] }"
	"] }";
	test_fixture_world env(track_test_str);

	//if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 2);
}

TEST_CASE( "deserialisation/error/extravalues", "Test unknown extra value detection" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{ \"type\" : \"trackseg\", \"length\" : 0, \"foobar\" : \"baz\" }"
	"] }";
	test_fixture_world env(track_test_str);

	//if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 1);
}

TEST_CASE( "deserialisation/error/flagcontradiction", "Test contradictory flags" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{ \"type\" : \"routingmarker\", \"through\" : { \"allow\" : \"all\", \"deny\" : \"route\" } }"
	"] }";
	test_fixture_world env(track_test_str);

	//if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 1);
}

TEST_CASE( "deserialisation/error/flagnoncontradiction", "Test non-contradictory flags" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{ \"type\" : \"routingmarker\", \"through\" : { \"allowonly\" : \"all\", \"allow\" : \"route\" } }"
	"] }";
	test_fixture_world env(track_test_str);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);
}

TEST_CASE( "deserialisation/template/basic", "Test basic templating" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "template", "name" : "T1", "content" : { "through" : { "allow" : "all" } } }, )"
		R"({ "type" : "template", "name" : "T2", "content" : { "through" : { "deny" : "all" } } }, )"
		R"({ "type" : "template", "name" : "T3", "content" : { "through_rev" : { "deny" : "shunt" } } }, )"
		R"({ "type" : "routingmarker", "name" : "R1", "preparse" : "T1", "through" : { "deny" : "route" } }, )"
		R"({ "type" : "routingmarker", "name" : "R2", "preparse" : ["T1", "T2"], "through" : { "allow" : "route" } }, )"
		R"({ "type" : "routingmarker", "name" : "R3", "postparse" : ["T1", "T3"], "through" : { "deny" : "route" } } )"
	"] }";
	test_fixture_world env(track_test_str);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	auto checkrmrtflags = [&](const char *name, RPRT frontflags, RPRT backflags) {
		INFO("Routing Marker check for: " << name);

		routingmarker *rm = dynamic_cast<routingmarker *>(env.w->FindTrackByName(name));
		REQUIRE(rm != 0);
		CHECK(rm->GetAvailableRouteTypes(EDGE_FRONT) == frontflags);
		CHECK(rm->GetAvailableRouteTypes(EDGE_BACK) == backflags);
	};

	checkrmrtflags("R1", RPRT(0, route_class::All() & ~ route_class::Flag(route_class::RTC_ROUTE), 0), RPRT(0, route_class::All(), 0));
	checkrmrtflags("R2", RPRT(0, route_class::Flag(route_class::RTC_ROUTE), 0), RPRT(0, route_class::All(), 0));
	checkrmrtflags("R3", RPRT(0, route_class::All(), 0), RPRT(0, route_class::All() & ~ route_class::Flag(route_class::RTC_SHUNT), 0));
}

TEST_CASE( "deserialisation/template/nested", "Test nested templating" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "template", "name" : "T1", "content" : { "preparse" : "T2", "through" : { "allow" : "all" } } }, )"
		R"({ "type" : "template", "name" : "T2", "content" : { "postparse" : "T3", "through_rev" : { "deny" : "all" }, "through" : { "deny" : "shunt" } } }, )"
		R"({ "type" : "template", "name" : "T3", "content" : { "through_rev" : { "allow" : "all" } } }, )"
		R"({ "type" : "routingmarker", "name" : "R1", "preparse" : "T1", "through_rev" : { "deny" : [ "overlap" ] } } )"
	"] }";
	test_fixture_world env(track_test_str);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	routingmarker *rm = dynamic_cast<routingmarker *>(env.w->FindTrackByName("R1"));
	REQUIRE(rm != 0);
	CHECK(rm->GetAvailableRouteTypes(EDGE_FRONT) == RPRT(0, route_class::All(), 0));
	CHECK(rm->GetAvailableRouteTypes(EDGE_BACK) == RPRT(0, route_class::All() & ~route_class::Flag(route_class::RTC_OVERLAP), 0));
}

TEST_CASE( "deserialisation/typedef/nested", "Test nested type declaration" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "typedef", "newtype" : "base", "basetype" : "routingmarker", "content" : { "through" : { "deny" : "all" }, "through_rev" : { "deny" : "all" } } }, )"
		R"({ "type" : "typedef", "newtype" : "derived", "basetype" : "base", "content" : { "through_rev" : { "allow" : "all" } } }, )"
		R"({ "type" : "derived", "name" : "R1", "through_rev" : { "deny" : "overlap" } } )"
	"] }";
	test_fixture_world env(track_test_str);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	routingmarker *rm = dynamic_cast<routingmarker *>(env.w->FindTrackByName("R1"));
	REQUIRE(rm != 0);
	CHECK(rm->GetAvailableRouteTypes(EDGE_FRONT) == RPRT());
	CHECK(rm->GetAvailableRouteTypes(EDGE_BACK) == RPRT(0, route_class::All() & ~route_class::Flag(route_class::RTC_OVERLAP), 0));
}

TEST_CASE( "deserialisation/error/template", "Test reference to non-existent template" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "routingmarker", "name" : "R1", "preparse" : "T1" } )"
	"] }";
	test_fixture_world env(track_test_str);

	//if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 1);
}

TEST_CASE( "deserialisation/error/templaterecursion", "Test check for template recursion" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "template", "name" : "T1", "content" : { "preparse" : "T2" } }, )"
		R"({ "type" : "template", "name" : "T2", "content" : { "preparse" : "T1" } }, )"
		R"({ "type" : "routingmarker", "name" : "R1", "preparse" : "T1" } )"
	"] }";
	test_fixture_world env(track_test_str);

	//if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 1);
}

TEST_CASE( "deserialisation/error/typedefrecursion", "Test check for typedef recursion" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "typedef", "newtype" : "T1", "basetype" : "T2" }, )"
		R"({ "type" : "typedef", "newtype" : "T2", "basetype" : "T1" }, )"
		R"({ "type" : "T1", "name" : "R1" } )"
	"] }";
	test_fixture_world env(track_test_str);

	//if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 1);
}

TEST_CASE( "deserialisation/error/templateextravalues", "Test templating extra values check" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "template", "name" : "T1", "foobar" : true, "content" : { } } )"
	"] }";
	test_fixture_world env(track_test_str);

	//if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 1);
}

TEST_CASE( "deserialisation/error/typedefextravalues", "Test typedef extra values check" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "typedef", "newtype" : "derived", "basetype" : "base", "foobar" : true, "content" : { } } )"
	"] }";
	test_fixture_world env(track_test_str);

	//if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 1);
}

TEST_CASE( "deserialisation/scalartypeconv/length", "Test scalar type conversion (length)" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "trackseg", "length" : 1000 }, )"
		R"({ "type" : "trackseg", "length" : "1000  m" }, )"
		R"({ "type" : "trackseg", "length" : "2.4km" }, )"
		R"({ "type" : "trackseg", "length" : "0.56 miles" }, )"
		R"({ "type" : "trackseg", "length" : "4699088.432 yd" } )"
	"] }";
	test_fixture_world env(track_test_str);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
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
		//if(env_err.ec.GetErrorCount()) { WARN("Error Collection: " << env_err.ec); }
		REQUIRE(env_err.ec.GetErrorCount() >= 1);
	};
	check_parse_err(R"({ "content" : [ )"
		R"({ "type" : "trackseg", "length" : "1000 ergs" } )"
	"] }");
	check_parse_err(R"({ "content" : [ )"
		R"({ "type" : "trackseg", "length" : "foobar 1000m" } )"
	"] }");
	check_parse_err(R"({ "content" : [ )"
		R"({ "type" : "trackseg", "length" : "." } )"
	"] }");
	check_parse_err(R"({ "content" : [ )"
		R"({ "type" : "trackseg", "length" : "9999999999999999999999999999999999999999999999999" } )"
	"] }");
	check_parse_err(R"({ "content" : [ )"
		R"({ "type" : "trackseg", "length" : "3000 miles" } )"
	"] }");
	check_parse_err(R"({ "content" : [ )"
		R"({ "type" : "trackseg", "length" : "0x10 mm" } )"
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
	check_parse_err(R"({ "gamestate" : [ )"
		R"({ "type" : "template" } )"
	"] }");
	check_parse_err(R"({ "gamestate" : [ )"
		R"({ "type" : "typedef" } )"
	"] }");
	check_parse_err(R"({ "gamestate" : [ )"
		R"({ "type" : "tractiontype" } )"
	"] }");
	check_parse_err(R"({ "gamestate" : [ )"
		R"({ "type" : "couplepoints" } )"
	"] }");
	check_parse_err(R"({ "gamestate" : [ )"
		R"({ "type" : "vehicleclass" } )"
	"] }");
}
