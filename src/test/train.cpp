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
#include "test/deserialisation-test.h"
#include "test/world-test.h"
#include "test/testutil.h"
#include "core/train.h"
#include "core/trackcircuit.h"
#include "core/util.h"
#include "core/points.h"
#include <sstream>

static void checkvc(world &w, const std::string &name, unsigned int length, unsigned int max_speed, unsigned int tractive_force, unsigned int tractive_power,
	unsigned int braking_force, unsigned int nominal_rail_traction_limit, unsigned int cumul_drag_const, unsigned int cumul_drag_v, unsigned int cumul_drag_v2,
	unsigned int face_drag_v2, unsigned int fullmass, unsigned int emptymass, const std::string &traction_str) {
	INFO("checkvc for: " + name);
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

	checkvc(*(env.w), "VC1", 25000, 56098, 500000, 746000, 1000000, 1200000, 500, 100, 200, 50, 15000, 10000, "AC,diesel");
	checkvc(*(env.w), "VC2", 27420, 47260, 500000, 746000, 1000000, 1200000, 500, 100, 0, 0, 15000, 15000, "AC");
	checkvc(*(env.w), "VC3", 36600, UINT_MAX, 4555, 684, 1000000, 1000000, 0, 0, 0, 0, 15000, 15000, "");

	auto parsecheckerr = [&](const std::string &json, const std::string &errstr) {
		test_fixture_world env(json);
		INFO("Error Collection: " << env.ec);
		CHECK(env.ec.GetErrorCount() == 1);
		CHECK_CONTAINS(env.ec, errstr);
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
		env.ws->DeserialiseGameState(env.ec);
		INFO("Error Collection: " << env.ec);
		CHECK(env.ec.GetErrorCount() == 1);
		CHECK_CONTAINS(env.ec, errstr);
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
		env->ws->DeserialiseGameState(env->ec);
		return env;
	};

	auto test_one_train = [&](const std::string &vehstring, const std::string &activetractions, std::function<void(train &)> f) {
		test_fixture_world &env = *make_test_tenv(vehstring, activetractions);
		INFO("Error Collection: " << env.ec);
		REQUIRE(env.ec.GetErrorCount() == 0);

		unsigned int count = 0;
		unsigned int traincount = env.w->EnumerateTrains([&](train &t) {
			count++;
			f(t);
		});
		CHECK(count == 1);
		CHECK(traincount == 1);
		delete &env;
	};

	auto test_one_train_expect_err = [&](const std::string &vehstring, const std::string &activetractions, unsigned int errcount) {
		test_fixture_world &env = *make_test_tenv(vehstring, activetractions);
		INFO("Error Collection: " << env.ec);
		CHECK(env.ec.GetErrorCount() == errcount);
		delete &env;
	};

	test_one_train(veh1, "[\"diesel\"]", [&](train &t) {
		INFO("Test 1");
		checktd(t.GetTrainDynamics(), 55000, 30000, 500000, 746000, 1000000 * 2, 1000, 200, 250, 25000);
	});
	test_one_train(veh1, "[\"AC\"]", [&](train &t) {
		INFO("Test 2");
		checktd(t.GetTrainDynamics(), 55000, 30000, 500000 * 2, 746000 * 2, 1000000 * 2, 1000, 200, 250, 25000);
	});
	test_one_train(veh1, "[\"diesel\",\"AC\"]", [&](train &t) {
		INFO("Test 3");
		checktd(t.GetTrainDynamics(), 55000, 30000, 500000 * 2, 746000 * 2, 1000000 * 2, 1000, 200, 250, 25000);
	});
	test_one_train(veh1, "[]", [&](train &t) {
		INFO("Test 4");
		checktd(t.GetTrainDynamics(), 55000, 30000, 0, 0, 1000000 * 2, 1000, 200, 250, 25000);
	});
	test_one_train(veh2, "[\"AC\"]", [&](train &t) {
		INFO("Test 5");
		checktd(t.GetTrainDynamics(), 85000, 30000, 500000 * 3, 746000 * 3, 1000000 * 3, 1500, 300, 250, 45000);
	});
	{
		INFO("Test 6");
		test_one_train_expect_err(veh3, "[\"AC\"]", 1);
	}
	test_one_train(veh4, "[\"AC\"]", [&](train &t) {
		INFO("Test 7");
		checktd(t.GetTrainDynamics(), 50000, 30000, 500000 * 2, 746000 * 2, 1000000 * 2, 1000, 200, 500, 24000);
	});
}

static void check_position(const track_location &l, std::string trackname, EDGETYPE direction, unsigned int trackpos) {
	REQUIRE(l.GetTrack() != nullptr);
	CHECK(l.GetTrack()->GetName() == trackname);
	CHECK(l.GetDirection() == direction);
	CHECK(l.GetOffset() == trackpos);
};

static void check_train_position(const train *t, std::string head_trackname, EDGETYPE head_direction, unsigned int head_trackpos,
		std::string tail_trackname, EDGETYPE tail_direction, unsigned int tail_trackpos) {
	REQUIRE(t != nullptr);
	INFO("Checking train: " << t->GetName());
	{
		INFO("Head");
		check_position(t->GetTrainMotionState().head_pos, head_trackname, head_direction, head_trackpos);
	}
	{
		INFO("Tail");
		check_position(t->GetTrainMotionState().tail_pos, tail_trackname, tail_direction, tail_trackpos);
	}
};

static void check_train_is_uprooted(const train *t) {
	REQUIRE(t != nullptr);
	INFO("Checking train: " << t->GetName());
	CHECK(t->GetTrainMotionState().head_pos == track_location());
	CHECK(t->GetTrainMotionState().tail_pos == track_location());
};

template <typename F> static void generic_drop_train(train *t, const track_location &l, F func) {
	INFO("Checking train: "  << t->GetName());
	error_collection ec;
	t->DropTrainIntoPosition(l, ec);
	INFO("Head: "  << t->GetTrainMotionState().head_pos);
	INFO("Tail: "  << t->GetTrainMotionState().tail_pos);
	func(ec);
}

static void check_drop_train(train *t, const track_location &l) {
	INFO("check_drop_train");
	generic_drop_train(t, l, [&](error_collection &ec) {
		INFO("Error Collection: " << ec);
		CHECK(ec.GetErrorCount() == 0);
		CHECK(t->GetTrainMotionState().head_pos.IsValid() == true);
		CHECK(t->GetTrainMotionState().tail_pos.IsValid() == true);
	});
}

static void check_drop_train_insufficient_track(train *t, const track_location &l) {
	INFO("check_drop_train_insufficient_track");
	generic_drop_train(t, l, [&](error_collection &ec) {
		CHECK(ec.GetErrorCount() == 1);
		CHECK_CONTAINS(ec, "Insufficient track to drop train");
	});
}

static void check_uproot_train(train *t) {
	error_collection ec;
	t->UprootTrain(ec);
	INFO("Error Collection: " << ec);
	CHECK(ec.GetErrorCount() == 0);
}

TEST_CASE("/train/train/dropanduproot", "Drop train into position and train uprooting tests") {
	std::string test_train_drop_into_position =
	R"({ "content" : [ )"
		R"({ "type" : "startofline", "name" : "A"}, )"
		R"({ "type" : "trackseg", "name" : "T0", "length" : "50m", "trackcircuit" : "TC0", "tracktriggers" : "TT0" }, )"
		R"({ "type" : "points", "name" : "P0", "connect" : { "to" : "C" } }, )"
		R"({ "type" : "trackseg", "name" : "T1", "length" : "50m" }, )"
		R"({ "type" : "routesignal", "name" : "S0", "shuntsignal" : true }, )"
		R"({ "type" : "trackseg", "name" : "T3", "length" : "200m" }, )"
		R"({ "type" : "endofline", "name" : "B" }, )"

		R"({ "type" : "endofline", "name" : "C" }, )"

		R"({ "type" : "tractiontype", "name" : "diesel", "alwaysavailable" : true }, )"
		R"({ "type" : "tractiontype", "name" : "AC" }, )"
		R"({ "type" : "vehicleclass", "name" : "VC1", "length" : "30m", "mass" : "15t", "tractiontypes" : [ "diesel" ] }, )"
		R"({ "type" : "vehicleclass", "name" : "VC2", "length" : "30m", "mass" : "15t", "tractiontypes" : [ "AC" ] } )"
	R"( ], )"
	R"( "gamestate" : [ )"
		"%s"
	"] }";

	auto make_test_tenv = [&](const std::string &trainstring) {
		std::unique_ptr<test_fixture_world> env {
			new test_fixture_world_init_checked(string_format(test_train_drop_into_position, trainstring.c_str()), true, true, true)
		};
		return std::move(env);
	};

	auto check_ttcbs = [&](const test_fixture_world &tenv, std::string logtag, bool occupied) {
		INFO(logtag << ": checking TC0 and TT0: expect: " << (occupied ? "occupied" : "clear"));
		CHECK(PTR_CHECK(tenv.w->FindTrackTrainBlockOrTrackCircuitByName("TC0"))->Occupied() == occupied);
		CHECK(PTR_CHECK(tenv.w->FindTrackTrainBlockOrTrackCircuitByName("TT0"))->Occupied() == occupied);
	};

	{
		INFO("1: Deserialisation drop");
		std::unique_ptr<test_fixture_world> tenv = make_test_tenv(
				R"({ "type" : "train", "name" : "TR0", "activetractions" : ["diesel"], "vehicleclasses" : [ { "classname" : "VC1", "count" : 2} ], )"
				R"( "position" : { "piece" : "S0", "dir" : "front", "offset" : 0 } } )"
		);
		train *t = tenv->w->FindTrainByName("TR0");
		REQUIRE(t != nullptr);
		check_train_position(t, "S0", EDGE_FRONT, 0, "T0", EDGE_FRONT, 40000);
		check_ttcbs(*tenv, "Pre-uproot", true);

		check_uproot_train(t);
		check_train_is_uprooted(t);
		check_ttcbs(*tenv, "Post-uproot", false);
	}
	{
		INFO("2: Direct drop");
		std::unique_ptr<test_fixture_world> tenv = make_test_tenv(
				R"({ "type" : "train", "name" : "TR0", "activetractions" : ["diesel"], "vehicleclasses" : [ { "classname" : "VC1", "count" : 2} ] } )"
		);
		check_ttcbs(*tenv, "Pre-drop", false);
		train *t = tenv->w->FindTrainByName("TR0");
		REQUIRE(t != nullptr);
		check_train_is_uprooted(t);

		check_drop_train(t, track_location(tenv->w->FindTrackByName("S0"), EDGE_FRONT, 0));
		check_train_position(t, "S0", EDGE_FRONT, 0, "T0", EDGE_FRONT, 40000);
		check_ttcbs(*tenv, "Post-drop, pre-uproot", true);

		check_uproot_train(t);
		check_train_is_uprooted(t);
		check_ttcbs(*tenv, "Post-uproot", false);
	}
	{
		INFO("3: Direct drop: train too long error");
		std::unique_ptr<test_fixture_world> tenv = make_test_tenv(
				R"({ "type" : "train", "name" : "TR0", "activetractions" : ["diesel"], "vehicleclasses" : [ { "classname" : "VC1", "count" : 4} ] } )"
		);
		train *t = PTR_CHECK(tenv->w->FindTrainByName("TR0"));
		check_drop_train_insufficient_track(t, track_location(tenv->w->FindTrackByName("S0"), EDGE_FRONT, 0));
	}
	{
		INFO("4: Direct drop: non-zero offset, just short of TTCBs");
		std::unique_ptr<test_fixture_world> tenv = make_test_tenv(
				R"({ "type" : "train", "name" : "TR0", "activetractions" : ["diesel"], "vehicleclasses" : [ { "classname" : "VC1", "count" : 1} ] } )"
		);
		check_ttcbs(*tenv, "Pre-drop", false);
		train *t = PTR_CHECK(tenv->w->FindTrainByName("TR0"));
		check_drop_train(t, track_location(tenv->w->FindTrackByName("T1"), EDGE_FRONT, 30000));
		check_train_position(t, "T1", EDGE_FRONT, 30000, "T1", EDGE_FRONT, 0);
		check_ttcbs(*tenv, "Post-drop", false);
	}
	{
		INFO("5: Direct drop: non-zero offset, reverse, just short of end");
		std::unique_ptr<test_fixture_world> tenv = make_test_tenv(
				R"({ "type" : "train", "name" : "TR0", "activetractions" : ["diesel"], "vehicleclasses" : [ { "classname" : "VC1", "count" : 1} ] } )"
		);
		train *t = PTR_CHECK(tenv->w->FindTrainByName("TR0"));
		check_drop_train(t, track_location(tenv->w->FindTrackByName("T3"), EDGE_BACK, 170000));
		check_train_position(t, "T3", EDGE_BACK, 170000, "T3", EDGE_BACK, 200000);
	}
	{
		INFO("6: Direct drop: non-zero offset, reverse, just over end error");
		std::unique_ptr<test_fixture_world> tenv = make_test_tenv(
				R"({ "type" : "train", "name" : "TR0", "activetractions" : ["diesel"], "vehicleclasses" : [ { "classname" : "VC1", "count" : 1} ] } )"
		);
		train *t = PTR_CHECK(tenv->w->FindTrainByName("TR0"));
		check_drop_train_insufficient_track(t, track_location(tenv->w->FindTrackByName("T3"), EDGE_BACK, 170001));
	}
	{
		INFO("7: Direct drop: over OOC points");
		std::unique_ptr<test_fixture_world> tenv = make_test_tenv(
				R"({ "type" : "train", "name" : "TR0", "activetractions" : ["diesel"], "vehicleclasses" : [ { "classname" : "VC1", "count" : 2} ] } )"
		);
		train *t = PTR_CHECK(tenv->w->FindTrainByName("TR0"));
		PTR_CHECK(tenv->w->FindTrackByNameCast<points>("P0"))->SetPointsFlagsMasked(0, points::PTF::OOC, points::PTF::OOC);
		check_drop_train_insufficient_track(t, track_location(tenv->w->FindTrackByName("S0"), EDGE_FRONT, 0));
	}
	{
		INFO("8: Direct drop: collision test: overlap error at back");
		std::unique_ptr<test_fixture_world> tenv = make_test_tenv(
				R"({ "type" : "train", "name" : "TR0", "activetractions" : ["diesel"], "vehicleclasses" : [ { "classname" : "VC1", "count" : 1} ] }, )"
				R"({ "type" : "train", "name" : "TR1", "activetractions" : ["diesel"], "vehicleclasses" : [ { "classname" : "VC1", "count" : 1} ] } )"
		);
		check_drop_train(PTR_CHECK(tenv->w->FindTrainByName("TR0")), track_location(tenv->w->FindTrackByName("S0"), EDGE_FRONT, 0));
		check_drop_train_insufficient_track(PTR_CHECK(tenv->w->FindTrainByName("TR1")), track_location(tenv->w->FindTrackByName("T3"), EDGE_FRONT, 29999));
	}
	{
		INFO("9: Direct drop: collision test: just fits behind");
		std::unique_ptr<test_fixture_world> tenv = make_test_tenv(
				R"({ "type" : "train", "name" : "TR0", "activetractions" : ["diesel"], "vehicleclasses" : [ { "classname" : "VC1", "count" : 1} ] }, )"
				R"({ "type" : "train", "name" : "TR1", "activetractions" : ["diesel"], "vehicleclasses" : [ { "classname" : "VC1", "count" : 1} ] } )"
		);
		check_drop_train(PTR_CHECK(tenv->w->FindTrainByName("TR0")), track_location(tenv->w->FindTrackByName("S0"), EDGE_FRONT, 0));
		check_drop_train(PTR_CHECK(tenv->w->FindTrainByName("TR1")), track_location(tenv->w->FindTrackByName("T3"), EDGE_FRONT, 30000));
	}
	{
		INFO("10: Direct drop: collision test: overlap error at front");
		std::unique_ptr<test_fixture_world> tenv = make_test_tenv(
				R"({ "type" : "train", "name" : "TR0", "activetractions" : ["diesel"], "vehicleclasses" : [ { "classname" : "VC1", "count" : 1} ] }, )"
				R"({ "type" : "train", "name" : "TR1", "activetractions" : ["diesel"], "vehicleclasses" : [ { "classname" : "VC1", "count" : 1} ] } )"
		);
		check_drop_train(PTR_CHECK(tenv->w->FindTrainByName("TR0")), track_location(tenv->w->FindTrackByName("S0"), EDGE_FRONT, 0));
		check_drop_train_insufficient_track(PTR_CHECK(tenv->w->FindTrainByName("TR1")), track_location(tenv->w->FindTrackByName("T1"), EDGE_BACK, 49999));
	}
	{
		INFO("11: Direct drop: collision test: just fits in front");
		std::unique_ptr<test_fixture_world> tenv = make_test_tenv(
				R"({ "type" : "train", "name" : "TR0", "activetractions" : ["diesel"], "vehicleclasses" : [ { "classname" : "VC1", "count" : 1} ] }, )"
				R"({ "type" : "train", "name" : "TR1", "activetractions" : ["diesel"], "vehicleclasses" : [ { "classname" : "VC1", "count" : 1} ] } )"
		);
		check_drop_train(PTR_CHECK(tenv->w->FindTrainByName("TR0")), track_location(tenv->w->FindTrackByName("S0"), EDGE_FRONT, 0));
		check_drop_train(PTR_CHECK(tenv->w->FindTrainByName("TR1")), track_location(tenv->w->FindTrackByName("S0"), EDGE_BACK, 0));
	}
	{
		INFO("12: Direct drop: collision test: overlap error across whole length");
		std::unique_ptr<test_fixture_world> tenv = make_test_tenv(
				R"({ "type" : "train", "name" : "TR0", "activetractions" : ["diesel"], "vehicleclasses" : [ { "classname" : "VC1", "count" : 2} ] }, )"
				R"({ "type" : "train", "name" : "TR1", "activetractions" : ["diesel"], "vehicleclasses" : [ { "classname" : "VC1", "count" : 1} ] } )"
		);
		check_drop_train(PTR_CHECK(tenv->w->FindTrainByName("TR0")), track_location(tenv->w->FindTrackByName("S0"), EDGE_FRONT, 0));
		check_drop_train_insufficient_track(PTR_CHECK(tenv->w->FindTrainByName("TR1")), track_location(tenv->w->FindTrackByName("T1"), EDGE_FRONT, 49000));
	}
}
