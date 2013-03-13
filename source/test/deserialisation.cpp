//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://grss.sourceforge.net
//
//  NOTE: This software is licensed under the GPL. See: COPYING-GPL.txt
//
//  This program  is distributed in the  hope that it will  be useful, but
//  WITHOUT   ANY  WARRANTY;   without  even   the  implied   warranty  of
//  MERCHANTABILITY  or FITNESS  FOR A  PARTICULAR PURPOSE.   See  the GNU
//  General Public License for more details.
//
//  Jonathan Rennison (or anybody else) is in no way responsible, or liable
//  for this program or its use in relation to users, 3rd parties or to any
//  persons in any way whatsoever.
//
//  You  should have  received a  copy of  the GNU  General Public
//  License along  with this program; if  not, write to  the Free Software
//  Foundation, Inc.,  59 Temple Place,  Suite 330, Boston,  MA 02111-1307
//  USA
//
//  2013 - Jonathan Rennison <j.g.rennison@gmail.com>
//==========================================================================

#include "catch.hpp"
#include "track.h"
#include "train.h"
#include "traverse.h"
#include "world.h"
#include "signal.h"
#include "world_serialisation.h"
#include "deserialisation-test.h"

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
		"{ \"type\" : \"trackseg\", \"length\" : \"foo\"}"
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
		"{ \"type\" : \"routingmarker\", \"through\" : true, \"routethrough\" : false }"
	"] }";
	test_fixture_world env(track_test_str);
	
	//if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 1);
}

TEST_CASE( "deserialisation/error/flagnoncontradiction", "Test non-contradictory flags" ) {
	std::string track_test_str = 
	"{ \"content\" : [ "
		"{ \"type\" : \"routingmarker\", \"through\" : true, \"routethrough\" : true }"
	"] }";
	test_fixture_world env(track_test_str);
	
	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);
}

TEST_CASE( "deserialisation/template/basic", "Test basic templating" ) {
	std::string track_test_str = 
	R"({ "content" : [ )"
		R"({ "type" : "template", "name" : "T1", "content" : { "through" : true } }, )"
		R"({ "type" : "template", "name" : "T2", "content" : { "through" : false } }, )"
		R"({ "type" : "template", "name" : "T3", "content" : { "shuntthrough_rev" : false } }, )"
		R"({ "type" : "routingmarker", "name" : "R1", "preparse" : "T1", "routethrough" : false }, )"
		R"({ "type" : "routingmarker", "name" : "R2", "preparse" : ["T1", "T2"], "routethrough" : true }, )"
		R"({ "type" : "routingmarker", "name" : "R3", "postparse" : ["T1", "T3"], "routethrough" : false } )"
	"] }";
	test_fixture_world env(track_test_str);
	
	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);
	
	auto checkrmrtflags = [&](const char *name, unsigned int frontflags, unsigned int backflags) {
		SCOPED_INFO("Routing Marker check for: " << name);
		
		routingmarker *rm = dynamic_cast<routingmarker *>(env.w.FindTrackByName(name));
		REQUIRE(rm != 0);
		CHECK(rm->GetAvailableRouteTypes(EDGE_FRONT) == frontflags);
		CHECK(rm->GetAvailableRouteTypes(EDGE_BACK) == backflags);
	};
	
	checkrmrtflags("R1", routingpoint::RPRT_MASK_TRANS & ~routingpoint::RPRT_ROUTETRANS, routingpoint::RPRT_MASK_TRANS);
	checkrmrtflags("R2", routingpoint::RPRT_ROUTETRANS, routingpoint::RPRT_MASK_TRANS);
	checkrmrtflags("R3", routingpoint::RPRT_MASK_TRANS, routingpoint::RPRT_MASK_TRANS & ~routingpoint::RPRT_SHUNTTRANS);
}

TEST_CASE( "deserialisation/template/nested", "Test nested templating" ) {
	std::string track_test_str = 
	R"({ "content" : [ )"
		R"({ "type" : "template", "name" : "T1", "content" : { "preparse" : "T2", "through" : true } }, )"
		R"({ "type" : "template", "name" : "T2", "content" : { "postparse" : "T3", "through_rev" : false, "shuntthrough" : false } }, )"
		R"({ "type" : "template", "name" : "T3", "content" : { "shuntthrough_rev" : true, "through_rev" : true } }, )"
		R"({ "type" : "routingmarker", "name" : "R1", "preparse" : "T1", "overlapthrough_rev" : false } )"
	"] }";
	test_fixture_world env(track_test_str);
	
	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);
	
	routingmarker *rm = dynamic_cast<routingmarker *>(env.w.FindTrackByName("R1"));
	REQUIRE(rm != 0);
	CHECK(rm->GetAvailableRouteTypes(EDGE_FRONT) == routingpoint::RPRT_MASK_TRANS);
	CHECK(rm->GetAvailableRouteTypes(EDGE_BACK) == (routingpoint::RPRT_SHUNTTRANS | routingpoint::RPRT_ROUTETRANS));
}

TEST_CASE( "deserialisation/typedef/nested", "Test nested type declaration" ) {
	std::string track_test_str = 
	R"({ "content" : [ )"
		R"({ "type" : "typedef", "newtype" : "base", "basetype" : "routingmarker", "content" : { "through" : false, "through_rev" : false } }, )"
		R"({ "type" : "typedef", "newtype" : "derived", "basetype" : "base", "content" : { "through_rev" : true } }, )"
		R"({ "type" : "derived", "name" : "R1", "overlapthrough_rev" : false } )"
	"] }";
	test_fixture_world env(track_test_str);
	
	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);
	
	routingmarker *rm = dynamic_cast<routingmarker *>(env.w.FindTrackByName("R1"));
	REQUIRE(rm != 0);
	CHECK(rm->GetAvailableRouteTypes(EDGE_FRONT) == 0);
	CHECK(rm->GetAvailableRouteTypes(EDGE_BACK) == (routingpoint::RPRT_SHUNTTRANS | routingpoint::RPRT_ROUTETRANS));
}
