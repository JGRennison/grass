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
#include "test/deserialisation-test.h"
#include "test/world-test.h"
#include "test/testutil.h"
#include "core/track_ops.h"
#include "core/track.h"
#include "core/points.h"
#include "core/textpool.h"
#include "core/var.h"
#include "core/signal.h"

std::string points_move_ops_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "startofline", "name" : "A" }, )"
	R"({ "type" : "points", "name" : "P1" }, )"
	R"({ "type" : "endofline", "name" : "B" }, )"
	R"({ "type" : "endofline", "name" : "C", "connect" : { "to" : "P1" } } )"
"] }";

TEST_CASE( "track/ops/points/movement", "Test basic points movement future" ) {
	test_fixture_world_init_checked env(points_move_ops_test_str_1);
	points *p1;
	auto setup = [&]() {
		p1 = PTR_CHECK(env.w->FindTrackByNameCast<points>("P1"));
	};

	auto test = [&](std::function<void()> RoundTrip) {
		env.w->SubmitAction(action_pointsaction(*(env.w), *p1, 0, genericpoints::PTF::REV, genericpoints::PTF::REV));

		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == genericpoints::PTF::ZERO);
		env.w->GameStep(500);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::REV | genericpoints::PTF::OOC));
		env.w->GameStep(4500);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == genericpoints::PTF::REV);

		env.w->SubmitAction(action_pointsaction(*(env.w), *p1, 0, genericpoints::PTF::LOCKED, genericpoints::PTF::LOCKED));
		env.w->GameStep(50);
		env.w->SubmitAction(action_pointsaction(*(env.w), *p1, 0, genericpoints::PTF::REV, genericpoints::PTF::REV));
		env.w->GameStep(50);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::REV | genericpoints::PTF::LOCKED));
		env.w->SubmitAction(action_pointsaction(*(env.w), *p1, 0, genericpoints::PTF::ZERO, genericpoints::PTF::REV));
		env.w->GameStep(4900);
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::REV | genericpoints::PTF::LOCKED));
		REQUIRE(env.w->GetLogText() == "Points P1 not movable: Locked\n");

		env.w->ResetLogText();

		auto test_norm_rev_action = [&](bool reverse, genericpoints::PTF expected) {
			RoundTrip();
			env.w->SubmitAction(action_pointsaction(*(env.w), *p1, 0, reverse));
			env.w->GameStep(5000);
			REQUIRE(p1->GetPointsFlags(0) == expected);
			REQUIRE(env.w->GetLogText().empty());
		};

		test_norm_rev_action(true, genericpoints::PTF::REV | genericpoints::PTF::LOCKED);
		test_norm_rev_action(false, genericpoints::PTF::REV);
		test_norm_rev_action(false, genericpoints::PTF::ZERO);
		test_norm_rev_action(false, genericpoints::PTF::ZERO | genericpoints::PTF::LOCKED);
		test_norm_rev_action(false, genericpoints::PTF::ZERO | genericpoints::PTF::LOCKED);
		test_norm_rev_action(true, genericpoints::PTF::ZERO);
		test_norm_rev_action(true, genericpoints::PTF::REV);
	};

	ExecTestFixtureWorldWithRoundTrip(env, setup, test);
}

void AutoNormaliseExpectReversalTest(test_fixture_world &env, std::function<genericpoints *()> RoundTrip) {
	genericpoints *p = RoundTrip();
	REQUIRE(action_points_auto_normalise::HasFutures(p, 0) == true);
	env.w->GameStep(1950);
	p = RoundTrip();
	REQUIRE(p->GetPointsFlags(0) == (genericpoints::PTF::AUTO_NORMALISE | genericpoints::PTF::REV));
	env.w->GameStep(100);
	env.w->GameStep(1); // additional step for points action future
	p = RoundTrip();
	REQUIRE(p->GetPointsFlags(0) == (genericpoints::PTF::AUTO_NORMALISE | genericpoints::PTF::ZERO | genericpoints::PTF::OOC));
	env.w->GameStep(5000);
	p = RoundTrip();
	REQUIRE(p->GetPointsFlags(0) == (genericpoints::PTF::AUTO_NORMALISE | genericpoints::PTF::ZERO));
}

std::string catchpoints_ops_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "startofline", "name" : "A" }, )"
	R"({ "type" : "routesignal", "routesignal" : true, "name" : "S1" }, )"
	R"({ "type" : "catchpoints", "name" : "P1" }, )"
	R"({ "type" : "endofline", "name" : "B" } )"
"] }";

TEST_CASE( "track/ops/catchpoints", "Test catchpoints movement on reservation" ) {
	test_fixture_world_init_checked env(catchpoints_ops_test_str_1);
	catchpoints *p1;
	routesignal *s1;
	routingpoint *b;
	auto setup = [&]() {
		p1 = PTR_CHECK(env.w->FindTrackByNameCast<catchpoints>("P1"));
		s1 = PTR_CHECK(env.w->FindTrackByNameCast<routesignal>("S1"));
		b = PTR_CHECK(env.w->FindTrackByNameCast<routingpoint>("B"));
	};

	auto test = [&](std::function<void()> RoundTrip) {
		auto expect_auto_reverse = [&](const std::string &subtest_name) {
			INFO("Subtest: " + subtest_name);
			AutoNormaliseExpectReversalTest(env, [&]() -> genericpoints* {
				RoundTrip();
				return p1;
			});
		};

		env.w->SubmitAction(action_reservepath(*(env.w), s1, b));

		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::AUTO_NORMALISE | genericpoints::PTF::ZERO));
		env.w->GameStep(501);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::AUTO_NORMALISE | genericpoints::PTF::REV | genericpoints::PTF::OOC));
		env.w->GameStep(4500);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::AUTO_NORMALISE | genericpoints::PTF::REV));

		env.w->SubmitAction(action_unreservetrack(*(env.w), *s1));
		expect_auto_reverse("Track unreserved");

		CHECK(env.w->GetLogText() == "");

		env.w->SubmitAction(action_pointsaction(*(env.w), *p1, 0, genericpoints::PTF::LOCKED, genericpoints::PTF::LOCKED));
		RoundTrip();
		env.w->GameStep(1);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::AUTO_NORMALISE | genericpoints::PTF::ZERO | genericpoints::PTF::LOCKED));
		env.w->SubmitAction(action_reservepath(*(env.w), s1, b));
		env.w->GameStep(1);
		REQUIRE(env.w->GetLogText() == "Cannot reserve route: Locked\n");

		env.w->ResetLogText();

		env.w->SubmitAction(action_pointsaction(*(env.w), *p1, 0, genericpoints::PTF::ZERO, genericpoints::PTF::LOCKED));
		RoundTrip();
		env.w->GameStep(1);
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::AUTO_NORMALISE | genericpoints::PTF::ZERO));
		env.w->SubmitAction(action_reservepath(*(env.w), s1, b));
		RoundTrip();
		env.w->GameStep(5001);
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::AUTO_NORMALISE | genericpoints::PTF::REV | genericpoints::PTF::ZERO));
		env.w->SubmitAction(action_pointsaction(*(env.w), *p1, 0, genericpoints::PTF::LOCKED, genericpoints::PTF::LOCKED));
		RoundTrip();
		env.w->GameStep(1);
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::AUTO_NORMALISE | genericpoints::PTF::REV | genericpoints::PTF::LOCKED));

		env.w->SubmitAction(action_unreservetrack(*(env.w), *s1));
		RoundTrip();
		env.w->GameStep(10000);
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::AUTO_NORMALISE | genericpoints::PTF::REV | genericpoints::PTF::LOCKED));
		env.w->SubmitAction(action_pointsaction(*(env.w), *p1, 0, genericpoints::PTF::ZERO, genericpoints::PTF::LOCKED));
		expect_auto_reverse("Track unlocked");

		env.w->SubmitAction(action_reservepath(*(env.w), s1, b));
		RoundTrip();
		env.w->GameStep(10000);
		env.w->SubmitAction(action_unreservetrack(*(env.w), *s1));
		RoundTrip();
		REQUIRE(action_points_auto_normalise::HasFutures(p1, 0) == true);
		env.w->GameStep(1000);
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::AUTO_NORMALISE | genericpoints::PTF::REV));
		env.w->SubmitAction(action_reservepath(*(env.w), s1, b));
		RoundTrip();
		REQUIRE(action_points_auto_normalise::HasFutures(p1, 0) == false);
		env.w->GameStep(10000);
		// Check that auto-normalisation is not too over-eager
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::AUTO_NORMALISE | genericpoints::PTF::REV));
		env.w->SubmitAction(action_unreservetrack(*(env.w), *s1));
		expect_auto_reverse("Track unreserved 2");

		CHECK(env.w->GetLogText() == "");
	};

	ExecTestFixtureWorldWithRoundTrip(env, setup, test);
}

std::string catchpoints_ops_test_str_2 =
R"({ "content" : [ )"
	R"({ "type" : "startofline", "name" : "A" }, )"
	R"({ "type" : "routesignal", "routesignal" : true, "name" : "S1" }, )"
	R"({ "type" : "catchpoints", "name" : "P1", "auto_normalise" : false }, )"
	R"({ "type" : "endofline", "name" : "B" } )"
"] }";

TEST_CASE( "track/ops/catchpoints/no_auto_normalise", "Test catchpoints with auto_normalise off" ) {
	test_fixture_world_init_checked env(catchpoints_ops_test_str_2);
	catchpoints *p1;
	routesignal *s1;
	routingpoint *b;
	auto setup = [&]() {
		p1 = PTR_CHECK(env.w->FindTrackByNameCast<catchpoints>("P1"));
		s1 = PTR_CHECK(env.w->FindTrackByNameCast<routesignal>("S1"));
		b = PTR_CHECK(env.w->FindTrackByNameCast<routingpoint>("B"));
	};

	auto test = [&](std::function<void()> RoundTrip) {
		env.w->SubmitAction(action_reservepath(*(env.w), s1, b));

		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::ZERO));
		env.w->GameStep(501);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::REV | genericpoints::PTF::OOC));
		env.w->GameStep(4500);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::REV));

		env.w->SubmitAction(action_unreservetrack(*(env.w), *s1));
		env.w->GameStep(10000);
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::REV));
		REQUIRE(action_points_auto_normalise::HasFutures(p1, 0) == false);
		CHECK(env.w->GetLogText() == "");
	};

	ExecTestFixtureWorldWithRoundTrip(env, setup, test);
}

std::string points_move_ops_test_str_2 =
R"({ "content" : [ )"
	R"({ "type" : "startofline", "name" : "A" }, )"
	R"({ "type" : "routesignal", "routesignal" : true, "name" : "S1" }, )"
	R"({ "type" : "points", "name" : "P1", "auto_normalise" : true }, )"
	R"({ "type" : "endofline", "name" : "B" }, )"
	R"({ "type" : "endofline", "name" : "C", "connect" : { "to" : "P1" } } )"
"] }";

TEST_CASE( "track/ops/points/auto_normalise", "Test points with auto_normalise on" ) {
	test_fixture_world_init_checked env(points_move_ops_test_str_2);
	points *p1;
	routesignal *s1;
	routingpoint *c;
	auto setup = [&]() {
		p1 = PTR_CHECK(env.w->FindTrackByNameCast<points>("P1"));
		s1 = PTR_CHECK(env.w->FindTrackByNameCast<routesignal>("S1"));
		c = PTR_CHECK(env.w->FindTrackByNameCast<routingpoint>("C"));
	};

	auto test = [&](std::function<void()> RoundTrip) {
		env.w->SubmitAction(action_reservepath(*(env.w), s1, c));

		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::AUTO_NORMALISE | genericpoints::PTF::ZERO));
		env.w->GameStep(501);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::AUTO_NORMALISE | genericpoints::PTF::REV | genericpoints::PTF::OOC));
		env.w->GameStep(4500);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (genericpoints::PTF::AUTO_NORMALISE | genericpoints::PTF::REV));

		env.w->SubmitAction(action_unreservetrack(*(env.w), *s1));
		AutoNormaliseExpectReversalTest(env, [&]() -> genericpoints* {
			RoundTrip();
			return p1;
		});
	};

	ExecTestFixtureWorldWithRoundTrip(env, setup, test);
}

std::string overlap_ops_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "newtype" : "4aspectauto", "basetype" : "autosignal", "content" : { "maxaspect" : 3 } }, )"
	R"({ "type" : "typedef", "newtype" : "4aspectroute", "basetype" : "routesignal", "content" : { "maxaspect" : 3, "routesignal" : true } }, )"
	R"({ "type" : "startofline", "name" : "A" }, )"
	R"({ "type" : "trackseg", "length" : 50000, "trackcircuit" : "T1" }, )"
	R"({ "type" : "4aspectauto", "name" : "S1" }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S1ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T2" }, )"
	R"({ "type" : "4aspectroute", "name" : "S2", "overlapswingable" : true }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S2ovlp" }, )"
	R"({ "type" : "points", "name" : "P1" }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "P1ovlp_norm" }, )"
	R"({ "type" : "routingmarker", "name" : "Bovlp", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T3" }, )"
	R"({ "type" : "endofline", "name" : "B" }, )"

	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "P1ovlp_rev", "connect" : { "to" : "P1" } }, )"
	R"({ "type" : "points", "name" : "P2" }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "P1ovlp_rev" }, )"
	R"({ "type" : "routingmarker", "name" : "Covlp", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T4" }, )"
	R"({ "type" : "endofline", "name" : "C" }, )"

	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T5", "connect" : { "to" : "P2" } }, )"
	R"({ "type" : "endofline", "name" : "D" } )"
"] }";

class overlap_ops_test_class_1 {
	public:
	world *w;
	error_collection ec;
	genericsignal *s1;
	genericsignal *s2;
	genericpoints *p1;
	genericpoints *p2;
	routingpoint *a;
	routingpoint *b;
	routingpoint *c;
	routingpoint *d;
	routingpoint *bovlp;
	routingpoint *covlp;
	routingpoint *dovlp;

	overlap_ops_test_class_1(world *w_) : w(w_) {
		s1 = w->FindTrackByNameCast<genericsignal>("S1");
		s2 = w->FindTrackByNameCast<genericsignal>("S2");
		p1 = w->FindTrackByNameCast<genericpoints>("P1");
		p2 = w->FindTrackByNameCast<genericpoints>("P2");
		a = w->FindTrackByNameCast<routingpoint>("A");
		b = w->FindTrackByNameCast<routingpoint>("B");
		c = w->FindTrackByNameCast<routingpoint>("C");
		d = w->FindTrackByNameCast<routingpoint>("D");
		bovlp = w->FindTrackByNameCast<routingpoint>("Bovlp");
		covlp = w->FindTrackByNameCast<routingpoint>("Covlp");
		dovlp = w->FindTrackByNameCast<routingpoint>("Dovlp");
	}
	void checksignal(genericsignal *signal, unsigned int aspect, route_class::ID aspect_type, routingpoint *aspect_target, routingpoint *aspect_route_target, routingpoint *overlapend) {
		REQUIRE(signal != 0);
		INFO("Signal check for: " << signal->GetName() << ", at time: " << w->GetGameTime());

		CHECK(signal->GetAspect() == aspect);
		CHECK(signal->GetAspectType() == aspect_type);
		CHECK(signal->GetAspectNextTarget() == aspect_target);
		CHECK(signal->GetAspectRouteTarget() == aspect_route_target);
		const route *ovlp = signal->GetCurrentForwardOverlap();
		if (overlapend) {
			CHECK(ovlp != 0);
			if (ovlp) {
				CHECK(ovlp->end.track == overlapend);
			}
		} else {
			CHECK(ovlp == 0);
		}
	}
};

using overlap_ops_test_func = std::function<void(test_fixture_world_init_checked &env, overlap_ops_test_class_1 &tenv, std::function<void()> RoundTrip)>;
void OverlapOpsRoundTripMultiTest(overlap_ops_test_func test_func) {
	test_fixture_world_init_checked env(overlap_ops_test_str_1);
	overlap_ops_test_class_1 tenv(env.w.get());

	SECTION("No serialisation round-trip") {
		test_func(env, tenv, []() { });
	}
	SECTION("With serialisation round-trip") {
		info_rescoped_unique roundtrip_msg;
		env.w->round_trip_actions = true;
		test_func(env, tenv, [&]() {
			CHECK(env.w->GetLogText() == "");
			env = RoundTripCloneTestFixtureWorld(env, &roundtrip_msg);
			tenv = overlap_ops_test_class_1(env.w.get());
		});
	}
}

TEST_CASE( "track/ops/overlap/reservationswing", "Test track reservation overlap swinging" ) {
	OverlapOpsRoundTripMultiTest([](test_fixture_world_init_checked &env, overlap_ops_test_class_1 &tenv, std::function<void()> RoundTrip) {
		std::vector<routingpoint::gmr_routeitem> routeset;
		REQUIRE(tenv.s2->GetMatchingRoutes(routeset, tenv.b, route_class::All(), GMRF::DONTSORT) == 1);
		REQUIRE(tenv.s2->GetMatchingRoutes(routeset, tenv.c, route_class::All(), GMRF::DONTCLEARVECTOR | GMRF::DONTSORT) == 1);
		REQUIRE(tenv.s2->GetMatchingRoutes(routeset, tenv.covlp, route_class::All(), GMRF::DONTCLEARVECTOR | GMRF::DONTSORT) == 1);
		REQUIRE(routeset.size() == 3);

		CHECK(routeset[0].rt->IsRouteSubSet(routeset[2].rt) == false);
		CHECK(routeset[1].rt->IsRouteSubSet(routeset[2].rt) == true);

		tenv.checksignal(tenv.s1, 0, route_class::RTC_NULL, 0, 0, 0);
		tenv.checksignal(tenv.s2, 0, route_class::RTC_NULL, 0, 0, tenv.bovlp);

		env.w->GameStep(1);
		tenv.checksignal(tenv.s1, 1, route_class::RTC_ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 0, route_class::RTC_NULL, 0, 0, tenv.bovlp);

		env.w->SubmitAction(action_reservepath(*(env.w), tenv.s2, tenv.c));
		RoundTrip();
		env.w->GameStep(1);
		CHECK(env.w->GetLogText() == "");
		REQUIRE(tenv.p1 != 0);
		CHECK(tenv.p1->GetPointsFlags(0) == (genericpoints::PTF::OOC | genericpoints::PTF::REV));
		tenv.checksignal(tenv.s1, 1, route_class::RTC_ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 0, route_class::RTC_NULL, 0, 0, tenv.covlp);

		RoundTrip();
		env.w->GameStep(100000);
		CHECK(env.w->GetLogText() == "");
		CHECK(tenv.p1->GetPointsFlags(0) == genericpoints::PTF::REV);
		tenv.checksignal(tenv.s1, 2, route_class::RTC_ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 1, route_class::RTC_ROUTE, tenv.c, tenv.c, tenv.covlp);

		env.w->SubmitAction(action_reservepath(*(env.w), tenv.s2, tenv.b));
		RoundTrip();
		env.w->GameStep(1);
		CHECK(env.w->GetLogText() != "");
		CHECK(tenv.p1->GetPointsFlags(0) == genericpoints::PTF::REV);
		tenv.checksignal(tenv.s1, 2, route_class::RTC_ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 1, route_class::RTC_ROUTE, tenv.c, tenv.c, tenv.covlp);
		env.w->ResetLogText();

		env.w->SubmitAction(action_unreservetrack(*(env.w), *tenv.s2));
		RoundTrip();
		env.w->GameStep(1);
		CHECK(env.w->GetLogText() == "");
		tenv.checksignal(tenv.s1, 1, route_class::RTC_ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 0, route_class::RTC_NULL, 0, 0, tenv.covlp);

		env.w->SubmitAction(action_reservepath(*(env.w), tenv.s2, tenv.d));
		RoundTrip();
		env.w->GameStep(1);
		CHECK(env.w->GetLogText() != "");
		tenv.checksignal(tenv.s1, 1, route_class::RTC_ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 0, route_class::RTC_NULL, 0, 0, tenv.covlp);
		env.w->ResetLogText();
	});
}

TEST_CASE( "track/ops/overlap/pointsswing", "Test points movement overlap swinging" ) {
	OverlapOpsRoundTripMultiTest([](test_fixture_world_init_checked &env, overlap_ops_test_class_1 &tenv, std::function<void()> RoundTrip) {
		env.w->GameStep(1);
		tenv.checksignal(tenv.s1, 1, route_class::RTC_ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 0, route_class::RTC_NULL, 0, 0, tenv.bovlp);

		REQUIRE(tenv.p1 != 0);
		env.w->SubmitAction(action_pointsaction(*(env.w), *tenv.p1, 0, genericpoints::PTF::REV, genericpoints::PTF::REV));
		RoundTrip();
		env.w->GameStep(1);

		CHECK(env.w->GetLogText() == "");
		CHECK(tenv.p1->GetPointsFlags(0) == (genericpoints::PTF::OOC | genericpoints::PTF::REV));
		tenv.checksignal(tenv.s2, 0, route_class::RTC_NULL, 0, 0, tenv.covlp);

		REQUIRE(tenv.p2 != 0);
		env.w->SubmitAction(action_pointsaction(*(env.w), *tenv.p2, 0, genericpoints::PTF::REV, genericpoints::PTF::REV));
		RoundTrip();
		env.w->GameStep(1);

		CHECK(env.w->GetLogText() != "");
		CHECK(tenv.p1->GetPointsFlags(0) == (genericpoints::PTF::OOC | genericpoints::PTF::REV));
		tenv.checksignal(tenv.s2, 0, route_class::RTC_NULL, 0, 0, tenv.covlp);
		env.w->ResetLogText();

		env.w->SubmitAction(action_pointsaction(*(env.w), *tenv.p1, 0, genericpoints::PTF::ZERO, genericpoints::PTF::REV));
		env.w->GameStep(1);
		env.w->SubmitAction(action_pointsaction(*(env.w), *tenv.p2, 0, genericpoints::PTF::REV | genericpoints::PTF::REMINDER, genericpoints::PTF::REV | genericpoints::PTF::REMINDER));
		env.w->GameStep(1);
		RoundTrip();
		CHECK(env.w->GetLogText() == "");
		tenv.checksignal(tenv.s2, 0, route_class::RTC_NULL, 0, 0, tenv.bovlp);

		env.w->SubmitAction(action_pointsaction(*(env.w), *tenv.p1, 0, genericpoints::PTF::REV, genericpoints::PTF::REV));
		RoundTrip();
		env.w->GameStep(1);
		CHECK(env.w->GetLogText() != "");
		tenv.checksignal(tenv.s2, 0, route_class::RTC_NULL, 0, 0, tenv.bovlp);
	});
}

TEST_CASE( "track/ops/reservation/overset", "Test overset track reservation and dereservation" ) {
	OverlapOpsRoundTripMultiTest([](test_fixture_world_init_checked &env, overlap_ops_test_class_1 &tenv, std::function<void()> RoundTrip) {
		env.w->GameStep(1);
		tenv.checksignal(tenv.s1, 1, route_class::RTC_ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 0, route_class::RTC_NULL, 0, 0, tenv.bovlp);

		CHECK(tenv.s2->ReservationEnumeration([&](const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) { }, RRF::RESERVE) == 2);

		env.w->SubmitAction(action_reservepath(*(env.w), tenv.s2, tenv.c));
		env.w->SubmitAction(action_reservepath(*(env.w), tenv.s2, tenv.c));
		RoundTrip();
		env.w->GameStep(100000);
		RoundTrip();
		CHECK(env.w->GetLogText() == "");
		tenv.checksignal(tenv.s1, 2, route_class::RTC_ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 1, route_class::RTC_ROUTE, tenv.c, tenv.c, tenv.covlp);

		env.w->SubmitAction(action_reservepath(*(env.w), tenv.s2, tenv.c));
		RoundTrip();
		env.w->GameStep(100000);
		RoundTrip();
		CHECK(env.w->GetLogText() == "");
		tenv.checksignal(tenv.s1, 2, route_class::RTC_ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 1, route_class::RTC_ROUTE, tenv.c, tenv.c, tenv.covlp);

		CHECK(tenv.s2->ReservationEnumeration([&](const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) { }, RRF::RESERVE) == 3);

		env.w->SubmitAction(action_unreservetrack(*(env.w), *tenv.s2));
		RoundTrip();
		env.w->GameStep(100000);
		RoundTrip();
		CHECK(env.w->GetLogText() == "");
		tenv.checksignal(tenv.s1, 1, route_class::RTC_ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 0, route_class::RTC_NULL, 0, 0, tenv.covlp);

		CHECK(tenv.s2->ReservationEnumeration([&](const route *reserved_route, EDGETYPE direction, unsigned int index, RRF rr_flags) { }, RRF::RESERVE) == 2);
	});
}
