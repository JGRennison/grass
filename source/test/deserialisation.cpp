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
