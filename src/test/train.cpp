//  grass - Generic Rail And Signalling Simulator
//
//  WEBSITE: http://bitbucket.org/JGRennison/grass
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

#include "test/catch.hpp"
#include "test/deserialisation-test.h"
#include "test/world-test.h"
#include "test/testutil.h"
#include "core/train.h"
#include "core/util.h"
#include <sstream>

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

static void checktd(const train_dynamics &td, unsigned int length, unsigned int maxspeed, unsigned int tractive_force, unsigned int tractive_power, unsigned int braking_force, unsigned int dragc, unsigned int dragv, unsigned int dragv2, unsigned int mass) {
	CHECK(td.total_length == length);
	CHECK(td.veh_max_speed == maxspeed);
	CHECK(td.total_tractive_force == tractive_force);
	CHECK(td.total_tractive_power == tractive_power);
	CHECK(td.total_braking_force == braking_force);
	CHECK(td.total_drag_const == dragc);
	CHECK(td.total_drag_v == dragv);
	CHECK(td.total_drag_v2 == dragv2);
	CHECK(td.total_mass == mass);
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
		R"({ "type" : "vehicleclass", "name" : "VC3", "length" : "120ft", "tractiveforce" : "1024lbf", "tractivepower" : "512 ft lbf/s", "brakingforce" : "1MN",)"
			R"( "mass" : "15t" } )"
	"] }";

	test_fixture_world env(test_vc_deserialisation_1);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	checkvc(env.w, "VC1", 25000, 56098, 500000, 746000, 1000000, 1200000, 500, 100, 200, 50, 15000, 10000, "AC,diesel");
	checkvc(env.w, "VC2", 27420, 47260, 500000, 746000, 1000000, 1200000, 500, 100, 0, 0, 15000, 15000, "AC");
	checkvc(env.w, "VC3", 36600, UINT_MAX, 4555, 684, 1000000, 1000000, 0, 0, 0, 0, 15000, 15000, "");

	auto parsecheckerr = [&](const std::string &json, const std::string &errstr) {
		test_fixture_world env(json);
		SCOPED_INFO("Error Collection: " << env.ec);
		CHECK(env.ec.GetErrorCount() == 1);
		std::stringstream s;
		s << env.ec;
		CHECK(s.str().find(errstr) != std::string::npos);
	};

	parsecheckerr(
		R"({ "content" : [ )"
		R"({ "type" : "vehicleclass", "name" : "VCE", "length" : "25m", "fullmass" : "15t", "emptymass" : "20t" } )"
		"] }"
		, "full mass < empty mass");
	parsecheckerr(
		R"({ "content" : [ )"
		R"({ "type" : "vehicleclass", "name" : "VCE", "length" : "25m", "mass" : "15t", "emptymass" : "20t" } )"
		"] }"
		, "Unknown object property");
	parsecheckerr(
		R"({ "content" : [ )"
		R"({ "type" : "vehicleclass", "name" : "VCE", "length" : "0", "mass" : "15t" } )"
		"] }"
		, "non-zero");
	parsecheckerr(
		R"({ "content" : [ )"
		R"({ "type" : "vehicleclass", "name" : "VCE", "length" : "30m" } )"
		"] }"
		, "non-zero");
}

TEST_CASE("/train/train/deserialisation/typeerror", "Check that trains cannot appear in content section") {

	auto parsecheckerr = [&](const std::string &json, const std::string &errstr) {
		test_fixture_world env(json);
		env.ws.DeserialiseGameState(env.ec);
		SCOPED_INFO("Error Collection: " << env.ec);
		CHECK(env.ec.GetErrorCount() == 1);
		std::stringstream s;
		s << env.ec;
		CHECK(s.str().find(errstr) != std::string::npos);
	};

	parsecheckerr(
		R"({ "content" : [ )"
		R"({ "type" : "train", "nosuch" : "field" } )"
		"] }"
		, "Unknown object type");
	parsecheckerr(
		R"({ "gamestate" : [ )"
		R"({ "type" : "train", "nosuch" : "field" } )"
		"] }"
		, "Unknown object property");
}

TEST_CASE("/train/train/deserialisation/dynamics", "Train deserialisation and dynamics") {
	std::string test_train_deserialisation_dynamics =
	R"({ "content" : [ )"
		R"({ "type" : "tractiontype", "name" : "diesel", "alwaysavailable" : true }, )"
		R"({ "type" : "tractiontype", "name" : "AC" }, )"
		R"({ "type" : "vehicleclass", "name" : "VC1", "length" : "25m", "maxspeed" : "30m/s", "tractiveforce" : "500kN", "tractivepower" : "1000hp", "brakingforce" : "1MN",)"
			R"( "tractivelimit" : "1.2MN", "dragc" : 500, "dragv" : 100, "dragv2" : 200, "facedragv2" : 50, "fullmass" : "15t", "emptymass" : "10t",)"
			R"( "tractiontypes" : [ "diesel", "AC" ] }, )"
		R"({ "type" : "vehicleclass", "name" : "VC2", "length" : "30m", "maxspeed" : "170km/h", "tractiveforce" : "500kN", "tractivepower" : "1000hp", "brakingforce" : "1MN",)"
			R"( "tractivelimit" : "1.2MN", "dragc" : 500, "dragv" : 100, "mass" : "15t",)"
			R"( "tractiontypes" : [ "AC" ] } )"
	R"( ], )"
	R"( "gamestate" : [ )"
		"%s"
	"] }";

	std::string veh1 = R"({ "type" : "train", "activetractions" : %s, "vehicleclasses" : [ "VC1", "VC2" ] })";
	std::string veh2 = R"({ "type" : "train", "activetractions" : %s, "vehicleclasses" : [ { "classname" : "VC1", "full" : true }, { "classname" : "VC2", "count" : 2 } ] })";
	std::string veh3 = R"({ "type" : "train", "activetractions" : %s, "vehicleclasses" : [ { "classname" : "VC1", "full" : true }, { "classname" : "VC2", "count" : 2, "segmentmass" : "0" } ] })";
	std::string veh4 = R"({ "type" : "train", "activetractions" : %s, "vehicleclasses" : [ { "classname" : "VC1", "count" : 2, "segmentmass" : "24t" }, { "classname" : "VC2", "count" : 0 } ] })";

	auto mktenv = [&](const std::string &str) -> test_fixture_world* {
		return new test_fixture_world(str);
	};

	auto make_test_tenv = [&](const std::string &vehstring, const std::string &activetractions) {
		test_fixture_world *env = mktenv(string_format(string_format(test_train_deserialisation_dynamics, vehstring.c_str()), activetractions.c_str()));
		env->ws.DeserialiseGameState(env->ec);
		return env;
	};

	auto test_one_train = [&](const std::string &vehstring, const std::string &activetractions, std::function<void(train &)> f) {
		test_fixture_world &env = *make_test_tenv(vehstring, activetractions);
		SCOPED_INFO("Error Collection: " << env.ec);
		REQUIRE(env.ec.GetErrorCount() == 0);

		unsigned int count = 0;
		unsigned int traincount = env.w.EnumerateTrains([&](train &t) {
			count++;
			f(t);
		});
		CHECK(count == 1);
		CHECK(traincount == 1);
		delete &env;
	};

	auto test_one_train_expect_err = [&](const std::string &vehstring, const std::string &activetractions, unsigned int errcount) {
		test_fixture_world &env = *make_test_tenv(vehstring, activetractions);
		SCOPED_INFO("Error Collection: " << env.ec);
		CHECK(env.ec.GetErrorCount() == errcount);
		delete &env;
	};

	test_one_train(veh1, "[\"diesel\"]", [&](train &t) {
		SCOPED_INFO("Test 1");
		checktd(t.GetTrainDynamics(), 55000, 30000, 500000, 746000, 1000000 * 2, 1000, 200, 250, 25000);
	});
	test_one_train(veh1, "[\"AC\"]", [&](train &t) {
		SCOPED_INFO("Test 2");
		checktd(t.GetTrainDynamics(), 55000, 30000, 500000 * 2, 746000 * 2, 1000000 * 2, 1000, 200, 250, 25000);
	});
	test_one_train(veh1, "[\"diesel\",\"AC\"]", [&](train &t) {
		SCOPED_INFO("Test 3");
		checktd(t.GetTrainDynamics(), 55000, 30000, 500000 * 2, 746000 * 2, 1000000 * 2, 1000, 200, 250, 25000);
	});
	test_one_train(veh1, "[]", [&](train &t) {
		SCOPED_INFO("Test 4");
		checktd(t.GetTrainDynamics(), 55000, 30000, 0, 0, 1000000 * 2, 1000, 200, 250, 25000);
	});
	test_one_train(veh2, "[\"AC\"]", [&](train &t) {
		SCOPED_INFO("Test 5");
		checktd(t.GetTrainDynamics(), 85000, 30000, 500000 * 3, 746000 * 3, 1000000 * 3, 1500, 300, 250, 45000);
	});
	{
		SCOPED_INFO("Test 6");
		test_one_train_expect_err(veh3, "[\"AC\"]", 1);
	}
	test_one_train(veh4, "[\"AC\"]", [&](train &t) {
		SCOPED_INFO("Test 7");
		checktd(t.GetTrainDynamics(), 50000, 30000, 500000 * 2, 746000 * 2, 1000000 * 2, 1000, 200, 500, 24000);
	});
}