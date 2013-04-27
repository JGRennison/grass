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
	test_fixture_world env(tcdereservation_test_str_1);

	env.w.LayoutInit(env.ec);
	env.w.PostLayoutInit(env.ec);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

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
