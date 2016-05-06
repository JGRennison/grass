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
#include "core/track_ops.h"
#include "core/track.h"
#include "core/points.h"
#include "core/text_pool.h"
#include "core/var.h"
#include "core/signal.h"

std::string points_move_ops_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "points", "name" : "P1" }, )"
	R"({ "type" : "end_of_line", "name" : "B" }, )"
	R"({ "type" : "end_of_line", "name" : "C", "connect" : { "to" : "P1" } } )"
"] }";

TEST_CASE( "track/ops/points/movement", "Test basic points movement future" ) {
	test_fixture_world_init_checked env(points_move_ops_test_str_1);
	points *p1;
	auto setup = [&]() {
		p1 = PTR_CHECK(env.w->FindTrackByNameCast<points>("P1"));
	};

	auto test = [&](std::function<void()> RoundTrip) {
		env.w->SubmitAction(action_points_action(*(env.w), *p1, 0, generic_points::PTF::REV, generic_points::PTF::REV));

		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == generic_points::PTF::ZERO);
		env.w->GameStep(500);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::REV | generic_points::PTF::OOC));
		env.w->GameStep(4500);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == generic_points::PTF::REV);

		env.w->SubmitAction(action_points_action(*(env.w), *p1, 0, generic_points::PTF::LOCKED, generic_points::PTF::LOCKED));
		env.w->GameStep(50);
		env.w->SubmitAction(action_points_action(*(env.w), *p1, 0, generic_points::PTF::REV, generic_points::PTF::REV));
		env.w->GameStep(50);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::REV | generic_points::PTF::LOCKED));
		env.w->SubmitAction(action_points_action(*(env.w), *p1, 0, generic_points::PTF::ZERO, generic_points::PTF::REV));
		env.w->GameStep(4900);
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::REV | generic_points::PTF::LOCKED));
		REQUIRE(env.w->GetLogText() == "Points P1 not movable: Locked\n");

		env.w->ResetLogText();

		auto test_norm_rev_action = [&](bool reverse, generic_points::PTF expected) {
			RoundTrip();
			env.w->SubmitAction(action_points_action(*(env.w), *p1, 0, reverse));
			env.w->GameStep(5000);
			REQUIRE(p1->GetPointsFlags(0) == expected);
			REQUIRE(env.w->GetLogText().empty());
		};

		test_norm_rev_action(true, generic_points::PTF::REV | generic_points::PTF::LOCKED);
		test_norm_rev_action(false, generic_points::PTF::REV);
		test_norm_rev_action(false, generic_points::PTF::ZERO);
		test_norm_rev_action(false, generic_points::PTF::ZERO | generic_points::PTF::LOCKED);
		test_norm_rev_action(false, generic_points::PTF::ZERO | generic_points::PTF::LOCKED);
		test_norm_rev_action(true, generic_points::PTF::ZERO);
		test_norm_rev_action(true, generic_points::PTF::REV);
	};

	ExecTestFixtureWorldWithRoundTrip(env, setup, test);
}

void AutoNormaliseExpectReversalTest(test_fixture_world &env, std::function<generic_points *()> RoundTrip,
		generic_points::PTF extraflags = generic_points::PTF::ZERO) {
	generic_points *p = RoundTrip();
	REQUIRE(action_points_auto_normalise::HasFutures(p, 0) == true);
	env.w->GameStep(1950);
	p = RoundTrip();
	REQUIRE(p->GetPointsFlags(0) == (generic_points::PTF::AUTO_NORMALISE | generic_points::PTF::REV | extraflags));
	env.w->GameStep(100);
	env.w->GameStep(1); // additional step for points action future
	p = RoundTrip();
	REQUIRE(p->GetPointsFlags(0) == (generic_points::PTF::AUTO_NORMALISE | generic_points::PTF::ZERO | generic_points::PTF::OOC | extraflags));
	env.w->GameStep(5000);
	p = RoundTrip();
	REQUIRE(p->GetPointsFlags(0) == (generic_points::PTF::AUTO_NORMALISE | generic_points::PTF::ZERO | extraflags));
}

std::string catchpoints_ops_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "route_signal", "route_signal" : true, "name" : "S1" }, )"
	R"({ "type" : "catchpoints", "name" : "P1" }, )"
	R"({ "type" : "end_of_line", "name" : "B" } )"
"] }";

TEST_CASE( "track/ops/catchpoints", "Test catchpoints movement on reservation" ) {
	test_fixture_world_init_checked env(catchpoints_ops_test_str_1);
	catchpoints *p1;
	route_signal *s1;
	routing_point *b;
	auto setup = [&]() {
		p1 = PTR_CHECK(env.w->FindTrackByNameCast<catchpoints>("P1"));
		s1 = PTR_CHECK(env.w->FindTrackByNameCast<route_signal>("S1"));
		b = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("B"));
	};

	auto test = [&](std::function<void()> RoundTrip) {
		auto expect_auto_reverse = [&](const std::string &subtest_name) {
			INFO("Subtest: " + subtest_name);
			AutoNormaliseExpectReversalTest(env, [&]() -> generic_points* {
				RoundTrip();
				return p1;
			});
		};

		env.w->SubmitAction(action_reserve_path(*(env.w), s1, b));

		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::AUTO_NORMALISE | generic_points::PTF::ZERO));
		env.w->GameStep(501);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::AUTO_NORMALISE | generic_points::PTF::REV | generic_points::PTF::OOC));
		env.w->GameStep(4500);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::AUTO_NORMALISE | generic_points::PTF::REV));

		env.w->SubmitAction(action_unreserve_track(*(env.w), *s1));
		expect_auto_reverse("Track unreserved");

		CHECK(env.w->GetLogText() == "");

		env.w->SubmitAction(action_points_action(*(env.w), *p1, 0, generic_points::PTF::LOCKED, generic_points::PTF::LOCKED));
		RoundTrip();
		env.w->GameStep(1);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::AUTO_NORMALISE | generic_points::PTF::ZERO | generic_points::PTF::LOCKED));
		env.w->SubmitAction(action_reserve_path(*(env.w), s1, b));
		env.w->GameStep(1);
		REQUIRE(env.w->GetLogText() == "Cannot reserve route: Locked\n");

		env.w->ResetLogText();

		env.w->SubmitAction(action_points_action(*(env.w), *p1, 0, generic_points::PTF::ZERO, generic_points::PTF::LOCKED));
		RoundTrip();
		env.w->GameStep(1);
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::AUTO_NORMALISE | generic_points::PTF::ZERO));
		env.w->SubmitAction(action_reserve_path(*(env.w), s1, b));
		RoundTrip();
		env.w->GameStep(5001);
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::AUTO_NORMALISE | generic_points::PTF::REV | generic_points::PTF::ZERO));
		env.w->SubmitAction(action_points_action(*(env.w), *p1, 0, generic_points::PTF::LOCKED, generic_points::PTF::LOCKED));
		RoundTrip();
		env.w->GameStep(1);
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::AUTO_NORMALISE | generic_points::PTF::REV | generic_points::PTF::LOCKED));

		env.w->SubmitAction(action_unreserve_track(*(env.w), *s1));
		RoundTrip();
		env.w->GameStep(10000);
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::AUTO_NORMALISE | generic_points::PTF::REV | generic_points::PTF::LOCKED));
		env.w->SubmitAction(action_points_action(*(env.w), *p1, 0, generic_points::PTF::ZERO, generic_points::PTF::LOCKED));
		expect_auto_reverse("Track unlocked");

		env.w->SubmitAction(action_reserve_path(*(env.w), s1, b));
		RoundTrip();
		env.w->GameStep(10000);
		env.w->SubmitAction(action_unreserve_track(*(env.w), *s1));
		RoundTrip();
		REQUIRE(action_points_auto_normalise::HasFutures(p1, 0) == true);
		env.w->GameStep(1000);
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::AUTO_NORMALISE | generic_points::PTF::REV));
		env.w->SubmitAction(action_reserve_path(*(env.w), s1, b));
		RoundTrip();
		REQUIRE(action_points_auto_normalise::HasFutures(p1, 0) == false);
		env.w->GameStep(10000);
		// Check that auto-normalisation is not too over-eager
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::AUTO_NORMALISE | generic_points::PTF::REV));
		env.w->SubmitAction(action_unreserve_track(*(env.w), *s1));
		expect_auto_reverse("Track unreserved 2");

		CHECK(env.w->GetLogText() == "");
	};

	ExecTestFixtureWorldWithRoundTrip(env, setup, test);
}

std::string catchpoints_ops_test_str_2 =
R"({ "content" : [ )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "route_signal", "route_signal" : true, "name" : "S1" }, )"
	R"({ "type" : "catchpoints", "name" : "P1", "auto_normalise" : false }, )"
	R"({ "type" : "end_of_line", "name" : "B" } )"
"] }";

TEST_CASE( "track/ops/catchpoints/no_auto_normalise", "Test catchpoints with auto_normalise off" ) {
	test_fixture_world_init_checked env(catchpoints_ops_test_str_2);
	catchpoints *p1;
	route_signal *s1;
	routing_point *b;
	auto setup = [&]() {
		p1 = PTR_CHECK(env.w->FindTrackByNameCast<catchpoints>("P1"));
		s1 = PTR_CHECK(env.w->FindTrackByNameCast<route_signal>("S1"));
		b = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("B"));
	};

	auto test = [&](std::function<void()> RoundTrip) {
		env.w->SubmitAction(action_reserve_path(*(env.w), s1, b));

		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::ZERO));
		env.w->GameStep(501);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::REV | generic_points::PTF::OOC));
		env.w->GameStep(4500);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::REV));

		env.w->SubmitAction(action_unreserve_track(*(env.w), *s1));
		env.w->GameStep(10000);
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::REV));
		REQUIRE(action_points_auto_normalise::HasFutures(p1, 0) == false);
		CHECK(env.w->GetLogText() == "");
	};

	ExecTestFixtureWorldWithRoundTrip(env, setup, test);
}

std::string points_move_ops_test_str_2 =
R"({ "content" : [ )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "route_signal", "route_signal" : true, "name" : "S1" }, )"
	R"({ "type" : "points", "name" : "P1", "auto_normalise" : true }, )"
	R"({ "type" : "end_of_line", "name" : "B" }, )"
	R"({ "type" : "end_of_line", "name" : "C", "connect" : { "to" : "P1" } } )"
"] }";

TEST_CASE( "track/ops/points/auto_normalise", "Test points with auto_normalise on" ) {
	test_fixture_world_init_checked env(points_move_ops_test_str_2);
	points *p1;
	route_signal *s1;
	routing_point *c;
	auto setup = [&]() {
		p1 = PTR_CHECK(env.w->FindTrackByNameCast<points>("P1"));
		s1 = PTR_CHECK(env.w->FindTrackByNameCast<route_signal>("S1"));
		c = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("C"));
	};

	auto test = [&](std::function<void()> RoundTrip) {
		env.w->SubmitAction(action_reserve_path(*(env.w), s1, c));

		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::AUTO_NORMALISE | generic_points::PTF::ZERO));
		env.w->GameStep(501);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::AUTO_NORMALISE | generic_points::PTF::REV | generic_points::PTF::OOC));
		env.w->GameStep(4500);
		RoundTrip();
		REQUIRE(p1->GetPointsFlags(0) == (generic_points::PTF::AUTO_NORMALISE | generic_points::PTF::REV));

		env.w->SubmitAction(action_unreserve_track(*(env.w), *s1));
		AutoNormaliseExpectReversalTest(env, [&]() -> generic_points* {
			RoundTrip();
			return p1;
		});
	};

	ExecTestFixtureWorldWithRoundTrip(env, setup, test);
}

std::string points_coupled_reserve_test_str1 =
R"({ "content" : [ )"
	R"({ "type" : "couple_points", "points" : [ { "name" : "P1", "edge" : "normal" }, { "name" : "P2", "edge" : "%s" } ] }, )"

	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "route_signal", "route_signal" : true, "name" : "S1" }, )"
	R"({ "type" : "points", "name" : "P1" }, )"
	R"({ "type" : "end_of_line", "name" : "B" }, )"
	R"({ "type" : "end_of_line", "name" : "C", "connect" : { "to" : "P1" } }, )"

	R"({ "type" : "start_of_line", "name" : "D" }, )"
	R"({ "type" : "route_signal", "route_signal" : true, "name" : "S2" }, )"
	R"({ "type" : "points", "name" : "P2" }, )"
	R"({ "type" : "end_of_line", "name" : "E" }, )"
	R"({ "type" : "end_of_line", "name" : "F", "connect" : { "to" : "P2" } } )"
"] }";

TEST_CASE( "track/ops/points/coupling/contradictory-reservation", "Test contradictory reservation over coupled points" ) {
	using PTF = generic_points::PTF;

	std::function<void()> setup;
	points *p1;
	route_signal *s1;
	routing_point *c;
	points *p2;
	route_signal *s2;
	routing_point *f;
	auto setup_test = [&](test_fixture_world_init_checked &env) {
		setup = [&]() {
			p1 = PTR_CHECK(env.w->FindTrackByNameCast<points>("P1"));
			s1 = PTR_CHECK(env.w->FindTrackByNameCast<route_signal>("S1"));
			c = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("C"));
			p2 = PTR_CHECK(env.w->FindTrackByNameCast<points>("P2"));
			s2 = PTR_CHECK(env.w->FindTrackByNameCast<route_signal>("S2"));
			f = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("F"));
		};
	};

	SECTION("Normal") {
		test_fixture_world_init_checked env(string_format(points_coupled_reserve_test_str1.c_str(), "normal"));
		setup_test(env);
		auto test = [&](std::function<void()> RoundTrip) {
			env.w->SubmitAction(action_reserve_path(*(env.w), s1, c));
			env.w->SubmitAction(action_reserve_path(*(env.w), s2, f));

			RoundTrip();
			CHECK(p1->GetPointsFlags(0) == (PTF::COUPLED | PTF::ZERO));
			CHECK(p2->GetPointsFlags(0) == (PTF::COUPLED | PTF::ZERO));
			env.w->GameStep(501);
			RoundTrip();
			CHECK(p1->GetPointsFlags(0) == (PTF::COUPLED | PTF::REV | PTF::OOC));
			CHECK(p2->GetPointsFlags(0) == (PTF::COUPLED | PTF::REV | PTF::OOC));
			CHECK(s1->GetCurrentForwardRoute() != nullptr);
			CHECK(s2->GetCurrentForwardRoute() != nullptr);
			env.w->GameStep(4500);
			RoundTrip();
			CHECK(p1->GetPointsFlags(0) == (PTF::COUPLED | PTF::REV));
			CHECK(p2->GetPointsFlags(0) == (PTF::COUPLED | PTF::REV));
			CHECK(env.w->GetLogText() == "");
		};
		ExecTestFixtureWorldWithRoundTrip(env, setup, test);
	}

	SECTION("Reverse") {
		test_fixture_world_init_checked env(string_format(points_coupled_reserve_test_str1.c_str(), "reverse"));
		setup_test(env);
		auto test = [&](std::function<void()> RoundTrip) {
			env.w->SubmitAction(action_reserve_path(*(env.w), s1, c));
			env.w->SubmitAction(action_reserve_path(*(env.w), s2, f));

			RoundTrip();
			CHECK(p1->GetPointsFlags(0) == (PTF::COUPLED | PTF::ZERO));
			CHECK(p2->GetPointsFlags(0) == (PTF::COUPLED | PTF::REV));
			env.w->GameStep(501);
			CHECK_CONTAINS(env.w->GetLogText(), "Cannot reserve route: Conflicts with existing route");
			RoundTrip();
			CHECK(p1->GetPointsFlags(0) == (PTF::COUPLED | PTF::REV | PTF::OOC));
			CHECK(p2->GetPointsFlags(0) == (PTF::COUPLED | PTF::ZERO | PTF::OOC));
			CHECK(s1->GetCurrentForwardRoute() != nullptr);
			CHECK(s2->GetCurrentForwardRoute() == nullptr);
			env.w->GameStep(4500);
			RoundTrip();
			CHECK(p1->GetPointsFlags(0) == (PTF::COUPLED | PTF::REV));
			CHECK(p2->GetPointsFlags(0) == (PTF::COUPLED | PTF::ZERO));
		};
		ExecTestFixtureWorldWithRoundTrip(env, setup, test);
	}
}

TEST_CASE( "track/ops/points/coupling/ooc-time", "Test OOC times over coupled points" ) {
	using PTF = generic_points::PTF;

	test_fixture_world_init_checked env(string_format(points_coupled_reserve_test_str1.c_str(), "normal"));
	points *p1;
	points *p2;
	auto setup = [&]() {
		p1 = PTR_CHECK(env.w->FindTrackByNameCast<points>("P1"));
		p2 = PTR_CHECK(env.w->FindTrackByNameCast<points>("P2"));
	};

	auto test = [&](std::function<void()> RoundTrip) {
		env.w->SubmitAction(action_points_action(*(env.w), *p1, 0, generic_points::PTF::REV, generic_points::PTF::REV));
		RoundTrip();
		CHECK(p1->GetPointsFlags(0) == (PTF::COUPLED | PTF::ZERO));
		CHECK(p2->GetPointsFlags(0) == (PTF::COUPLED | PTF::ZERO));
		env.w->GameStep(501);
		RoundTrip();
		CHECK(p1->GetPointsFlags(0) == (PTF::COUPLED | PTF::REV | PTF::OOC));
		CHECK(p2->GetPointsFlags(0) == (PTF::COUPLED | PTF::REV | PTF::OOC));
		env.w->GameStep(4000);
		RoundTrip();
		env.w->SubmitAction(action_points_action(*(env.w), *p2, 0, generic_points::PTF::ZERO, generic_points::PTF::REV));
		env.w->GameStep(501);
		RoundTrip();
		CHECK(p1->GetPointsFlags(0) == (PTF::COUPLED | PTF::ZERO | PTF::OOC));
		CHECK(p2->GetPointsFlags(0) == (PTF::COUPLED | PTF::ZERO | PTF::OOC));
		env.w->GameStep(2000);
		RoundTrip();
		CHECK(p1->GetPointsFlags(0) == (PTF::COUPLED | PTF::ZERO | PTF::OOC));
		CHECK(p2->GetPointsFlags(0) == (PTF::COUPLED | PTF::ZERO | PTF::OOC));
		env.w->GameStep(2500);
		RoundTrip();
		CHECK(p1->GetPointsFlags(0) == (PTF::COUPLED | PTF::ZERO));
		CHECK(p2->GetPointsFlags(0) == (PTF::COUPLED | PTF::ZERO));
		CHECK(env.w->GetLogText() == "");
	};
	ExecTestFixtureWorldWithRoundTrip(env, setup, test);
}

std::string points_coupled_reserve_test_str2 =
R"({ "content" : [ )"
	R"({ "type" : "couple_points", "points" : [ { "name" : "DS1", "edge" : "left_front" }, { "name" : "DS2", "edge" : "%s" } ] }, )"

	R"({ "type" : "double_slip", "name" : "DS1" }, )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "route_signal", "route_signal" : true, "name" : "S1", "connect" : { "from_direction" : "back", "to" : "DS1", "to_direction" : "left_front" } }, )"
	R"({ "type" : "start_of_line", "connect" : { "to" : "DS1", "to_direction" : "right_front" } }, )"
	R"({ "type" : "end_of_line", "name" : "B", "connect" : { "to" : "DS1", "to_direction" : "left_back" } }, )"
	R"({ "type" : "end_of_line", "name" : "C", "connect" : { "to" : "DS1", "to_direction" : "right_back" } }, )"

	R"({ "type" : "double_slip", "name" : "DS2" }, )"
	R"({ "type" : "start_of_line", "name" : "D" }, )"
	R"({ "type" : "route_signal", "route_signal" : true, "name" : "S2", "connect" : { "from_direction" : "back", "to" : "DS2", "to_direction" : "left_front" } }, )"
	R"({ "type" : "start_of_line", "connect" : { "to" : "DS2", "to_direction" : "right_front" } }, )"
	R"({ "type" : "end_of_line", "name" : "E", "connect" : { "to" : "DS2", "to_direction" : "left_back" } }, )"
	R"({ "type" : "end_of_line", "name" : "F", "connect" : { "to" : "DS2", "to_direction" : "right_back" } } )"
"] }";

TEST_CASE( "track/ops/points/coupling/contradictory-reservation/double-slip", "Test contradictory reservation over coupled double-slips" ) {
	std::function<void()> setup;
	double_slip *ds1;
	route_signal *s1;
	routing_point *c;
	double_slip *ds2;
	route_signal *s2;
	routing_point *f;
	auto setup_test = [&](test_fixture_world_init_checked &env) {
		setup = [&]() {
			ds1 = PTR_CHECK(env.w->FindTrackByNameCast<double_slip>("DS1"));
			s1 = PTR_CHECK(env.w->FindTrackByNameCast<route_signal>("S1"));
			c = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("C"));
			ds2 = PTR_CHECK(env.w->FindTrackByNameCast<double_slip>("DS2"));
			s2 = PTR_CHECK(env.w->FindTrackByNameCast<route_signal>("S2"));
			f = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("F"));
		};
	};

	SECTION("Normal") {
		test_fixture_world_init_checked env(string_format(points_coupled_reserve_test_str2.c_str(), "left_front"));
		setup_test(env);
		auto test = [&](std::function<void()> RoundTrip) {
			env.w->SubmitAction(action_reserve_path(*(env.w), s1, c));
			env.w->SubmitAction(action_reserve_path(*(env.w), s2, f));
			env.w->GameStep(501);
			RoundTrip();
			CHECK(s1->GetCurrentForwardRoute() != nullptr);
			CHECK(s2->GetCurrentForwardRoute() != nullptr);
			env.w->GameStep(4500);
			RoundTrip();
			CHECK(ds1->GetConnectingPiece(EDGE::DS_FL).track == c);
			CHECK(ds2->GetConnectingPiece(EDGE::DS_FL).track == f);
			CHECK(env.w->GetLogText() == "");
		};
		ExecTestFixtureWorldWithRoundTrip(env, setup, test);
	}

	SECTION("Reverse") {
		test_fixture_world_init_checked env(string_format(points_coupled_reserve_test_str2.c_str(), "right_front"));
		setup_test(env);
		auto test = [&](std::function<void()> RoundTrip) {
			env.w->SubmitAction(action_reserve_path(*(env.w), s1, c));
			env.w->SubmitAction(action_reserve_path(*(env.w), s2, f));
			env.w->GameStep(501);
			CHECK_CONTAINS(env.w->GetLogText(), "Cannot reserve route: Conflicts with existing route");
			CHECK(s1->GetCurrentForwardRoute() != nullptr);
			CHECK(s2->GetCurrentForwardRoute() == nullptr);
			env.w->GameStep(4500);
			RoundTrip();
			CHECK(ds1->GetConnectingPiece(EDGE::DS_FL).track == c);
			CHECK(ds2->GetConnectingPiece(EDGE::DS_FL).track != f);
		};
		ExecTestFixtureWorldWithRoundTrip(env, setup, test);
	}
}

std::string points_coupled_reserve_test_str3 =
R"({ "content" : [ )"
	R"({ "type" : "couple_points", "points" : [ { "name" : "P1", "edge" : "normal" }, { "name" : "P2", "edge" : "%s" } ] }, )"

	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "route_signal", "route_signal" : true, "name" : "S1" }, )"
	R"({ "type" : "catchpoints", "name" : "P1", "auto_normalise" : %s }, )"
	R"({ "type" : "end_of_line", "name" : "B" }, )"

	R"({ "type" : "start_of_line", "name" : "D" }, )"
	R"({ "type" : "route_signal", "route_signal" : true, "name" : "S2" }, )"
	R"({ "type" : "catchpoints", "name" : "P2", "auto_normalise" : %s }, )"
	R"({ "type" : "end_of_line", "name" : "E" } )"
"] }";

TEST_CASE( "track/ops/points/coupling/catch-points", "Test reservation over coupled catch-points" ) {
	using PTF = generic_points::PTF;

	std::function<void()> setup;
	catchpoints *p1;
	route_signal *s1;
	routing_point *b;
	catchpoints *p2;
	route_signal *s2;
	routing_point *e;
	auto setup_test = [&](test_fixture_world_init_checked &env) {
		setup = [&]() {
			p1 = PTR_CHECK(env.w->FindTrackByNameCast<catchpoints>("P1"));
			s1 = PTR_CHECK(env.w->FindTrackByNameCast<route_signal>("S1"));
			b = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("B"));
			p2 = PTR_CHECK(env.w->FindTrackByNameCast<catchpoints>("P2"));
			s2 = PTR_CHECK(env.w->FindTrackByNameCast<route_signal>("S2"));
			e = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("E"));
		};
	};

	SECTION("Normal") {
		test_fixture_world_init_checked env(string_format(points_coupled_reserve_test_str3.c_str(), "normal", "false", "false"));
		setup_test(env);
		auto test = [&](std::function<void()> RoundTrip) {
			env.w->SubmitAction(action_reserve_path(*(env.w), s1, b));
			env.w->SubmitAction(action_reserve_path(*(env.w), s2, e));

			RoundTrip();
			CHECK(p1->GetPointsFlags(0) == (PTF::COUPLED | PTF::ZERO));
			CHECK(p2->GetPointsFlags(0) == (PTF::COUPLED | PTF::ZERO));
			env.w->GameStep(501);
			RoundTrip();
			CHECK(p1->GetPointsFlags(0) == (PTF::COUPLED | PTF::REV | PTF::OOC));
			CHECK(p2->GetPointsFlags(0) == (PTF::COUPLED | PTF::REV | PTF::OOC));
			CHECK(s1->GetCurrentForwardRoute() != nullptr);
			CHECK(s2->GetCurrentForwardRoute() != nullptr);
			env.w->GameStep(4500);
			RoundTrip();
			CHECK(p1->GetPointsFlags(0) == (PTF::COUPLED | PTF::REV));
			CHECK(p2->GetPointsFlags(0) == (PTF::COUPLED | PTF::REV));
			CHECK(env.w->GetLogText() == "");
		};
		ExecTestFixtureWorldWithRoundTrip(env, setup, test);
	}

	SECTION("Reverse") {
		test_fixture_world_init_checked env(string_format(points_coupled_reserve_test_str3.c_str(), "reverse", "false", "false"));
		setup_test(env);
		auto test = [&](std::function<void()> RoundTrip) {
			env.w->SubmitAction(action_reserve_path(*(env.w), s1, b));
			env.w->SubmitAction(action_reserve_path(*(env.w), s2, e));

			RoundTrip();
			CHECK(p1->GetPointsFlags(0) == (PTF::COUPLED | PTF::ZERO));
			CHECK(p2->GetPointsFlags(0) == (PTF::COUPLED | PTF::REV));
			env.w->GameStep(501);
			CHECK_CONTAINS(env.w->GetLogText(), "Cannot reserve route: Conflicts with existing route");
			RoundTrip();
			CHECK(p1->GetPointsFlags(0) == (PTF::COUPLED | PTF::REV | PTF::OOC));
			CHECK(p2->GetPointsFlags(0) == (PTF::COUPLED | PTF::ZERO | PTF::OOC));
			CHECK(s1->GetCurrentForwardRoute() != nullptr);
			CHECK(s2->GetCurrentForwardRoute() == nullptr);
			env.w->GameStep(4500);
			RoundTrip();
			CHECK(p1->GetPointsFlags(0) == (PTF::COUPLED | PTF::REV));
			CHECK(p2->GetPointsFlags(0) == (PTF::COUPLED | PTF::ZERO));
		};
		ExecTestFixtureWorldWithRoundTrip(env, setup, test);
	}

	auto expect_auto_reverse = [&](test_fixture_world &env, catchpoints *&p, std::function<void()> &RoundTrip) {
		AutoNormaliseExpectReversalTest(env, [&]() -> generic_points* {
			RoundTrip();
			return p;
		}, PTF::COUPLED);
	};

	SECTION("Auto-normalise: 1") {
		test_fixture_world_init_checked env(string_format(points_coupled_reserve_test_str3.c_str(), "normal", "true", "true"));
		setup_test(env);
		auto test = [&](std::function<void()> RoundTrip) {
			env.w->SubmitAction(action_reserve_path(*(env.w), s1, b));
			env.w->SubmitAction(action_reserve_path(*(env.w), s2, e));

			RoundTrip();
			env.w->GameStep(501);
			CHECK(s1->GetCurrentForwardRoute() != nullptr);
			CHECK(s2->GetCurrentForwardRoute() != nullptr);
			RoundTrip();
			env.w->GameStep(4500);
			RoundTrip();
			CHECK(p1->GetPointsFlags(0) == (PTF::AUTO_NORMALISE | PTF::COUPLED | PTF::REV));
			CHECK(p2->GetPointsFlags(0) == (PTF::AUTO_NORMALISE | PTF::COUPLED | PTF::REV));

			env.w->SubmitAction(action_unreserve_track(*(env.w), *s1));
			RoundTrip();
			env.w->GameStep(501);
			RoundTrip();
			env.w->GameStep(4500);
			RoundTrip();
			CHECK(p1->GetPointsFlags(0) == (PTF::AUTO_NORMALISE | PTF::COUPLED | PTF::REV));
			CHECK(p2->GetPointsFlags(0) == (PTF::AUTO_NORMALISE | PTF::COUPLED | PTF::REV));

			env.w->SubmitAction(action_unreserve_track(*(env.w), *s2));
			SECTION("1") {
				expect_auto_reverse(env, p1, RoundTrip);
			}
			SECTION("2") {
				expect_auto_reverse(env, p2, RoundTrip);
			}
			CHECK(env.w->GetLogText() == "");
		};
		ExecTestFixtureWorldWithRoundTrip(env, setup, test);
	}

	SECTION("Auto-normalise: 2") {
		test_fixture_world_init_checked env(string_format(points_coupled_reserve_test_str3.c_str(), "normal", "true", "true"));
		setup_test(env);
		auto test = [&](std::function<void()> RoundTrip) {
			env.w->SubmitAction(action_reserve_path(*(env.w), s1, b));

			RoundTrip();
			env.w->GameStep(5000);
			RoundTrip();
			CHECK(p1->GetPointsFlags(0) == (PTF::AUTO_NORMALISE | PTF::COUPLED | PTF::REV));
			CHECK(p2->GetPointsFlags(0) == (PTF::AUTO_NORMALISE | PTF::COUPLED | PTF::REV));

			env.w->SubmitAction(action_unreserve_track(*(env.w), *s1));
			RoundTrip();
			env.w->GameStep(501);
			RoundTrip();
			env.w->SubmitAction(action_reserve_path(*(env.w), s2, e));
			RoundTrip();
			env.w->GameStep(501);
			CHECK(p1->GetPointsFlags(0) == (PTF::AUTO_NORMALISE | PTF::COUPLED | PTF::REV));
			CHECK(p2->GetPointsFlags(0) == (PTF::AUTO_NORMALISE | PTF::COUPLED | PTF::REV));
			env.w->GameStep(4500);
			RoundTrip();
			CHECK(p1->GetPointsFlags(0) == (PTF::AUTO_NORMALISE | PTF::COUPLED | PTF::REV));
			CHECK(p2->GetPointsFlags(0) == (PTF::AUTO_NORMALISE | PTF::COUPLED | PTF::REV));

			env.w->SubmitAction(action_unreserve_track(*(env.w), *s2));
			SECTION("1") {
				expect_auto_reverse(env, p1, RoundTrip);
			}
			SECTION("2") {
				expect_auto_reverse(env, p2, RoundTrip);
			}
			CHECK(env.w->GetLogText() == "");
		};
		ExecTestFixtureWorldWithRoundTrip(env, setup, test);
	}
}

std::string overlap_ops_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "new_type" : "4aspectauto", "base_type" : "auto_signal", "content" : { "max_aspect" : 3 } }, )"
	R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"
	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "track_seg", "length" : 50000, "track_circuit" : "T1" }, )"
	R"({ "type" : "4aspectauto", "name" : "S1" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S1ovlp" }, )"
	R"({ "type" : "routing_marker", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T2" }, )"
	R"({ "type" : "4aspectroute", "name" : "S2", "overlap_swingable" : true }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S2ovlp" }, )"
	R"({ "type" : "points", "name" : "P1" }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "P1ovlp_norm" }, )"
	R"({ "type" : "routing_marker", "name" : "Bovlp", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T3" }, )"
	R"({ "type" : "end_of_line", "name" : "B" }, )"

	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "P1ovlp_rev", "connect" : { "to" : "P1" } }, )"
	R"({ "type" : "points", "name" : "P2" }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "P1ovlp_rev" }, )"
	R"({ "type" : "routing_marker", "name" : "Covlp", "overlap_end" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T4" }, )"
	R"({ "type" : "end_of_line", "name" : "C" }, )"

	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T5", "connect" : { "to" : "P2" } }, )"
	R"({ "type" : "end_of_line", "name" : "D" } )"
"] }";

class overlap_ops_test_class_1 {
	public:
	world *w;
	error_collection ec;
	generic_signal *s1;
	generic_signal *s2;
	generic_points *p1;
	generic_points *p2;
	routing_point *a;
	routing_point *b;
	routing_point *c;
	routing_point *d;
	routing_point *bovlp;
	routing_point *covlp;
	routing_point *dovlp;

	overlap_ops_test_class_1(world *w_) : w(w_) {
		s1 = w->FindTrackByNameCast<generic_signal>("S1");
		s2 = w->FindTrackByNameCast<generic_signal>("S2");
		p1 = w->FindTrackByNameCast<generic_points>("P1");
		p2 = w->FindTrackByNameCast<generic_points>("P2");
		a = w->FindTrackByNameCast<routing_point>("A");
		b = w->FindTrackByNameCast<routing_point>("B");
		c = w->FindTrackByNameCast<routing_point>("C");
		d = w->FindTrackByNameCast<routing_point>("D");
		bovlp = w->FindTrackByNameCast<routing_point>("Bovlp");
		covlp = w->FindTrackByNameCast<routing_point>("Covlp");
		dovlp = w->FindTrackByNameCast<routing_point>("Dovlp");
	}
	void checksignal(generic_signal *signal, unsigned int aspect, route_class::ID aspect_type, routing_point *aspect_target, routing_point *aspect_route_target, routing_point *overlapend) {
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
		std::vector<routing_point::gmr_route_item> route_set;
		REQUIRE(tenv.s2->GetMatchingRoutes(route_set, tenv.b, route_class::All(), GMRF::DONT_SORT) == 1);
		REQUIRE(tenv.s2->GetMatchingRoutes(route_set, tenv.c, route_class::All(), GMRF::DONT_CLEAR_VECTOR | GMRF::DONT_SORT) == 1);
		REQUIRE(tenv.s2->GetMatchingRoutes(route_set, tenv.covlp, route_class::All(), GMRF::DONT_CLEAR_VECTOR | GMRF::DONT_SORT) == 1);
		REQUIRE(route_set.size() == 3);

		CHECK(route_set[0].rt->IsRouteSubSet(route_set[2].rt) == false);
		CHECK(route_set[1].rt->IsRouteSubSet(route_set[2].rt) == true);

		tenv.checksignal(tenv.s1, 0, route_class::ID::NONE, 0, 0, 0);
		tenv.checksignal(tenv.s2, 0, route_class::ID::NONE, 0, 0, tenv.bovlp);

		env.w->GameStep(1);
		tenv.checksignal(tenv.s1, 1, route_class::ID::ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 0, route_class::ID::NONE, 0, 0, tenv.bovlp);

		env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s2, tenv.c));
		RoundTrip();
		env.w->GameStep(1);
		CHECK(env.w->GetLogText() == "");
		REQUIRE(tenv.p1 != 0);
		CHECK(tenv.p1->GetPointsFlags(0) == (generic_points::PTF::OOC | generic_points::PTF::REV));
		tenv.checksignal(tenv.s1, 1, route_class::ID::ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 0, route_class::ID::NONE, 0, 0, tenv.covlp);

		RoundTrip();
		env.w->GameStep(100000);
		CHECK(env.w->GetLogText() == "");
		CHECK(tenv.p1->GetPointsFlags(0) == generic_points::PTF::REV);
		tenv.checksignal(tenv.s1, 2, route_class::ID::ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 1, route_class::ID::ROUTE, tenv.c, tenv.c, tenv.covlp);

		env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s2, tenv.b));
		RoundTrip();
		env.w->GameStep(1);
		CHECK(env.w->GetLogText() != "");
		CHECK(tenv.p1->GetPointsFlags(0) == generic_points::PTF::REV);
		tenv.checksignal(tenv.s1, 2, route_class::ID::ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 1, route_class::ID::ROUTE, tenv.c, tenv.c, tenv.covlp);
		env.w->ResetLogText();

		env.w->SubmitAction(action_unreserve_track(*(env.w), *tenv.s2));
		RoundTrip();
		env.w->GameStep(1);
		CHECK(env.w->GetLogText() == "");
		tenv.checksignal(tenv.s1, 1, route_class::ID::ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 0, route_class::ID::NONE, 0, 0, tenv.covlp);

		env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s2, tenv.d));
		RoundTrip();
		env.w->GameStep(1);
		CHECK(env.w->GetLogText() != "");
		tenv.checksignal(tenv.s1, 1, route_class::ID::ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 0, route_class::ID::NONE, 0, 0, tenv.covlp);
		env.w->ResetLogText();
	});
}

TEST_CASE( "track/ops/overlap/pointsswing", "Test points movement overlap swinging" ) {
	using PTF = generic_points::PTF;
	OverlapOpsRoundTripMultiTest([](test_fixture_world_init_checked &env, overlap_ops_test_class_1 &tenv, std::function<void()> RoundTrip) {
		env.w->GameStep(1);
		tenv.checksignal(tenv.s1, 1, route_class::ID::ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 0, route_class::ID::NONE, 0, 0, tenv.bovlp);

		REQUIRE(tenv.p1 != 0);
		env.w->SubmitAction(action_points_action(*(env.w), *tenv.p1, 0, PTF::REV, PTF::REV));
		RoundTrip();
		env.w->GameStep(1);

		CHECK(env.w->GetLogText() == "");
		CHECK(tenv.p1->GetPointsFlags(0) == (PTF::OOC | PTF::REV));
		tenv.checksignal(tenv.s2, 0, route_class::ID::NONE, 0, 0, tenv.covlp);

		env.w->SubmitAction(action_points_action(*(env.w), *tenv.p1, 0, PTF::LOCKED, PTF::LOCKED));
		RoundTrip();
		env.w->GameStep(1);

		REQUIRE(tenv.p2 != 0);
		env.w->SubmitAction(action_points_action(*(env.w), *tenv.p2, 0, PTF::REV, PTF::REV));
		RoundTrip();
		env.w->GameStep(1);

		CHECK(env.w->GetLogText() != "");
		CHECK(tenv.p1->GetPointsFlags(0) == (PTF::LOCKED | PTF::OOC | PTF::REV));
		CHECK(tenv.p2->GetPointsFlags(0) == PTF::ZERO);
		tenv.checksignal(tenv.s2, 0, route_class::ID::NONE, 0, 0, tenv.covlp);
		env.w->ResetLogText();

		env.w->SubmitAction(action_points_action(*(env.w), *tenv.p1, 0, PTF::ZERO, PTF::LOCKED));
		RoundTrip();
		env.w->GameStep(1);

		env.w->SubmitAction(action_points_action(*(env.w), *tenv.p2, 0, PTF::REV, PTF::REV));
		RoundTrip();
		env.w->GameStep(1);

		CHECK(env.w->GetLogText() == "");
		CHECK(tenv.p1->GetPointsFlags(0) == PTF::OOC);
		CHECK(tenv.p2->GetPointsFlags(0) == (PTF::OOC | PTF::REV));
		tenv.checksignal(tenv.s2, 0, route_class::ID::NONE, 0, 0, tenv.bovlp);
		env.w->ResetLogText();

		env.w->SubmitAction(action_points_action(*(env.w), *tenv.p1, 0, PTF::REV, PTF::REV));
		RoundTrip();
		env.w->GameStep(1);

		CHECK(env.w->GetLogText() == "");
		CHECK(tenv.p1->GetPointsFlags(0) == (PTF::OOC | PTF::REV));
		CHECK(tenv.p2->GetPointsFlags(0) == PTF::OOC);
		tenv.checksignal(tenv.s2, 0, route_class::ID::NONE, 0, 0, tenv.covlp);
		env.w->ResetLogText();

		env.w->SubmitAction(action_points_action(*(env.w), *tenv.p1, 0, PTF::ZERO, PTF::REV));
		env.w->GameStep(1);
		env.w->SubmitAction(action_points_action(*(env.w), *tenv.p2, 0, PTF::REV | PTF::REMINDER, PTF::REV | PTF::REMINDER));
		env.w->GameStep(1);
		RoundTrip();
		CHECK(env.w->GetLogText() == "");
		tenv.checksignal(tenv.s2, 0, route_class::ID::NONE, 0, 0, tenv.bovlp);

		env.w->SubmitAction(action_points_action(*(env.w), *tenv.p1, 0, PTF::REV, PTF::REV));
		RoundTrip();
		env.w->GameStep(1);
		CHECK(env.w->GetLogText() != "");
		tenv.checksignal(tenv.s2, 0, route_class::ID::NONE, 0, 0, tenv.bovlp);
	});
}

TEST_CASE( "track/ops/reservation/overset", "Test overset track reservation and dereservation" ) {
	OverlapOpsRoundTripMultiTest([](test_fixture_world_init_checked &env, overlap_ops_test_class_1 &tenv, std::function<void()> RoundTrip) {
		env.w->GameStep(1);
		tenv.checksignal(tenv.s1, 1, route_class::ID::ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 0, route_class::ID::NONE, 0, 0, tenv.bovlp);

		CHECK(tenv.s2->ReservationEnumeration([&](const route *reserved_route, EDGE direction, unsigned int index, RRF rr_flags) { }, RRF::RESERVE) == 2);

		env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s2, tenv.c));
		env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s2, tenv.c));
		RoundTrip();
		env.w->GameStep(100000);
		RoundTrip();
		CHECK(env.w->GetLogText() == "");
		tenv.checksignal(tenv.s1, 2, route_class::ID::ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 1, route_class::ID::ROUTE, tenv.c, tenv.c, tenv.covlp);

		env.w->SubmitAction(action_reserve_path(*(env.w), tenv.s2, tenv.c));
		RoundTrip();
		env.w->GameStep(100000);
		RoundTrip();
		CHECK(env.w->GetLogText() == "");
		tenv.checksignal(tenv.s1, 2, route_class::ID::ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 1, route_class::ID::ROUTE, tenv.c, tenv.c, tenv.covlp);

		CHECK(tenv.s2->ReservationEnumeration([&](const route *reserved_route, EDGE direction, unsigned int index, RRF rr_flags) { }, RRF::RESERVE) == 3);

		env.w->SubmitAction(action_unreserve_track(*(env.w), *tenv.s2));
		RoundTrip();
		env.w->GameStep(100000);
		RoundTrip();
		CHECK(env.w->GetLogText() == "");
		tenv.checksignal(tenv.s1, 1, route_class::ID::ROUTE, tenv.s2, tenv.s2, 0);
		tenv.checksignal(tenv.s2, 0, route_class::ID::NONE, 0, 0, tenv.covlp);

		CHECK(tenv.s2->ReservationEnumeration([&](const route *reserved_route, EDGE direction, unsigned int index, RRF rr_flags) { }, RRF::RESERVE) == 2);
	});
}

std::string overlap_ops_test_str_2 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "new_type" : "4aspectauto", "base_type" : "auto_signal", "content" : { "max_aspect" : 3 } }, )"
	R"({ "type" : "typedef", "new_type" : "4aspectroute", "base_type" : "route_signal", "content" : { "max_aspect" : 3, "route_signal" : true } }, )"

	R"({ "type" : "start_of_line", "name" : "A" }, )"
	R"({ "type" : "4aspectauto", "name" : "S1" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S1ovlp" }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T2" }, )"
	R"({ "type" : "4aspectroute", "name" : "S2", "overlap_swingable" : true }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S2ovlp" }, )"
	R"({ "type" : "points", "name" : "P1" }, )"
	R"({ "type" : "points", "name" : "P4", "reverse_auto_connection" : true  }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "P1ovlp_norm" }, )"
	R"({ "type" : "routing_marker", "name" : "B", "overlap_end" : true }, )"
	R"({ "type" : "end_of_line", "name" : "G" }, )"

	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "P1ovlp_common", "connect" : { "to" : "P1" } }, )"
	R"({ "type" : "points", "name" : "P2", "reverse_auto_connection" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "P1ovlp_common" }, )"
	R"({ "type" : "routing_marker", "name" : "C", "overlap_end" : true }, )"
	R"({ "type" : "end_of_line", "name" : "F" }, )"

	R"({ "type" : "start_of_line", "name" : "D" }, )"
	R"({ "type" : "4aspectauto", "name" : "S3" }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S3ovlp" }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T4" }, )"
	R"({ "type" : "4aspectroute", "name" : "S4", "overlap_swingable" : true }, )"
	R"({ "type" : "track_seg", "length" : 20000, "track_circuit" : "S4ovlp" }, )"
	R"({ "type" : "points", "name" : "P3", "connect" : { "to" : "P2" }, "reverse" : true }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "P2ovlp_norm" }, )"
	R"({ "type" : "routing_marker", "name" : "E", "overlap_end" : true }, )"
	R"({ "type" : "end_of_line" }, )"

	R"({ "type" : "start_of_line" }, )"
	R"({ "type" : "4aspectroute", "name" : "S5" }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T5" }, )"
	R"({ "type" : "4aspectroute", "name" : "S6" }, )"
	R"({ "type" : "track_seg", "length" : 30000, "track_circuit" : "T6", "connect" : { "to" : "P4" }  } )"
"] }";

TEST_CASE( "track/ops/reservation/overlap/dualswing", "Test points movement overlap dual swinging" ) {
	test_fixture_world_init_checked env(overlap_ops_test_str_2);
	points *p1 = PTR_CHECK(env.w->FindTrackByNameCast<points>("P1"));
	points *p3 = PTR_CHECK(env.w->FindTrackByNameCast<points>("P3"));
	route_signal *s2 = PTR_CHECK(env.w->FindTrackByNameCast<route_signal>("S2"));
	route_signal *s4 = PTR_CHECK(env.w->FindTrackByNameCast<route_signal>("S4"));
	route_signal *s5 = PTR_CHECK(env.w->FindTrackByNameCast<route_signal>("S5"));
	route_signal *s6 = PTR_CHECK(env.w->FindTrackByNameCast<route_signal>("S6"));
	routing_point *b = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("B"));
	routing_point *c = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("C"));
	routing_point *e = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("E"));
	routing_point *f = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("F"));
	routing_point *g = PTR_CHECK(env.w->FindTrackByNameCast<routing_point>("G"));

	CHECK(PTR_CHECK(s2->GetCurrentForwardOverlap())->end.track == b);
	CHECK(PTR_CHECK(s4->GetCurrentForwardOverlap())->end.track == c);

	auto check_swung = [&]() {
		env.w->GameStep(100);
		CHECK(p1->GetPointsFlags(0) == (generic_points::PTF::OOC | generic_points::PTF::REV));
		CHECK(p3->GetPointsFlags(0) == generic_points::PTF::OOC);
		CHECK(PTR_CHECK(s2->GetCurrentForwardOverlap())->end.track == c);
		CHECK(PTR_CHECK(s4->GetCurrentForwardOverlap())->end.track == e);
		CHECK(env.w->GetLogText() == "");
	};
	auto check_swing_failed = [&]() {
		env.w->GameStep(100);
		CHECK(p1->GetPointsFlags(0) == generic_points::PTF::ZERO);
		CHECK(p3->GetPointsFlags(0) == (generic_points::PTF::REV | generic_points::PTF::LOCKED));
		CHECK(PTR_CHECK(s2->GetCurrentForwardOverlap())->end.track == b);
		CHECK(PTR_CHECK(s4->GetCurrentForwardOverlap())->end.track == c);
		CHECK(env.w->GetLogText() != "");
	};

	SECTION("Swing") {
		env.w->SubmitAction(action_points_action(*(env.w), *p1, 0, generic_points::PTF::REV, generic_points::PTF::REV));
		check_swung();
	}
	SECTION("Route") {
		env.w->SubmitAction(action_reserve_path(*(env.w), s2, f));
		check_swung();
	}
	SECTION("Route reserve") {
		env.w->SubmitAction(action_reserve_path(*(env.w), s6, g));
		check_swung();
		CHECK(PTR_CHECK(s6->GetCurrentForwardRoute())->end.track == g);
	}
	SECTION("Overlap reserve") {
		env.w->SubmitAction(action_reserve_path(*(env.w), s5, s6));
		check_swung();
		CHECK(PTR_CHECK(s5->GetCurrentForwardRoute())->end.track == s6);
		CHECK(PTR_CHECK(s6->GetCurrentForwardOverlap())->end.track == b);
	}
	SECTION("Swing: Failed") {
		env.w->SubmitAction(action_points_action(*(env.w), *p3, 0, generic_points::PTF::LOCKED, generic_points::PTF::LOCKED));
		env.w->GameStep(100);
		env.w->SubmitAction(action_points_action(*(env.w), *p1, 0, generic_points::PTF::REV, generic_points::PTF::REV));
		env.w->GameStep(100);
		check_swing_failed();
	}
	SECTION("Route: Failed") {
		env.w->SubmitAction(action_points_action(*(env.w), *p3, 0, generic_points::PTF::LOCKED, generic_points::PTF::LOCKED));
		env.w->GameStep(100);
		env.w->SubmitAction(action_reserve_path(*(env.w), s2, f));
		check_swing_failed();
	}
	SECTION("Route reserve: Failed") {
		env.w->SubmitAction(action_points_action(*(env.w), *p3, 0, generic_points::PTF::LOCKED, generic_points::PTF::LOCKED));
		env.w->GameStep(100);
		env.w->SubmitAction(action_reserve_path(*(env.w), s6, g));
		check_swing_failed();
		CHECK(s6->GetCurrentForwardRoute() == nullptr);
	}
	SECTION("Overlap reserve: Failed") {
		env.w->SubmitAction(action_points_action(*(env.w), *p3, 0, generic_points::PTF::LOCKED, generic_points::PTF::LOCKED));
		env.w->GameStep(100);
		env.w->SubmitAction(action_reserve_path(*(env.w), s5, s6));
		check_swing_failed();
		CHECK(s5->GetCurrentForwardRoute() == nullptr);
		CHECK(s6->GetCurrentForwardOverlap() == nullptr);
	}
}
