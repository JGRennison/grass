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

struct test_fixture_1 {
	world w;
	trackseg track1, track2;
	error_collection ec;
	test_fixture_1() : track1(w), track2(w) {
		track1.SetLength(50000);
		track1.SetElevationDelta(-100);
		track1.SetName("T1");
		track2.SetLength(100000);
		track2.SetElevationDelta(+50);
		track2.SetName("T2");
		track1.FullConnect(TDIR_REVERSE, track_target_ptr(&track2, TDIR_FORWARD), ec);
	}
};

struct test_fixture_2 {
	world w;
	trackseg track1, track2, track3;
	points pt1;
	error_collection ec;
	test_fixture_2() : track1(w), track2(w), track3(w), pt1(w) {
		track1.SetLength(50000);
		track1.SetElevationDelta(-100);
		track1.SetName("T1");
		track2.SetLength(100000);
		track2.SetElevationDelta(+50);
		track2.SetName("T2");
		track3.SetLength(200000);
		track3.SetElevationDelta(-50);
		track3.SetName("T3");
		track1.FullConnect(TDIR_REVERSE, track_target_ptr(&pt1, TDIR_PTS_FACE), ec);
		track2.FullConnect(TDIR_FORWARD, track_target_ptr(&pt1, TDIR_PTS_NORMAL), ec);
		track3.FullConnect(TDIR_FORWARD, track_target_ptr(&pt1, TDIR_PTS_REVERSE), ec);
		pt1.SetName("P1");
	}
};

TEST_CASE( "track/conn/track", "Test basic track segment connectivity" ) {
	test_fixture_1 env;

	REQUIRE(env.track1.GetConnectingPiece(TDIR_FORWARD) == track_target_ptr(&env.track2, TDIR_FORWARD));
	REQUIRE(env.track2.GetConnectingPiece(TDIR_REVERSE) == track_target_ptr(&env.track1, TDIR_REVERSE));
	REQUIRE(env.track1.GetConnectingPiece(TDIR_REVERSE) == track_target_ptr());
	REQUIRE(env.track1.GetConnectingPiece(TDIR_FORWARD).GetConnectingPiece() == track_target_ptr());
}

TEST_CASE( "track/conn/points", "Test basic points connectivity" ) {
	test_fixture_2 env;

	REQUIRE(env.pt1.GetPointFlags(0) == 0);
	REQUIRE(env.track1.GetConnectingPiece(TDIR_FORWARD) == track_target_ptr(&env.pt1, TDIR_PTS_FACE));
	REQUIRE(env.track2.GetConnectingPiece(TDIR_REVERSE) == track_target_ptr(&env.pt1, TDIR_PTS_NORMAL));
	REQUIRE(env.track3.GetConnectingPiece(TDIR_REVERSE) == track_target_ptr(&env.pt1, TDIR_PTS_REVERSE));
	//normal
	REQUIRE(env.track1.GetConnectingPiece(TDIR_FORWARD).GetConnectingPiece() == track_target_ptr(&env.track2, TDIR_FORWARD));
	REQUIRE(env.track2.GetConnectingPiece(TDIR_REVERSE).GetConnectingPiece() == track_target_ptr(&env.track1, TDIR_REVERSE));
	REQUIRE(env.track3.GetConnectingPiece(TDIR_REVERSE).GetConnectingPiece() == track_target_ptr());
	//reverse
	env.pt1.SetPointFlagsMasked(0, points::PTF_REV, points::PTF_REV);
	REQUIRE(env.track1.GetConnectingPiece(TDIR_FORWARD).GetConnectingPiece() == track_target_ptr(&env.track3, TDIR_FORWARD));
	REQUIRE(env.track3.GetConnectingPiece(TDIR_REVERSE).GetConnectingPiece() == track_target_ptr(&env.track1, TDIR_REVERSE));
	REQUIRE(env.track2.GetConnectingPiece(TDIR_REVERSE).GetConnectingPiece() == track_target_ptr());
	//OOC
	env.pt1.SetPointFlagsMasked(0, points::PTF_OOC, points::PTF_OOC);
	REQUIRE(env.track1.GetConnectingPiece(TDIR_FORWARD).GetConnectingPiece() == track_target_ptr());
	REQUIRE(env.track2.GetConnectingPiece(TDIR_REVERSE).GetConnectingPiece() == track_target_ptr());
	REQUIRE(env.track3.GetConnectingPiece(TDIR_REVERSE).GetConnectingPiece() == track_target_ptr());
}

TEST_CASE( "track/traverse/track", "Test basic track segment traversal" ) {
	test_fixture_1 env;

	unsigned int leftover;
	track_location loc;
	std::function<void(track_location &, track_location &)> stub;

	loc.SetTargetStartLocation(&env.track1, TDIR_FORWARD);

	leftover = AdvanceDisplacement(20000, loc, 0, stub);
	REQUIRE(leftover == 0);
	REQUIRE(loc == track_location(&env.track1, TDIR_FORWARD, 20000));

	leftover = AdvanceDisplacement(40000, loc, 0, stub);
	REQUIRE(leftover == 0);
	REQUIRE(loc == track_location(&env.track2, TDIR_FORWARD, 10000));

	leftover = AdvanceDisplacement(100000, loc, 0, stub);
	REQUIRE(leftover == 10000);
	REQUIRE(loc == track_location(&env.track2, TDIR_FORWARD, 100000));
}

TEST_CASE( "track/traverse/points", "Test basic points traversal" ) {
	test_fixture_2 env;

	unsigned int leftover;
	track_location loc;
	std::function<void(track_location &, track_location &)> stub;

	loc.SetTargetStartLocation(&env.track1, TDIR_FORWARD);

	//normal
	loc.SetTargetStartLocation(&env.track1, TDIR_FORWARD);
	leftover = AdvanceDisplacement(60000, loc, 0, stub);
	REQUIRE(leftover == 0);
	REQUIRE(loc == track_location(&env.track2, TDIR_FORWARD, 10000));

	loc.SetTargetStartLocation(&env.track2, TDIR_REVERSE);
	leftover = AdvanceDisplacement(110000, loc, 0, stub);
	REQUIRE(leftover == 0);
	REQUIRE(loc == track_location(&env.track1, TDIR_REVERSE, 40000));

	loc.SetTargetStartLocation(&env.track3, TDIR_REVERSE);
	leftover = AdvanceDisplacement(210000, loc, 0, stub);
	REQUIRE(leftover == 10000);
	REQUIRE(loc == track_location(&env.pt1, TDIR_PTS_REVERSE, 0));

	//reverse
	env.pt1.SetPointFlagsMasked(0, points::PTF_REV, points::PTF_REV);

	loc.SetTargetStartLocation(&env.track1, TDIR_FORWARD);
	leftover = AdvanceDisplacement(60000, loc, 0, stub);
	REQUIRE(leftover == 0);
	REQUIRE(loc == track_location(&env.track3, TDIR_FORWARD, 10000));

	loc.SetTargetStartLocation(&env.track3, TDIR_REVERSE);
	leftover = AdvanceDisplacement(210000, loc, 0, stub);
	REQUIRE(leftover == 0);
	REQUIRE(loc == track_location(&env.track1, TDIR_REVERSE, 40000));

	loc.SetTargetStartLocation(&env.track2, TDIR_REVERSE);
	leftover = AdvanceDisplacement(110000, loc, 0, stub);
	REQUIRE(leftover == 10000);
	REQUIRE(loc == track_location(&env.pt1, TDIR_PTS_NORMAL, 0));

	//OOC
	env.pt1.SetPointFlagsMasked(0, points::PTF_OOC, points::PTF_OOC);
	loc.SetTargetStartLocation(&env.track1, TDIR_FORWARD);
	leftover = AdvanceDisplacement(60000, loc, 0, stub);
	REQUIRE(leftover == 10000);
	REQUIRE(loc == track_location(&env.pt1, TDIR_PTS_FACE, 0));

	loc.SetTargetStartLocation(&env.track2, TDIR_REVERSE);
	leftover = AdvanceDisplacement(110000, loc, 0, stub);
	REQUIRE(leftover == 10000);
	REQUIRE(loc == track_location(&env.pt1, TDIR_PTS_NORMAL, 0));

	loc.SetTargetStartLocation(&env.track3, TDIR_REVERSE);
	leftover = AdvanceDisplacement(210000, loc, 0, stub);
	REQUIRE(leftover == 10000);
	REQUIRE(loc == track_location(&env.pt1, TDIR_PTS_REVERSE, 0));
}