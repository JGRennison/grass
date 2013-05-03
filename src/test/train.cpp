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
#include "train.h"
#include "deserialisation-test.h"
#include "world-test.h"
#include "testutil.h"

static void checkvc(world &w, const std::string &name, unsigned int length, unsigned int max_speed, unsigned int tractive_force, unsigned int tractive_power,
	unsigned int braking_force, unsigned int nominal_rail_traction_limit, unsigned int cumul_drag_const, unsigned int cumul_drag_v, unsigned int cumul_drag_v2,
	unsigned int face_drag_v2, unsigned int fullmass, unsigned int emptymass, const std::string &traction_str) {
	SCOPED_INFO("checkvc for: " + name);
	vehicle_class *vc = w.FindVehicleClassByName(name);
	REQUIRE(vc != 0);
	CHECK(vc->name == name);
	CHECK(vc->length == length);
	CHECK(vc->max_speed == max_speed);
	CHECK(vc->tractive_force == tractive_force);
	CHECK(vc->tractive_power == tractive_power);
	CHECK(vc->tractive_power == tractive_power);
	CHECK(vc->braking_force == braking_force);
	CHECK(vc->nominal_rail_traction_limit == nominal_rail_traction_limit);
	CHECK(vc->cumul_drag_const == cumul_drag_const);
	CHECK(vc->cumul_drag_v == cumul_drag_v);
	CHECK(vc->cumul_drag_v2 == cumul_drag_v2);
	CHECK(vc->face_drag_v2 == face_drag_v2);
	CHECK(vc->fullmass == fullmass);
	CHECK(vc->emptymass == emptymass);
	CHECK(vc->tractiontypes.DumpString() == traction_str);
}

TEST_CASE( "train/vehicle_class/deserialisation", "Test vehicle class deserialisation" ) {
	std::string test_vc_deserialisation_1 =
	R"({ "content" : [ )"
		R"({ "type" : "tractiontype", "name" : "diesel", "alwaysavailable" : true }, )"
		R"({ "type" : "tractiontype", "name" : "AC" }, )"
		R"({ "type" : "vehicleclass", "name" : "VC1", "length" : "25m", "maxspeed" : "125.50mph", "tractiveforce" : "500kN", "tractivepower" : "1000hp", "brakingforce" : "1MN",)"
			R"( "tractivelimit" : "1.2MN", "dragc" : 500, "dragv" : 100, "dragv2" : 200, "facedragv2" : 50, "fullmass" : "15t", "emptymass" : "10t",)"
			R"( "tractiontypes" : [ "diesel", "AC" ] }, )"
		R"({ "type" : "vehicleclass", "name" : "VC2", "length" : "30yd", "maxspeed" : "170km/h", "tractiveforce" : "500kN", "tractivepower" : "1000hp", "brakingforce" : "1MN",)"
			R"( "tractivelimit" : "1.2MN", "dragc" : 500, "dragv" : 100, "mass" : "15t",)"
			R"( "tractiontypes" : [ "AC", "AC" ] }, )"
		R"({ "type" : "vehicleclass", "name" : "VC3", "fullmass" : "15t", "emptymass" : "20t" } )"
	"] }";

	test_fixture_world env(test_vc_deserialisation_1);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	checkvc(env.w, "VC1", 25000, 56098, 500000, 746000, 1000000, 1200000, 500, 100, 200, 50, 15000, 10000, "AC,diesel");
	checkvc(env.w, "VC2", 27420, 47260, 500000, 746000, 1000000, 1200000, 500, 100, 0, 0, 15000, 15000, "AC");
	checkvc(env.w, "VC3", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 20000, 20000, "");
}
