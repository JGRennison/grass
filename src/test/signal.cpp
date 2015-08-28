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
#include "core/track.h"
#include "core/signal.h"
#include "core/traverse.h"
#include "core/track_circuit.h"
#include "core/track_ops.h"

std::string track_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "track_seg", "length" : 50000 }, )"
	R"({ "type" : "route_signal", "name" : "S1", "routeshuntsignal" : true }, )"
	R"({ "type" : "track_seg", "length" : 50000 }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true, "connect" : { "from_direction" : "back", "to" : "DS1", "to_direction" : "left_front" } }, )"
	R"({ "type" : "double_slip", "name" : "DS1", "no_track_fl_bl" : true }, )"
	R"({ "type" : "start_of_line", "name" : "C" }, )"
	R"({ "type" : "track_seg", "length" : 50000 }, )"
	R"({ "type" : "route_signal", "name" : "S3", "shunt_signal" : true, "connect" : { "from_direction" : "back", "to" : "DS1", "to_direction" : "left_back" } }, )"
	R"({ "type" : "start_of_line", "name" : "D" }, )"
	R"({ "type" : "track_seg", "length" : 50000 }, )"
	R"({ "type" : "route_signal", "name" : "S6", "routeshuntsignal" : true }, )"
	R"({ "type" : "track_seg", "length" : 50000 }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true, "through_rev" : { "deny" : "route" }, "connect" : { "from_direction" : "back", "to" : "DS1", "to_direction" : "right_back" } }, )"
	R"({ "type" : "start_of_line", "name" : "B" }, )"
	R"({ "type" : "track_seg", "length" : 50000 }, )"
	R"({ "type" : "route_signal", "name" : "S2", "routeshuntsignal" : true, "route_restrictions" : [ { "targets" : "C", "deny" : "route" } ] }, )"
	R"({ "type" : "track_seg", "length" : 50000 }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "points", "name" : "P1", "connect" : { "from_direction" : "reverse", "to" : "DS1", "to_direction" : "right_front" } }, )"
	R"({ "type" : "track_seg", "length" : 50000 }, )"
	R"({ "type" : "routing_marker", "overlap_end_rev" : true, "through" : { "deny" : "all" } }, )"
	R"({ "type" : "track_seg", "length" : 50000 }, )"
	R"({ "type" : "route_signal", "name" : "S4", "reverse_auto_connection" : true, "routeshuntsignal" : true, "through_rev" : { "deny" : "all" }, "overlap_end" : true}, )"
	R"({ "type" : "track_seg", "length" : 50000 }, )"
	R"({ "type" : "auto_signal", "name" : "S5", "reverse_auto_connection" : true }, )"
	R"({ "type" : "track_seg", "length" : 50000 }, )"
	R"({ "type" : "end_of_line", "name" : "E" } )"
"] }";

TEST_CASE( "signal/deserialisation/general", "Test basic signal and routing deserialisation" ) {

	test_fixture_world env(track_test_str_1);

	if (env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	const route_class::set shuntset = route_class::Flag(route_class::ID::SHUNT);
	const route_class::set route_set = route_class::Flag(route_class::ID::ROUTE);
	const route_class::set overlapset = route_class::Flag(route_class::ID::OVERLAP);

	start_of_line *a = dynamic_cast<start_of_line *>(env.w->FindTrackByName("A"));
	REQUIRE(a != nullptr);
	CHECK(a->GetAvailableRouteTypes(EDGE::FRONT) == RPRT(0, 0, route_class::AllNonOverlaps()));
	CHECK(a->GetAvailableRouteTypes(EDGE::BACK) == RPRT());
	CHECK(a->GetSetRouteTypes(EDGE::FRONT) == RPRT());
	CHECK(a->GetSetRouteTypes(EDGE::BACK) == RPRT());

	route_signal *s1 = dynamic_cast<route_signal *>(env.w->FindTrackByName("S1"));
	REQUIRE(s1 != nullptr);
	CHECK(s1->GetAvailableRouteTypes(EDGE::FRONT) == RPRT(shuntset | route_set | route_class::AllOverlaps(), 0, shuntset | route_set));
	CHECK(s1->GetAvailableRouteTypes(EDGE::BACK) == RPRT(0, route_class::AllNonOverlaps(), 0));
	CHECK(s1->GetSetRouteTypes(EDGE::FRONT) == RPRT());
	CHECK(s1->GetSetRouteTypes(EDGE::BACK) == RPRT());

	routing_marker *rm = dynamic_cast<routing_marker *>(env.w->FindTrackByName("#4"));
	REQUIRE(rm != nullptr);
	CHECK(rm->GetAvailableRouteTypes(EDGE::FRONT) == RPRT(0, route_class::All(), overlapset));
	CHECK(rm->GetAvailableRouteTypes(EDGE::BACK) == RPRT(0, route_class::All(), 0));
	CHECK(rm->GetSetRouteTypes(EDGE::FRONT) == RPRT());
	CHECK(rm->GetSetRouteTypes(EDGE::BACK) == RPRT());

	route_signal *s3 = dynamic_cast<route_signal *>(env.w->FindTrackByName("S3"));
	REQUIRE(s3 != nullptr);
	CHECK(s3->GetAvailableRouteTypes(EDGE::FRONT) == RPRT(shuntset | route_class::AllOverlaps(), 0, shuntset));
	CHECK(s3->GetAvailableRouteTypes(EDGE::BACK) == RPRT(0, route_class::AllNonOverlaps(), 0));
	CHECK(s3->GetSetRouteTypes(EDGE::FRONT) == RPRT());
	CHECK(s3->GetSetRouteTypes(EDGE::BACK) == RPRT());

	routing_marker *rm2 = dynamic_cast<routing_marker *>(env.w->FindTrackByName("#13"));
	REQUIRE(rm2 != nullptr);
	CHECK(rm2->GetAvailableRouteTypes(EDGE::FRONT) == RPRT(0, route_class::All(), overlapset));
	CHECK(rm2->GetAvailableRouteTypes(EDGE::BACK) == RPRT(0, route_class::All() & ~route_set, 0));
	CHECK(rm2->GetSetRouteTypes(EDGE::FRONT) == RPRT());
	CHECK(rm2->GetSetRouteTypes(EDGE::BACK) == RPRT());

	route_signal *s2 = dynamic_cast<route_signal *>(env.w->FindTrackByName("S2"));
	REQUIRE(s2 != nullptr);
	CHECK(s2->GetRouteRestrictions().GetRestrictionCount() == 1);
}

TEST_CASE( "signal/routing/general", "Test basic signal and routing connectivity and route creation" ) {
	test_fixture_world env(track_test_str_1);

	if (env.ec.GetErrorCount()) {
		WARN("Error Collection: " << env.ec);
	}
	REQUIRE(env.ec.GetErrorCount() == 0);

	env.w->LayoutInit(env.ec);

	if (env.ec.GetErrorCount()) {
		WARN("Error Collection: " << env.ec);
	}
	REQUIRE(env.ec.GetErrorCount() == 0);

	env.w->PostLayoutInit(env.ec);

	if (env.ec.GetErrorCount()) {
		WARN("Error Collection: " << env.ec);
	}
	REQUIRE(env.ec.GetErrorCount() == 0);

	route_signal *s1 = dynamic_cast<route_signal *>(env.w->FindTrackByName("S1"));
	REQUIRE(s1 != nullptr);
	route_signal *s2 = dynamic_cast<route_signal *>(env.w->FindTrackByName("S2"));
	REQUIRE(s2 != nullptr);
	route_signal *s3 = dynamic_cast<route_signal *>(env.w->FindTrackByName("S3"));
	REQUIRE(s3 != nullptr);
	route_signal *s4 = dynamic_cast<route_signal *>(env.w->FindTrackByName("S4"));
	REQUIRE(s4 != nullptr);
	auto_signal *s5 = dynamic_cast<auto_signal *>(env.w->FindTrackByName("S5"));
	REQUIRE(s5 != nullptr);
	route_signal *s6 = dynamic_cast<route_signal *>(env.w->FindTrackByName("S6"));
	REQUIRE(s6 != nullptr);

	start_of_line *a = dynamic_cast<start_of_line *>(env.w->FindTrackByName("A"));
	REQUIRE(a != nullptr);
	start_of_line *b = dynamic_cast<start_of_line *>(env.w->FindTrackByName("B"));
	REQUIRE(b != nullptr);
	start_of_line *c = dynamic_cast<start_of_line *>(env.w->FindTrackByName("C"));
	REQUIRE(c != nullptr);
	start_of_line *d = dynamic_cast<start_of_line *>(env.w->FindTrackByName("D"));
	REQUIRE(d != nullptr);
	start_of_line *e = dynamic_cast<start_of_line *>(env.w->FindTrackByName("E"));
	REQUIRE(e != nullptr);

	auto s5check = [&](unsigned int index, route_class::ID type) {
		REQUIRE(s5->GetRouteByIndex(index) != nullptr);
		CHECK(s5->GetRouteByIndex(index)->type == type);
		CHECK(s5->GetRouteByIndex(index)->end == vartrack_target_ptr<routing_point>(s4, EDGE::FRONT));
		CHECK(s5->GetRouteByIndex(index)->pieces.size() == 1);
	};
	s5check(0, route_class::ID::ROUTE);
	s5check(1, route_class::ID::OVERLAP);

	std::vector<routing_point::gmr_route_item> route_set;
	CHECK(s1->GetMatchingRoutes(route_set, a, route_class::All()) == 0);
	CHECK(s1->GetMatchingRoutes(route_set, b, route_class::All()) == 0);
	CHECK(s1->GetMatchingRoutes(route_set, c, route_class::All()) == 0);
	CHECK(s1->GetMatchingRoutes(route_set, d, route_class::All()) == 1);
	CHECK(s1->GetMatchingRoutes(route_set, e, route_class::All()) == 0);
	CHECK(s1->GetMatchingRoutes(route_set, s1, route_class::All()) == 0);

	CHECK(s2->GetMatchingRoutes(route_set, a, route_class::All()) == 0);
	CHECK(s2->GetMatchingRoutes(route_set, b, route_class::All()) == 0);
	CHECK(s2->GetMatchingRoutes(route_set, c, route_class::All()) == 1);
	CHECK(s2->GetMatchingRoutes(route_set, d, route_class::All()) == 1);
	CHECK(s2->GetMatchingRoutes(route_set, e, route_class::All()) == 0);
	CHECK(s2->GetMatchingRoutes(route_set, s2, route_class::All()) == 0);

	CHECK(s3->GetMatchingRoutes(route_set, a, route_class::All()) == 0);
	CHECK(s3->GetMatchingRoutes(route_set, b, route_class::All()) == 1);
	CHECK(s3->GetMatchingRoutes(route_set, c, route_class::All()) == 0);
	CHECK(s3->GetMatchingRoutes(route_set, d, route_class::All()) == 0);
	CHECK(s3->GetMatchingRoutes(route_set, e, route_class::All()) == 0);
	CHECK(s3->GetMatchingRoutes(route_set, s3, route_class::All()) == 0);

	CHECK(s4->GetMatchingRoutes(route_set, a, route_class::All()) == 0);
	CHECK(s4->GetMatchingRoutes(route_set, b, route_class::All()) == 2);
	CHECK(s4->GetMatchingRoutes(route_set, c, route_class::All()) == 0);
	CHECK(s4->GetMatchingRoutes(route_set, d, route_class::All()) == 0);
	CHECK(s4->GetMatchingRoutes(route_set, e, route_class::All()) == 0);
	CHECK(s4->GetMatchingRoutes(route_set, s4, route_class::All()) == 0);

	CHECK(s5->GetMatchingRoutes(route_set, a, route_class::All()) == 0);
	CHECK(s5->GetMatchingRoutes(route_set, b, route_class::All()) == 0);
	CHECK(s5->GetMatchingRoutes(route_set, c, route_class::All()) == 0);
	CHECK(s5->GetMatchingRoutes(route_set, d, route_class::All()) == 0);
	CHECK(s5->GetMatchingRoutes(route_set, e, route_class::All()) == 0);
	CHECK(s5->GetMatchingRoutes(route_set, s4, route_class::All()) == 2);
	CHECK(s5->GetMatchingRoutes(route_set, s5, route_class::All()) == 0);

	CHECK(s6->GetMatchingRoutes(route_set, a, route_class::All()) == 2);
	CHECK(s6->GetMatchingRoutes(route_set, b, route_class::All()) == 2);
	CHECK(s6->GetMatchingRoutes(route_set, c, route_class::All()) == 0);
	CHECK(s6->GetMatchingRoutes(route_set, d, route_class::All()) == 0);
	CHECK(s6->GetMatchingRoutes(route_set, e, route_class::All()) == 0);
	CHECK(s6->GetMatchingRoutes(route_set, s6, route_class::All()) == 0);

	auto overlapcheck = [&](generic_signal *s, const std::string &target) {
		routing_point *rp = dynamic_cast<routing_point *>(env.w->FindTrackByName(target));
		REQUIRE(s->GetMatchingRoutes(route_set, rp, route_class::All()) == 1);
		CHECK(route_set[0].rt->end.track == rp);
		CHECK(route_class::IsOverlap(route_set[0].rt->type));

		unsigned int overlapcount = 0;
		auto callback = [&](const route *r) {
			if (route_class::IsOverlap(r->type)) {
				overlapcount++;
			}
		};
		s->EnumerateRoutes(callback);
		CHECK(overlapcount == 1);
	};
	overlapcheck(s1, "#4");
	overlapcheck(s2, "#18");
	overlapcheck(s4, "#21");
	overlapcheck(s6, "#13");
}

std::string autosig_test_str_1 =
R"({ "content" : [ )"

	R"({ "type" : "typedef", "new_type" : "4aspectauto", "base_type" : "auto_signal", "content" : { "max_aspect" : 3 } }, )"
	R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "track_seg", "length" : 50000, "track_circuit" : "T1" }, )"
	R"({ "type" : "4aspectauto", "name" : "S1" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S1ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T2" }, )"
	R"({ "type" : "4aspectauto", "name" : "S2" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S2ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T3" }, )"
	R"({ "type" : "4aspectauto", "name" : "S3" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S3ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T4" }, )"
	R"({ "type" : "4aspectauto", "name" : "S4" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S4ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T5" }, )"
	R"({ "type" : "4aspectroute", "name" : "S5" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S5ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T6" }, )"
	R"({ "type" : "4aspectroute", "name" : "S6" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S6ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T7" }, )"
	R"({ "type" : "end_of_line", "name" : "B" } )"
"] }";

std::function<void(routing_point *, unsigned int, route_class::ID, routing_point *, routing_point *)> makechecksignal(world &w) {
	return [&w](routing_point *signal, unsigned int aspect, route_class::ID aspect_type, routing_point *aspect_target, routing_point *aspect_route_target) {
		REQUIRE(signal != nullptr);
		INFO("Signal check for: " << signal->GetName() << ", at time: " << w.GetGameTime());

		CHECK(signal->GetAspect() == aspect);
		CHECK(signal->GetAspectType() == aspect_type);
		CHECK(signal->GetAspectNextTarget() == aspect_target);
		CHECK(signal->GetAspectRouteTarget() == aspect_route_target);
	};
}

class autosig_test_class_1 {
	public:
	generic_signal *s1;
	generic_signal *s2;
	generic_signal *s3;
	generic_signal *s4;
	generic_signal *s5;
	generic_signal *s6;
	routing_point *a;
	routing_point *b;
	std::function<void(routing_point *signal, unsigned int aspect, route_class::ID aspect_type, routing_point *aspect_target, routing_point *aspect_route_target)> checksignal;

	autosig_test_class_1(world &w) {
		s1 = w.FindTrackByNameCast<generic_signal>("S1");
		s2 = w.FindTrackByNameCast<generic_signal>("S2");
		s3 = w.FindTrackByNameCast<generic_signal>("S3");
		s4 = w.FindTrackByNameCast<generic_signal>("S4");
		s5 = w.FindTrackByNameCast<generic_signal>("S5");
		s6 = w.FindTrackByNameCast<generic_signal>("S6");
		a = w.FindTrackByNameCast<routing_point>("A");
		b = w.FindTrackByNameCast<routing_point>("B");
		checksignal = makechecksignal(w);
	}
};

TEST_CASE( "signal/propagation/auto_signal", "Test basic auto_signal aspect propagation" ) {
	test_fixture_world_init_checked env(autosig_test_str_1);

	autosig_test_class_1 tenv(*(env.w));

	tenv.checksignal(tenv.a, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.b, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s1, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s2, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s3, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s4, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s5, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s6, 0, route_class::ID::NONE, 0, 0);

	env.w->GameStep(1);

	tenv.checksignal(tenv.a, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.b, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s1, 3, route_class::ID::ROUTE, tenv.s2, tenv.s2);
	tenv.checksignal(tenv.s2, 3, route_class::ID::ROUTE, tenv.s3, tenv.s3);
	tenv.checksignal(tenv.s3, 2, route_class::ID::ROUTE, tenv.s4, tenv.s4);
	tenv.checksignal(tenv.s4, 1, route_class::ID::ROUTE, tenv.s5, tenv.s5);
	tenv.checksignal(tenv.s5, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s6, 0, route_class::ID::NONE, 0, 0);

	track_circuit *s3ovlp = env.w->track_circuits.FindOrMakeByName("S3ovlp");
	s3ovlp->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->GameStep(1);

	tenv.checksignal(tenv.s1, 1, route_class::ID::ROUTE, tenv.s2, tenv.s2);
	tenv.checksignal(tenv.s2, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s3, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s4, 1, route_class::ID::ROUTE, tenv.s5, tenv.s5);
	tenv.checksignal(tenv.s5, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s6, 0, route_class::ID::NONE, 0, 0);
}

std::string signalmixture_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "track_seg", "length" : 50000, "track_circuit" : "T1" }, )"
	R"({ "type" : "4aspectroute", "name" : "S1" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S1ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T2" }, )"
	R"({ "type" : "route_signal", "name" : "S2", "shunt_signal" : true }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S2ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T3" }, )"
	R"({ "type" : "4aspectroute", "name" : "S3" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S3ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T4" }, )"
	R"({ "type" : "route_signal", "name" : "S4", "shunt_signal" : true, "end" : { "allow" : "route" } }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S4ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T5" }, )"
	R"({ "type" : "4aspectroute", "name" : "S5" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S5ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T6" }, )"
	R"({ "type" : "4aspectroute", "name" : "S6" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S6ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T7" }, )"
	R"({ "type" : "end_of_line", "name" : "B" } )"
"] }";

TEST_CASE( "signal/propagation/typemixture", "Test aspect propagation and route creation with a mixture of signal types" ) {
	test_fixture_world_init_checked env(signalmixture_test_str_1);

	autosig_test_class_1 tenv(*(env.w));

	std::vector<routing_point::gmr_route_item> route_set;
	CHECK(tenv.s1->GetMatchingRoutes(route_set, tenv.s2, route_class::All()) == 0);
	if (route_set.size()) {
		WARN("First route found has type: " << route_set[0].rt->type);
	}

	env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s1, tenv.s2));
	env.w->GameStep(1);
	CHECK(env.w->GetLogText() != "");

	env.w->ResetLogText();

	env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s2, tenv.s3));
	env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s3, tenv.s4));
	env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s4, tenv.s5));
	env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s5, tenv.s6));
	env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s6, tenv.b));

	env.w->GameStep(1);
	CHECK(env.w->GetLogText() == "");

	tenv.checksignal(tenv.a, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.b, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s1, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s2, 1, route_class::ID::SHUNT, tenv.s3, tenv.s3);
	tenv.checksignal(tenv.s3, 1, route_class::ID::ROUTE, tenv.s4, tenv.s4);
	tenv.checksignal(tenv.s4, 1, route_class::ID::SHUNT, tenv.s5, tenv.s5);
	tenv.checksignal(tenv.s5, 2, route_class::ID::ROUTE, tenv.s6, tenv.s6);
	tenv.checksignal(tenv.s6, 1, route_class::ID::ROUTE, tenv.b, tenv.b);
}

TEST_CASE( "signal/route_signal/reserveaction", "Test basic route_signal reservation action and aspect propagation" ) {
	test_fixture_world_init_checked env(autosig_test_str_1);

	autosig_test_class_1 tenv(*(env.w));

	std::vector<routing_point::gmr_route_item> out;
	unsigned int routecount = tenv.s5->GetMatchingRoutes(out, tenv.s6, route_class::All());
	REQUIRE(routecount == 1);

	env.w->SubmitAction(action_reserve_track(*(env.w), *(out[0].rt)));

	tenv.checksignal(tenv.a, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.b, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s1, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s2, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s3, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s4, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s5, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s6, 0, route_class::ID::NONE, 0, 0);

	env.w->GameStep(1);

	tenv.checksignal(tenv.a, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.b, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s1, 3, route_class::ID::ROUTE, tenv.s2, tenv.s2);
	tenv.checksignal(tenv.s2, 3, route_class::ID::ROUTE, tenv.s3, tenv.s3);
	tenv.checksignal(tenv.s3, 3, route_class::ID::ROUTE, tenv.s4, tenv.s4);
	tenv.checksignal(tenv.s4, 2, route_class::ID::ROUTE, tenv.s5, tenv.s5);
	tenv.checksignal(tenv.s5, 1, route_class::ID::ROUTE, tenv.s6, tenv.s6);
	tenv.checksignal(tenv.s6, 0, route_class::ID::NONE, 0, 0);

	routecount = tenv.s6->GetMatchingRoutes(out, tenv.b, route_class::All());
	REQUIRE(routecount == 1);
	REQUIRE(out[0].rt != nullptr);
	env.w->SubmitAction(action_reserve_track(*(env.w), *(out[0].rt)));

	env.w->GameStep(1);

	tenv.checksignal(tenv.a, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.b, 0, route_class::ID::NONE, 0, 0);
	tenv.checksignal(tenv.s1, 3, route_class::ID::ROUTE, tenv.s2, tenv.s2);
	tenv.checksignal(tenv.s2, 3, route_class::ID::ROUTE, tenv.s3, tenv.s3);
	tenv.checksignal(tenv.s3, 3, route_class::ID::ROUTE, tenv.s4, tenv.s4);
	tenv.checksignal(tenv.s4, 3, route_class::ID::ROUTE, tenv.s5, tenv.s5);
	tenv.checksignal(tenv.s5, 2, route_class::ID::ROUTE, tenv.s6, tenv.s6);
	tenv.checksignal(tenv.s6, 1, route_class::ID::ROUTE, tenv.b, tenv.b);
}

std::string approachlocking_test_str_1 =
R"({ "content" : [ )"

	R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "track_seg", "length" : 50000, "track_circuit" : "T1" }, )"
	R"({ "type" : "4aspectroute", "name" : "S1" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S1ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T2" }, )"
	R"({ "type" : "4aspectroute", "name" : "S2" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S2ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T3" }, )"
	R"({ "type" : "4aspectroute", "name" : "S3" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S3ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T4" }, )"
	R"({ "type" : "4aspectroute", "name" : "S4", "approach_locking_timeout" : 1000, "route_restrictions" : [ { "approach_locking_timeout" : 120000, "targets" : "S5" } ] }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S4ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T5", "name" : "TS5" }, )"
	R"({ "type" : "4aspectroute", "name" : "S5", "approach_locking_timeout" : [ { "routeclass" : "shunt", "timeout" : 1000 }, { "routeclass" : "route", "timeout" : 60000 } ] }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S5ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T6" }, )"
	R"({ "type" : "route_signal", "name" : "S6", "shunt_signal" : true, "end" : { "allow" : "route" }, "approach_locking_timeout" : 45000 }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S6ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T7" }, )"
	R"({ "type" : "end_of_line", "name" : "B" } )"
"] }";

TEST_CASE( "signal/approachlocking/general", "Test basic approach locking route locking, timing and parameter serialisation" ) {
	test_fixture_world_init_checked env(approachlocking_test_str_1);

	autosig_test_class_1 tenv(*(env.w));
	generic_track *t5 = PTR_CHECK(env.w->FindTrackByName("TS5"));

	env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s1, tenv.s2));
	env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s2, tenv.s3));
	env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s3, tenv.s4));
	env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s4, tenv.s5));
	env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s5, tenv.s6));
	env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s6, tenv.b));

	env.w->GameStep(1);
	CHECK(env.w->GetLogText() == "");

	auto checksignal2 = [&](routing_point *signal, unsigned int aspect, unsigned int reserved_aspect, route_class::ID aspect_type, routing_point *target, bool approachlocking) {
		tenv.checksignal(signal, aspect, aspect_type, target, target);
		INFO("Signal check for: " << signal->GetName() << ", at time: " << env.w->GetGameTime());
		CHECK(signal->GetReservedAspect() == reserved_aspect);
		generic_signal *sig = dynamic_cast<generic_signal*>(signal);
		if (sig) {
			CHECK(approachlocking == ((bool) (sig->GetSignalFlags() & GSF::APPROACH_LOCKING_MODE)));
		}
	};

	auto check_track_reserved = [&](generic_track *ts, bool should_be_reserved) {
		INFO("Reservation check for: " << ts->GetName() << ", at time: " << env.w->GetGameTime());
		reservation_count_set rcs;
		ts->ReservationTypeCount(rcs);
		bool is_reserved = rcs.route_set > 0;
		CHECK(is_reserved == should_be_reserved);
	};

	checksignal2(tenv.a, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.b, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s1, 3, 3, route_class::ID::ROUTE, tenv.s2, false);
	checksignal2(tenv.s2, 3, 3, route_class::ID::ROUTE, tenv.s3, false);
	checksignal2(tenv.s3, 3, 3, route_class::ID::ROUTE, tenv.s4, false);
	checksignal2(tenv.s4, 2, 2, route_class::ID::ROUTE, tenv.s5, false);
	checksignal2(tenv.s5, 1, 1, route_class::ID::ROUTE, tenv.s6, false);
	checksignal2(tenv.s6, 1, 1, route_class::ID::SHUNT, tenv.b, false);

	auto routecheck = [&](generic_signal *s, routing_point *backtarget, routing_point *forwardtarget, world_time timeout) {
		INFO("Signal route check for: " << s->GetName());
		unsigned int count = 0;
		s->EnumerateCurrentBackwardsRoutes([&](const route *rt){
			count++;
			CHECK(rt->start.track == backtarget);
		});
		CHECK(count == 1);
		const route *rt = s->GetCurrentForwardRoute();
		REQUIRE(rt != 0);
		CHECK(rt->approach_locking_timeout == timeout);
		CHECK(rt->end.track == forwardtarget);
	};
	routecheck(tenv.s4, tenv.s3, tenv.s5, 120000);
	routecheck(tenv.s5, tenv.s4, tenv.s6, 60000);
	routecheck(tenv.s6, tenv.s5, tenv.b, 45000);

	check_track_reserved(t5, true);

	env.w->track_circuits.FindOrMakeByName("T3")->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->track_circuits.FindOrMakeByName("T4")->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->GameStep(1);

	checksignal2(tenv.s1, 1, 1, route_class::ID::ROUTE, tenv.s2, false);
	checksignal2(tenv.s2, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s4, 2, 2, route_class::ID::ROUTE, tenv.s5, false);
	checksignal2(tenv.s5, 1, 1, route_class::ID::ROUTE, tenv.s6, false);
	checksignal2(tenv.s6, 1, 1, route_class::ID::SHUNT, tenv.b, false);

	env.w->SubmitAction(action_unreserve_track(*(env.w), *tenv.s1));
	env.w->SubmitAction(action_unreserve_track(*(env.w), *tenv.s4));
	env.w->GameStep(1);

	checksignal2(tenv.s1, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s4, 0, 2, route_class::ID::ROUTE, tenv.s5, true);
	checksignal2(tenv.s5, 1, 1, route_class::ID::ROUTE, tenv.s6, false);
	checksignal2(tenv.s6, 1, 1, route_class::ID::SHUNT, tenv.b, false);

	env.w->SubmitAction(action_unreserve_track(*(env.w), *tenv.s5));
	env.w->GameStep(1);

	checksignal2(tenv.s1, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s4, 0, 2, route_class::ID::ROUTE, tenv.s5, true);
	checksignal2(tenv.s5, 0, 1, route_class::ID::ROUTE, tenv.s6, true);
	checksignal2(tenv.s6, 1, 1, route_class::ID::SHUNT, tenv.b, false);
	check_track_reserved(t5, true);

	env.w->SubmitAction(action_unreserve_track(*(env.w), *tenv.s6));
	env.w->GameStep(1);

	checksignal2(tenv.s1, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s4, 0, 2, route_class::ID::ROUTE, tenv.s5, true);
	checksignal2(tenv.s5, 0, 1, route_class::ID::ROUTE, tenv.s6, true);
	checksignal2(tenv.s6, 0, 0, route_class::ID::NONE, 0, false);

	env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s6, tenv.b));
	env.w->track_circuits.FindOrMakeByName("T6")->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->GameStep(1);
	env.w->SubmitAction(action_unreserve_track(*(env.w), *tenv.s6));
	env.w->GameStep(1);

	checksignal2(tenv.s1, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s4, 0, 2, route_class::ID::ROUTE, tenv.s5, true);
	checksignal2(tenv.s5, 0, 1, route_class::ID::ROUTE, tenv.s6, true);
	checksignal2(tenv.s6, 0, 1, route_class::ID::SHUNT, tenv.b, true);

	//timing

	tenv.s6->EnumerateFutures([&](const future &f) {
		CHECK(f.GetTriggerTime() > 45000);
		CHECK(f.GetTriggerTime() < 45100);
		CHECK(f.GetTypeSerialisationName() == "future_action_wrapper");
	});
	CHECK(tenv.s6->HaveFutures() == true);

	env.w->GameStep(44500);

	checksignal2(tenv.s1, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s4, 0, 2, route_class::ID::ROUTE, tenv.s5, true);
	checksignal2(tenv.s5, 0, 1, route_class::ID::ROUTE, tenv.s6, true);
	checksignal2(tenv.s6, 0, 1, route_class::ID::SHUNT, tenv.b, true);

	env.w->GameStep(1000);
	env.w->GameStep(1);

	checksignal2(tenv.s1, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s4, 0, 2, route_class::ID::ROUTE, tenv.s5, true);
	checksignal2(tenv.s5, 0, 1, route_class::ID::ROUTE, tenv.s6, true);
	checksignal2(tenv.s6, 0, 0, route_class::ID::NONE, 0, false);

	env.w->GameStep(14000);

	checksignal2(tenv.s1, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s4, 0, 2, route_class::ID::ROUTE, tenv.s5, true);
	checksignal2(tenv.s5, 0, 1, route_class::ID::ROUTE, tenv.s6, true);
	checksignal2(tenv.s6, 0, 0, route_class::ID::NONE, 0, false);

	env.w->GameStep(1000);
	env.w->GameStep(1);

	checksignal2(tenv.s1, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s4, 0, 1, route_class::ID::ROUTE, tenv.s5, true);    //TODO: would 2 be better than 1?
	checksignal2(tenv.s5, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s6, 0, 0, route_class::ID::NONE, 0, false);

	env.w->GameStep(59000);

	checksignal2(tenv.s1, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s4, 0, 1, route_class::ID::ROUTE, tenv.s5, true);    //TODO: would 2 be better than 1?
	checksignal2(tenv.s5, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s6, 0, 0, route_class::ID::NONE, 0, false);
	check_track_reserved(t5, true);

	env.w->GameStep(1000);
	env.w->GameStep(1);

	checksignal2(tenv.s1, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s4, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s5, 0, 0, route_class::ID::NONE, 0, false);
	checksignal2(tenv.s6, 0, 0, route_class::ID::NONE, 0, false);
	check_track_reserved(t5, false);
}

std::string overlaptimeout_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "new_type" : "2aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 1, "route_signal" : true } }, )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "track_seg", "length" : 50000, "track_circuit" : "T1" }, )"
	R"({ "type" : "auto_signal", "name" : "S1" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S1ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T2" }, )"
	R"({ "type" : "auto_signal", "name" : "S2" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S2ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T3" }, )"
	R"({ "type" : "2aspectroute", "name" : "S3", "overlap_timeout" : 30000 }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S3ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T4" }, )"
	R"({ "type" : "2aspectroute", "name" : "S4", "route_restrictions" : [ { "overlap_timeout" : 90000, "targets" : "S4ovlpend" } ] }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S4ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true, "name" : "S4ovlpend" }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T5" }, )"
	R"({ "type" : "2aspectroute", "name" : "S5", "overlap_timeout" : 30000 }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S5ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T6" }, )"
	R"({ "type" : "2aspectroute", "name" : "S6", "overlap_timeout" : 0 }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S6ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T7" }, )"
	R"({ "type" : "end_of_line", "name" : "B" } )"
"] }";

TEST_CASE( "signal/overlap/timeout", "Test overlap timeouts" ) {
	test_fixture_world_init_checked env(overlaptimeout_test_str_1);

	autosig_test_class_1 tenv(*(env.w));

	env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s3, tenv.s4));
	env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s4, tenv.s5));
	env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s5, tenv.s6));

	env.w->GameStep(1);
	CHECK(env.w->GetLogText() == "");

	auto overlapparamcheck = [&](generic_signal *s, world_time timeout) {
		INFO("Overlap parameter check for signal: " << s->GetName());
		const route *ovlp = s->GetCurrentForwardOverlap();
		REQUIRE(ovlp != nullptr);
		CHECK(ovlp->overlap_timeout == timeout);
	};
	overlapparamcheck(tenv.s4, 90000);
	overlapparamcheck(tenv.s5, 30000);
	overlapparamcheck(tenv.s6, 0);

	auto occupytcandcancelroute = [&](std::string tc, generic_signal *s) {
		env.w->track_circuits.FindOrMakeByName(tc)->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
		env.w->SubmitAction(action_unreserve_track(*(env.w), *s));
		env.w->GameStep(1);
	};

	occupytcandcancelroute("T6", tenv.s5);
	occupytcandcancelroute("T5", tenv.s4);
	occupytcandcancelroute("T4", tenv.s3);
	CHECK(env.w->GetLogText() == "");

	//timing

	auto overlapcheck = [&](generic_signal *s, bool exists) {
		INFO("Overlap check for signal: " << s->GetName() << ", at time: " << env.w->GetGameTime());
		const route *ovlp = s->GetCurrentForwardOverlap();
		if (exists) {
			CHECK(ovlp != nullptr);
		} else {
			CHECK(ovlp == nullptr);
		}
	};

	overlapcheck(tenv.s2, true);
	overlapcheck(tenv.s3, true);
	overlapcheck(tenv.s4, true);
	overlapcheck(tenv.s5, true);
	overlapcheck(tenv.s6, true);

	env.w->GameStep(29500);

	overlapcheck(tenv.s2, true);
	overlapcheck(tenv.s3, true);
	overlapcheck(tenv.s4, true);
	overlapcheck(tenv.s5, true);
	overlapcheck(tenv.s6, true);

	env.w->GameStep(1000);
	env.w->GameStep(1);

	overlapcheck(tenv.s2, true);
	overlapcheck(tenv.s3, true);
	overlapcheck(tenv.s4, true);
	overlapcheck(tenv.s5, false);
	overlapcheck(tenv.s6, true);

	env.w->GameStep(59000);

	overlapcheck(tenv.s2, true);
	overlapcheck(tenv.s3, true);
	overlapcheck(tenv.s4, true);
	overlapcheck(tenv.s5, false);
	overlapcheck(tenv.s6, true);

	env.w->GameStep(1000);
	env.w->GameStep(1);

	overlapcheck(tenv.s2, true);
	overlapcheck(tenv.s3, true);
	overlapcheck(tenv.s4, false);
	overlapcheck(tenv.s5, false);
	overlapcheck(tenv.s6, true);

	env.w->GameStep(1000000);

	overlapcheck(tenv.s2, true);
	overlapcheck(tenv.s3, true);
	overlapcheck(tenv.s4, false);
	overlapcheck(tenv.s5, false);
	overlapcheck(tenv.s6, true);
}


std::string overlaptimeout_test_str_2 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "new_type" : "2aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 1, "route_signal" : true } }, )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "track_seg", "length" : 50000 }, )"
	R"({ "type" : "2aspectroute", "name" : "S1" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S1ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T2" }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T2", "track_triggers" : "TT1" }, )"
	R"({ "type" : "2aspectroute", "name" : "S2", "route_restrictions" : [ { "overlap_timeout" : 90000, "targets" : "S2ovlpend", "overlap_timeout_trigger" : "TT1" } ] }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S2ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true, "name" : "S2ovlpend" }, )"
	R"({ "type" : "end_of_line", "name" : "B" } )"
"] }";

TEST_CASE( "signal/overlap/tracktrigger/timeout", "Test overlap timeouts for triggers" ) {
	test_fixture_world_init_checked env(overlaptimeout_test_str_2);

	generic_signal *s1 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S1"));
	generic_signal *s2 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S2"));

	env.w->SubmitAction(action_reserve_path(*(env.w), s1, s2));
	CHECK(env.w->GetLogText() == "");
	env.w->GameStep(1);

	CHECK(PTR_CHECK(s2->GetCurrentForwardOverlap())->overlap_timeout == 90000);
	CHECK(PTR_CHECK(PTR_CHECK(s2->GetCurrentForwardOverlap())->overlap_timeout_trigger)->GetName() == "TT1");

	env.w->track_circuits.FindOrMakeByName("T2")->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->SubmitAction(action_unreserve_track(*(env.w), *s1));
	env.w->GameStep(1);

	env.w->GameStep(100000);
	env.w->GameStep(100000);
	CHECK(s2->GetCurrentForwardOverlap() != nullptr);

	env.w->track_triggers.FindOrMakeByName("TT1")->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->GameStep(1);
	CHECK((s2->GetSignalFlags() & GSF::OVERLAP_TIMEOUT_STARTED) == GSF::OVERLAP_TIMEOUT_STARTED);

	env.w->GameStep(89998);
	CHECK(s2->GetCurrentForwardOverlap() != nullptr);
	env.w->GameStep(4);
	env.w->GameStep(4);
	CHECK(s2->GetCurrentForwardOverlap() == nullptr);
}

TEST_CASE( "signal/deserialisation/flagchecks", "Test signal/route flags contradiction detection and sanity checks" ) {
	auto check = [&](const std::string &str, unsigned int errcount) {
		test_fixture_world env(str);
		if (env.ec.GetErrorCount() && env.ec.GetErrorCount() != errcount) { WARN("Error Collection: " << env.ec); }
		REQUIRE(env.ec.GetErrorCount() == errcount);
	};

	check(
	R"({ "content" : [ )"
		R"({ "type" : "route_signal", "name" : "S1", "overlap_end" : true, "end" : { "allow" : "overlap" } } )"
	"] }"
	, 0);

	check(
	R"({ "content" : [ )"
		R"({ "type" : "route_signal", "name" : "S1", "overlap_end" : true, "end" : { "deny" : "overlap" } } )"
	"] }"
	, 1);

	check(
	R"({ "content" : [ )"
		R"({ "type" : "route_signal", "name" : "S1", "start" : { "deny" : "overlap" } } )"
	"] }"
	, 1);

	check(
	R"({ "content" : [ )"
		R"({ "type" : "route_signal", "name" : "S1", "route_restrictions" : [ { "approach_control" : true, "approach_control_trigger_delay" : 100} ] } )"
	"] }"
	, 0);

	check(
	R"({ "content" : [ )"
		R"({ "type" : "route_signal", "name" : "S1", "route_restrictions" : [ { "approach_control" : false, "approach_control_trigger_delay" : 100} ] } )"
	"] }"
	, 1);
}

std::string approachcontrol_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "track_seg", "length" : 50000, "track_circuit" : "T1" }, )"
	R"({ "type" : "4aspectroute", "name" : "S1" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S1ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T3" }, )"
	R"({ "type" : "4aspectroute", "name" : "S3", "route_restrictions" : [ { "approach_control" : true } ] }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S3ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000 }, )"
	R"({ "type" : "auto_signal", "name" : "AS" }, )"
	R"({ "type" : "track_seg", "length" : 20000 }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T4" }, )"
	R"({ "type" : "4aspectroute", "name" : "S4", "route_restrictions" : [ { "approach_control_trigger_delay" : 3000 } ] }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S4ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T5" }, )"
	R"({ "type" : "4aspectroute", "name" : "S5", "route_restrictions" : [ { "targets" : "C", "approach_control" : true } ] }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S5ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"

	R"({ "type" : "points", "name" : "P1" }, )"
	R"({ "type" : "track_seg", "length" : 500000, "track_circuit" : "T6"  }, )"
	R"({ "type" : "route_signal", "name" : "S6", "shunt_signal" : true, "end" : { "allow" : "route" } }, )"
	R"({ "type" : "track_seg", "length" : 500000 }, )"
	R"({ "type" : "end_of_line", "name" : "B", "end" : { "allow" : "overlap" } }, )"

	R"({ "type" : "track_seg", "length" : 400000, "connect" : { "to" : "P1" } }, )"
	R"({ "type" : "end_of_line", "name" : "C" } )"
"] }";

TEST_CASE( "signal/approachcontrol/general", "Test basic approach control" ) {
	test_fixture_world_init_checked env(approachcontrol_test_str_1);

	generic_signal *s1 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S1"));
	generic_signal *s3 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S3"));
	generic_signal *as = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("AS"));
	generic_signal *s4 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S4"));
	generic_signal *s5 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S5"));
	generic_signal *s6 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S6"));
	routing_point *b = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("B"));

	env.w->SubmitAction(action_reserve_path(*(env.w), s1, s3));
	env.w->SubmitAction(action_reserve_path(*(env.w), s3, as));
	env.w->SubmitAction(action_reserve_path(*(env.w), s4, s5));
	env.w->SubmitAction(action_reserve_path(*(env.w), s5, s6));
	env.w->SubmitAction(action_reserve_path(*(env.w), s6, b));

	env.w->GameStep(1);

	CHECK(env.w->GetLogText() == "");

	auto checksignals = [&](unsigned int line, unsigned int s1a, unsigned int s3a, unsigned int s4a, unsigned int s5a, unsigned int s6a) {
		INFO("Signal check at time: " << env.w->GetGameTime() << ", line: " << line);
		CHECK(s1->GetAspect() == s1a);
		CHECK(s3->GetAspect() == s3a);
		CHECK(s4->GetAspect() == s4a);
		CHECK(s5->GetAspect() == s5a);
		CHECK(s6->GetAspect() == s6a);
	};


	checksignals(__LINE__, 1, 0, 0, 1, 1);

	env.w->track_circuits.FindOrMakeByName("S1ovlp")->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->track_circuits.FindOrMakeByName("T6")->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->GameStep(1);
	checksignals(__LINE__, 0, 0, 0, 0, 1);
	env.w->track_circuits.FindOrMakeByName("S1ovlp")->SetTCFlagsMasked(track_circuit::TCF::ZERO, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->track_circuits.FindOrMakeByName("T6")->SetTCFlagsMasked(track_circuit::TCF::ZERO, track_circuit::TCF::FORCE_OCCUPIED);

	env.w->track_circuits.FindOrMakeByName("T3")->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->track_circuits.FindOrMakeByName("T4")->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->GameStep(1);
	checksignals(__LINE__, 0, 1, 0, 1, 1);

	env.w->GameStep(2998);
	checksignals(__LINE__, 0, 1, 0, 1, 1);

	env.w->track_circuits.FindOrMakeByName("T3")->SetTCFlagsMasked(track_circuit::TCF::ZERO, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->track_circuits.FindOrMakeByName("T4")->SetTCFlagsMasked(track_circuit::TCF::ZERO, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->GameStep(1);
	checksignals(__LINE__, 3, 2, 0, 1, 1);

	env.w->track_circuits.FindOrMakeByName("T4")->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->GameStep(3000);
	checksignals(__LINE__, 2, 1, 2, 1, 1);
}

std::string approachcontrol_test_str_2 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "new_type" : "2aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 1, "route_signal" : true } }, )"

	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "auto_signal", "name" : "S1" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T2" }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T2", "track_triggers" : "TT1" }, )"
	R"({ "type" : "2aspectroute", "name" : "S2", "route_restrictions" : [ { "approach_control_trigger" : "TT1" } ] }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S2ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true, "name" : "S2ovlpend" }, )"
	R"({ "type" : "end_of_line", "name" : "B", "end" : { "allow" : "route" } } )"
"] }";

TEST_CASE( "signal/approachcontrol/tracktrigger", "Test approach control for triggers" ) {
	test_fixture_world_init_checked env(approachcontrol_test_str_2);

	generic_signal *s2 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S2"));
	routing_point *b = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("B"));

	auto test = [&](std::string tag) {
		INFO(tag);

		env.w->SubmitAction(action_reserve_path(*(env.w), s2, b));
		CHECK(env.w->GetLogText() == "");
		env.w->GameStep(1);
		CHECK(s2->GetAspect() == 0);

		env.w->track_circuits.FindOrMakeByName("T2")->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
		env.w->GameStep(1);

		env.w->GameStep(100000);
		env.w->GameStep(100000);
		CHECK(s2->GetAspect() == 0);

		env.w->track_triggers.FindOrMakeByName("TT1")->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
		env.w->GameStep(1);
		CHECK(s2->GetAspect() == 1);
	};

	test("Initial test");

	env.w->track_circuits.FindOrMakeByName("T2")->SetTCFlagsMasked(track_circuit::TCF::ZERO, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->track_triggers.FindOrMakeByName("TT1")->SetTCFlagsMasked(track_circuit::TCF::ZERO, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->GameStep(1);
	//Check that signal remins at aspect of 1 when trigger unset
	CHECK(s2->GetAspect() == 1);

	env.w->SubmitAction(action_unreserve_track(*(env.w), *s2));
	env.w->GameStep(1);
	CHECK(s2->GetAspect() == 0);

	test("Test again after unsetting trigger");
}

std::string approachcontrol_test_str_3 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
	R"({ "type" : "typedef", "new_type" : "4aspectauto", "base_type" : "auto_signal", "content" : { "max_aspect" : 3 } }, )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "4aspectauto", "name" : "S0" }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T0" }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T1" }, )"
	R"({ "type" : "4aspectauto", "name" : "S1" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T2" }, )"
	R"({ "type" : "4aspectroute", "name" : "S2", "route_end_restrictions" : [ { "approach_control_if_noforward_route" : true } ] }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S2ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true, "name" : "S2ovlpend" }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T3" }, )"
	R"({ "type" : "end_of_line", "name" : "B", "end" : { "allow" : "route" } } )"
"] }";

TEST_CASE( "signal/approachcontrol/onlyifnoforwardroute", "Test approach control only if no forward route mode" ) {
	test_fixture_world_init_checked env(approachcontrol_test_str_3);

	generic_signal *s1 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S1"));
	generic_signal *s2 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S2"));
	routing_point *b = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("B"));
	track_circuit *t0 = env.w->track_circuits.FindOrMakeByName("T0");
	track_circuit *t1 = env.w->track_circuits.FindOrMakeByName("T1");
	track_circuit *t3 = env.w->track_circuits.FindOrMakeByName("T3");

	auto test_aspects = [&](unsigned int s1aspect, unsigned int s2aspect, const std::string &test_name) {
		INFO(test_name);
		INFO(s2->GetCurrentForwardRoute());
		CHECK(s1->GetAspect() == s1aspect);
		CHECK(s2->GetAspect() == s2aspect);
	};

	env.w->GameStep(1);
	test_aspects(0, 0, "Test 1"); // No route set from S2, approach control should be on

	t1->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->GameStep(1);
	test_aspects(1, 0, "Test 2"); // Train has approached S1, approach control should clear S1

	t1->SetTCFlagsMasked(track_circuit::TCF::ZERO, track_circuit::TCF::FORCE_OCCUPIED);

	t3->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->SubmitAction(action_reserve_path(*(env.w), s2, b));
	env.w->GameStep(1);
	CHECK(env.w->GetLogText() == "");
	test_aspects(1, 0, "Test 3"); // S2 now has a route set, but it's occupied, S1 should clear

	t3->SetTCFlagsMasked(track_circuit::TCF::ZERO, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->GameStep(1);
	test_aspects(2, 1, "Test 4"); // S2 now has a route set, and it's clear, S1 should clear to aspect 2

	env.w->SubmitAction(action_unreserve_track(*(env.w), *s2));
	env.w->GameStep(1);
	CHECK(env.w->GetLogText() == "");
	test_aspects(0, 0, "Test 5"); // No route set from S2, approach control should be on, even though the aspect was non-zero before

	t0->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
	t3->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
	env.w->SubmitAction(action_reserve_path(*(env.w), s2, b));
	env.w->GameStep(1);
	CHECK(env.w->GetLogText() == "");
	env.w->SubmitAction(action_unreserve_track(*(env.w), *s2));
	env.w->GameStep(1);
	test_aspects(0, 0, "Test 6"); // Repeat test 5, only this time occupy a TC in front of S1, and the route from S2, before unreserving from S2
	CHECK((s2->GetSignalFlags() & GSF::APPROACH_LOCKING_MODE) == GSF::ZERO); // Check that approach locking didn't get turned on
}

std::string callon_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "track_seg", "length" : 50000, "track_circuit" : "T1" }, )"
	R"({ "type" : "4aspectroute", "name" : "S1" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S1ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T2" }, )"
	R"({ "type" : "4aspectroute", "name" : "S2", "start" : { "allow" : "call_on" }, "route_restrictions" : [ { "apply_only" : "call_on", "exit_signal_control" : true } ] }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S2ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T3" }, )"
	R"({ "type" : "4aspectroute", "name" : "S3", "end" : { "allow" : "call_on" } }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S3ovlp" }, )"
	R"({ "type" : "end_of_line", "name" : "B", "end" : { "allow" : "overlap" } } )"
"] }";

TEST_CASE( "signal/callon/general", "Test call-on routes" ) {
	test_fixture_world_init_checked env(callon_test_str_1);

	generic_signal *s1 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S1"));
	generic_signal *s2 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S2"));
	generic_signal *s3 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S3"));
	routing_point *b = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("B"));

	auto settcstate = [&](const std::string &tcname, bool enter) {
		track_circuit *tc = env.w->track_circuits.FindOrMakeByName(tcname);
		tc->SetTCFlagsMasked(enter ? track_circuit::TCF::FORCE_OCCUPIED : track_circuit::TCF::ZERO, track_circuit::TCF::FORCE_OCCUPIED);
	};

	auto checkaspects = [&](unsigned int s1asp, route_class::ID s1type, unsigned int s2asp, route_class::ID s2type, unsigned int s3asp, route_class::ID s3type) {
		CHECK(s1asp == s1->GetAspect());
		CHECK(s1type == s1->GetAspectType());
		CHECK(s2asp == s2->GetAspect());
		CHECK(s2type == s2->GetAspectType());
		CHECK(s3asp == s3->GetAspect());
		CHECK(s3type == s3->GetAspectType());
	};

	env.w->SubmitAction(action_reserve_path(*(env.w), s1, s2));
	env.w->SubmitAction(action_reserve_path(*(env.w), s2, s3));
	env.w->SubmitAction(action_reserve_path(*(env.w), s3, b));
	env.w->GameStep(1);

	checkaspects(3, route_class::ID::ROUTE, 2, route_class::ID::ROUTE, 1, route_class::ID::ROUTE);

	settcstate("T3", true);
	env.w->GameStep(1);

	checkaspects(1, route_class::ID::ROUTE, 0, route_class::ID::NONE, 1, route_class::ID::ROUTE);

	env.w->SubmitAction(action_unreserve_track(*(env.w), *s2));
	env.w->GameStep(1);

	env.w->SubmitAction(action_reserve_path(*(env.w), s2, s3));
	env.w->GameStep(1);

	CHECK(env.w->GetLogText() != "");
	if (s2->GetCurrentForwardRoute()) {
		FAIL(s2->GetCurrentForwardRoute()->type);
	}

	env.w->ResetLogText();

	settcstate("T3", false);    //do this to avoid triggering approach locking
	env.w->SubmitAction(action_unreserve_track(*(env.w), *s3));
	env.w->GameStep(1);
	settcstate("T3", true);

	env.w->SubmitAction(action_reserve_path(*(env.w), s2, s3));
	env.w->GameStep(1);

	CHECK(env.w->GetLogText() == "");
	REQUIRE(s2->GetCurrentForwardRoute() != nullptr);
	CHECK(s2->GetCurrentForwardRoute()->type == route_class::ID::CALLON);

	checkaspects(1, route_class::ID::ROUTE, 0, route_class::ID::NONE, 0, route_class::ID::NONE);

	settcstate("T2", true);
	env.w->GameStep(1);

	checkaspects(0, route_class::ID::NONE, 1, route_class::ID::CALLON, 0, route_class::ID::NONE);

	env.w->SubmitAction(action_reserve_path(*(env.w), s3, b));
	env.w->GameStep(1);

	CHECK(env.w->GetLogText() != "");
	if (s3->GetCurrentForwardRoute()) {
		FAIL(s3->GetCurrentForwardRoute()->type);
	}
}

TEST_CASE( "signal/overlap/general", "Test basic overlap assignment" ) {
	test_fixture_world_init_checked env(
		R"({ "content" : [ )"
			R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
			R"({ "type" : "start_of_line", "name" : "A" }, )"
			R"({ "type" : "4aspectroute", "name" : "S1" }, )"
			R"({ "type" : "track_seg", "length" : 30000 }, )"
			R"({ "type" : "4aspectroute", "name" : "S2" }, )"
			R"({ "type" : "track_seg", "length" : 20000 }, )"
			R"({ "type" : "end_of_line", "name" : "B", "end" : { "allow" : "overlap" } } )"
		"] }"
	);

	generic_signal *s1 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S1"));
	generic_signal *s2 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S2"));

	env.w->SubmitAction(action_reserve_path(*(env.w), s1, s2));
	env.w->GameStep(1);
	CHECK(env.w->GetLogText() == "");

	CHECK(PTR_CHECK(s2->GetCurrentForwardOverlap())->type == route_class::ID::OVERLAP);
}

TEST_CASE( "signal/overlap/nooverlapflag", "Test signal no overlap flag" ) {
	test_fixture_world_init_checked env(
		R"({ "content" : [ )"
			R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
			R"({ "type" : "start_of_line", "name" : "A" }, )"
			R"({ "type" : "4aspectroute", "name" : "S1" }, )"
			R"({ "type" : "track_seg", "length" : 30000 }, )"
			R"({ "type" : "4aspectroute", "name" : "S2", "nooverlap" : true }, )"
			R"({ "type" : "track_seg", "length" : 20000 }, )"
			R"({ "type" : "end_of_line", "name" : "B" } )"
		"] }"
	);

	generic_signal *s1 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S1"));
	generic_signal *s2 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S2"));

	env.w->SubmitAction(action_reserve_path(*(env.w), s1, s2));
	env.w->GameStep(1);
	CHECK(env.w->GetLogText() == "");

	CHECK(s2->GetCurrentForwardOverlap() == nullptr);
}

TEST_CASE( "signal/overlap/missingoverlap", "Test that a missing overlap triggers an error" ) {
	test_fixture_world env(
		R"({ "content" : [ )"
			R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
			R"({ "type" : "start_of_line", "name" : "A" }, )"
			R"({ "type" : "4aspectroute", "name" : "S1" }, )"
			R"({ "type" : "track_seg", "length" : 30000 }, )"
			R"({ "type" : "4aspectroute", "name" : "S2" }, )"
			R"({ "type" : "track_seg", "length" : 20000 }, )"
			R"({ "type" : "end_of_line", "name" : "B" } )"
		"] }"
	);
	env.w->LayoutInit(env.ec);
	CHECK(env.ec.GetErrorCount() == 0);
	env.w->PostLayoutInit(env.ec);
	CHECK(env.ec.GetErrorCount() > 0);
}

TEST_CASE( "signal/overlap/alt", "Test basic alternative overlap assignment" ) {
	test_fixture_world_init_checked env(
		R"({ "content" : [ )"
			R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
			R"({ "type" : "start_of_line", "name" : "A" }, )"
			R"({ "type" : "4aspectroute", "name" : "S1", "route_restrictions" : [ { "targets" : "S2" , "overlap" : "alt_overlap_1" } ] }, )"
			R"({ "type" : "track_seg", "length" : 30000 }, )"
			R"({ "type" : "4aspectroute", "name" : "S2" }, )"
			R"({ "type" : "track_seg", "length" : 20000 }, )"
			R"({ "type" : "end_of_line", "name" : "B", "end" : { "allow" : "alt_overlap_1" } } )"
		"] }"
	);

	generic_signal *s1 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S1"));
	generic_signal *s2 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S2"));

	env.w->SubmitAction(action_reserve_path(*(env.w), s1, s2));
	env.w->GameStep(1);
	CHECK(env.w->GetLogText() == "");

	CHECK(PTR_CHECK(s2->GetCurrentForwardOverlap())->type == route_class::ID::ALTOVERLAP1);
}

TEST_CASE( "signal/overlap/missingaltoverlap", "Test that a missing alternative overlap triggers an error" ) {
	test_fixture_world env(
		R"({ "content" : [ )"
			R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
			R"({ "type" : "start_of_line", "name" : "A" }, )"
			R"({ "type" : "4aspectroute", "name" : "S1", "route_restrictions" : [ { "targets" : "S2" , "overlap" : "alt_overlap_1" } ] }, )"
			R"({ "type" : "track_seg", "length" : 30000 }, )"
			R"({ "type" : "4aspectroute", "name" : "S2" }, )"
			R"({ "type" : "track_seg", "length" : 20000 }, )"
			R"({ "type" : "end_of_line", "name" : "B", "end" : { "allow" : "overlap" }  } )"
		"] }"
	);
	env.w->LayoutInit(env.ec);
	CHECK(env.ec.GetErrorCount() == 0);
	env.w->PostLayoutInit(env.ec);
	CHECK(env.ec.GetErrorCount() > 0);
}

TEST_CASE( "signal/overlap/multi", "Test multiple overlap types" ) {
	test_fixture_world_init_checked env(
		R"({ "content" : [ )"
			R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
			R"({ "type" : "start_of_line", "name" : "A" }, )"
			R"({ "type" : "4aspectroute", "name" : "S1", "start" : { "allow" : "call_on" }, "route_restrictions" : [ { "apply_only" : "call_on" , "overlap" : "alt_overlap_1" } ] }, )"
			R"({ "type" : "track_seg", "length" : 30000 }, )"
			R"({ "type" : "4aspectroute", "name" : "S2", "end" : { "allow" : "call_on" } }, )"
			R"({ "type" : "track_seg", "length" : 20000 }, )"
			R"({ "type" : "routing_marker", "name" : "C", "end" : { "allow" : "alt_overlap_1" } }, )"
			R"({ "type" : "track_seg", "length" : 20000 }, )"
			R"({ "type" : "end_of_line", "name" : "B",  "end" : { "allow" : "overlap" } } )"
		"] }"
	);

	generic_signal *s1 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S1"));
	generic_signal *s2 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S2"));

	env.w->SubmitAction(action_reserve_path(*(env.w), s1, s2).SetAllowedRouteTypes(route_class::Flag(route_class::ID::ROUTE)));
	env.w->GameStep(1);
	CHECK(env.w->GetLogText() == "");
	CHECK(PTR_CHECK(s2->GetCurrentForwardOverlap())->type == route_class::ID::OVERLAP);
	CHECK(PTR_CHECK(s2->GetCurrentForwardOverlap())->end.track->GetName() == "B");

	env.w->SubmitAction(action_unreserve_track(*(env.w), *s1));
	env.w->GameStep(1);

	env.w->SubmitAction(action_reserve_path(*(env.w), s1, s2).SetAllowedRouteTypes(route_class::Flag(route_class::ID::CALLON)));
	env.w->GameStep(1);
	CHECK(env.w->GetLogText() == "");
	CHECK(PTR_CHECK(s2->GetCurrentForwardOverlap())->type == route_class::ID::ALTOVERLAP1);
	CHECK(PTR_CHECK(s2->GetCurrentForwardOverlap())->end.track->GetName() == "C");
}

TEST_CASE( "route/restrictions/end", "Test route end restrictions" ) {
	test_fixture_world_init_checked env(
		R"({ "content" : [ )"
			R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
			R"({ "type" : "start_of_line", "name" : "A" }, )"
			R"({ "type" : "4aspectroute", "name" : "S1", "start" : { "allow" : "call_on" } }, )"
			R"({ "type" : "track_seg", "length" : 30000 }, )"
			R"({ "type" : "4aspectroute", "name" : "S2", "start" : { "allow" : "shunt" }, "end" : { "allow" : "call_on" }, "route_end_restrictions" : { "route_start" : "S1", "apply_only" : "call_on" , "overlap" : "alt_overlap_1" } }, )"
			R"({ "type" : "track_seg", "length" : 20000 }, )"
			R"({ "type" : "routing_marker", "name" : "C", "end" : { "allow" : "alt_overlap_1" } }, )"
			R"({ "type" : "track_seg", "length" : 20000 }, )"
			R"({ "type" : "end_of_line", "name" : "B",  "end" : { "allow" : "overlap" }, "route_end_restrictions" : { "route_start" : "nonexistant", "deny" : "shunt" } } )"
		"] }"
	);

	generic_signal *s1 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S1"));
	generic_signal *s2 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S2"));
	routing_point *b = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("B"));

	env.w->SubmitAction(action_reserve_path(*(env.w), s1, s2).SetAllowedRouteTypes(route_class::Flag(route_class::ID::ROUTE)));
	env.w->GameStep(1);
	CHECK(env.w->GetLogText() == "");
	CHECK(PTR_CHECK(s2->GetCurrentForwardOverlap())->type == route_class::ID::OVERLAP);
	CHECK(PTR_CHECK(s2->GetCurrentForwardOverlap())->end.track->GetName() == "B");

	env.w->SubmitAction(action_unreserve_track(*(env.w), *s1));
	env.w->GameStep(1);

	env.w->SubmitAction(action_reserve_path(*(env.w), s1, s2).SetAllowedRouteTypes(route_class::Flag(route_class::ID::CALLON)));
	env.w->GameStep(1);
	CHECK(env.w->GetLogText() == "");
	CHECK(PTR_CHECK(s2->GetCurrentForwardOverlap())->type == route_class::ID::ALTOVERLAP1);
	CHECK(PTR_CHECK(s2->GetCurrentForwardOverlap())->end.track->GetName() == "C");

	env.w->SubmitAction(action_unreserve_track(*(env.w), *s1));
	env.w->GameStep(1);

	env.w->SubmitAction(action_reserve_path(*(env.w), s2, b).SetAllowedRouteTypes(route_class::Flag(route_class::ID::SHUNT)));
	env.w->GameStep(1);
	CHECK(env.w->GetLogText() == "");
}

TEST_CASE( "signal/updates", "Test signal state and reservation state change updates" ) {
	test_fixture_world_init_checked env(autosig_test_str_1);

	autosig_test_class_1 tenv(*(env.w));
	track_circuit *s5ovlp = env.w->track_circuits.FindOrMakeByName("S5ovlp");
	track_circuit *s6ovlp = env.w->track_circuits.FindOrMakeByName("S6ovlp");
	track_circuit *t6 = env.w->track_circuits.FindOrMakeByName("T6");
	track_circuit *t7 = env.w->track_circuits.FindOrMakeByName("T7");

	env.w->GameStep(1);

	env.w->GameStep(1);
	CHECK(env.w->GetLastUpdateSet().size() == 0);
	CHECK(s5ovlp->IsAnyPieceReserved() == true);
	CHECK(s6ovlp->IsAnyPieceReserved() == false);
	CHECK(t6->IsAnyPieceReserved() == false);
	CHECK(t7->IsAnyPieceReserved() == false);

	env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s5, tenv.s6));
	env.w->GameStep(1);
	CHECK(env.w->GetLastUpdateSet().size() == 11);    // 5 pieces on route, overlap (2), 2 preceding signals and track circuits S6ovlp and T6
	CHECK(s5ovlp->IsAnyPieceReserved() == true);
	CHECK(s6ovlp->IsAnyPieceReserved() == true);
	CHECK(t6->IsAnyPieceReserved() == true);
	CHECK(t7->IsAnyPieceReserved() == false);

	env.w->GameStep(1);
	CHECK(env.w->GetLastUpdateSet().size() == 0);

	env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s6, tenv.b));
	env.w->GameStep(1);
	CHECK(env.w->GetLastUpdateSet().size() == 8);    // 5 pieces on route, 2 preceding signals and track circuit T7
	CHECK(s5ovlp->IsAnyPieceReserved() == true);
	CHECK(s6ovlp->IsAnyPieceReserved() == true);
	CHECK(t6->IsAnyPieceReserved() == true);
	CHECK(t7->IsAnyPieceReserved() == true);

	env.w->GameStep(1);
	CHECK(env.w->GetLastUpdateSet().size() == 0);

	env.w->SubmitAction(action_unreserve_track(*(env.w), *tenv.s6));
	env.w->GameStep(1);
	CHECK(env.w->GetLastUpdateSet().size() == 8);    // 5 pieces on route, 2 preceding signals and track circuit T7
	CHECK(s5ovlp->IsAnyPieceReserved() == true);
	CHECK(s6ovlp->IsAnyPieceReserved() == true);
	CHECK(t6->IsAnyPieceReserved() == true);
	CHECK(t7->IsAnyPieceReserved() == false);
}

TEST_CASE( "signal/propagation/repeater", "Test aspect propagation and route creation with aspected and non-aspected repeater signals") {
	auto test = [&](bool nonaspected) {
		INFO("Test: " << (nonaspected ? "non-" : "") << "aspected");
		test_fixture_world_init_checked env(
			string_format(
				R"({ "content" : [ )"
					R"({ "type" : "typedef", "new_type" : "4aspectauto", "base_type" : "auto_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
					R"({ "type" : "typedef", "new_type" : "4aspectrepeater", "base_type" : "repeater_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
					R"({ "type" : "start_of_line", "name" : "A" }, )"
					R"({ "type" : "4aspectauto", "name" : "S1" }, )"
					R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "T1" }, )"
					R"({ "type" : "4aspectrepeater", "name" : "RS2", "nonaspected" : %s }, )"
					R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "T1" }, )"
					R"({ "type" : "4aspectauto", "name" : "S3" }, )"
					R"({ "type" : "track_seg", "length" : 20000 }, )"
					R"({ "type" : "end_of_line", "name" : "B",  "end" : { "allow" : [ "overlap", "route" ] } } )"
				"] }"
				, nonaspected ? "true" : "false"
			)
		);

		CHECK(env.w->GetLogText() == "");

		generic_signal *s1 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S1"));
		generic_signal *rs2 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("RS2"));
		generic_signal *s3 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S3"));
		routing_point *b = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("B"));
		track_circuit *t1 = env.w->track_circuits.FindOrMakeByName("T1");

		env.w->GameStep(1);
		CHECK(env.w->GetLogText() == "");

		auto checksignal = makechecksignal(*(env.w));

		checksignal(s1, nonaspected ? 2 : 3, route_class::ID::ROUTE, nonaspected ? s3 : rs2, s3);
		checksignal(rs2, 2, route_class::ID::ROUTE, s3, s3);
		checksignal(s3, 1, route_class::ID::ROUTE, b, b);

		t1->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
		env.w->GameStep(1);

		checksignal(s1, 0, route_class::ID::NONE, 0, 0);
		checksignal(rs2, 2, route_class::ID::ROUTE, s3, s3);
		checksignal(s3, 1, route_class::ID::ROUTE, b, b);
	};
	test(false);
	test(true);
}

TEST_CASE( "signal/aspect/delayed", "Test delay before setting non-zero signal aspect based on route prove, clear, or set time") {
	auto test = [&](std::string testname, std::string sigparam, std::string rrparam, world_time expected_time) {
		INFO("Test: " << testname << ", Signal Parameter: " << sigparam << ", Route Restriction Parameter: " << rrparam << ", Expected Time: " << expected_time);
		test_fixture_world_init_checked env(
			string_format(
				R"({ "content" : [ )"
					R"({ "type" : "start_of_line", "name" : "A" }, )"
					R"({ "type" : "route_signal", "name" : "S1", "route_signal" : true %s, "route_restrictions" : [ { "targets" : "B" %s } ] }, )"
					R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "T1" }, )"
					R"({ "type" : "points", "name" : "P1" }, )"
					R"({ "type" : "track_seg", "length" : 20000 }, )"
					R"({ "type" : "end_of_line", "name" : "B",  "end" : { "allow" : [ "overlap", "route" ] } }, )"

					R"({ "type" : "end_of_line", "name" : "C", "connect" : { "to" : "P1" } } )"
				"] }"
				, !sigparam.empty() ? (", " + sigparam).c_str() : ""
				, !rrparam.empty() ? (", " + rrparam).c_str() : ""
			)
		);

		CHECK(env.w->GetLogText() == "");
		generic_signal *s1 = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>("S1"));
		routing_point *b = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("B"));
		generic_points *p1 = PTR_CHECK(env.w->FindTrackByNameCast<generic_points>("P1"));
		track_circuit *t1 = env.w->track_circuits.FindOrMakeByName("T1");

		t1->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
		p1->SetPointsFlagsMasked(0, generic_points::PTF::FAILED_NORM | generic_points::PTF::FAILED_REV, generic_points::PTF::FAILED_NORM | generic_points::PTF::FAILED_REV);

		world_time cumuldelay = 0;
		auto checkdelay = [&](world_time delay) {
			INFO("Test at cumulative delay: " << cumuldelay);
			world_time newcumuldelay = cumuldelay + delay;
			if (newcumuldelay > expected_time) {
				world_time initial_delay = expected_time - cumuldelay - 1;
				if (initial_delay) {
					if (initial_delay > 1) {
						env.w->GameStep(1);
					}
					env.w->GameStep(initial_delay - 1);
					cumuldelay += initial_delay;
				}
				{
					INFO("Pre-check at cumulative delay: " << cumuldelay << ", Game Time: " << env.w->GetGameTime());
					CHECK(s1->GetAspect() == 0);
				}
				env.w->GameStep(2);
				cumuldelay += 2;
				{
					INFO("Post-check at cumulative delay: " << cumuldelay << ", Game Time: " << env.w->GetGameTime());
					CHECK(s1->GetAspect() == 1);
				}
				env.w->GameStep(newcumuldelay - cumuldelay);
			} else {
				env.w->GameStep(1);
				CHECK(s1->GetAspect() == 0);
				env.w->GameStep(delay - 1);
				CHECK(s1->GetAspect() == 0);
			}
			cumuldelay = newcumuldelay;
		};

		env.w->GameStep(1000);
		CHECK(s1->GetAspect() == 0);

		env.w->SubmitAction(action_reserve_path(*(env.w), s1, b));
		CHECK(env.w->GetLogText() == "");

		checkdelay(1000);
		p1->SetPointsFlagsMasked(0, generic_points::PTF::ZERO, generic_points::PTF::FAILED_NORM | generic_points::PTF::FAILED_REV);
		checkdelay(1000);
		t1->SetTCFlagsMasked(track_circuit::TCF::ZERO, track_circuit::TCF::FORCE_OCCUPIED);
		checkdelay(20000);
		CHECK(s1->GetAspect() == 1);
	};

	auto multitest = [&](std::string paramname, unsigned int paramvalue, world_time expected_time) {
		INFO("Multi-Test: Parameter: " << paramname << ", Value: " << paramvalue << ", Expected Time: " << expected_time);
		auto mkparam = [&](unsigned int value) {
			return string_format(R"( "%s" : %d )", paramname.c_str(), value);
		};
		test("Signal", mkparam(paramvalue), "", expected_time);
		test("Restriction", "", mkparam(paramvalue), expected_time);
		test("Both", mkparam(paramvalue * 2), mkparam(paramvalue), expected_time);
	};

	multitest("route_prove_delay", 10000, 11000);
	multitest("route_clear_delay", 10000, 12000);
	multitest("route_set_delay", 10000, 10000);
	multitest("route_prove_delay", 500, 2001);
	multitest("route_set_delay", 500, 2001);
}

void SignalAspectTest(std::string allowed_aspects, aspect_mask_type expected_mask, const std::vector<unsigned int> &aspects, bool repeatermode, const std::string &conditional_aspects) {
	INFO("Allowed aspects: " << allowed_aspects << ", Repeater Mode: " << repeatermode << ", Conditional aspects: " << conditional_aspects);

	std::string targsigname = repeatermode ? "Srep" : "S0";

	unsigned int signals = aspects.size();
	std::string content =
		R"({ "content" : [ )"
		R"({ "type" : "start_of_line", "name" : "A" }, )";


	std::string default_aspectstr = ", \"allowed_aspects\" : \"0-31\"";
	std::string param_aspectstr = ", \"allowed_aspects\" : \"" + allowed_aspects + "\"";
	if (!conditional_aspects.empty()) {
		param_aspectstr += R"(, "conditional_allowed_aspects" : [ )" + conditional_aspects + "]";
	}

	for (unsigned int i = 0; i < signals; i++) {
		std::string aspectstr;
		if (i == 0) {
			if (repeatermode) {
				content += string_format(
					R"({ "type" : "auto_signal", "name" : "Spre", "route_signal" : true }, )"
					R"({ "type" : "routing_marker", "end" : { "allow" : "overlap" } }, )"
					R"({ "type" : "repeater_signal", "name" : "Srep", "route_signal" : true %s }, )"
					, param_aspectstr.c_str()
				);
				aspectstr = default_aspectstr;
			} else {
				aspectstr = param_aspectstr;
			}
		} else {
			aspectstr = default_aspectstr;
		}

		content += string_format(
			R"({ "type" : "auto_signal", "name" : "S%d", "route_signal" : true %s }, )"
			R"({ "type" : "routing_marker", "end" : { "allow" : "overlap" } }, )"
			R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "T%d" }, )"
			, i
			, aspectstr.c_str()
			, i
		);
	}
	content +=
		R"({ "type" : "end_of_line", "name" : "B",  "end" : { "allow" : [ "overlap", "route" ] } } )"
		"] }";

	test_fixture_world_init_checked env(content);
	CHECK(env.w->GetLogText() == "");
	generic_signal *targsig = PTR_CHECK(env.w->FindTrackByNameCast<generic_signal>(targsigname));
	env.w->GameStep(1);

	const route *rt = targsig->GetCurrentForwardRoute();
	REQUIRE(rt != 0);
	if (repeatermode) {
		//Signal Spre "owns" the route, it has the default aspect mask value of 3, the route should have the same value
		CHECK(rt->aspect_mask == 3);

		//Repeater signals don't have routes of their own, the route defaults value is used for aspect masking instead
		CHECK(targsig->GetRouteDefaults().aspect_mask == expected_mask);
	} else {
		//Normal signals use the route's aspect mask
		CHECK(rt->aspect_mask == expected_mask);
	}

	unsigned int currentaspect = aspects.size();
	while (currentaspect--) {    //count down from aspects.size()-1 to 0
		unsigned int expectedaspect = aspects[currentaspect];
		INFO("Current Aspect: " << currentaspect << ", Expected Aspect: " << expectedaspect);
		track_circuit *t = env.w->track_circuits.FindOrMakeByName(string_format("T%d", currentaspect));
		t->SetTCFlagsMasked(track_circuit::TCF::FORCE_OCCUPIED, track_circuit::TCF::FORCE_OCCUPIED);
		env.w->GameStep(1);
		CHECK(targsig->GetAspect() == expectedaspect);
	}
};

TEST_CASE("signal/aspect/discontinous", "Test discontinuous allowed signal aspects") {
	auto test = [&](std::string allowed_aspects, aspect_mask_type expected_mask, const std::vector<unsigned int> &aspects, bool repeatermode) {
		SignalAspectTest(allowed_aspects, expected_mask, aspects, repeatermode, "");
	};

	test("0-3", 0xF, std::vector<unsigned int> { 0, 1, 2, 3, 3 }, false);
	test("1-3", 0xE, std::vector<unsigned int> { 0, 1, 2, 3, 3 }, false);
	test("3-2", 0xC, std::vector<unsigned int> { 0, 0, 2, 3, 3 }, false);
	test("1,3,3-5", 0x3A, std::vector<unsigned int> { 0, 1, 1, 3, 4, 5 }, false);
	test("1-3,2-4,6", 0x5E, std::vector<unsigned int> { 0, 1, 2, 3, 4, 4, 6 }, false);
	test("1-31", 0xFFFFFFFE, std::vector<unsigned int> { 0, 1, 2, 3, 4, 5 }, false);

	test("1-3", 0xE, std::vector<unsigned int> { 1, 2, 3, 3, 3 }, true);
	test("3-2", 0xC, std::vector<unsigned int> { 0, 2, 3, 3, 3 }, true);
	test("1,3,3-5", 0x3A, std::vector<unsigned int> { 1, 1, 3, 4, 5, 5 }, true);
	test("1-3,2-4,6", 0x5E, std::vector<unsigned int> { 1, 2, 3, 4, 4, 6, 6 }, true);
	test("1-31", 0xFFFFFFFE, std::vector<unsigned int> { 1, 2, 3, 4, 5, 6 }, true);
}

TEST_CASE("signal/aspect/conditional", "Test conditionally allowed signal aspects") {
	auto test = [&](std::string allowed_aspects, aspect_mask_type expected_mask, const std::vector<unsigned int> &aspects, bool repeatermode, const std::string &conditional_aspects) {
		SignalAspectTest(allowed_aspects, expected_mask, aspects, repeatermode, conditional_aspects);
	};

	test("1-3", 0xE, std::vector<unsigned int> { 0, 0, 0, 0, 3 }, false, R"( { "condition" : "0-2", "allowed_aspects" : "0" } )");
	test("1-3", 0xE, std::vector<unsigned int> { 0, 1, 0, 0, 3 }, false, R"( { "condition" : "1-2", "allowed_aspects" : "0" } )");
	test("1-4", 0x1E, std::vector<unsigned int> { 0, 1, 2, 2, 4 }, false, R"( { "condition" : "2", "allowed_aspects" : "0-2" } )");
	test("1-4", 0x1E, std::vector<unsigned int> { 0, 1, 1, 1, 3 }, false, R"( { "condition" : "1,2", "allowed_aspects" : "0-1" }, { "condition" : "3", "allowed_aspects" : "0-3" } )");
	test("1-2", 0x6, std::vector<unsigned int> { 0, 1, 1, 1, 2 }, false, R"( { "condition" : "1,2", "allowed_aspects" : "0-1" }, { "condition" : "3", "allowed_aspects" : "0-3" } )");
	test("1-3", 0xE, std::vector<unsigned int> { 1, 0, 0, 3, 3 }, true, R"( { "condition" : "1-2", "allowed_aspects" : "0" } )");
	test("1-3", 0xE, std::vector<unsigned int> { 1, 0, 3, 3, 0 }, true, R"( { "condition" : "1-4", "allowed_aspects" : "3" }, { "condition" : "4", "allowed_aspects" : "4" } )");
}
