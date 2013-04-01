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
#include "trackpiece.h"
#include "points.h"
#include "train.h"
#include "traverse.h"
#include "world.h"
#include "world_serialisation.h"
#include "deserialisation-test.h"
#include "world-test.h"


struct test_fixture_track_1 {
	world_test w;
	trackseg track1, track2;
	error_collection ec;
	test_fixture_track_1() : track1(w), track2(w) {
		track1.SetLength(50000);
		track1.SetElevationDelta(-100);
		track1.SetName("T1");
		track2.SetLength(100000);
		track2.SetElevationDelta(+50);
		track2.SetName("T2");
		track1.FullConnect(EDGE_BACK, track_target_ptr(&track2, EDGE_FRONT), ec);
	}
};

struct test_fixture_track_2 {
	world_test w;
	trackseg track1, track2, track3;
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
		track1.FullConnect(EDGE_BACK, track_target_ptr(&pt1, EDGE_PTS_FACE), ec);
		track2.FullConnect(EDGE_FRONT, track_target_ptr(&pt1, EDGE_PTS_NORMAL), ec);
		track3.FullConnect(EDGE_FRONT, track_target_ptr(&pt1, EDGE_PTS_REVERSE), ec);
		pt1.SetName("P1");
	}
};

struct test_fixture_track_3 {
	world_test w;
	trackseg track1, track2, track3, track4;
	catchpoints cp1;
	springpoints sp1;
	error_collection ec;
	test_fixture_track_3() : track1(w), track2(w), track3(w), track4(w), cp1(w), sp1(w) {
		track1.FullConnect(EDGE_BACK, track_target_ptr(&sp1, EDGE_PTS_FACE), ec);
		track2.FullConnect(EDGE_FRONT, track_target_ptr(&sp1, EDGE_PTS_NORMAL), ec);
		track3.FullConnect(EDGE_FRONT, track_target_ptr(&sp1, EDGE_PTS_REVERSE), ec);
		cp1.FullConnect(EDGE_FRONT, track_target_ptr(&track2, EDGE_BACK), ec);
		track4.FullConnect(EDGE_FRONT, track_target_ptr(&cp1, EDGE_BACK), ec);
	}
};

TEST_CASE( "track/conn/track", "Test basic track segment connectivity" ) {
	test_fixture_track_1 env;

	REQUIRE(env.track1.GetConnectingPiece(EDGE_FRONT) == track_target_ptr(&env.track2, EDGE_FRONT));
	REQUIRE(env.track2.GetConnectingPiece(EDGE_BACK) == track_target_ptr(&env.track1, EDGE_BACK));
	REQUIRE(env.track1.GetConnectingPiece(EDGE_BACK) == track_target_ptr());
	REQUIRE(env.track1.GetConnectingPiece(EDGE_FRONT).GetConnectingPiece() == track_target_ptr());
}

TEST_CASE( "track/conn/points", "Test basic points connectivity" ) {
	test_fixture_track_2 env;

	REQUIRE(env.pt1.GetPointsFlags(0) == genericpoints::PTF::ZERO);
	REQUIRE(env.track1.GetConnectingPiece(EDGE_FRONT) == track_target_ptr(&env.pt1, EDGE_PTS_FACE));
	REQUIRE(env.track2.GetConnectingPiece(EDGE_BACK) == track_target_ptr(&env.pt1, EDGE_PTS_NORMAL));
	REQUIRE(env.track3.GetConnectingPiece(EDGE_BACK) == track_target_ptr(&env.pt1, EDGE_PTS_REVERSE));
	//normal
	REQUIRE(env.track1.GetConnectingPiece(EDGE_FRONT).GetConnectingPiece() == track_target_ptr(&env.track2, EDGE_FRONT));
	REQUIRE(env.track2.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr(&env.track1, EDGE_BACK));
	REQUIRE(env.track3.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr());
	//reverse
	env.pt1.SetPointsFlagsMasked(0, points::PTF::REV, points::PTF::REV);
	REQUIRE(env.track1.GetConnectingPiece(EDGE_FRONT).GetConnectingPiece() == track_target_ptr(&env.track3, EDGE_FRONT));
	REQUIRE(env.track3.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr(&env.track1, EDGE_BACK));
	REQUIRE(env.track2.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr());
	//OOC
	env.pt1.SetPointsFlagsMasked(0, points::PTF::OOC, points::PTF::OOC);
	REQUIRE(env.track1.GetConnectingPiece(EDGE_FRONT).GetConnectingPiece() == track_target_ptr());
	REQUIRE(env.track2.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr());
	REQUIRE(env.track3.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr());
}

TEST_CASE( "track/conn/catchpoints", "Test basic catchpoints connectivity" ) {
	test_fixture_track_3 env;

	REQUIRE(env.cp1.GetPointsFlags(0) == catchpoints::PTF::REV);
	REQUIRE(env.track2.GetConnectingPiece(EDGE_FRONT) == track_target_ptr(&env.cp1, EDGE_FRONT));
	REQUIRE(env.track4.GetConnectingPiece(EDGE_BACK) == track_target_ptr(&env.cp1, EDGE_BACK));
	//reverse
	REQUIRE(env.track2.GetConnectingPiece(EDGE_FRONT).GetConnectingPiece() == track_target_ptr());
	REQUIRE(env.track4.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr());
	//normal
	env.cp1.SetPointsFlagsMasked(0, genericpoints::PTF::ZERO, points::PTF::REV);
	REQUIRE(env.track2.GetConnectingPiece(EDGE_FRONT).GetConnectingPiece() == track_target_ptr(&env.track4, EDGE_FRONT));
	REQUIRE(env.track4.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr(&env.track2, EDGE_BACK));
	//OOC
	env.cp1.SetPointsFlagsMasked(0, points::PTF::OOC, points::PTF::OOC);
	REQUIRE(env.track2.GetConnectingPiece(EDGE_FRONT).GetConnectingPiece() == track_target_ptr());
	REQUIRE(env.track4.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr());
}

TEST_CASE( "track/conn/springpoints", "Test basic springpoints connectivity" ) {
	test_fixture_track_3 env;

	REQUIRE(env.sp1.GetSendReverseFlag() == false);
	REQUIRE(env.track1.GetConnectingPiece(EDGE_FRONT) == track_target_ptr(&env.sp1, EDGE_PTS_FACE));
	REQUIRE(env.track2.GetConnectingPiece(EDGE_BACK) == track_target_ptr(&env.sp1, EDGE_PTS_NORMAL));
	REQUIRE(env.track3.GetConnectingPiece(EDGE_BACK) == track_target_ptr(&env.sp1, EDGE_PTS_REVERSE));
	//normal
	REQUIRE(env.track1.GetConnectingPiece(EDGE_FRONT).GetConnectingPiece() == track_target_ptr(&env.track2, EDGE_FRONT));
	REQUIRE(env.track2.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr(&env.track1, EDGE_BACK));
	REQUIRE(env.track3.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr(&env.track1, EDGE_BACK));
	//reverse
	env.sp1.SetSendReverseFlag(true);
	REQUIRE(env.track1.GetConnectingPiece(EDGE_FRONT).GetConnectingPiece() == track_target_ptr(&env.track3, EDGE_FRONT));
	REQUIRE(env.track2.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr(&env.track1, EDGE_BACK));
	REQUIRE(env.track3.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr(&env.track1, EDGE_BACK));
}

TEST_CASE( "track/traverse/track", "Test basic track segment traversal" ) {
	test_fixture_track_1 env;

	unsigned int leftover;
	track_location loc;
	std::function<void(track_location &, track_location &)> stub;

	loc.SetTargetStartLocation(&env.track1, EDGE_FRONT);

	leftover = AdvanceDisplacement(20000, loc, 0, stub);
	REQUIRE(leftover == 0);
	REQUIRE(loc == track_location(&env.track1, EDGE_FRONT, 20000));

	leftover = AdvanceDisplacement(40000, loc, 0, stub);
	REQUIRE(leftover == 0);
	REQUIRE(loc == track_location(&env.track2, EDGE_FRONT, 10000));

	leftover = AdvanceDisplacement(100000, loc, 0, stub);
	REQUIRE(leftover == 10000);
	REQUIRE(loc == track_location(&env.track2, EDGE_FRONT, 100000));
}

TEST_CASE( "track/traverse/points", "Test basic points traversal" ) {
	test_fixture_track_2 env;

	unsigned int leftover;
	track_location loc;
	std::function<void(track_location &, track_location &)> stub;

	loc.SetTargetStartLocation(&env.track1, EDGE_FRONT);

	//normal
	loc.SetTargetStartLocation(&env.track1, EDGE_FRONT);
	leftover = AdvanceDisplacement(60000, loc, 0, stub);
	REQUIRE(leftover == 0);
	REQUIRE(loc == track_location(&env.track2, EDGE_FRONT, 10000));

	loc.SetTargetStartLocation(&env.track2, EDGE_BACK);
	leftover = AdvanceDisplacement(110000, loc, 0, stub);
	REQUIRE(leftover == 0);
	REQUIRE(loc == track_location(&env.track1, EDGE_BACK, 40000));

	loc.SetTargetStartLocation(&env.track3, EDGE_BACK);
	leftover = AdvanceDisplacement(210000, loc, 0, stub);
	REQUIRE(leftover == 10000);
	REQUIRE(loc == track_location(&env.pt1, EDGE_PTS_REVERSE, 0));

	//reverse
	env.pt1.SetPointsFlagsMasked(0, points::PTF::REV, points::PTF::REV);

	loc.SetTargetStartLocation(&env.track1, EDGE_FRONT);
	leftover = AdvanceDisplacement(60000, loc, 0, stub);
	REQUIRE(leftover == 0);
	REQUIRE(loc == track_location(&env.track3, EDGE_FRONT, 10000));

	loc.SetTargetStartLocation(&env.track3, EDGE_BACK);
	leftover = AdvanceDisplacement(210000, loc, 0, stub);
	REQUIRE(leftover == 0);
	REQUIRE(loc == track_location(&env.track1, EDGE_BACK, 40000));

	loc.SetTargetStartLocation(&env.track2, EDGE_BACK);
	leftover = AdvanceDisplacement(110000, loc, 0, stub);
	REQUIRE(leftover == 10000);
	REQUIRE(loc == track_location(&env.pt1, EDGE_PTS_NORMAL, 0));

	//OOC
	env.pt1.SetPointsFlagsMasked(0, points::PTF::OOC, points::PTF::OOC);
	loc.SetTargetStartLocation(&env.track1, EDGE_FRONT);
	leftover = AdvanceDisplacement(60000, loc, 0, stub);
	REQUIRE(leftover == 10000);
	REQUIRE(loc == track_location(&env.pt1, EDGE_PTS_FACE, 0));

	loc.SetTargetStartLocation(&env.track2, EDGE_BACK);
	leftover = AdvanceDisplacement(110000, loc, 0, stub);
	REQUIRE(leftover == 10000);
	REQUIRE(loc == track_location(&env.pt1, EDGE_PTS_NORMAL, 0));

	loc.SetTargetStartLocation(&env.track3, EDGE_BACK);
	leftover = AdvanceDisplacement(210000, loc, 0, stub);
	REQUIRE(leftover == 10000);
	REQUIRE(loc == track_location(&env.pt1, EDGE_PTS_REVERSE, 0));
}

TEST_CASE( "track/deserialisation/track", "Test basic track segment deserialisation" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{ \"type\" : \"trackseg\", \"name\" : \"T1\", \"length\" : 50000, \"elevationdelta\" : -1000, \"traincount\" : 1, "
			"\"speedlimits\" : [ { \"speedclass\" : \"foo\", \"speed\" : 27778 } ] }"
	"] }";
	test_fixture_world env(track_test_str);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	trackseg *t = dynamic_cast<trackseg *>(env.w.FindTrackByName("T1"));
	REQUIRE(t != 0);
	REQUIRE(t->GetLength(EDGE_FRONT) == 50000);
	REQUIRE(t->GetElevationDelta(EDGE_FRONT) == -1000);

	const speedrestrictionset *sr = t->GetSpeedRestrictions();
	REQUIRE(sr != 0);
	REQUIRE(sr->GetTrackSpeedLimitByClass("foo", UINT_MAX) == 27778);
	REQUIRE(sr->GetTrackSpeedLimitByClass("bar", UINT_MAX) == UINT_MAX);
}

TEST_CASE( "track/deserialisation/points", "Test basic points deserialisation" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{ \"type\" : \"points\", \"name\" : \"P1\", \"reverse\" : true, \"failednorm\" : false, \"reminder\" : true}"
	"] }";
	test_fixture_world env(track_test_str);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	points *p = dynamic_cast<points *>(env.w.FindTrackByName("P1"));
	REQUIRE(p != 0);
	REQUIRE(p->GetPointsFlags(0) == (points::PTF::REV | points::PTF::REMINDER));
}

TEST_CASE( "track/deserialisation/autoname", "Test deserialisation automatic naming" ) {
	std::string track_test_str =
	"{ \"content\" : [ "
		"{ \"type\" : \"points\"}, "
		"{ \"type\" : \"trackseg\"}"
	"] }";
	test_fixture_world env(track_test_str);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	trackseg *p = dynamic_cast<trackseg *>(env.w.FindTrackByName("#1"));
	REQUIRE(p != 0);
}

std::string track_test_str_ds =
R"({ "content" : [ )"
	R"({ "type" : "doubleslip", "name" : "DS1" }, )"
	R"({ "type" : "doubleslip", "name" : "DS2", "degreesoffreedom" : 1 }, )"
	R"({ "type" : "doubleslip", "name" : "DS3", "degreesoffreedom" : 2 }, )"
	R"({ "type" : "doubleslip", "name" : "DS4", "degreesoffreedom" : 4 }, )"
	R"({ "type" : "doubleslip", "name" : "DS5", "notrack_fl_bl" : true }, )"
	R"({ "type" : "doubleslip", "name" : "DS6", "notrack_fl_br" : true, "rightfrontpoints" : { "reverse" : true } }, )"
	R"({ "type" : "doubleslip", "name" : "DS7", "notrack_fr_br" : true, "degreesoffreedom" : 1, "leftfrontpoints" : { "reverse" : true } }, )"
	R"({ "type" : "doubleslip", "name" : "DS8", "notrack_fr_bl" : true, "degreesoffreedom" : 4 }, )"
	R"({ "type" : "doubleslip", "name" : "DS9", "degreesoffreedom" : 1, "leftbackpoints" : { "locked" : true, "reverse" : true } }, )"
	R"({ "type" : "doubleslip", "name" : "DS10", "degreesoffreedom" : 2, "leftbackpoints" : { "failedrev" : true, "reverse" : true } }, )"
	R"({ "type" : "endofline", "name" : "FL" }, )"
	R"({ "type" : "endofline", "name" : "FR" }, )"
	R"({ "type" : "endofline", "name" : "BL" }, )"
	R"({ "type" : "endofline", "name" : "BR" } )"
"] }";

TEST_CASE( "track/deserialisation/doubleslip", "Test basic doubleslip deserialisation" ) {
	test_fixture_world env(track_test_str_ds);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	auto checkds = [&](const char *name, genericpoints::PTF pffl, genericpoints::PTF pffr, genericpoints::PTF pfbl, genericpoints::PTF pfbr) {
		SCOPED_INFO("Double-slip check for: " << name);

		doubleslip *ds = dynamic_cast<doubleslip *>(env.w.FindTrackByName(name));
		REQUIRE(ds != 0);
		REQUIRE(ds->GetCurrentPointFlags(EDGE_DS_FL) == pffl);
		REQUIRE(ds->GetCurrentPointFlags(EDGE_DS_FR) == pffr);
		REQUIRE(ds->GetCurrentPointFlags(EDGE_DS_BL) == pfbl);
		REQUIRE(ds->GetCurrentPointFlags(EDGE_DS_BR) == pfbr);
	};

	checkds("DS1", genericpoints::PTF::ZERO, points::PTF::REV, points::PTF::REV, genericpoints::PTF::ZERO);
	checkds("DS2", genericpoints::PTF::ZERO, genericpoints::PTF::ZERO, genericpoints::PTF::ZERO, genericpoints::PTF::ZERO);
	checkds("DS3", genericpoints::PTF::ZERO, points::PTF::REV, points::PTF::REV, genericpoints::PTF::ZERO);
	checkds("DS4", genericpoints::PTF::ZERO, genericpoints::PTF::ZERO, genericpoints::PTF::ZERO, genericpoints::PTF::ZERO);
	checkds("DS5", points::PTF::FIXED, genericpoints::PTF::ZERO, points::PTF::FIXED, genericpoints::PTF::ZERO);
	checkds("DS6", points::PTF::FIXED | points::PTF::REV, points::PTF::REV, genericpoints::PTF::ZERO, points::PTF::FIXED | points::PTF::REV);
	checkds("DS7", points::PTF::REV, points::PTF::FIXED, points::PTF::REV, points::PTF::FIXED);
	checkds("DS8", genericpoints::PTF::ZERO, points::PTF::FIXED | points::PTF::REV, points::PTF::FIXED | points::PTF::REV, genericpoints::PTF::ZERO);
	checkds("DS9", points::PTF::LOCKED | points::PTF::REV, points::PTF::LOCKED | points::PTF::REV, points::PTF::LOCKED | points::PTF::REV, points::PTF::LOCKED | points::PTF::REV);
	checkds("DS10", points::PTF::ZERO, points::PTF::REV, points::PTF::FAILEDREV | points::PTF::REV, points::PTF::FAILEDNORM);
}

class test_doubleslip {
	public:
	static inline bool HalfConnect(doubleslip *ds, EDGETYPE this_entrance_direction, const track_target_ptr &target_entrance) {
		return ds->HalfConnect(this_entrance_direction, target_entrance);
	}
	static inline unsigned int GetCurrentPointIndex(doubleslip *ds, EDGETYPE direction) {
		return ds->GetPointsIndexByEdge(direction);
	}
};

TEST_CASE( "track/conn/doubleslip", "Test basic doubleslip connectivity" ) {
	test_fixture_world env(track_test_str_ds);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	generictrack *fl = env.w.FindTrackByName("FL");
	generictrack *fr = env.w.FindTrackByName("FR");
	generictrack *bl = env.w.FindTrackByName("BL");
	generictrack *br = env.w.FindTrackByName("BR");

	auto checkdstraverse = [&](const char *name, generictrack *flt, generictrack *frt, generictrack *blt, generictrack *brt) {
		SCOPED_INFO("Double-slip check for: " << name);

		doubleslip *ds = dynamic_cast<doubleslip *>(env.w.FindTrackByName(name));
		REQUIRE(ds != 0);

		REQUIRE(test_doubleslip::HalfConnect(ds, EDGE_DS_FL, track_target_ptr(fl, EDGE_FRONT)) == true);
		REQUIRE(test_doubleslip::HalfConnect(ds, EDGE_DS_FR, track_target_ptr(fr, EDGE_FRONT)) == true);
		REQUIRE(test_doubleslip::HalfConnect(ds, EDGE_DS_BL, track_target_ptr(bl, EDGE_FRONT)) == true);
		REQUIRE(test_doubleslip::HalfConnect(ds, EDGE_DS_BR, track_target_ptr(br, EDGE_FRONT)) == true);
		REQUIRE(ds->GetConnectingPiece(EDGE_DS_FL).track == flt);
		REQUIRE(ds->GetConnectingPiece(EDGE_DS_FR).track == frt);
		REQUIRE(ds->GetConnectingPiece(EDGE_DS_BL).track == blt);
		REQUIRE(ds->GetConnectingPiece(EDGE_DS_BR).track == brt);
	};

	auto dsmovepoints = [&](const char *name, EDGETYPE direction, bool rev) {
		SCOPED_INFO("Double-slip check for: " << name);

		doubleslip *ds = dynamic_cast<doubleslip *>(env.w.FindTrackByName(name));
		REQUIRE(ds != 0);

		ds->SetPointsFlagsMasked(test_doubleslip::GetCurrentPointIndex(ds, direction), rev ? genericpoints::PTF::REV : genericpoints::PTF::ZERO, genericpoints::PTF::REV);
	};

	checkdstraverse("DS1", br, 0, 0, fl);
	checkdstraverse("DS2", br, bl, fr, fl);
	dsmovepoints("DS2", EDGE_DS_FL, true);
	checkdstraverse("DS2", bl, br, fl, fr);
	checkdstraverse("DS3", br, 0, 0, fl);
	dsmovepoints("DS3", EDGE_DS_FL, true);
	checkdstraverse("DS3", bl, 0, fl, 0);
	checkdstraverse("DS4", br, bl, fr, fl);
	dsmovepoints("DS4", EDGE_DS_FL, true);
	checkdstraverse("DS4", 0, bl, fr, 0);
	checkdstraverse("DS5", br, bl, fr, fl);
	checkdstraverse("DS6", 0, br, 0, fr);
	checkdstraverse("DS7", bl, 0, fl, 0);
	dsmovepoints("DS7", EDGE_DS_FL, false);
	checkdstraverse("DS7", br, bl, fr, fl);
	checkdstraverse("DS8", br, 0, 0, fl);
	checkdstraverse("DS9", bl, br, fl, fr);
}

TEST_CASE( "track/deserialisation/partialconnection", "Test partial track connection declaration deserialisation" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "startofline", "name" : "A"}, )"
		R"({ "type" : "points", "name" : "P1", "connect" : { "to" : "T2" } }, )"
		R"({ "type" : "points", "name" : "P2" }, )"
		R"({ "type" : "trackseg", "name" : "T1" }, )"
		R"({ "type" : "endofline", "name" : "B" }, )"
		R"({ "type" : "trackseg", "name" : "T2" }, )"
		R"({ "type" : "endofline", "name" : "C" }, )"
		R"({ "type" : "trackseg", "name" : "T3", "connect" : { "to" : "P2" } }, )"
		R"({ "type" : "endofline", "name" : "D" } )"
	"] }";
	test_fixture_world env(track_test_str);

	env.w.LayoutInit(env.ec);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	points *p1 = dynamic_cast<points *>(env.w.FindTrackByName("P1"));
	REQUIRE(p1 != 0);
	points *p2 = dynamic_cast<points *>(env.w.FindTrackByName("P2"));
	REQUIRE(p2 != 0);
	trackseg *t2 = dynamic_cast<trackseg *>(env.w.FindTrackByName("T2"));
	REQUIRE(t2 != 0);
	trackseg *t3 = dynamic_cast<trackseg *>(env.w.FindTrackByName("T3"));
	REQUIRE(t3 != 0);

	REQUIRE(p1->GetEdgeConnectingPiece(EDGE_PTS_REVERSE) == track_target_ptr(t2, EDGE_FRONT));
	REQUIRE(p2->GetEdgeConnectingPiece(EDGE_PTS_REVERSE) == track_target_ptr(t3, EDGE_FRONT));
}

TEST_CASE( "track/deserialisation/ambiguouspartialconnection/1", "Test handling of ambiguous partial track connection declaration deserialisation" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "startofline", "name" : "A"}, )"
		R"({ "type" : "points", "name" : "P1", "connect" : { "to" : "T2" } }, )"
		R"({ "type" : "endofline", "name" : "B" }, )"
		R"({ "type" : "trackseg", "name" : "T2" } )"
	"] }";
	test_fixture_world env(track_test_str);

	env.w.LayoutInit(env.ec);

	//if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() >= 1);
}

TEST_CASE( "track/deserialisation/ambiguouspartialconnection/2", "Test handling of ambiguous partial track connection declaration deserialisation" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "startofline", "name" : "A"}, )"
		R"({ "type" : "points", "name" : "P1" }, )"
		R"({ "type" : "endofline", "name" : "B" }, )"
		R"({ "type" : "trackseg", "name" : "T2" , "connect" : { "to" : "P1" } } )"
	"] }";
	test_fixture_world env(track_test_str);

	env.w.LayoutInit(env.ec);

	//if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() >= 1);
}

TEST_CASE( "track/deserialisation/ambiguouspartialconnection/3", "Test handling of ambiguous partial track connection declaration deserialisation" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "startofline", "name" : "A"}, )"
		R"({ "type" : "trackseg", "name" : "T1" }, )"
		R"({ "type" : "endofline", "name" : "B" }, )"
		R"({ "type" : "trackseg", "name" : "T2" , "connect" : { "to" : "T1" } } )"
	"] }";
	test_fixture_world env(track_test_str);

	env.w.LayoutInit(env.ec);

	//if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() >= 1);
}

std::string track_test_str_coupling =
R"({ "content" : [ )"
	R"({ "type" : "couplepoints", "points" : [ { "name" : "P1", "edge" : "reverse"}, { "name" : "P2", "edge" : "normal"}, { "name" : "DS1", "edge" : "rightfront"} ] }, )"
	R"({ "type" : "couplepoints", "points" : [ { "name" : "DS2", "edge" : "rightfront"}, { "name" : "DS1", "edge" : "rightback"} ] }, )"
	R"({ "type" : "doubleslip", "name" : "DS1", "degreesoffreedom" : 2 }, )"
	R"({ "type" : "doubleslip", "name" : "DS2", "notrack_fl_bl" : true, "rightfrontpoints" : { "reverse" : true } }, )"
	R"({ "type" : "points", "name" : "P1" }, )"
	R"({ "type" : "points", "name" : "P2" } )"
"] }";

TEST_CASE( "track/points/coupling", "Test points coupling" ) {
	test_fixture_world env(track_test_str_coupling);

	env.w.CapAllTrackPieceUnconnectedEdges();
	env.w.LayoutInit(env.ec);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	auto checkds = [&](const char *name, genericpoints::PTF pffl, genericpoints::PTF pffr, genericpoints::PTF pfbl, genericpoints::PTF pfbr) {
		SCOPED_INFO("Double-slip check for: " << name);

		doubleslip *ds = dynamic_cast<doubleslip *>(env.w.FindTrackByName(name));
		REQUIRE(ds != 0);
		CHECK(ds->GetCurrentPointFlags(EDGE_DS_FL) == pffl);
		CHECK(ds->GetCurrentPointFlags(EDGE_DS_FR) == pffr);
		CHECK(ds->GetCurrentPointFlags(EDGE_DS_BL) == pfbl);
		CHECK(ds->GetCurrentPointFlags(EDGE_DS_BR) == pfbr);
	};
	auto checkp = [&](const char *name, genericpoints::PTF pflags) {
		SCOPED_INFO("Points check for: " << name);

		points *p = dynamic_cast<points *>(env.w.FindTrackByName(name));
		REQUIRE(p != 0);
		CHECK(p->GetPointsFlags(0) == pflags);
	};
	checkp("P1", genericpoints::PTF::ZERO);
	checkp("P2", genericpoints::PTF::REV);
	checkds("DS1", genericpoints::PTF::REV, points::PTF::ZERO, points::PTF::REV, genericpoints::PTF::ZERO);
	checkds("DS2", genericpoints::PTF::FIXED, genericpoints::PTF::REV, genericpoints::PTF::FIXED, genericpoints::PTF::ZERO);

}
