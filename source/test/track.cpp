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

	REQUIRE(env.pt1.GetPointFlags(0) == 0);
	REQUIRE(env.track1.GetConnectingPiece(EDGE_FRONT) == track_target_ptr(&env.pt1, EDGE_PTS_FACE));
	REQUIRE(env.track2.GetConnectingPiece(EDGE_BACK) == track_target_ptr(&env.pt1, EDGE_PTS_NORMAL));
	REQUIRE(env.track3.GetConnectingPiece(EDGE_BACK) == track_target_ptr(&env.pt1, EDGE_PTS_REVERSE));
	//normal
	REQUIRE(env.track1.GetConnectingPiece(EDGE_FRONT).GetConnectingPiece() == track_target_ptr(&env.track2, EDGE_FRONT));
	REQUIRE(env.track2.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr(&env.track1, EDGE_BACK));
	REQUIRE(env.track3.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr());
	//reverse
	env.pt1.SetPointFlagsMasked(0, points::PTF_REV, points::PTF_REV);
	REQUIRE(env.track1.GetConnectingPiece(EDGE_FRONT).GetConnectingPiece() == track_target_ptr(&env.track3, EDGE_FRONT));
	REQUIRE(env.track3.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr(&env.track1, EDGE_BACK));
	REQUIRE(env.track2.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr());
	//OOC
	env.pt1.SetPointFlagsMasked(0, points::PTF_OOC, points::PTF_OOC);
	REQUIRE(env.track1.GetConnectingPiece(EDGE_FRONT).GetConnectingPiece() == track_target_ptr());
	REQUIRE(env.track2.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr());
	REQUIRE(env.track3.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr());
}

TEST_CASE( "track/conn/catchpoints", "Test basic catchpoints connectivity" ) {
	test_fixture_track_3 env;

	REQUIRE(env.cp1.GetPointFlags(0) == catchpoints::PTF_REV);
	REQUIRE(env.track2.GetConnectingPiece(EDGE_FRONT) == track_target_ptr(&env.cp1, EDGE_FRONT));
	REQUIRE(env.track4.GetConnectingPiece(EDGE_BACK) == track_target_ptr(&env.cp1, EDGE_BACK));
	//reverse
	REQUIRE(env.track2.GetConnectingPiece(EDGE_FRONT).GetConnectingPiece() == track_target_ptr());
	REQUIRE(env.track4.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr());
	//normal
	env.cp1.SetPointFlagsMasked(0, 0, points::PTF_REV);
	REQUIRE(env.track2.GetConnectingPiece(EDGE_FRONT).GetConnectingPiece() == track_target_ptr(&env.track4, EDGE_FRONT));
	REQUIRE(env.track4.GetConnectingPiece(EDGE_BACK).GetConnectingPiece() == track_target_ptr(&env.track2, EDGE_BACK));
	//OOC
	env.cp1.SetPointFlagsMasked(0, points::PTF_OOC, points::PTF_OOC);
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
	env.pt1.SetPointFlagsMasked(0, points::PTF_REV, points::PTF_REV);

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
	env.pt1.SetPointFlagsMasked(0, points::PTF_OOC, points::PTF_OOC);
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
	REQUIRE(p->GetPointFlags(0) == (points::PTF_REV | points::PTF_REMINDER));
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
