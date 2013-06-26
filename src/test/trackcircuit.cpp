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
#include "world_serialisation.h"
#include "deserialisation-test.h"
#include "world-test.h"
#include "trackcircuit.h"
#include "train.h"
#include "signal.h"
#include "track_ops.h"
#include "testutil.h"
#include "util.h"

std::string tcdereservation_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "newtype" : "4aspectroute", "basetype" : "routesignal", "content" : { "maxaspect" : 3, "routesignal" : true } }, )"
	R"({ "type" : "startofline", "name" : "A" }, )"
	R"({ "type" : "trackseg", "name" : "TS1", "length" : 50000, "trackcircuit" : "T1" }, )"
	R"({ "type" : "4aspectroute", "name" : "S1", "routerestrictions" : [{ "torr" : true }] }, )"
	R"({ "type" : "trackseg", "name" : "TS2", "length" : 20000, "trackcircuit" : "T2" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "name" : "TS3", "length" : 30000, "trackcircuit" : "T3" }, )"
	R"({ "type" : "routingmarker" }, )"
	R"({ "type" : "trackseg", "name" : "TS4", "length" : 30000, "trackcircuit" : "T4" }, )"

	R"({ "type" : "points", "name" : "P1" }, )"

	R"({ "type" : "trackseg", "name" : "TS5", "length" : 30000, "trackcircuit" : "T5" }, )"
	R"({ "type" : "4aspectroute", "name" : "S2", "routerestrictions" : [{ "torr" : false }] }, )"
	R"({ "type" : "trackseg", "name" : "TS6", "length" : 20000, "trackcircuit" : "T6" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "name" : "TS7", "length" : 20000, "trackcircuit" : "T7" }, )"
	R"({ "type" : "endofline", "name" : "B", "end" : { "allow" : "route" } }, )"

	R"({ "type" : "endofline", "name" : "C", "connect" : { "to" : "P1" } } )"
"] }";

TEST_CASE( "track_circuit/dereservation", "Test track circuit deoccupation route dereservation" ) {
	test_fixture_world_init_checked env(tcdereservation_test_str_1);

	genericsignal *s1 = PTR_CHECK(env.w.FindTrackByNameCast<genericsignal>("S1"));
	genericsignal *s2 = PTR_CHECK(env.w.FindTrackByNameCast<genericsignal>("S2"));
	routingpoint *b = PTR_CHECK(env.w.FindTrackByNameCast<routingpoint>("B"));

	auto settcstate = [&](const std::string &tcname, bool enter) {
		track_circuit *tc = env.w.FindOrMakeTrackCircuitByName(tcname);
		tc->SetTCFlagsMasked(enter ? track_circuit::TCF::FORCEOCCUPIED : track_circuit::TCF::ZERO, track_circuit::TCF::FORCEOCCUPIED);
	};

	auto hasroute = [&](generictrack *piece, const route *r) -> bool {
		if(!piece) return false;
		bool found = false;
		piece->ReservationEnumeration([&](const route *reserved_route, EDGETYPE r_direction, unsigned int r_index, RRF rr_flags) {
			if(reserved_route == r) found = true;
		}, RRF::RESERVE);
		return found;
	};

	auto checkstate = [&](generictrack *start, generictrack *end, const route *rt) {
		std::function<void(generictrack *piece)> checkpiece = [&](generictrack *piece) {
			CHECK(piece != 0);
			if(!piece) return;
			bool found = false;
			piece->ReservationEnumeration([&](const route *reserved_route, EDGETYPE r_direction, unsigned int r_index, RRF rr_flags) {
				if(reserved_route == rt) {
					SCOPED_INFO("checkstate: piece: " << piece->GetName());
					found = true;
					if(piece == end) {
						CHECK(hasroute(piece->GetConnectingPieceByIndex(r_direction, r_index).track, reserved_route) == false);
					}
					else {
						checkpiece(piece->GetConnectingPieceByIndex(r_direction, r_index).track);
						if(piece == start) {
							CHECK(hasroute(piece->GetEdgeConnectingPiece(r_direction).track, reserved_route) == false);
						}
					}
				}
			}, RRF::RESERVE);
			CHECK(found == true);
			if(found != true) {
				WARN("Expected route to be reserved at piece " << piece);
			}
		};
		checkpiece(start);
	};

	auto checkunreserved = [&](const route *rt) {
		REQUIRE(rt != 0);
		auto checkpiece = [&](generictrack *piece) {
			bool found = false;
			piece->ReservationEnumeration([&](const route *reserved_route, EDGETYPE r_direction, unsigned int r_index, RRF rr_flags) {
				if(reserved_route == rt) found = true;
			}, RRF::RESERVE);
			if(found) {
				WARN("Piece unexpectedly reserved: " << piece->GetName());
			}
			CHECK(found == false);
		};
		checkpiece(rt->start.track);
		for(auto &it : rt->pieces) checkpiece(it.location.track);
		checkpiece(rt->end.track);
	};

	env.w.SubmitAction(action_reservepath(env.w, s1, s2));
	env.w.SubmitAction(action_reservepath(env.w, s2, b));
	env.w.GameStep(1);

	const route *s1rt = s1->GetCurrentForwardRoute();
	const route *s2rt = s2->GetCurrentForwardRoute();
	CHECK(s1rt != 0);
	CHECK(s2rt != 0);

	env.w.GameStep(1);
	CHECK(env.w.GetLogText() == "");

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

	env.w.SubmitAction(action_reservepath(env.w, s1, s2));
	env.w.GameStep(1);

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
	checkstate(env.w.FindTrackByName("TS3"), s2, s1rt);
	settcstate("T4", true);
	settcstate("T4", false);
	INFO("Check 7");
	checkstate(env.w.FindTrackByName("TS3"), s2, s1rt);
	settcstate("T3", false);
	INFO("Check 8");
	checkunreserved(s1rt);

	settcstate("T6", true);
	settcstate("T6", false);
	INFO("Check 9");
	checkstate(s2, b, s2rt);

	settcstate("T7", true);
	env.w.SubmitAction(action_unreservetrack(env.w, *s2));
	env.w.GameStep(1);
	settcstate("T7", false);
	INFO("Check 10");
	checkunreserved(s2rt);
}

std::string berth_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "newtype" : "4aspectauto", "basetype" : "autosignal", "content" : { "maxaspect" : 3 } }, )"
	R"({ "type" : "typedef", "newtype" : "4aspectroute", "basetype" : "routesignal", "content" : { "maxaspect" : 3, "routesignal" : true } }, )"
	R"({ "type" : "startofline", "name" : "A" }, )"
	R"({ "type" : "trackseg", "name" : "TS0", "length" : 50000, "berth" : true  }, )"
	R"({ "type" : "4aspectauto", "name" : "S0" }, )"
	R"({ "type" : "trackseg", "name" : "TS0A", "length" : 50000, "trackcircuit" : "T0A", "berth" : true }, )"
	R"({ "type" : "4aspectauto", "name" : "S0A", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "name" : "TS1", "length" : 50000, "trackcircuit" : "T1" }, )"
	R"({ "type" : "4aspectauto", "name" : "S1", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "name" : "TS2", "length" : 20000, "trackcircuit" : "T2", "berth" : true }, )"
	R"({ "type" : "4aspectroute", "name" : "S2", "overlapend" : true, "overlapswingable" : true }, )"
	R"({ "type" : "trackseg", "name" : "TS3", "length" : 30000, "trackcircuit" : "T3" }, )"

	R"({ "type" : "points", "name" : "P1" }, )"

	R"({ "type" : "trackseg", "name" : "TS4", "length" : 30000, "trackcircuit" : "T4", "berth" : true }, )"
	R"({ "type" : "4aspectauto", "name" : "S3", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "name" : "TS5", "length" : 20000, "trackcircuit" : "T5" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "name" : "TS6", "length" : 20000, "trackcircuit" : "T6", "berth" : true }, )"
	R"({ "type" : "endofline", "name" : "B", "end" : { "allow" : "route" } }, )"

	R"({ "type" : "trackseg", "name" : "TS7", "length" : 30000, "connect" : { "to" : "P1" }, "berth" : true }, )"
	R"({ "type" : "endofline", "name" : "C", "end" : { "allow" : [ "route", "overlap" ] } } )"
"] }";

TEST_CASE( "berth/step/1", "Berth stepping test no 1: basic stepping" ) {
	test_fixture_world_init_checked env(berth_test_str_1);

	genericsignal *s2 = PTR_CHECK(env.w.FindTrackByNameCast<genericsignal>("S2"));
	genericsignal *s3 = PTR_CHECK(env.w.FindTrackByNameCast<genericsignal>("S3"));
	generictrack *t[7];
	trackberth *b[7];
	for(unsigned int i = 0; i < sizeof(t)/sizeof(t[0]); i++) {
		SCOPED_INFO("Load loop: " << i);
		t[i] = PTR_CHECK(env.w.FindTrackByName(string_format("TS%d", i)));
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

	env.w.SubmitAction(action_reservepath(env.w, s2, s3));
	env.w.GameStep(1);

	b[2]->contents = "test";
	PTR_CHECK(t[1]->GetTrackCircuit())->SetTCFlagsMasked(track_circuit::TCF::FORCEOCCUPIED, track_circuit::TCF::FORCEOCCUPIED);

	auto advance = [&](unsigned int index, unsigned int berthindex) {
		SCOPED_INFO("Berth advance: " << index << ", expected berth: " << berthindex);
		PTR_CHECK(t[index]->GetTrackCircuit())->SetTCFlagsMasked(track_circuit::TCF::FORCEOCCUPIED, track_circuit::TCF::FORCEOCCUPIED);
		PTR_CHECK(t[index-1]->GetTrackCircuit())->SetTCFlagsMasked(track_circuit::TCF::ZERO, track_circuit::TCF::FORCEOCCUPIED);
		for(unsigned int i = 0; i < sizeof(b)/sizeof(b[0]); i++) {
			SCOPED_INFO("Testing berth: " << i << ", expected berth: " << berthindex);
			if(i == berthindex) {
				REQUIRE(b[i] != 0);
				CHECK(b[i]->contents == "test");
			}
			else if(b[i]) {
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
	SCOPED_INFO("FillBerth: " << name << ", " << val);
	generictrack *gt = PTR_CHECK(env.w.FindTrackByName(name));
	REQUIRE(gt->HasBerth() == true);
	PTR_CHECK(gt->GetBerth())->contents = std::move(val);
}

void SetTrackTC(test_fixture_world &env, const std::string &name, bool val) {
	SCOPED_INFO("SetTrackTC: " << name << ", " << val);
	generictrack *gt = PTR_CHECK(env.w.FindTrackByName(name));
	PTR_CHECK(gt->GetTrackCircuit())->SetTCFlagsMasked(val ? track_circuit::TCF::FORCEOCCUPIED : track_circuit::TCF::ZERO, track_circuit::TCF::FORCEOCCUPIED);
}

void CheckBerth(test_fixture_world &env, const std::string &name, const std::string &val) {
	SCOPED_INFO("CheckBerth: " << name << ", " << val);
	generictrack *gt = PTR_CHECK(env.w.FindTrackByName(name));
	REQUIRE(gt->HasBerth() == true);
	CHECK(PTR_CHECK(gt->GetBerth())->contents == val);
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

	genericsignal *s2 = PTR_CHECK(env.w.FindTrackByNameCast<genericsignal>("S2"));
	routingpoint *c = PTR_CHECK(env.w.FindTrackByNameCast<routingpoint>("C"));

	env.w.SubmitAction(action_reservepath(env.w, s2, c));
	env.w.GameStep(100000);		//give points enough time to move
	CHECK(env.w.GetLogText() == "");
	if(env.ec.GetErrorCount()) {
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

