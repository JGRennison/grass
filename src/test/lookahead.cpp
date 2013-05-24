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
#include "points.h"
#include "deserialisation-test.h"
#include "world-test.h"
#include "signal.h"
#include "lookahead.h"
#include "track_ops.h"

std::string lookahead_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "startofline", "name" : "A" }, )"
	R"({ "type" : "trackseg", "length" : 500000 }, )"
	R"({ "type" : "autosignal", "name" : "S1", "sighting" : 200000, "maxaspect" : 3 }, )"
	R"({ "type" : "trackseg", "length" : 500000, "name" : "TS1" }, )"
	R"({ "type" : "autosignal", "name" : "S2", "overlapend" : true, "sighting" : 200000, "maxaspect" : 3 }, )"
	R"({ "type" : "trackseg", "length" : 500000, "speedlimits" : [ { "speedclass" : "", "speed" : 200 } ], "name" : "TS2" }, )"
	R"({ "type" : "routesignal", "name" : "S3", "routesignal" : true, "overlapend" : true, "sighting" : 200000, "maxaspect" : 3 }, )"
	R"({ "type" : "trackseg", "length" : 500000, "name" : "TS3"  }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "points", "name" : "P1", "sighting" : 50000 }, )"
	R"({ "type" : "trackseg", "length" : 500000, "speedlimits" : [ { "speedclass" : "", "speed" : 100 } ], "name" : "TS4" }, )"
	R"({ "type" : "routesignal", "name" : "S4", "shuntsignal" : true, "sighting" : 200000, "end" : { "allow" : "route" } }, )"
	R"({ "type" : "trackseg", "length" : 500000 }, )"
	R"({ "type" : "endofline", "name" : "B", "end" : { "allow" : "overlap" } }, )"

	R"({ "type" : "trackseg", "length" : 400000, "connect" : { "to" : "P1", "fromdirection" : "front" } }, )"
	R"({ "type" : "endofline", "name" : "C" } )"
"] }";

void CheckLookahead(lookahead &l, const track_location &pos, std::map<unsigned int, unsigned int> &map, lookahead::LA_ERROR expected_error = lookahead::LA_ERROR::NONE, const track_target_ptr &error_piece = track_target_ptr()) {
	map.clear();
	auto func = [&](unsigned int distance, unsigned int speed) {
		map[distance] = speed;
	};

	auto errfunc = [&](lookahead::LA_ERROR err, const track_target_ptr &piece) {
		CHECK(err == expected_error);
		CHECK(piece == error_piece);
		expected_error = lookahead::LA_ERROR::NONE;
	};

	l.CheckLookaheads(0, pos, func, errfunc);

	CHECK(expected_error == lookahead::LA_ERROR::NONE);
}

void CheckLookaheadResult(const track_location &pos, std::map<unsigned int, unsigned int> &map, unsigned int distance, unsigned int speed) {
	auto it = map.find(distance);
	if(it != map.end()) {
		SCOPED_INFO("CheckLookaheadResult: " << distance << " -> " << speed << " (" << pos << ")");
		CHECK(it->second == speed);
		map.erase(it);
	}
	else {
		WARN("CheckLookaheadResult: No such result in map: " << distance << " -> " << speed << " (" << pos << ")");
		CHECK(true == false);
	}
}

void FinaliseLookaheadCheck(const track_location &pos, std::map<unsigned int, unsigned int> &map) {
	CHECK(map.empty() == true);
	for(auto it : map) {
		WARN("FinaliseLookaheadCheck: map still has values: " << it.first << " -> " << it.second << " (" << pos << ")");
	}
}

TEST_CASE( "lookahead/routed", "Test routed track lookahead" ) {

	test_fixture_world env(lookahead_test_str_1);

	env.w.LayoutInit(env.ec);
	env.w.PostLayoutInit(env.ec);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	env.w.GameStep(1);

	generictrack *ts1 = env.w.FindTrackByName("TS1");
	REQUIRE(ts1 != 0);
	generictrack *ts2 = env.w.FindTrackByName("TS2");
	REQUIRE(ts2 != 0);
	genericsignal *s1 = env.w.FindTrackByNameCast<genericsignal>("S1");
	REQUIRE(s1 != 0);
	genericsignal *s2 = env.w.FindTrackByNameCast<genericsignal>("S2");
	REQUIRE(s2 != 0);
	genericsignal *s3 = env.w.FindTrackByNameCast<genericsignal>("S3");
	REQUIRE(s3 != 0);
	genericsignal *s4 = env.w.FindTrackByNameCast<genericsignal>("S4");
	REQUIRE(s4 != 0);
	routingpoint *b = env.w.FindTrackByNameCast<routingpoint>("B");
	REQUIRE(b != 0);
	routingpoint *c = env.w.FindTrackByNameCast<routingpoint>("C");
	REQUIRE(c != 0);
	points *p1 = env.w.FindTrackByNameCast<points>("P1");
	REQUIRE(p1 != 0);

	lookahead l;
	std::map<unsigned int, unsigned int> map;
	track_location pos(ts1, EDGE_FRONT, 100000);
	const route *s1rt = s1->GetCurrentForwardRoute();
	CHECK(s1->GetAspect() > 0);
	CHECK(s1rt != 0);
	l.Init(0, pos, s1rt);
	CheckLookahead(l, pos, map);
	CheckLookaheadResult(pos, map, 400000, 0);
	FinaliseLookaheadCheck(pos, map);

	l.Advance(300000);
	pos = track_location(ts1, EDGE_FRONT, 400000);
	CheckLookahead(l, pos, map);
	CheckLookaheadResult(pos, map, 100000, 200);
	CheckLookaheadResult(pos, map, 600000, 0);
	FinaliseLookaheadCheck(pos, map);

	p1->SetPointsFlagsMasked(0, points::PTF::REV, points::PTF::REV);
	env.w.SubmitAction(action_reservepath(env.w, s3, c));
	env.w.GameStep(1);
	CHECK(env.w.GetLogText() == "");
	CheckLookahead(l, pos, map);
	CheckLookaheadResult(pos, map, 100000, 200);
	CheckLookaheadResult(pos, map, 1500000, 0);
	FinaliseLookaheadCheck(pos, map);

	CHECK(s3->GetCurrentForwardRoute() != 0);
	env.w.SubmitAction(action_unreservetrackroute(env.w, *s3->GetCurrentForwardRoute()));
	env.w.GameStep(1);
	CheckLookahead(l, pos, map, lookahead::LA_ERROR::SIG_ASPECT_LESS_THAN_EXPECTED, track_target_ptr(s2, EDGE_FRONT));
	CheckLookaheadResult(pos, map, 100000, 200);
	CheckLookaheadResult(pos, map, 600000, 0);
	FinaliseLookaheadCheck(pos, map);

	env.w.SubmitAction(action_reservepath(env.w, s3, c));
	env.w.GameStep(1);

	l.Advance(500000);
	pos = track_location(ts2, EDGE_FRONT, 400000);
	CheckLookahead(l, pos, map);
	CheckLookaheadResult(pos, map, 0, 200);
	CheckLookaheadResult(pos, map, 1000000, 0);
	FinaliseLookaheadCheck(pos, map);

	CHECK(s3->GetCurrentForwardRoute() != 0);
	env.w.SubmitAction(action_unreservetrackroute(env.w, *s3->GetCurrentForwardRoute()));
	env.w.GameStep(1);

	p1->SetPointsFlagsMasked(0, points::PTF::ZERO, points::PTF::REV);
	env.w.SubmitAction(action_reservepath(env.w, s3, s4));
	env.w.SubmitAction(action_reservepath(env.w, s4, b));
	env.w.GameStep(1);
	CHECK(env.w.GetLogText() == "");

	CheckLookahead(l, pos, map, lookahead::LA_ERROR::SIG_TARGET_CHANGE, track_target_ptr(s3, EDGE_FRONT));
	CheckLookaheadResult(pos, map, 0, 200);
	CheckLookaheadResult(pos, map, 600000, 100);
	CheckLookaheadResult(pos, map, 1100000, 0);
	FinaliseLookaheadCheck(pos, map);
}

TEST_CASE( "lookahead/unrouted", "Test unrouted track lookahead" ) {

	test_fixture_world env(lookahead_test_str_1);

	env.w.LayoutInit(env.ec);
	env.w.PostLayoutInit(env.ec);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	env.w.GameStep(1);

	generictrack *ts3 = env.w.FindTrackByName("TS3");
	REQUIRE(ts3 != 0);
	generictrack *ts4 = env.w.FindTrackByName("TS4");
	REQUIRE(ts4 != 0);
	genericsignal *s3 = env.w.FindTrackByNameCast<genericsignal>("S3");
	REQUIRE(s3 != 0);
	genericsignal *s4 = env.w.FindTrackByNameCast<genericsignal>("S4");
	REQUIRE(s4 != 0);
	routingpoint *b = env.w.FindTrackByNameCast<routingpoint>("B");
	REQUIRE(b != 0);
	points *p1 = env.w.FindTrackByNameCast<points>("P1");
	REQUIRE(p1 != 0);

	lookahead l;
	std::map<unsigned int, unsigned int> map;
	track_location pos(ts3, EDGE_FRONT, 100000);
	l.Init(0, pos, 0);
	CheckLookahead(l, pos, map);
	CheckLookaheadResult(pos, map, 400000, 0);
	FinaliseLookaheadCheck(pos, map);

	l.Advance(350000);
	pos = track_location(ts3, EDGE_FRONT, 450000);
	CheckLookahead(l, pos, map);
	CheckLookaheadResult(pos, map, 50000, 100);
	CheckLookaheadResult(pos, map, 550000, 0);
	FinaliseLookaheadCheck(pos, map);

	p1->SetPointsFlagsMasked(0, points::PTF::OOC, points::PTF::OOC);
	CheckLookahead(l, pos, map);
	CheckLookaheadResult(pos, map, 50000, 0);
	FinaliseLookaheadCheck(pos, map);

	p1->SetPointsFlagsMasked(0, points::PTF::REV, points::PTF::OOC | points::PTF::REV);
	CheckLookahead(l, pos, map);
	CheckLookaheadResult(pos, map, 450000, 0);
	FinaliseLookaheadCheck(pos, map);

	p1->SetPointsFlagsMasked(0, points::PTF::ZERO, points::PTF::REV);
	CheckLookahead(l, pos, map);
	CheckLookaheadResult(pos, map, 50000, 100);
	CheckLookaheadResult(pos, map, 550000, 0);
	FinaliseLookaheadCheck(pos, map);

	l.Advance(350000);
	pos = track_location(ts4, EDGE_FRONT, 300000);
	CheckLookahead(l, pos, map);
	CheckLookaheadResult(pos, map, 0, 100);
	CheckLookaheadResult(pos, map, 200000, 0);
	FinaliseLookaheadCheck(pos, map);

	l.Advance(200000);
	pos = track_location(s4, EDGE_FRONT, 0);
	CheckLookahead(l, pos, map, lookahead::LA_ERROR::WAITING_AT_RED_SIG, track_target_ptr(s4, EDGE_FRONT));
	CheckLookaheadResult(pos, map, 0, 0);
	FinaliseLookaheadCheck(pos, map);

	env.w.SubmitAction(action_reservepath(env.w, s4, b));
	env.w.GameStep(1);
	CheckLookahead(l, pos, map);
	CheckLookaheadResult(pos, map, 0, 100);
	CheckLookaheadResult(pos, map, 500000, 0);
	FinaliseLookaheadCheck(pos, map);
}
