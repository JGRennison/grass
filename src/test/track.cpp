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
#include "test/world_test.h"
#include "test/test_util.h"
#include "core/track.h"
#include "core/track_piece.h"
#include "core/points.h"
#include "core/train.h"
#include "core/traverse.h"
#include "core/world.h"
#include "core/world_serialisation.h"
#include "core/track_ops.h"
#include "core/param.h"

struct test_fixture_track_1 {
	world_test w;
	track_seg track1, track2;
	error_collection ec;
	test_fixture_track_1() : track1(w), track2(w) {
		track1.SetLength(50000);
		track1.SetElevationDelta(-100);
		track1.SetName("T1");
		track2.SetLength(100000);
		track2.SetElevationDelta(+50);
		track2.SetName("T2");
		track1.FullConnect(EDGE::BACK, track_target_ptr(&track2, EDGE::FRONT), ec);
	}
};

struct test_fixture_track_2 {
	world_test w;
	track_seg track1, track2, track3;
	points pt1;
	error_collection ec;
	test_fixture_track_2() : track1(w), track2(w), track3(w), pt1(w) {
		track1.SetLength(50000);
		track1.SetElevationDelta(-100);
		track1.SetName("T1");
		track2.SetLength(100000);
		track2.SetElevationDelta(+50);
		track2.SetName("T2");
		track3.SetLength(200000);
		track3.SetElevationDelta(-50);
		track3.SetName("T3");
		track1.FullConnect(EDGE::BACK, track_target_ptr(&pt1, EDGE::PTS_FACE), ec);
		track2.FullConnect(EDGE::FRONT, track_target_ptr(&pt1, EDGE::PTS_NORMAL), ec);
		track3.FullConnect(EDGE::FRONT, track_target_ptr(&pt1, EDGE::PTS_REVERSE), ec);
		pt1.SetName("P1");
	}
};

struct test_fixture_track_3 {
	world_test w;
	track_seg track1, track2, track3, track4;
	catchpoints cp1;
	spring_points sp1;
	error_collection ec;
	test_fixture_track_3() : track1(w), track2(w), track3(w), track4(w), cp1(w), sp1(w) {
		track1.FullConnect(EDGE::BACK, track_target_ptr(&sp1, EDGE::PTS_FACE), ec);
		track2.FullConnect(EDGE::FRONT, track_target_ptr(&sp1, EDGE::PTS_NORMAL), ec);
		track3.FullConnect(EDGE::FRONT, track_target_ptr(&sp1, EDGE::PTS_REVERSE), ec);
		cp1.FullConnect(EDGE::FRONT, track_target_ptr(&track2, EDGE::BACK), ec);
		track4.FullConnect(EDGE::FRONT, track_target_ptr(&cp1, EDGE::BACK), ec);
	}
};

TEST_CASE( "track/conn/track", "Test basic track segment connectivity" ) {
	test_fixture_track_1 env;

	REQUIRE(env.track1.GetConnectingPiece(EDGE::FRONT) == track_target_ptr(&env.track2, EDGE::FRONT));
	REQUIRE(env.track2.GetConnectingPiece(EDGE::BACK) == track_target_ptr(&env.track1, EDGE::BACK));
	REQUIRE(env.track1.GetConnectingPiece(EDGE::BACK) == track_target_ptr());
	REQUIRE(env.track1.GetConnectingPiece(EDGE::FRONT).GetConnectingPiece() == track_target_ptr());
}

TEST_CASE( "track/conn/points", "Test basic points connectivity" ) {
	test_fixture_track_2 env;

	REQUIRE(env.pt1.GetPointsFlags(0) == generic_points::PTF::ZERO);
	REQUIRE(env.track1.GetConnectingPiece(EDGE::FRONT) == track_target_ptr(&env.pt1, EDGE::PTS_FACE));
	REQUIRE(env.track2.GetConnectingPiece(EDGE::BACK) == track_target_ptr(&env.pt1, EDGE::PTS_NORMAL));
	REQUIRE(env.track3.GetConnectingPiece(EDGE::BACK) == track_target_ptr(&env.pt1, EDGE::PTS_REVERSE));
	//normal
	REQUIRE(env.track1.GetConnectingPiece(EDGE::FRONT).GetConnectingPiece() == track_target_ptr(&env.track2, EDGE::FRONT));
	REQUIRE(env.track2.GetConnectingPiece(EDGE::BACK).GetConnectingPiece() == track_target_ptr(&env.track1, EDGE::BACK));
	REQUIRE(env.track3.GetConnectingPiece(EDGE::BACK).GetConnectingPiece() == track_target_ptr());
	//reverse
	env.pt1.SetPointsFlagsMasked(0, points::PTF::REV, points::PTF::REV);
	REQUIRE(env.track1.GetConnectingPiece(EDGE::FRONT).GetConnectingPiece() == track_target_ptr(&env.track3, EDGE::FRONT));
	REQUIRE(env.track3.GetConnectingPiece(EDGE::BACK).GetConnectingPiece() == track_target_ptr(&env.track1, EDGE::BACK));
	REQUIRE(env.track2.GetConnectingPiece(EDGE::BACK).GetConnectingPiece() == track_target_ptr());
	//OOC
	env.pt1.SetPointsFlagsMasked(0, points::PTF::OOC, points::PTF::OOC);
	REQUIRE(env.track1.GetConnectingPiece(EDGE::FRONT).GetConnectingPiece() == track_target_ptr());
	REQUIRE(env.track2.GetConnectingPiece(EDGE::BACK).GetConnectingPiece() == track_target_ptr());
	REQUIRE(env.track3.GetConnectingPiece(EDGE::BACK).GetConnectingPiece() == track_target_ptr());
}

TEST_CASE( "track/conn/catchpoints", "Test basic catchpoints connectivity" ) {
	test_fixture_track_3 env;

	REQUIRE(env.cp1.GetPointsFlags(0) == catchpoints::PTF::AUTO_NORMALISE);
	REQUIRE(env.track2.GetConnectingPiece(EDGE::FRONT) == track_target_ptr(&env.cp1, EDGE::FRONT));
	REQUIRE(env.track4.GetConnectingPiece(EDGE::BACK) == track_target_ptr(&env.cp1, EDGE::BACK));
	//reverse
	REQUIRE(env.track2.GetConnectingPiece(EDGE::FRONT).GetConnectingPiece() == track_target_ptr());
	REQUIRE(env.track4.GetConnectingPiece(EDGE::BACK).GetConnectingPiece() == track_target_ptr());
	//normal
	env.cp1.SetPointsFlagsMasked(0, generic_points::PTF::REV, points::PTF::REV);
	REQUIRE(env.track2.GetConnectingPiece(EDGE::FRONT).GetConnectingPiece() == track_target_ptr(&env.track4, EDGE::FRONT));
	REQUIRE(env.track4.GetConnectingPiece(EDGE::BACK).GetConnectingPiece() == track_target_ptr(&env.track2, EDGE::BACK));
	//OOC
	env.cp1.SetPointsFlagsMasked(0, points::PTF::OOC, points::PTF::OOC);
	REQUIRE(env.track2.GetConnectingPiece(EDGE::FRONT).GetConnectingPiece() == track_target_ptr());
	REQUIRE(env.track4.GetConnectingPiece(EDGE::BACK).GetConnectingPiece() == track_target_ptr());
}

TEST_CASE( "track/conn/spring_points", "Test basic spring_points connectivity" ) {
	test_fixture_track_3 env;

	REQUIRE(env.sp1.GetSendReverseFlag() == false);
	REQUIRE(env.track1.GetConnectingPiece(EDGE::FRONT) == track_target_ptr(&env.sp1, EDGE::PTS_FACE));
	REQUIRE(env.track2.GetConnectingPiece(EDGE::BACK) == track_target_ptr(&env.sp1, EDGE::PTS_NORMAL));
	REQUIRE(env.track3.GetConnectingPiece(EDGE::BACK) == track_target_ptr(&env.sp1, EDGE::PTS_REVERSE));
	//normal
	REQUIRE(env.track1.GetConnectingPiece(EDGE::FRONT).GetConnectingPiece() == track_target_ptr(&env.track2, EDGE::FRONT));
	REQUIRE(env.track2.GetConnectingPiece(EDGE::BACK).GetConnectingPiece() == track_target_ptr(&env.track1, EDGE::BACK));
	REQUIRE(env.track3.GetConnectingPiece(EDGE::BACK).GetConnectingPiece() == track_target_ptr(&env.track1, EDGE::BACK));
	//reverse
	env.sp1.SetSendReverseFlag(true);
	REQUIRE(env.track1.GetConnectingPiece(EDGE::FRONT).GetConnectingPiece() == track_target_ptr(&env.track3, EDGE::FRONT));
	REQUIRE(env.track2.GetConnectingPiece(EDGE::BACK).GetConnectingPiece() == track_target_ptr(&env.track1, EDGE::BACK));
	REQUIRE(env.track3.GetConnectingPiece(EDGE::BACK).GetConnectingPiece() == track_target_ptr(&env.track1, EDGE::BACK));
}

TEST_CASE( "track/traverse/track", "Test basic track segment traversal" ) {
	test_fixture_track_1 env;

	unsigned int leftover;
	track_location loc;
	std::function<void(track_location &, track_location &)> stub;

	loc.SetTargetStartLocation(&env.track1, EDGE::FRONT);

	leftover = AdvanceDisplacement(20000, loc, 0, stub);
	REQUIRE(leftover == 0);
	REQUIRE(loc == track_location(&env.track1, EDGE::FRONT, 20000));

	leftover = AdvanceDisplacement(40000, loc, 0, stub);
	REQUIRE(leftover == 0);
	REQUIRE(loc == track_location(&env.track2, EDGE::FRONT, 10000));

	leftover = AdvanceDisplacement(100000, loc, 0, stub);
	REQUIRE(leftover == 10000);
	REQUIRE(loc == track_location(&env.track2, EDGE::FRONT, 100000));
}

TEST_CASE( "track/traverse/points", "Test basic points traversal" ) {
	test_fixture_track_2 env;

	unsigned int leftover;
	track_location loc;
	std::function<void(track_location &, track_location &)> stub;

	loc.SetTargetStartLocation(&env.track1, EDGE::FRONT);

	//normal
	loc.SetTargetStartLocation(&env.track1, EDGE::FRONT);
	leftover = AdvanceDisplacement(60000, loc, 0, stub);
	REQUIRE(leftover == 0);
	REQUIRE(loc == track_location(&env.track2, EDGE::FRONT, 10000));

	loc.SetTargetStartLocation(&env.track2, EDGE::BACK);
	leftover = AdvanceDisplacement(110000, loc, 0, stub);
	REQUIRE(leftover == 0);
	REQUIRE(loc == track_location(&env.track1, EDGE::BACK, 40000));

	loc.SetTargetStartLocation(&env.track3, EDGE::BACK);
	leftover = AdvanceDisplacement(210000, loc, 0, stub);
	REQUIRE(leftover == 10000);
	REQUIRE(loc == track_location(&env.pt1, EDGE::PTS_REVERSE, 0));

	//reverse
	env.pt1.SetPointsFlagsMasked(0, points::PTF::REV, points::PTF::REV);

	loc.SetTargetStartLocation(&env.track1, EDGE::FRONT);
	leftover = AdvanceDisplacement(60000, loc, 0, stub);
	REQUIRE(leftover == 0);
	REQUIRE(loc == track_location(&env.track3, EDGE::FRONT, 10000));

	loc.SetTargetStartLocation(&env.track3, EDGE::BACK);
	leftover = AdvanceDisplacement(210000, loc, 0, stub);
	REQUIRE(leftover == 0);
	REQUIRE(loc == track_location(&env.track1, EDGE::BACK, 40000));

	loc.SetTargetStartLocation(&env.track2, EDGE::BACK);
	leftover = AdvanceDisplacement(110000, loc, 0, stub);
	REQUIRE(leftover == 10000);
	REQUIRE(loc == track_location(&env.pt1, EDGE::PTS_NORMAL, 0));

	//OOC
	env.pt1.SetPointsFlagsMasked(0, points::PTF::OOC, points::PTF::OOC);
	loc.SetTargetStartLocation(&env.track1, EDGE::FRONT);
	leftover = AdvanceDisplacement(60000, loc, 0, stub);
	REQUIRE(leftover == 10000);
	REQUIRE(loc == track_location(&env.pt1, EDGE::PTS_FACE, 0));

	loc.SetTargetStartLocation(&env.track2, EDGE::BACK);
	leftover = AdvanceDisplacement(110000, loc, 0, stub);
	REQUIRE(leftover == 10000);
	REQUIRE(loc == track_location(&env.pt1, EDGE::PTS_NORMAL, 0));

	loc.SetTargetStartLocation(&env.track3, EDGE::BACK);
	leftover = AdvanceDisplacement(210000, loc, 0, stub);
	REQUIRE(leftover == 10000);
	REQUIRE(loc == track_location(&env.pt1, EDGE::PTS_REVERSE, 0));
}

TEST_CASE( "track/deserialisation/track", "Test basic track segment deserialisation" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{ \"type\" : \"track_seg\", \"name\" : \"T1\", \"length\" : 50000, \"elevation_delta\" : -1000, \"train_count\" : 1, "
			"\"speed_limits\" : [ { \"speed_class\" : \"foo\", \"speed\" : 27778 } ] }"
	"] }";
	test_fixture_world env(track_test_str);

	if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	track_seg *t = dynamic_cast<track_seg *>(env.w->FindTrackByName("T1"));
	REQUIRE(t != nullptr);
	REQUIRE(t->GetLength(EDGE::FRONT) == 50000);
	REQUIRE(t->GetElevationDelta(EDGE::FRONT) == -1000);

	const speed_restriction_set *sr = t->GetSpeedRestrictions();
	REQUIRE(sr != nullptr);
	REQUIRE(sr->GetTrackSpeedLimitByClass("foo", UINT_MAX) == 27778);
	REQUIRE(sr->GetTrackSpeedLimitByClass("bar", UINT_MAX) == UINT_MAX);
}

TEST_CASE( "track/deserialisation/points", "Test basic points deserialisation" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{ \"type\" : \"points\", \"name\" : \"P1\", \"reverse\" : true, \"failed_norm\" : false, \"reminder\" : true}"
	"] }";
	test_fixture_world env(track_test_str);

	if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	points *p = dynamic_cast<points *>(env.w->FindTrackByName("P1"));
	REQUIRE(p != nullptr);
	REQUIRE(p->GetPointsFlags(0) == (points::PTF::REV | points::PTF::REMINDER));
}

TEST_CASE( "track/deserialisation/autoname", "Test deserialisation automatic naming" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{ \"type\" : \"points\"}, "
		"{ \"type\" : \"track_seg\"}"
	"] }";
	test_fixture_world env(track_test_str);

	if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	track_seg *p = dynamic_cast<track_seg *>(env.w->FindTrackByName("#1"));
	REQUIRE(p != nullptr);
}

std::string track_test_str_ds =
R"({ "content" : [ )"
	R"({ "type" : "double_slip", "name" : "DS1" }, )"
	R"({ "type" : "double_slip", "name" : "DS2", "degrees_of_freedom" : 1 }, )"
	R"({ "type" : "double_slip", "name" : "DS3", "degrees_of_freedom" : 2 }, )"
	R"({ "type" : "double_slip", "name" : "DS4", "degrees_of_freedom" : 4 }, )"
	R"({ "type" : "double_slip", "name" : "DS5", "no_track_fl_bl" : true }, )"
	R"({ "type" : "double_slip", "name" : "DS6", "no_track_fl_br" : true, "right_front_points" : { "reverse" : true } }, )"
	R"({ "type" : "double_slip", "name" : "DS7", "no_track_fr_br" : true, "degrees_of_freedom" : 1, "left_front_points" : { "reverse" : true } }, )"
	R"({ "type" : "double_slip", "name" : "DS8", "no_track_fr_bl" : true, "degrees_of_freedom" : 4 }, )"
	R"({ "type" : "double_slip", "name" : "DS9", "degrees_of_freedom" : 1, "left_back_points" : { "locked" : true, "reverse" : true } }, )"
	R"({ "type" : "double_slip", "name" : "DS10", "degrees_of_freedom" : 2, "left_back_points" : { "failed_rev" : true, "reverse" : true } }, )"
	R"({ "type" : "end_of_line", "name" : "FL" }, )"
	R"({ "type" : "end_of_line", "name" : "FR" }, )"
	R"({ "type" : "end_of_line", "name" : "BL" }, )"
	R"({ "type" : "end_of_line", "name" : "BR" } )"
"] }";

TEST_CASE( "track/deserialisation/double_slip", "Test basic double_slip deserialisation" ) {
	test_fixture_world env(track_test_str_ds);

	if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	auto checkds = [&](const char *name, generic_points::PTF pffl, generic_points::PTF pffr, generic_points::PTF pfbl, generic_points::PTF pfbr) {
		INFO("Double-slip check for: " << name);

		double_slip *ds = dynamic_cast<double_slip *>(env.w->FindTrackByName(name));
		REQUIRE(ds != nullptr);
		REQUIRE(ds->GetCurrentPointFlags(EDGE::DS_FL) == pffl);
		REQUIRE(ds->GetCurrentPointFlags(EDGE::DS_FR) == pffr);
		REQUIRE(ds->GetCurrentPointFlags(EDGE::DS_BL) == pfbl);
		REQUIRE(ds->GetCurrentPointFlags(EDGE::DS_BR) == pfbr);
	};

	checkds("DS1", generic_points::PTF::ZERO, points::PTF::REV, points::PTF::REV, generic_points::PTF::ZERO);
	checkds("DS2", generic_points::PTF::ZERO, generic_points::PTF::ZERO, generic_points::PTF::ZERO, generic_points::PTF::ZERO);
	checkds("DS3", generic_points::PTF::ZERO, points::PTF::REV, points::PTF::REV, generic_points::PTF::ZERO);
	checkds("DS4", generic_points::PTF::ZERO, generic_points::PTF::ZERO, generic_points::PTF::ZERO, generic_points::PTF::ZERO);
	checkds("DS5", points::PTF::FIXED, generic_points::PTF::ZERO, points::PTF::FIXED, generic_points::PTF::ZERO);
	checkds("DS6", points::PTF::FIXED | points::PTF::REV, points::PTF::REV, generic_points::PTF::ZERO, points::PTF::FIXED | points::PTF::REV);
	checkds("DS7", points::PTF::REV, points::PTF::FIXED, points::PTF::REV, points::PTF::FIXED);
	checkds("DS8", generic_points::PTF::ZERO, points::PTF::FIXED | points::PTF::REV, points::PTF::FIXED | points::PTF::REV, generic_points::PTF::ZERO);
	checkds("DS9", points::PTF::LOCKED | points::PTF::REV, points::PTF::LOCKED | points::PTF::REV, points::PTF::LOCKED | points::PTF::REV, points::PTF::LOCKED | points::PTF::REV);
	checkds("DS10", points::PTF::ZERO, points::PTF::REV, points::PTF::FAILED_REV | points::PTF::REV, points::PTF::FAILED_NORM);
}

class test_double_slip {
	public:
	static inline bool HalfConnect(double_slip *ds, EDGE this_entrance_direction, const track_target_ptr &target_entrance) {
		return ds->HalfConnect(this_entrance_direction, target_entrance);
	}
	static inline unsigned int GetCurrentPointIndex(double_slip *ds, EDGE direction) {
		return ds->GetPointsIndexByEdge(direction);
	}
};

TEST_CASE( "track/conn/double_slip", "Test basic double_slip connectivity" ) {
	test_fixture_world env(track_test_str_ds);

	if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	generic_track *fl = env.w->FindTrackByName("FL");
	generic_track *fr = env.w->FindTrackByName("FR");
	generic_track *bl = env.w->FindTrackByName("BL");
	generic_track *br = env.w->FindTrackByName("BR");

	auto checkdstraverse = [&](const char *name, generic_track *flt, generic_track *frt, generic_track *blt, generic_track *brt) {
		INFO("Double-slip check for: " << name);

		double_slip *ds = dynamic_cast<double_slip *>(env.w->FindTrackByName(name));
		REQUIRE(ds != nullptr);

		REQUIRE(test_double_slip::HalfConnect(ds, EDGE::DS_FL, track_target_ptr(fl, EDGE::FRONT)) == true);
		REQUIRE(test_double_slip::HalfConnect(ds, EDGE::DS_FR, track_target_ptr(fr, EDGE::FRONT)) == true);
		REQUIRE(test_double_slip::HalfConnect(ds, EDGE::DS_BL, track_target_ptr(bl, EDGE::FRONT)) == true);
		REQUIRE(test_double_slip::HalfConnect(ds, EDGE::DS_BR, track_target_ptr(br, EDGE::FRONT)) == true);
		REQUIRE(ds->GetConnectingPiece(EDGE::DS_FL).track == flt);
		REQUIRE(ds->GetConnectingPiece(EDGE::DS_FR).track == frt);
		REQUIRE(ds->GetConnectingPiece(EDGE::DS_BL).track == blt);
		REQUIRE(ds->GetConnectingPiece(EDGE::DS_BR).track == brt);
	};

	auto dsmovepoints = [&](const char *name, EDGE direction, bool rev) {
		INFO("Double-slip check for: " << name);

		double_slip *ds = dynamic_cast<double_slip *>(env.w->FindTrackByName(name));
		REQUIRE(ds != nullptr);

		ds->SetPointsFlagsMasked(test_double_slip::GetCurrentPointIndex(ds, direction), rev ? generic_points::PTF::REV : generic_points::PTF::ZERO, generic_points::PTF::REV);
	};

	checkdstraverse("DS1", br, 0, 0, fl);
	checkdstraverse("DS2", br, bl, fr, fl);
	dsmovepoints("DS2", EDGE::DS_FL, true);
	checkdstraverse("DS2", bl, br, fl, fr);
	checkdstraverse("DS3", br, 0, 0, fl);
	dsmovepoints("DS3", EDGE::DS_FL, true);
	checkdstraverse("DS3", bl, 0, fl, 0);
	checkdstraverse("DS4", br, bl, fr, fl);
	dsmovepoints("DS4", EDGE::DS_FL, true);
	checkdstraverse("DS4", 0, bl, fr, 0);
	checkdstraverse("DS5", br, bl, fr, fl);
	checkdstraverse("DS6", 0, br, 0, fr);
	checkdstraverse("DS7", bl, 0, fl, 0);
	dsmovepoints("DS7", EDGE::DS_FL, false);
	checkdstraverse("DS7", br, bl, fr, fl);
	checkdstraverse("DS8", br, 0, 0, fl);
	checkdstraverse("DS9", bl, br, fl, fr);
}

TEST_CASE( "track/deserialisation/partialconnection", "Test partial track connection declaration deserialisation" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "start_of_line", "name" : "A"}, )"
		R"({ "type" : "points", "name" : "P1", "connect" : { "to" : "T2" } }, )"
		R"({ "type" : "points", "name" : "P2" }, )"
		R"({ "type" : "track_seg", "name" : "T1" }, )"
		R"({ "type" : "end_of_line", "name" : "B" }, )"
		R"({ "type" : "track_seg", "name" : "T2" }, )"
		R"({ "type" : "end_of_line", "name" : "C" }, )"
		R"({ "type" : "track_seg", "name" : "T3", "connect" : { "to" : "P2" } }, )"
		R"({ "type" : "end_of_line", "name" : "D" } )"
	"] }";
	test_fixture_world env(track_test_str);

	env.w->LayoutInit(env.ec);

	if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	points *p1 = dynamic_cast<points *>(env.w->FindTrackByName("P1"));
	REQUIRE(p1 != nullptr);
	points *p2 = dynamic_cast<points *>(env.w->FindTrackByName("P2"));
	REQUIRE(p2 != nullptr);
	track_seg *t2 = dynamic_cast<track_seg *>(env.w->FindTrackByName("T2"));
	REQUIRE(t2 != nullptr);
	track_seg *t3 = dynamic_cast<track_seg *>(env.w->FindTrackByName("T3"));
	REQUIRE(t3 != nullptr);

	REQUIRE(p1->GetEdgeConnectingPiece(EDGE::PTS_REVERSE) == track_target_ptr(t2, EDGE::FRONT));
	REQUIRE(p2->GetEdgeConnectingPiece(EDGE::PTS_REVERSE) == track_target_ptr(t3, EDGE::FRONT));
}

TEST_CASE( "track/deserialisation/ambiguouspartialconnection/1", "Test handling of ambiguous partial track connection declaration deserialisation" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "start_of_line", "name" : "A"}, )"
		R"({ "type" : "points", "name" : "P1", "connect" : { "to" : "T2" } }, )"
		R"({ "type" : "end_of_line", "name" : "B" }, )"
		R"({ "type" : "track_seg", "name" : "T2" } )"
	"] }";
	test_fixture_world env(track_test_str);

	env.w->LayoutInit(env.ec);

	//if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() >= 1);
}

TEST_CASE( "track/deserialisation/ambiguouspartialconnection/2", "Test handling of ambiguous partial track connection declaration deserialisation" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "start_of_line", "name" : "A"}, )"
		R"({ "type" : "points", "name" : "P1" }, )"
		R"({ "type" : "end_of_line", "name" : "B" }, )"
		R"({ "type" : "track_seg", "name" : "T2" , "connect" : { "to" : "P1" } } )"
	"] }";
	test_fixture_world env(track_test_str);

	env.w->LayoutInit(env.ec);

	//if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() >= 1);
}

TEST_CASE( "track/deserialisation/ambiguouspartialconnection/3", "Test handling of ambiguous partial track connection declaration deserialisation" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "start_of_line", "name" : "A"}, )"
		R"({ "type" : "track_seg", "name" : "T1" }, )"
		R"({ "type" : "end_of_line", "name" : "B" }, )"
		R"({ "type" : "track_seg", "name" : "T2" , "connect" : { "to" : "T1" } } )"
	"] }";
	test_fixture_world env(track_test_str);

	env.w->LayoutInit(env.ec);

	//if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() >= 1);
}

TEST_CASE( "track/points/coupling", "Test points coupling" ) {
	std::string track_test_str_coupling =
	R"({ "content" : [ )"
		R"({ "type" : "couple_points", "points" : [ { "name" : "P1", "edge" : "reverse"}, { "name" : "P2", "edge" : "normal"}, { "name" : "DS1", "edge" : "right_front"} ] }, )"
		R"({ "type" : "couple_points", "points" : [ { "name" : "DS2", "edge" : "right_front"}, { "name" : "DS1", "edge" : "right_back"} ] }, )"
		R"({ "type" : "double_slip", "name" : "DS1", "degrees_of_freedom" : 2 }, )"
		R"({ "type" : "double_slip", "name" : "DS2", "no_track_fl_bl" : true, "right_front_points" : { "reverse" : true } }, )"
		R"({ "type" : "points", "name" : "P1" }, )"
		R"({ "type" : "points", "name" : "P2" } )"
	"] }";

	test_fixture_world env(track_test_str_coupling);

	env.w->CapAllTrackPieceUnconnectedEdges();
	env.w->LayoutInit(env.ec);

	if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	auto checkds = [&](const char *name, generic_points::PTF pffl, generic_points::PTF pffr, generic_points::PTF pfbl, generic_points::PTF pfbr) {
		INFO("Double-slip check for: " << name << ", at time: " << env.w->GetGameTime());

		double_slip *ds = dynamic_cast<double_slip *>(env.w->FindTrackByName(name));
		REQUIRE(ds != nullptr);
		CHECK(ds->GetCurrentPointFlags(EDGE::DS_FL) == pffl);
		CHECK(ds->GetCurrentPointFlags(EDGE::DS_FR) == pffr);
		CHECK(ds->GetCurrentPointFlags(EDGE::DS_BL) == pfbl);
		CHECK(ds->GetCurrentPointFlags(EDGE::DS_BR) == pfbr);
	};
	auto checkp = [&](const char *name, generic_points::PTF pflags) {
		INFO("Points check for: " << name << ", at time: " << env.w->GetGameTime());

		points *p = dynamic_cast<points *>(env.w->FindTrackByName(name));
		REQUIRE(p != nullptr);
		CHECK(p->GetPointsFlags(0) == (pflags | generic_points::PTF::COUPLED));
	};
	auto checkds1 = [&](generic_points::PTF pffl, generic_points::PTF pffr, generic_points::PTF pfbl, generic_points::PTF pfbr)  {
		return checkds("DS1", pffl | generic_points::PTF::COUPLED, pffr | generic_points::PTF::COUPLED, pfbl | generic_points::PTF::COUPLED, pfbr | generic_points::PTF::COUPLED);
	};
	auto checkds2 = [&](generic_points::PTF pffl, generic_points::PTF pffr, generic_points::PTF pfbl, generic_points::PTF pfbr)  {
		return checkds("DS2", pffl, pffr, pfbl, pfbr | generic_points::PTF::COUPLED);
	};

	checkp("P1", generic_points::PTF::ZERO);
	checkp("P2", generic_points::PTF::REV);
	checkds1(generic_points::PTF::REV, generic_points::PTF::ZERO, generic_points::PTF::REV, generic_points::PTF::ZERO);
	checkds2(generic_points::PTF::FIXED, generic_points::PTF::REV, generic_points::PTF::FIXED, generic_points::PTF::ZERO);

	points *p1 = dynamic_cast<points *>(env.w->FindTrackByName("P1"));
	points *p2 = dynamic_cast<points *>(env.w->FindTrackByName("P2"));

	env.w->SubmitAction(action_points_action(*(env.w), *p1, 0, points::PTF::REV, points::PTF::REV));
	env.w->GameStep(1);

	p1->SetPointsFlagsMasked(0, points::PTF::FAILED_NORM, points::PTF::FAILED_NORM);

	checkp("P1", generic_points::PTF::REV | generic_points::PTF::OOC | generic_points::PTF::FAILED_NORM);
	checkp("P2",  generic_points::PTF::OOC | generic_points::PTF::FAILED_REV);
	checkds1(generic_points::PTF::REV, generic_points::PTF::ZERO, generic_points::PTF::OOC | generic_points::PTF::FAILED_REV, generic_points::PTF::REV | generic_points::PTF::OOC | generic_points::PTF::FAILED_NORM);
	checkds2(generic_points::PTF::FIXED, generic_points::PTF::REV, generic_points::PTF::FIXED, generic_points::PTF::ZERO);

	env.w->GameStep(4000);

	CHECK(p1->HaveFutures() == true);
	env.w->SubmitAction(action_points_action(*(env.w), *p2, 0, points::PTF::REV, points::PTF::REV));
	env.w->GameStep(1);
	CHECK(p1->HaveFutures() == false);

	env.w->GameStep(4000);

	checkp("P1", generic_points::PTF::OOC | generic_points::PTF::FAILED_NORM);
	checkp("P2",  generic_points::PTF::REV | generic_points::PTF::OOC | generic_points::PTF::FAILED_REV);
	checkds1(generic_points::PTF::REV, generic_points::PTF::ZERO, generic_points::PTF::REV | generic_points::PTF::OOC | generic_points::PTF::FAILED_REV, generic_points::PTF::OOC | generic_points::PTF::FAILED_NORM);
	checkds2(generic_points::PTF::FIXED, generic_points::PTF::REV, generic_points::PTF::FIXED, generic_points::PTF::ZERO);

	env.w->GameStep(2000);

	checkp("P1", generic_points::PTF::FAILED_NORM);
	checkp("P2",  generic_points::PTF::REV | generic_points::PTF::FAILED_REV);
	checkds1(generic_points::PTF::REV, generic_points::PTF::ZERO, generic_points::PTF::REV | generic_points::PTF::FAILED_REV, generic_points::PTF::FAILED_NORM);
	checkds2(generic_points::PTF::FIXED, generic_points::PTF::REV, generic_points::PTF::FIXED, generic_points::PTF::ZERO);

	if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);
}

TEST_CASE( "track/points/coupling/duplicate", "Test points coupling duplicate error detection" ) {
	std::string track_test_str_coupling =
	R"({ "content" : [ )"
		R"({ "type" : "couple_points", "points" : [ { "name" : "P1", "edge" : "reverse"}, { "name" : "P2", "edge" : "normal"} ] }, )"
		R"({ "type" : "couple_points", "points" : [ { "name" : "P1", "edge" : "reverse"}, { "name" : "P3", "edge" : "normal"} ] }, )"
		R"({ "type" : "points", "name" : "P1" }, )"
		R"({ "type" : "points", "name" : "P2" }, )"
		R"({ "type" : "points", "name" : "P3" } )"
	"] }";

	test_fixture_world env(track_test_str_coupling);

	env.w->CapAllTrackPieceUnconnectedEdges();
	env.w->LayoutInit(env.ec);
	CHECK(env.ec.GetErrorCount() == 1);
	CHECK_CONTAINS(env.ec, "points cannot be coupled: P1");
}

TEST_CASE( "track/points/coupling/auto-normalise", "Test points coupling auto-normalisation error detection" ) {
	std::string track_test_str_coupling =
	R"({ "content" : [ )"
		R"({ "type" : "couple_points", "points" : [ { "name" : "P1", "edge" : "normal"}, { "name" : "P2", "edge" : "normal"} ] }, )"
		R"({ "type" : "points", "name" : "P1", "auto_normalise" : true }, )"
		R"({ "type" : "points", "name" : "P2" } )"
	"] }";

	test_fixture_world env(track_test_str_coupling);

	env.w->CapAllTrackPieceUnconnectedEdges();
	env.w->LayoutInit(env.ec);
	CHECK(env.ec.GetErrorCount() == 0);
	env.w->PostLayoutInit(env.ec);
	CHECK(env.ec.GetErrorCount() == 1);
	CHECK_CONTAINS(env.ec, "Coupled points cannot be auto-normalised: P1");
}

TEST_CASE( "track/points/coupling/double-slip/auto-normalise", "Test double-slip self-coupling auto-normalisation error detection" ) {
	std::string track_test_str_coupling =
	R"({ "content" : [ )"
		R"({ "type" : "double_slip", "name" : "DS1", "right_front_points" : { "auto_normalise" : true } } )"
	"] }";

	test_fixture_world env(track_test_str_coupling);

	env.w->CapAllTrackPieceUnconnectedEdges();
	env.w->LayoutInit(env.ec);
	CHECK(env.ec.GetErrorCount() == 1);
	CHECK_CONTAINS(env.ec, "Self-coupled double-slips cannot have auto-normalise set");
}

TEST_CASE( "track/deserialisation/sighting", "Test sighting distance deserialisation" ) {
	std::string track_test_str_sighting =
	R"({ "content" : [ )"
		R"({ "type" : "points", "name" : "P1", "sighting" : 40000 }, )"
		R"({ "type" : "points", "name" : "P2", "sighting" : { "direction" : "normal", "distance" : 40000 } }, )"
		R"({ "type" : "points", "name" : "P3", "sighting" : [ { "direction" : "normal", "distance" : 40000 }, { "direction" : "reverse", "distance" : 30000 } ] }, )"
		R"({ "type" : "route_signal", "name" : "S1", "sighting" : 50000 }, )"
		R"({ "type" : "route_signal", "name" : "S2", "sighting" : { "direction" : "front", "distance" : 50000 } }, )"
		R"({ "type" : "route_signal", "name" : "S3" } )"
	"] }";

	test_fixture_world env(track_test_str_sighting);

	if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	auto checkpoints = [&](const std::string &name, unsigned int norm_dist, unsigned int rev_dist, unsigned int face_dist) {
		INFO("Points check for: " << name);
		generic_track *p = env.w->FindTrackByName(name);
		REQUIRE(p != nullptr);
		CHECK(p->GetSightingDistance(EDGE::PTS_NORMAL) == norm_dist);
		CHECK(p->GetSightingDistance(EDGE::PTS_REVERSE) == rev_dist);
		CHECK(p->GetSightingDistance(EDGE::PTS_FACE) == face_dist);
		CHECK(p->GetSightingDistance(EDGE::FRONT) == 0);
	};
	auto checksig = [&](const std::string &name, unsigned int dist) {
		INFO("Signal check for: " << name);
		generic_track *s = env.w->FindTrackByName(name);
		REQUIRE(s != nullptr);
		CHECK(s->GetSightingDistance(EDGE::FRONT) == dist);
		CHECK(s->GetSightingDistance(EDGE::BACK) == 0);
	};
	checkpoints("P1", 40000, 40000, 40000);
	checkpoints("P2", 40000, SIGHTING_DISTANCE_POINTS, SIGHTING_DISTANCE_POINTS);
	checkpoints("P3", 40000, 30000, SIGHTING_DISTANCE_POINTS);
	checksig("S1", 50000);
	checksig("S2", 50000);
	checksig("S3", SIGHTING_DISTANCE_SIG);

	auto check_parse_err = [&](const std::string &str) {
		test_fixture_world env_err(str);
		//if (env_err.ec.GetErrorCount()) { WARN("Error Collection: " << env_err.ec); }
		REQUIRE(env_err.ec.GetErrorCount() == 1);
	};
	check_parse_err(R"({ "content" : [ )"
		R"({ "type" : "points", "name" : "P1", "sighting" : "front" } )"
	"] }");
	check_parse_err(R"({ "content" : [ )"
		R"({ "type" : "points", "name" : "P1", "sighting" : { "direction" : "reverse" } } )"
	"] }");
	check_parse_err(R"({ "content" : [ )"
		R"({ "type" : "points", "name" : "P1", "sighting" : { "distance" : 40000 } } )"
	"] }");
	check_parse_err(R"({ "content" : [ )"
		R"({ "type" : "points", "name" : "P1", "sighting" : { "direction" : "front", "distance" : 40000 } } )"
	"] }");
}

TEST_CASE( "track/deserialisation/gamestate/basicload", "Test basic gamestate loading, including track creation check" ) {
	test_fixture_world env1(R"({ "game_state" : [ )"
		R"({ "type" : "points", "name" : "P1", "reverse" : true } )"
	"] }");
	INFO("Error Collection: " << env1.ec);
	CHECK(env1.ec.GetErrorCount() == 0);
	env1.ws->DeserialiseGameState(env1.ec);
	INFO("Error Collection: " << env1.ec);
	CHECK(env1.ec.GetErrorCount() == 1);

	test_fixture_world env2(R"({ "game_state" : [ )"
		R"({ "type" : "points", "name" : "P1", "reverse" : true } )"
	"],"
	R"("content" : [ )"
		R"({ "type" : "points", "name" : "P1" } )"
	"] }");
	INFO("Error Collection: " << env2.ec);
	CHECK(env2.ec.GetErrorCount() == 0);

	points *p1 = PTR_CHECK(dynamic_cast<points *>(env2.w->FindTrackByName("P1")));
	CHECK((p1->GetPointsFlags(0) & points::PTF::REV) == points::PTF::ZERO);

	env2.ws->DeserialiseGameState(env2.ec);
	INFO("Error Collection: " << env2.ec);
	CHECK(env2.ec.GetErrorCount() == 0);

	CHECK((p1->GetPointsFlags(0) & points::PTF::REV) == points::PTF::REV);
}

TEST_CASE( "track/points/updates", "Test basic points updates" ) {
	test_fixture_track_2 env;

	env.w.GameStep(1);
	CHECK(env.w.GetLastUpdateSet().size() == 0);

	env.w.SubmitAction(action_points_action(env.w, env.pt1, 0, generic_points::PTF::REV, generic_points::PTF::REV));

	CHECK(env.w.GetLastUpdateSet().size() == 0);
	env.w.GameStep(1);
	CHECK(env.w.GetLastUpdateSet().size() == 1);
	env.w.GameStep(10000);
	CHECK(env.w.GetLastUpdateSet().size() == 1);

	env.w.SubmitAction(action_points_action(env.w, env.pt1, 0, generic_points::PTF::REV, generic_points::PTF::REV));
	env.w.GameStep(1);
	CHECK(env.w.GetLastUpdateSet().size() == 0);
}
