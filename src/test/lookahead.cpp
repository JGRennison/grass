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
#include "core/track.h"
#include "core/points.h"
#include "core/signal.h"
#include "core/lookahead.h"
#include "core/track_ops.h"
#include "core/train.h"
#include "core/traction_type.h"
#include "core/serialisable_impl.h"

std::string lookahead_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "track_seg", "length" : 500000 }, )"
	R"({ "type" : "auto_signal", "name" : "S1", "sighting" : 200000, "maxaspect" : 3 }, )"
	R"({ "type" : "track_seg", "length" : 500000, "name" : "TS1" }, )"
	R"({ "type" : "auto_signal", "name" : "S2", "overlapend" : true, "sighting" : 200000, "maxaspect" : 3 }, )"
	R"({ "type" : "track_seg", "length" : 500000, "speedlimits" : [ { "speed_class" : "", "speed" : 200 } ], "name" : "TS2" }, )"
	R"({ "type" : "route_signal", "name" : "S3", "route_signal" : true, "overlapend" : true, "sighting" : 200000, "maxaspect" : 3 }, )"
	R"({ "type" : "track_seg", "length" : 500000, "name" : "TS3"  }, )"
	R"({ "type" : "routing_marker", "overlapend" : true }, )"
	R"({ "type" : "points", "name" : "P1", "sighting" : 50000 }, )"
	R"({ "type" : "track_seg", "length" : 500000, "speedlimits" : [ { "speed_class" : "", "speed" : 100 } ], "name" : "TS4" }, )"
	R"({ "type" : "route_signal", "name" : "S4", "shuntsignal" : true, "sighting" : 200000, "end" : { "allow" : "route" } }, )"
	R"({ "type" : "track_seg", "length" : 500000 }, )"
	R"({ "type" : "end_of_line", "name" : "B", "end" : { "allow" : "overlap" } }, )"

	R"({ "type" : "track_seg", "length" : 400000, "connect" : { "to" : "P1", "fromdirection" : "front" } }, )"
	R"({ "type" : "end_of_line", "name" : "C" } )"
"] }";

std::string lookahead_test_str_2 =
R"({ "content" : [ )"
	R"({ "type" : "tractiontype", "name" : "AC" }, )"
	R"({ "type" : "tractiontype", "name" : "diesel", "always_available" : true }, )"
	R"({ "type" : "vehicleclass", "name" : "VC1", "traction_types" : [ "diesel", "AC" ], "length" : "25m", "mass" : "40t" },)"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "track_seg", "name" : "TS1", "length" : 500000, "traction_types" : [ "AC" ] }, )"
	R"({ "type" : "track_seg", "name" : "TS2", "length" : 500000 }, )"
	R"({ "type" : "track_seg", "name" : "TS3", "length" : 500000, "traction_types" : [ "AC" ] }, )"
	R"({ "type" : "auto_signal", "name" : "S1" }, )"
	R"({ "type" : "track_seg", "name" : "TS4", "length" : 500000, "traction_types" : [ "AC" ] }, )"
	R"({ "type" : "track_seg", "name" : "TS5", "length" : 500000 }, )"
	R"({ "type" : "track_seg", "name" : "TS6", "length" : 500000, "traction_types" : [ "AC" ] }, )"
	R"({ "type" : "end_of_line", "name" : "B", "end" : { "allow" : "overlap" } } )"
R"(], "gamestate" : [)"
	R"({ "type" : "train", "vehicleclasses" : [ "VC1" ] })"
"] }";

void CheckLookahead(const train *t, lookahead &l, const track_location &pos, std::map<unsigned int, unsigned int> &map, lookahead::LA_ERROR expected_error = lookahead::LA_ERROR::NONE, const track_target_ptr &error_piece = track_target_ptr()) {
	map.clear();
	auto func = [&](unsigned int distance, unsigned int speed) {
		map[distance] = speed;
	};

	auto errfunc = [&](lookahead::LA_ERROR err, const track_target_ptr &piece) {
		CHECK(err == expected_error);
		CHECK(piece == error_piece);
		expected_error = lookahead::LA_ERROR::NONE;
	};

	l.CheckLookaheads(t, pos, func, errfunc);

	CHECK(expected_error == lookahead::LA_ERROR::NONE);
}

void CheckLookaheadResult(const track_location &pos, std::map<unsigned int, unsigned int> &map, unsigned int distance, unsigned int speed) {
	auto it = map.find(distance);
	if (it != map.end()) {
		INFO("CheckLookaheadResult: " << distance << " -> " << speed << " (" << pos << ")");
		CHECK(it->second == speed);
		map.erase(it);
	} else {
		WARN("CheckLookaheadResult: No such result in map: " << distance << " -> " << speed << " (" << pos << ")");
		CHECK(true == false);
	}
}

void FinaliseLookaheadCheck(const track_location &pos, std::map<unsigned int, unsigned int> &map) {
	CHECK(map.empty() == true);
	for (auto it : map) {
		WARN("FinaliseLookaheadCheck: map still has values: " << it.first << " -> " << it.second << " (" << pos << ")");
	}
}

TEST_CASE( "lookahead/routed", "Test routed track lookahead" ) {

	test_fixture_world_init_checked env(lookahead_test_str_1);

	env.w->GameStep(1);

	generic_track *ts1 = env.w->FindTrackByName("TS1");
	REQUIRE(ts1 != nullptr);
	generic_track *ts2 = env.w->FindTrackByName("TS2");
	REQUIRE(ts2 != nullptr);
	generic_signal *s1 = env.w->FindTrackByNameCast<generic_signal>("S1");
	REQUIRE(s1 != nullptr);
	generic_signal *s2 = env.w->FindTrackByNameCast<generic_signal>("S2");
	REQUIRE(s2 != nullptr);
	generic_signal *s3 = env.w->FindTrackByNameCast<generic_signal>("S3");
	REQUIRE(s3 != nullptr);
	generic_signal *s4 = env.w->FindTrackByNameCast<generic_signal>("S4");
	REQUIRE(s4 != nullptr);
	routing_point *b = env.w->FindTrackByNameCast<routing_point>("B");
	REQUIRE(b != nullptr);
	routing_point *c = env.w->FindTrackByNameCast<routing_point>("C");
	REQUIRE(c != nullptr);
	points *p1 = env.w->FindTrackByNameCast<points>("P1");
	REQUIRE(p1 != nullptr);

	lookahead l;
	std::map<unsigned int, unsigned int> map;
	track_location pos(ts1, EDGE_FRONT, 100000);
	const route *s1rt = s1->GetCurrentForwardRoute();
	CHECK(s1->GetAspect() > 0);
	CHECK(s1rt != nullptr);
	l.Init(nullptr, pos, s1rt);
	CheckLookahead(nullptr, l, pos, map);
	CheckLookaheadResult(pos, map, 400000, 0);
	FinaliseLookaheadCheck(pos, map);

	l.Advance(300000);
	pos = track_location(ts1, EDGE_FRONT, 400000);
	CheckLookahead(nullptr, l, pos, map);
	CheckLookaheadResult(pos, map, 100000, 200);
	CheckLookaheadResult(pos, map, 600000, 0);
	FinaliseLookaheadCheck(pos, map);

	p1->SetPointsFlagsMasked(0, points::PTF::REV, points::PTF::REV);
	env.w->SubmitAction(action_reserve_path(*(env.w), s3, c));
	env.w->GameStep(1);
	CHECK(env.w->GetLogText() == "");
	CheckLookahead(nullptr, l, pos, map);
	CheckLookaheadResult(pos, map, 100000, 200);
	CheckLookaheadResult(pos, map, 1500000, 0);
	FinaliseLookaheadCheck(pos, map);

	CHECK(s3->GetCurrentForwardRoute() != 0);
	env.w->SubmitAction(action_unreserve_track_route(*(env.w), *s3->GetCurrentForwardRoute()));
	env.w->GameStep(1);
	CheckLookahead(nullptr, l, pos, map, lookahead::LA_ERROR::SIG_ASPECT_LESS_THAN_EXPECTED, track_target_ptr(s2, EDGE_FRONT));
	CheckLookaheadResult(pos, map, 100000, 200);
	CheckLookaheadResult(pos, map, 600000, 0);
	FinaliseLookaheadCheck(pos, map);

	env.w->SubmitAction(action_reserve_path(*(env.w), s3, c));
	env.w->GameStep(1);

	l.Advance(500000);
	pos = track_location(ts2, EDGE_FRONT, 400000);
	CheckLookahead(nullptr, l, pos, map);
	CheckLookaheadResult(pos, map, 0, 200);
	CheckLookaheadResult(pos, map, 1000000, 0);
	FinaliseLookaheadCheck(pos, map);

	CHECK(s3->GetCurrentForwardRoute() != 0);
	env.w->SubmitAction(action_unreserve_track_route(*(env.w), *s3->GetCurrentForwardRoute()));
	env.w->GameStep(1);

	p1->SetPointsFlagsMasked(0, points::PTF::ZERO, points::PTF::REV);
	env.w->SubmitAction(action_reserve_path(*(env.w), s3, s4));
	env.w->SubmitAction(action_reserve_path(*(env.w), s4, b));
	env.w->GameStep(1);
	CHECK(env.w->GetLogText() == "");

	CheckLookahead(nullptr, l, pos, map, lookahead::LA_ERROR::SIG_TARGET_CHANGE, track_target_ptr(s3, EDGE_FRONT));
	CheckLookaheadResult(pos, map, 0, 200);
	CheckLookaheadResult(pos, map, 600000, 100);
	CheckLookaheadResult(pos, map, 1100000, 0);
	FinaliseLookaheadCheck(pos, map);
}

TEST_CASE( "lookahead/unrouted", "Test unrouted track lookahead" ) {

	test_fixture_world_init_checked env(lookahead_test_str_1);

	env.w->GameStep(1);

	generic_track *ts3 = env.w->FindTrackByName("TS3");
	REQUIRE(ts3 != nullptr);
	generic_track *ts4 = env.w->FindTrackByName("TS4");
	REQUIRE(ts4 != nullptr);
	generic_signal *s3 = env.w->FindTrackByNameCast<generic_signal>("S3");
	REQUIRE(s3 != nullptr);
	generic_signal *s4 = env.w->FindTrackByNameCast<generic_signal>("S4");
	REQUIRE(s4 != nullptr);
	routing_point *b = env.w->FindTrackByNameCast<routing_point>("B");
	REQUIRE(b != nullptr);
	points *p1 = env.w->FindTrackByNameCast<points>("P1");
	REQUIRE(p1 != nullptr);

	lookahead l;
	std::map<unsigned int, unsigned int> map;
	track_location pos(ts3, EDGE_FRONT, 100000);
	l.Init(nullptr, pos, 0);
	CheckLookahead(nullptr, l, pos, map);
	CheckLookaheadResult(pos, map, 400000, 0);
	FinaliseLookaheadCheck(pos, map);

	l.Advance(350000);
	pos = track_location(ts3, EDGE_FRONT, 450000);
	CheckLookahead(nullptr, l, pos, map);
	CheckLookaheadResult(pos, map, 50000, 100);
	CheckLookaheadResult(pos, map, 550000, 0);
	FinaliseLookaheadCheck(pos, map);

	p1->SetPointsFlagsMasked(0, points::PTF::OOC, points::PTF::OOC);
	CheckLookahead(nullptr, l, pos, map);
	CheckLookaheadResult(pos, map, 50000, 0);
	FinaliseLookaheadCheck(pos, map);

	p1->SetPointsFlagsMasked(0, points::PTF::REV, points::PTF::OOC | points::PTF::REV);
	CheckLookahead(nullptr, l, pos, map);
	CheckLookaheadResult(pos, map, 450000, 0);
	FinaliseLookaheadCheck(pos, map);

	p1->SetPointsFlagsMasked(0, points::PTF::ZERO, points::PTF::REV);
	CheckLookahead(nullptr, l, pos, map);
	CheckLookaheadResult(pos, map, 50000, 100);
	CheckLookaheadResult(pos, map, 550000, 0);
	FinaliseLookaheadCheck(pos, map);

	l.Advance(350000);
	pos = track_location(ts4, EDGE_FRONT, 300000);
	CheckLookahead(nullptr, l, pos, map);
	CheckLookaheadResult(pos, map, 0, 100);
	CheckLookaheadResult(pos, map, 200000, 0);
	FinaliseLookaheadCheck(pos, map);

	l.Advance(200000);
	pos = track_location(s4, EDGE_FRONT, 0);
	CheckLookahead(nullptr, l, pos, map, lookahead::LA_ERROR::WAITING_AT_RED_SIG, track_target_ptr(s4, EDGE_FRONT));
	CheckLookaheadResult(pos, map, 0, 0);
	FinaliseLookaheadCheck(pos, map);

	env.w->SubmitAction(action_reserve_path(*(env.w), s4, b));
	env.w->GameStep(1);
	CheckLookahead(nullptr, l, pos, map);
	CheckLookaheadResult(pos, map, 0, 100);
	CheckLookaheadResult(pos, map, 500000, 0);
	FinaliseLookaheadCheck(pos, map);
}

TEST_CASE( "lookahead/tractiontype", "Test traction types lookahead" ) {

	test_fixture_world_init_checked env(lookahead_test_str_2, true, true);

	generic_track *ts1 = env.w->FindTrackByName("TS1");
	REQUIRE(ts1 != nullptr);
	generic_track *s1 = env.w->FindTrackByName("S1");
	REQUIRE(s1 != nullptr);
	traction_type *ac = env.w->GetTractionTypeByName("AC");
	REQUIRE(ac != nullptr);
	traction_type *diesel = env.w->GetTractionTypeByName("diesel");
	REQUIRE(diesel != nullptr);

	env.w->GameStep(1);

	train *t = nullptr;
	unsigned int train_count = env.w->EnumerateTrains([&](train &t_) {
		t = &t_;
	});
	REQUIRE(train_count == 1);
	REQUIRE(t != nullptr);

	auto checklookahead = [&](const track_location &pos, bool iserror, unsigned int distance) {
		lookahead l;
		std::map<unsigned int, unsigned int> map;
		l.Init(t, pos, 0);
		if (iserror) {
			CheckLookahead(t, l, pos, map, lookahead::LA_ERROR::TRACTION_UNSUITABLE, pos.GetTrackTargetPtr());
		} else {
			CheckLookahead(t, l, pos, map);
		}
		CheckLookaheadResult(pos, map, distance, 0);
		FinaliseLookaheadCheck(pos, map);
	};

	INFO("Check 1");
	checklookahead(track_location(ts1, EDGE_FRONT, 0), true, 0);

	traction_set trs1;
	trs1.AddTractionType(ac);
	t->SetActiveTractionSet(trs1);
	INFO("Check 2");
	checklookahead(track_location(ts1, EDGE_FRONT, 0), false, 500000);
	INFO("Check 3");
	checklookahead(track_location(s1, EDGE_FRONT, 0), true, 0);

	traction_set trs2;
	trs2.AddTractionType(diesel);
	t->SetActiveTractionSet(trs2);
	INFO("Check 4");
	checklookahead(track_location(ts1, EDGE_FRONT, 0), false, 1500000);
	INFO("Check 5");
	checklookahead(track_location(s1, EDGE_FRONT, 0), false, 1500000);
}

TEST_CASE( "lookahead/serialisation", "Test lookahead serialisation and deserialisation" ) {
	test_fixture_world_init_checked env(lookahead_test_str_1);

	env.w->GameStep(1);

	routing_point *a = env.w->FindTrackByNameCast<routing_point>("A");
	REQUIRE(a != nullptr);
	generic_signal *s3 = env.w->FindTrackByNameCast<generic_signal>("S3");
	REQUIRE(s3 != nullptr);
	generic_signal *s4 = env.w->FindTrackByNameCast<generic_signal>("S4");
	REQUIRE(s4 != nullptr);
	env.w->SubmitAction(action_reserve_path(*(env.w), s3, s4));

	env.w->GameStep(1);

	auto serialise_lookahead = [&](const lookahead &l) -> std::string {
		std::string json;
		writestream wr(json);
		WriterHandler hn(wr);
		serialiser_output so(hn);
		so.json_out.StartObject();
		l.Serialise(so, env.ec);
		so.json_out.EndObject();
		return json;
	};
	auto deserialise_lookahead = [&](lookahead &l, const std::string &json) {
		rapidjson::Document dc;
		if (dc.Parse<0>(json.c_str()).HasParseError()) {
			env.ec.RegisterNewError<error_jsonparse>(json, dc.GetErrorOffset(), dc.GetParseError());
			FAIL("JSON parsing error: \n" << env.ec);
		}
		deserialiser_input di(dc, "lookahead", "lookahead", env.w.get());
		l.Deserialise(di, env.ec);
		di.PostDeserialisePropCheck(env.ec);
		if (env.ec.GetErrorCount()) { FAIL("JSON deserialisation error: \n" << env.ec); }
	};
	auto compare_lookaheads = [&](lookahead &l1, lookahead &l2, const track_location &pos) {
		std::forward_list<std::pair<unsigned int, unsigned int> > distances[2];
		std::forward_list<std::pair<lookahead::LA_ERROR, const track_target_ptr &> > errors[2];

		auto fill = [&](lookahead &l, unsigned int index) {
			l.CheckLookaheads(nullptr, pos, [&](unsigned int distance, unsigned int speed) {
				distances[index].emplace_front(distance, speed);
			}, [&](lookahead::LA_ERROR err, const track_target_ptr &piece){
				errors[index].emplace_front(err, piece);
			});
		};
		fill(l1, 0);
		fill(l2, 1);

		CHECK(distances[0] == distances[1]);
		CHECK(errors[0] == errors[1]);
	};

	auto statetest = [&](lookahead &l1, lookahead &l2, const track_location &pos) {
		INFO("Pre round-trip test");
		compare_lookaheads(l1, l2, pos);

		std::string l1json = serialise_lookahead(l1);
		std::string l2json = serialise_lookahead(l2);
		deserialise_lookahead(l2, l2json);
		std::string l2json_roundtrip = serialise_lookahead(l2);
		CHECK(l2json == l2json_roundtrip);
		CHECK(l1json == l2json_roundtrip);

		INFO("Post round-trip test");
		compare_lookaheads(l1, l2, pos);
	};

	track_location pos(a, EDGE_FRONT, 0);
	lookahead l1;
	lookahead l2;
	l1.Init(nullptr, pos, 0);
	l2.Init(nullptr, pos, 0);

	auto advance = [&](unsigned int distance) {
		l1.Advance(distance);
		l2.Advance(distance);
		AdvanceDisplacement(distance, pos);
	};

	unsigned int totaldistance = 500000;
	unsigned int step = 1000;
	unsigned int offset = 0;
	while (true) {
		INFO("State Test at offset: " << offset);
		statetest(l1, l2, pos);
		if (offset >= totaldistance) {
			break;
		}
		advance(step);
		offset += step;
	}
}
