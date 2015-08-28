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
#include "util/util.h"
#include "core/world_serialisation.h"
#include "core/track_circuit.h"
#include "core/train.h"
#include "core/signal.h"
#include "core/track_piece.h"
#include "core/track_ops.h"

TEST_CASE( "track_circuit/deserialisation", "Test basic deserialisation of track circuit name" ) {
	std::string track_test_str =
	R"({ "content" : [ )"
		R"({ "type" : "start_of_line", "name" : "A"}, )"
		R"({ "type" : "track_seg", "name" : "TS1", "track_circuit" : "T1" }, )"
		R"({ "type" : "routing_marker", "name" : "RM1", "track_circuit" : "T1" }, )"
		R"({ "type" : "track_seg", "name" : "TS2", "track_circuit" : "T2" }, )"
		R"({ "type" : "routing_marker", "name" : "RM2" }, )"
		R"({ "type" : "end_of_line", "name" : "B" } )"
	"] }";
	test_fixture_world_init_checked env(track_test_str);

	track_seg *ts1 = PTR_CHECK(env.w->FindTrackByNameCast<track_seg>("TS1"));
	track_seg *ts2 = PTR_CHECK(env.w->FindTrackByNameCast<track_seg>("TS2"));
	routing_marker *rm1 = PTR_CHECK(env.w->FindTrackByNameCast<routing_marker>("RM1"));
	routing_marker *rm2 = PTR_CHECK(env.w->FindTrackByNameCast<routing_marker>("RM2"));

	CHECK(PTR_CHECK(ts1->GetTrackCircuit())->GetName() == "T1");
	CHECK(PTR_CHECK(ts2->GetTrackCircuit())->GetName() == "T2");
	CHECK(PTR_CHECK(rm1->GetTrackCircuit())->GetName() == "T1");
	CHECK(rm2->GetTrackCircuit() == nullptr);
	CHECK(rm1->GetTrackCircuit() == ts1->GetTrackCircuit());
}

std::string tcdereservation_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "track_seg", "name" : "TS1", "length" : 50000, "track_circuit" : "T1" }, )"
	R"({ "type" : "4aspectroute", "name" : "S1", "route_restrictions" : [{ "torr" : true }] }, )"
	R"({ "type" : "track_seg", "name" : "TS2", "length" : 20000, "track_circuit" : "T2" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true, "track_circuit" : "T2" }, )"
	R"({ "type" : "track_seg", "name" : "TS3", "length" : 30000, "track_circuit" : "T3" }, )"
	R"({ "type" : "routing_marker" }, )"
	R"({ "type" : "track_seg", "name" : "TS4", "length" : 30000, "track_circuit" : "T4" }, )"

	R"({ "type" : "points", "name" : "P1" }, )"

	R"({ "type" : "track_seg", "name" : "TS5", "length" : 30000, "track_circuit" : "T5" }, )"
	R"({ "type" : "4aspectroute", "name" : "S2", "route_restrictions" : [{ "torr" : false }] }, )"
	R"({ "type" : "track_seg", "name" : "TS6", "length" : 20000, "track_circuit" : "T6" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "name" : "TS7", "length" : 20000, "track_circuit" : "T7" }, )"
	R"({ "type" : "end_of_line", "name" : "B", "end" : { "allow" : "route" } }, )"

	R"({ "type" : "track_seg", "name" : "TS8", "length" : 30000, "track_circuit" : "T4", "connect" : { "to" : "P1" } }, )"
	R"({ "type" : "end_of_line", "name" : "C" } )"
"] }";

TEST_CASE( "track_circuit/dereservation", "Test track circuit deoccupation route dereservation" ) {
	test_fixture_world_init_checked env(tcdereservation_test_str_1);

	generic_signal *s1 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S1"));
	generic_signal *s2 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S2"));
	routing_point *b = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("B"));

	auto settcstate = [&](const std::string &tcname, bool enter) {
		track_circuit *tc = env.w->track_circuits.FindOrMakeByName(tcname);
		tc->SetTCFlagsMasked(enter ? track_circuit::TCF::FORCE_OCCUPIED : track_circuit::TCF::ZERO, track_circuit::TCF::FORCE_OCCUPIED);
	};

	auto hasroute = [&](generic_track *piece, const route *r) -> bool {
		if (!piece) return false;
		bool found = false;
		piece->ReservationEnumeration([&](const route *reserved_route, EDGE r_direction, unsigned int r_index, RRF rr_flags) {
			if (reserved_route == r) {
				found = true;
			}
		}, RRF::RESERVE);
		return found;
	};

	auto checkstate = [&](generic_track *start, generic_track *end, const route *rt) {
		std::function<void(generic_track *piece)> check_piece = [&](generic_track *piece) {
			CHECK(piece != nullptr);
			if (!piece) return;
			bool found = false;
			piece->ReservationEnumeration([&](const route *reserved_route, EDGE r_direction, unsigned int r_index, RRF rr_flags) {
				if (reserved_route == rt) {
					INFO("checkstate: piece: " << piece->GetName());
					found = true;
					if (piece == end) {
						CHECK(hasroute(piece->GetConnectingPieceByIndex(r_direction, r_index).track, reserved_route) == false);
					} else {
						check_piece(piece->GetConnectingPieceByIndex(r_direction, r_index).track);
						if (piece == start) {
							CHECK(hasroute(piece->GetEdgeConnectingPiece(r_direction).track, reserved_route) == false);
						}
					}
				}
			}, RRF::RESERVE);
			CHECK(found == true);
			if (found != true) {
				WARN("Expected route to be reserved at piece " << piece);
			}
		};
		check_piece(start);
	};

	auto checkunreserved = [&](const route *rt) {
		REQUIRE(rt != nullptr);
		auto check_piece = [&](generic_track *piece) {
			bool found = false;
			piece->ReservationEnumeration([&](const route *reserved_route, EDGE r_direction, unsigned int r_index, RRF rr_flags) {
				if (reserved_route == rt) {
					found = true;
				}
			}, RRF::RESERVE);
			if (found) {
				WARN("Piece unexpectedly reserved: " << piece->GetName());
			}
			CHECK(found == false);
		};
		check_piece(rt->start.track);
		for (auto &it : rt->pieces) {
			check_piece(it.location.track);
		}
		check_piece(rt->end.track);
	};

	env.w->SubmitAction(action_reserve_path(*(env.w), s1, s2));
	env.w->SubmitAction(action_reserve_path(*(env.w), s2, b));
	env.w->GameStep(1);

	const route *s1rt = s1->GetCurrentForwardRoute();
	const route *s2rt = s2->GetCurrentForwardRoute();
	CHECK(s1rt != nullptr);
	CHECK(s2rt != nullptr);

	env.w->GameStep(1);
	CHECK(env.w->GetLogText() == "");

	settcstate("T1", true);
	INFO("Check 1");
	checkstate(s1, s2, s1rt);
	settcstate("T1", false);
	INFO("Check 2");
	checkstate(s1, s2, s1rt);

	settcstate("T2", true);
	settcstate("T2", false);
	INFO("Check 3");
	checkunreserved(s1rt);

	env.w->SubmitAction(action_reserve_path(*(env.w), s1, s2));
	env.w->GameStep(1);

	settcstate("T2", true);
	settcstate("T3", true);
	INFO("Check 4");
	checkstate(s1, s2, s1rt);
	settcstate("T3", false);
	INFO("Check 5");
	checkstate(s1, s2, s1rt);
	settcstate("T3", true);
	settcstate("T2", false);
	INFO("Check 6");
	checkstate(env.w->FindTrackByName("TS3"), s2, s1rt);
	settcstate("T4", true);
	settcstate("T4", false);
	INFO("Check 7");
	checkstate(env.w->FindTrackByName("TS3"), s2, s1rt);
	settcstate("T3", false);
	INFO("Check 8");
	checkunreserved(s1rt);

	settcstate("T6", true);
	settcstate("T6", false);
	INFO("Check 9");
	checkstate(s2, b, s2rt);

	settcstate("T7", true);
	env.w->SubmitAction(action_unreserve_track(*(env.w), *s2));
	env.w->GameStep(1);
	settcstate("T7", false);
	INFO("Check 10");
	checkunreserved(s2rt);
}

TEST_CASE( "track_circuit/reservation_state", "Test track circuit reservation state handling" ) {
	test_fixture_world_init_checked env(tcdereservation_test_str_1);
	track_circuit *t4;
	track_circuit *t5;
	track_seg *ts4;
	track_seg *ts5;
	track_seg *ts8;
	generic_signal *s1;
	generic_signal *s2;

	auto setup = [&]() {
		t4 = env.w->track_circuits.FindOrMakeByName("T4");
		t5 = env.w->track_circuits.FindOrMakeByName("T5");
		ts4 = PTR_CHECK(env.w->FindTrackByNameCast<track_seg>("TS4"));
		ts5 = PTR_CHECK(env.w->FindTrackByNameCast<track_seg>("TS5"));
		ts8 = PTR_CHECK(env.w->FindTrackByNameCast<track_seg>("TS8"));
		s1 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S1"));
		s2 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S2"));
	};

	auto test = [&](std::function<void()> RoundTrip) {
		env.w->SubmitAction(action_reserve_path(*(env.w), s1, s2));
		RoundTrip();
		CHECK(t4->IsAnyPieceReserved() == false);
		CHECK(t5->IsAnyPieceReserved() == false);

		env.w->GameStep(1);
		CHECK(t4->IsAnyPieceReserved() == true);
		CHECK(t5->IsAnyPieceReserved() == true);
		CHECK(env.w->GetLastUpdateSet().count(t4) == 1);
		CHECK(env.w->GetLastUpdateSet().count(t5) == 1);
		CHECK(env.w->GetLastUpdateSet().count(ts4) == 1);
		CHECK(env.w->GetLastUpdateSet().count(ts5) == 1);
		CHECK(env.w->GetLastUpdateSet().count(ts8) == 1);
		RoundTrip();
		CHECK(t4->IsAnyPieceReserved() == true);
		CHECK(t5->IsAnyPieceReserved() == true);

		env.w->SubmitAction(action_unreserve_track(*(env.w), *s1));
		RoundTrip();
		CHECK(t4->IsAnyPieceReserved() == true);
		CHECK(t5->IsAnyPieceReserved() == true);

		env.w->GameStep(1);
		CHECK(t4->IsAnyPieceReserved() == false);
		CHECK(t5->IsAnyPieceReserved() == false);
		CHECK(env.w->GetLastUpdateSet().count(t4) == 1);
		CHECK(env.w->GetLastUpdateSet().count(t5) == 1);
		CHECK(env.w->GetLastUpdateSet().count(ts4) == 1);
		CHECK(env.w->GetLastUpdateSet().count(ts5) == 1);
		CHECK(env.w->GetLastUpdateSet().count(ts8) == 1);
		RoundTrip();
		CHECK(t4->IsAnyPieceReserved() == false);
		CHECK(t5->IsAnyPieceReserved() == false);

		CHECK(env.w->GetLogText() == "");
	};

	ExecTestFixtureWorldWithRoundTrip(env, setup, test);
}

std::string berth_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "new_type" : "4aspectauto", "base_type" : "auto_signal", "content" : { "max_aspect" : 3 } }, )"
	R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "track_seg", "name" : "TS0", "length" : 50000, "berth" : true  }, )"
	R"({ "type" : "4aspectauto", "name" : "S0" }, )"
	R"({ "type" : "track_seg", "name" : "TS0A", "length" : 50000, "track_circuit" : "T0A", "berth" : true }, )"
	R"({ "type" : "4aspectauto", "name" : "S0A", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "name" : "TS1", "length" : 50000, "track_circuit" : "T1" }, )"
	R"({ "type" : "4aspectauto", "name" : "S1", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "name" : "TS2", "length" : 20000, "track_circuit" : "T2", "berth" : true }, )"
	R"({ "type" : "4aspectroute", "name" : "S2", "overlap_end" : true, "overlap_swingable" : true }, )"
	R"({ "type" : "track_seg", "name" : "TS3", "length" : 30000, "track_circuit" : "T3" }, )"

	R"({ "type" : "points", "name" : "P1" }, )"

	R"({ "type" : "track_seg", "name" : "TS4", "length" : 30000, "track_circuit" : "T4", "berth" : true }, )"
	R"({ "type" : "4aspectauto", "name" : "S3", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "name" : "TS5", "length" : 20000, "track_circuit" : "T5" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "name" : "TS6", "length" : 20000, "track_circuit" : "T6", "berth" : true }, )"
	R"({ "type" : "end_of_line", "name" : "B", "end" : { "allow" : "route" }, "track_circuit" : "T6" }, )"

	R"({ "type" : "track_seg", "name" : "TS7", "length" : 30000, "connect" : { "to" : "P1" }, "berth" : true }, )"
	R"({ "type" : "end_of_line", "name" : "C", "end" : { "allow" : [ "route", "overlap" ] } } )"
"] }";

TEST_CASE( "berth/step/1", "Berth stepping test no 1: basic stepping" ) {
	test_fixture_world_init_checked env(berth_test_str_1);

	generic_signal *s2 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S2"));
	generic_signal *s3 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S3"));
	generic_track *t[7];
	track_berth *b[7];
	for (unsigned int i = 0; i < sizeof(t)/sizeof(t[0]); i++) {
		INFO("Load loop: " << i);
		t[i] = PTR_CHECK(env.w->FindTrackByName(string_format("TS%d", i)));
		b[i] = t[i]->GetBerth();
	}

	PTR_CHECK(b[0]);
	PTR_CHECK(b[2]);
	PTR_CHECK(b[4]);
	PTR_CHECK(b[6]);
	CHECK(b[1] == 0);
	CHECK(b[3] == 0);
	CHECK(b[5] == 0);
	CHECK(b[0]->contents == "");
	CHECK(b[2]->contents == "");
	CHECK(b[4]->contents == "");
	CHECK(b[6]->contents == "");

	env.w->SubmitAction(action_reserve_path(*(env.w), s2, s3));
	env.w->GameStep(1);

	b[2]->contents = "test";
	PTR_CHECK(t[1]->GetTrackCircuit())->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);

	auto advance = [&](unsigned int index, unsigned int berthindex) {
		INFO("Berth advance: " << index << ", expected berth: " << berthindex);
		PTR_CHECK(t[index]->GetTrackCircuit())->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
		PTR_CHECK(t[index-1]->GetTrackCircuit())->SetTCFlagsMasked(track_circuit::TCF::ZERO, track_circuit::TCF::FORCE_OCCUPIED);
		for (unsigned int i = 0; i < sizeof(b)/sizeof(b[0]); i++) {
			INFO("Testing berth: " << i << ", expected berth: " << berthindex);
			if (i == berthindex) {
				REQUIRE(b[i] != 0);
				CHECK(b[i]->contents == "test");
			} else if (b[i]) {
				CHECK(b[i]->contents == "");
			}
		}
	};

	advance(2, 2);
	advance(3, 4);
	advance(4, 4);
	advance(5, 6);
	advance(6, 6);
}

void FillBerth(test_fixture_world &env, const std::string &name, std::string &&val) {
	INFO("FillBerth: " << name << ", " << val);
	generic_track *gt = PTR_CHECK(env.w->FindTrackByName(name));
	REQUIRE(gt->HasBerth() == true);
	PTR_CHECK(gt->GetBerth())->contents = std::move(val);
}

void SetTrackTC(test_fixture_world &env, const std::string &name, bool val) {
	INFO("SetTrackTC: " << name << ", " << val);
	generic_track *gt = PTR_CHECK(env.w->FindTrackByName(name));
	PTR_CHECK(gt->GetTrackCircuit())->SetTCFlagsMasked(val ? track_circuit::TCF::FORCE_OCCUPIED : track_circuit::TCF::ZERO, track_circuit::TCF::FORCE_OCCUPIED);
}

void CheckBerth(test_fixture_world &env, const std::string &name, const std::string &val) {
	INFO("CheckBerth: " << name << ", " << val);
	generic_track *gt = PTR_CHECK(env.w->FindTrackByName(name));
	REQUIRE(gt->HasBerth() == true);
	CHECK(PTR_CHECK(gt->GetBerth())->contents == val);
}

void SetRoute(test_fixture_world &env, const std::string &start, const std::string &end) {
	INFO("SetRoute: " << start << " -> " << end);
	routing_point *s = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>(start));
	routing_point *e = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>(end));

	std::string logtext = env.w->GetLogText();
	env.w->SubmitAction(action_reserve_path(*(env.w), s, e));
	env.w->GameStep(1);
	env.w->GameStep(10000);

	CHECK(logtext == env.w->GetLogText());
}

TEST_CASE( "berth/step/2", "Berth stepping test no 2: check no stepping when no route" ) {
	test_fixture_world_init_checked env(berth_test_str_1);

	FillBerth(env, "TS2", "test");
	SetTrackTC(env, "TS3", true);
	CheckBerth(env, "TS4", "");
	CheckBerth(env, "TS7", "");
	CheckBerth(env, "TS2", "test");
}

TEST_CASE( "berth/step/3", "Berth stepping test no 3: check stepping when route has partial TC coverage" ) {
	test_fixture_world_init_checked env(berth_test_str_1);

	generic_signal *s2 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S2"));
	routing_point *c = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("C"));

	env.w->SubmitAction(action_reserve_path(*(env.w), s2, c));
	env.w->GameStep(100000);    //give points enough time to move
	CHECK(env.w->GetLogText() == "");
	if (env.ec.GetErrorCount()) {
		FAIL("Error Collection: " << env.ec);
	}

	FillBerth(env, "TS2", "test");
	SetTrackTC(env, "TS3", true);
	CheckBerth(env, "TS7", "test");
	CheckBerth(env, "TS4", "");
	CheckBerth(env, "TS2", "");
}

TEST_CASE( "berth/step/4", "Berth stepping test no 4: check stepping when leaving bay" ) {
	test_fixture_world_init_checked env(berth_test_str_1);

	FillBerth(env, "TS0", "test");
	SetTrackTC(env, "TS0A", true);
	CheckBerth(env, "TS0A", "test");
	CheckBerth(env, "TS0", "");
}

std::string berth_test_str_2 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "track_seg", "name" : "TS1", "length" : 50000, "track_circuit" : "T1", "berth" : true  }, )"
	R"({ "type" : "4aspectroute", "name" : "S1" }, )"
	R"({ "type" : "track_seg", "name" : "TS2", "length" : 50000, "track_circuit" : "T2" }, )"

	R"({ "type" : "points", "name" : "P1", "reverse_auto_connection" : true }, )"

	R"({ "type" : "4aspectroute", "name" : "BS", "reverse_auto_connection" : true, "overlap_end_rev" : true }, )"
	R"({ "type" : "track_seg", "name" : "BTS0", "length" : 50000, "berth" : true }, )"
	R"({ "type" : "track_seg", "name" : "BTS1", "length" : 50000, "berth" : true }, )"
	R"({ "type" : "track_seg", "name" : "BTS2", "length" : 50000, "berth" : true }, )"
	R"({ "type" : "end_of_line", "name" : "B", "end" : { "allow" : [ "route", "overlap" ] } }, )"

	R"({ "type" : "track_seg", "name" : "RTS1", "length" : 30000, "track_circuit" : "RT1", "connect" : { "to" : "P1" }, "berth" : true }, )"
	R"({ "type" : "4aspectroute", "name" : "RS1", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "name" : "RTS2", "length" : 50000, "track_circuit" : "RT2", "berth" : true }, )"
	R"({ "type" : "end_of_line", "name" : "C", "end" : { "allow" : [ "route", "overlap" ] } } )"
"] }";

TEST_CASE( "berth/step/5", "Berth stepping test no 5: check stepping into multi-berth bay" ) {
	test_fixture_world_init_checked env(berth_test_str_2);

	FillBerth(env, "TS1", "test");
	SetRoute(env, "S1", "B");
	SetTrackTC(env, "TS2", true);
	CheckBerth(env, "BTS0", "");
	CheckBerth(env, "BTS1", "");
	CheckBerth(env, "BTS2", "test");
	CheckBerth(env, "TS1", "");
}

TEST_CASE( "berth/step/6", "Berth stepping test no 6: check stepping into multi-berth bay, partially filled at far end" ) {
	test_fixture_world_init_checked env(berth_test_str_2);

	FillBerth(env, "TS1", "test");
	FillBerth(env, "BTS2", "foo");
	SetRoute(env, "S1", "B");
	SetTrackTC(env, "TS2", true);
	CheckBerth(env, "BTS0", "");
	CheckBerth(env, "BTS1", "test");
	CheckBerth(env, "BTS2", "foo");
	CheckBerth(env, "TS1", "");
}

TEST_CASE( "berth/step/7", "Berth stepping test no 7: check stepping into multi-berth bay, partially filled in middle" ) {
	test_fixture_world_init_checked env(berth_test_str_2);

	FillBerth(env, "TS1", "test");
	FillBerth(env, "BTS1", "foo");
	SetRoute(env, "S1", "B");
	SetTrackTC(env, "TS2", true);
	CheckBerth(env, "BTS0", "test");
	CheckBerth(env, "BTS1", "foo");
	CheckBerth(env, "BTS2", "");
	CheckBerth(env, "TS1", "");
}

TEST_CASE( "berth/step/8", "Berth stepping test no 8: check stepping into multi-berth bay, partially filled at start" ) {
	test_fixture_world_init_checked env(berth_test_str_2);

	FillBerth(env, "TS1", "test");
	FillBerth(env, "BTS0", "foo");
	SetRoute(env, "S1", "B");
	SetTrackTC(env, "TS2", true);
	CheckBerth(env, "BTS0", "test");
	CheckBerth(env, "BTS1", "foo");
	CheckBerth(env, "BTS2", "");
	CheckBerth(env, "TS1", "");
}

TEST_CASE( "berth/step/9", "Berth stepping test no 9: check stepping into multi-berth bay, doubly partially filled at start" ) {
	test_fixture_world_init_checked env(berth_test_str_2);

	FillBerth(env, "TS1", "test");
	FillBerth(env, "BTS0", "foo");
	FillBerth(env, "BTS1", "bar");
	SetRoute(env, "S1", "B");
	SetTrackTC(env, "TS2", true);
	CheckBerth(env, "BTS0", "test");
	CheckBerth(env, "BTS1", "foo");
	CheckBerth(env, "BTS2", "bar");
	CheckBerth(env, "TS1", "");
}

TEST_CASE( "berth/step/10", "Berth stepping test no 10: check stepping into multi-berth bay, full" ) {
	test_fixture_world_init_checked env(berth_test_str_2);

	FillBerth(env, "TS1", "test");
	FillBerth(env, "BTS0", "foo");
	FillBerth(env, "BTS1", "bar");
	FillBerth(env, "BTS2", "baz");
	SetRoute(env, "S1", "B");
	SetTrackTC(env, "TS2", true);
	CheckBerth(env, "BTS0", "test");
	CheckBerth(env, "BTS1", "bar");
	CheckBerth(env, "BTS2", "baz");
	CheckBerth(env, "TS1", "");
}

std::string berth_test_str_3 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
	R"({ "type" : "typedef", "new_type" : "4aspectrouteshunt", "base_type" : "4aspectroute", "content" : { "shunt_signal" : true } }, )"
	R"({ "type" : "typedef", "new_type" : "shunt", "base_type" : "route_signal", "content" : { "shunt_signal" : true, "through" : { "allow" : "route" } } }, )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "track_seg", "name" : "TS1", "length" : 50000, "track_circuit" : "T1", "berth" : true  }, )"
	R"({ "type" : "4aspectrouteshunt", "name" : "S1" }, )"
	R"({ "type" : "track_seg", "name" : "TS2", "length" : 50000, "track_circuit" : "T2", "berth" : true  }, )"
	R"({ "type" : "shunt", "name" : "S2", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "name" : "BTS0", "length" : 50000, "berth" : true }, )"
	R"({ "type" : "end_of_line", "name" : "B", "end" : { "allow" : [ "route", "overlap" ] } } )"
"] }";

TEST_CASE( "berth/step/11", "Berth stepping test no 11: check stepping across unrelated berths" ) {
	test_fixture_world_init_checked env(berth_test_str_3);

	FillBerth(env, "TS1", "test");
	SetRoute(env, "S1", "B");
	SetTrackTC(env, "TS2", true);
	CheckBerth(env, "BTS0", "test");
	CheckBerth(env, "TS1", "");
	CheckBerth(env, "TS2", "");
}

TEST_CASE( "berth/step/12", "Berth stepping test no 12: check stepping across unrelated berths into filled berth" ) {
	test_fixture_world_init_checked env(berth_test_str_3);

	FillBerth(env, "TS1", "test");
	FillBerth(env, "BTS0", "foo");
	SetRoute(env, "S1", "B");
	SetTrackTC(env, "TS2", true);
	CheckBerth(env, "BTS0", "test");
	CheckBerth(env, "TS1", "");
	CheckBerth(env, "TS2", "");
}

std::string berth_test_str_4 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "new_type" : "shunt", "base_type" : "route_signal", "content" : { "shunt_signal" : true } }, )"
	R"({ "type" : "start_of_line", "name" : "A", "end" : { "allow" : "shunt" } }, )"
	R"({ "type" : "track_seg", "name" : "TS1", "length" : 50000, "track_circuit" : "T1", "berth" : true }, )"
	R"({ "type" : "shunt", "name" : "S1" }, )"
	R"({ "type" : "shunt", "name" : "RS1", "reverse_auto_connection" : true }, )"
	R"({ "type" : "track_seg", "name" : "TS2", "length" : 50000, "track_circuit" : "T2", "berth" : "back" }, )"
	R"({ "type" : "shunt", "name" : "RS2", "overlap_end" : true, "reverse_auto_connection" : true }, )"
	R"({ "type" : "track_seg", "name" : "BTS0", "length" : 50000, "berth" : true }, )"
	R"({ "type" : "end_of_line", "name" : "B", "end" : { "allow" : "shunt" } } )"
"] }";

TEST_CASE( "berth/step/13", "Berth stepping test no 13: check stepping across berth in opposite direction" ) {
	test_fixture_world_init_checked env(berth_test_str_4);

	FillBerth(env, "TS1", "test");
	SetRoute(env, "S1", "B");
	SetTrackTC(env, "TS2", true);
	CheckBerth(env, "BTS0", "test");
	CheckBerth(env, "TS1", "");
	CheckBerth(env, "TS2", "");
}

TEST_CASE( "berth/step/14", "Berth stepping test no 14: check stepping across berth in opposite direction into filled berth" ) {
	test_fixture_world_init_checked env(berth_test_str_4);

	FillBerth(env, "TS1", "test");
	FillBerth(env, "BTS0", "foo");
	SetRoute(env, "S1", "B");
	SetTrackTC(env, "TS2", true);
	CheckBerth(env, "BTS0", "test");
	CheckBerth(env, "TS1", "");
	CheckBerth(env, "TS2", "");
}

TEST_CASE( "berth/step/15", "Berth stepping test no 15: check stepping using berth in current direction" ) {
	test_fixture_world_init_checked env(berth_test_str_4);

	FillBerth(env, "BTS0", "test");
	SetRoute(env, "RS2", "RS1");
	SetRoute(env, "RS1", "A");
	SetTrackTC(env, "TS2", true);
	CheckBerth(env, "BTS0", "");
	CheckBerth(env, "TS1", "");
	CheckBerth(env, "TS2", "test");
	SetTrackTC(env, "TS1", true);
	CheckBerth(env, "BTS0", "");
	CheckBerth(env, "TS1", "test");
	CheckBerth(env, "TS2", "");
}

TEST_CASE( "berth/step/16", "Berth stepping test no 16: check stepping using berth in current direction into filled berth" ) {
	test_fixture_world_init_checked env(berth_test_str_4);

	FillBerth(env, "BTS0", "test");
	FillBerth(env, "TS1", "foo");
	FillBerth(env, "TS2", "bar");
	SetRoute(env, "RS2", "RS1");
	SetRoute(env, "RS1", "A");
	SetTrackTC(env, "TS2", true);
	CheckBerth(env, "BTS0", "");
	CheckBerth(env, "TS1", "foo");
	CheckBerth(env, "TS2", "test");
	SetTrackTC(env, "TS1", true);
	CheckBerth(env, "BTS0", "");
	CheckBerth(env, "TS1", "test");
	CheckBerth(env, "TS2", "");
}

std::string berth_test_str_5 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "new_type" : "shunt", "base_type" : "route_signal", "content" : { "shunt_signal" : true } }, )"
	R"({ "type" : "start_of_line", "name" : "A", "end" : { "allow" : "shunt" } }, )"
	R"({ "type" : "track_seg", "name" : "TS1", "length" : 50000, "track_circuit" : "T1", "berth" : true }, )"
	R"({ "type" : "shunt", "name" : "S1" }, )"
	R"({ "type" : "track_seg", "name" : "TS2", "length" : 50000, "track_circuit" : "T1" }, )"
	R"({ "type" : "shunt", "name" : "RS1", "reverse_auto_connection" : true }, )"
	R"({ "type" : "track_seg", "name" : "BTS0", "length" : 50000, "track_circuit" : "BT0", "berth" : true }, )"
	R"({ "type" : "track_seg", "name" : "BTS1", "length" : 50000, "track_circuit" : "BT1", "berth" : true }, )"
	R"({ "type" : "shunt", "name" : "S2", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "name" : "TS3", "length" : 50000, "track_circuit" : "T3", "berth" : true }, )"
	R"({ "type" : "end_of_line", "name" : "B", "end" : { "allow" : "shunt" } } )"
"] }";

TEST_CASE( "berth/step/17", "Berth stepping test no 17: check stepping out of non-last berth from bidirectional multi-berth section" ) {
	test_fixture_world_init_checked env(berth_test_str_5);

	FillBerth(env, "BTS0", "test");
	SetRoute(env, "S2", "B");
	SetTrackTC(env, "TS3", true);
	CheckBerth(env, "BTS0", "");
	CheckBerth(env, "BTS1", "");
	CheckBerth(env, "TS1", "");
	CheckBerth(env, "TS3", "test");
}

TEST_CASE( "berth/step/18", "Berth stepping test no 18: check stepping out of last berth from bidirectional multi-berth section, of which multiple berths are filled" ) {
	test_fixture_world_init_checked env(berth_test_str_5);

	FillBerth(env, "BTS0", "test");
	FillBerth(env, "BTS1", "foo");
	SetRoute(env, "S2", "B");
	SetTrackTC(env, "TS3", true);
	CheckBerth(env, "BTS0", "test");
	CheckBerth(env, "BTS1", "");
	CheckBerth(env, "TS1", "");
	CheckBerth(env, "TS3", "foo");
}

TEST_CASE( "track_circuit/updates", "Test basic track circuit updates" ) {
	test_fixture_world_init_checked env(tcdereservation_test_str_1);

	env.w->GameStep(1);

	env.w->GameStep(1);
	CHECK(env.w->GetLastUpdateSet().size() == 0);

	track_circuit *tc = PTR_CHECK(env.w->track_circuits.FindOrMakeByName("T1"));

	tc->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
	CHECK(env.w->GetLastUpdateSet().size() == 2);

	env.w->GameStep(1);
	CHECK(env.w->GetLastUpdateSet().size() == 0);

	tc->SetTCFlagsMasked(track_circuit::TCF::ZERO, track_circuit::TCF::FORCE_OCCUPIED);
	CHECK(env.w->GetLastUpdateSet().size() == 2);

	env.w->GameStep(1);
	CHECK(env.w->GetLastUpdateSet().size() == 0);
}

TEST_CASE( "track_circuit/deserialisation/gamestate/basicload", "Test basic gamestate loading, including track circuit/trigger creation check" ) {
	test_fixture_world env1(R"({ "game_state" : [ )"
		R"({ "type" : "track_circuit", "name" : "T1" } )"
	"] }");
	INFO("Error Collection: " << env1.ec);
	CHECK(env1.ec.GetErrorCount() == 0);
	env1.ws->DeserialiseGameState(env1.ec);
	INFO("Error Collection: " << env1.ec);
	CHECK(env1.ec.GetErrorCount() == 1);

	test_fixture_world env2(R"({ "game_state" : [ )"
		R"({ "type" : "track_train_counter_block", "name" : "T1", "force_occupied" : true } )"
	"],"
	R"("content" : [ )"
		R"({ "type" : "track_train_counter_block", "name" : "T1" } )"
	"] }");
	INFO("Error Collection: " << env2.ec);
	CHECK(env2.ec.GetErrorCount() == 0);

	track_train_counter_block *t1 = PTR_CHECK(env2.w->track_triggers.FindByName("T1"));
	CHECK(t1->GetTCFlags() == track_train_counter_block::TCF::ZERO);

	env2.ws->DeserialiseGameState(env2.ec);
	INFO("Error Collection: " << env2.ec);
	CHECK(env2.ec.GetErrorCount() == 0);

	CHECK(t1->GetTCFlags() == track_train_counter_block::TCF::FORCE_OCCUPIED);
}
