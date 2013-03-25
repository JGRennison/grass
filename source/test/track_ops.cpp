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
#include "track_ops.h"
#include "world-test.h"
#include "track.h"
#include "points.h"
#include "textpool.h"
#include "var.h"
#include "signal.h"
#include "deserialisation-test.h"

struct test_fixture_world_ops_1 {
	world_test w;
	points p1;
	test_fixture_world_ops_1() : p1(w) {
		p1.SetName("P1");
	}
};

TEST_CASE( "track/ops/points/movement", "Test basic points movement future" ) {
	test_fixture_world_ops_1 env;

	env.w.SubmitAction(action_pointsaction(env.w, env.p1, 0, genericpoints::PTF::REV, genericpoints::PTF::REV));

	REQUIRE(env.p1.GetPointFlags(0) == genericpoints::PTF::ZERO);
	env.w.GameStep(500);
	REQUIRE(env.p1.GetPointFlags(0) == (genericpoints::PTF::REV | genericpoints::PTF::OOC));
	env.w.GameStep(4500);
	REQUIRE(env.p1.GetPointFlags(0) == genericpoints::PTF::REV);

	env.w.SubmitAction(action_pointsaction(env.w, env.p1, 0, genericpoints::PTF::LOCKED, genericpoints::PTF::LOCKED));
	env.w.GameStep(50);
	env.w.SubmitAction(action_pointsaction(env.w, env.p1, 0, genericpoints::PTF::REV, genericpoints::PTF::REV));
	env.w.GameStep(50);
	REQUIRE(env.p1.GetPointFlags(0) == (genericpoints::PTF::REV | genericpoints::PTF::LOCKED));
	env.w.SubmitAction(action_pointsaction(env.w, env.p1, 0, genericpoints::PTF::ZERO, genericpoints::PTF::REV));
	env.w.GameStep(4900);
	REQUIRE(env.p1.GetPointFlags(0) == (genericpoints::PTF::REV | genericpoints::PTF::LOCKED));
	REQUIRE(env.w.GetLogText() == "Points P1 not movable: Locked\n");
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
	world &w;
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

	overlap_ops_test_class_1(world &w_) : w(w_) {
		s1 = w.FindTrackByNameCast<genericsignal>("S1");
		s2 = w.FindTrackByNameCast<genericsignal>("S2");
		p1 = w.FindTrackByNameCast<genericpoints>("P1");
		p2 = w.FindTrackByNameCast<genericpoints>("P2");
		a = w.FindTrackByNameCast<routingpoint>("A");
		b = w.FindTrackByNameCast<routingpoint>("B");
		c = w.FindTrackByNameCast<routingpoint>("C");
		d = w.FindTrackByNameCast<routingpoint>("D");
		bovlp = w.FindTrackByNameCast<routingpoint>("Bovlp");
		covlp = w.FindTrackByNameCast<routingpoint>("Covlp");
		dovlp = w.FindTrackByNameCast<routingpoint>("Dovlp");
	}
	void checksignal(genericsignal *signal, unsigned int aspect, ROUTE_CLASS aspect_type, routingpoint *aspect_target, routingpoint *aspect_route_target, routingpoint *overlapend) {
		REQUIRE(signal != 0);
		SCOPED_INFO("Signal check for: " << signal->GetName() << ", at time: " << w.GetGameTime());

		CHECK(signal->GetAspect() == aspect);
		CHECK(signal->GetAspectType() == aspect_type);
		CHECK(signal->GetAspectNextTarget() == aspect_target);
		CHECK(signal->GetAspectRouteTarget() == aspect_route_target);
		const route *ovlp = signal->GetCurrentForwardOverlap();
		if(overlapend) {
			CHECK(ovlp != 0);
			if(ovlp) CHECK(ovlp->end.track == overlapend);
		}
		else CHECK(ovlp == 0);
	}
};

TEST_CASE( "track/ops/overlap/swing", "Test track reservation overlap swinging" ) {
	test_fixture_world env(overlap_ops_test_str_1);

	env.w.LayoutInit(env.ec);
	env.w.PostLayoutInit(env.ec);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	overlap_ops_test_class_1 tenv(env.w);

	std::vector<routingpoint::gmr_routeitem> routeset;
	REQUIRE(tenv.s2->GetMatchingRoutes(routeset, tenv.b, GMRF::ALLROUTETYPES | GMRF::DONTSORT) == 1);
	REQUIRE(tenv.s2->GetMatchingRoutes(routeset, tenv.c, GMRF::ALLROUTETYPES | GMRF::DONTCLEARVECTOR | GMRF::DONTSORT) == 1);
	REQUIRE(tenv.s2->GetMatchingRoutes(routeset, tenv.covlp, GMRF::ALLROUTETYPES | GMRF::DONTCLEARVECTOR | GMRF::DONTSORT) == 1);
	REQUIRE(routeset.size() == 3);

	CHECK(routeset[0].rt->IsRouteSubSet(routeset[2].rt) == false);
	CHECK(routeset[1].rt->IsRouteSubSet(routeset[2].rt) == true);

	tenv.checksignal(tenv.s1, 0, RTC_NULL, 0, 0, 0);
	tenv.checksignal(tenv.s2, 0, RTC_NULL, 0, 0, tenv.bovlp);

	env.w.GameStep(1);
	tenv.checksignal(tenv.s1, 1, RTC_ROUTE, tenv.s2, tenv.s2, 0);
	tenv.checksignal(tenv.s2, 0, RTC_NULL, 0, 0, tenv.bovlp);

	env.w.SubmitAction(action_reservepath(env.w, tenv.s2, tenv.c));
	env.w.GameStep(1);
	CHECK(env.w.GetLogText() == "");
	REQUIRE(tenv.p1 != 0);
	CHECK(tenv.p1->GetPointFlags(0) == (genericpoints::PTF::OOC | genericpoints::PTF::REV));
	tenv.checksignal(tenv.s1, 1, RTC_ROUTE, tenv.s2, tenv.s2, 0);
	tenv.checksignal(tenv.s2, 0, RTC_NULL, 0, 0, tenv.covlp);

	env.w.GameStep(100000);
	CHECK(env.w.GetLogText() == "");
	CHECK(tenv.p1->GetPointFlags(0) == genericpoints::PTF::REV);
	tenv.checksignal(tenv.s1, 2, RTC_ROUTE, tenv.s2, tenv.s2, 0);
	tenv.checksignal(tenv.s2, 1, RTC_ROUTE, tenv.c, tenv.c, tenv.covlp);

	env.w.SubmitAction(action_reservepath(env.w, tenv.s2, tenv.b));
	env.w.GameStep(1);
	CHECK(env.w.GetLogText() != "");
	CHECK(tenv.p1->GetPointFlags(0) == genericpoints::PTF::REV);
	tenv.checksignal(tenv.s1, 2, RTC_ROUTE, tenv.s2, tenv.s2, 0);
	tenv.checksignal(tenv.s2, 1, RTC_ROUTE, tenv.c, tenv.c, tenv.covlp);
	env.w.ResetLogText();

	env.w.SubmitAction(action_unreservetrack(env.w, *tenv.s2));
	env.w.GameStep(1);
	CHECK(env.w.GetLogText() == "");
	tenv.checksignal(tenv.s1, 1, RTC_ROUTE, tenv.s2, tenv.s2, 0);
	tenv.checksignal(tenv.s2, 0, RTC_NULL, 0, 0, tenv.covlp);

	env.w.SubmitAction(action_reservepath(env.w, tenv.s2, tenv.d));
	env.w.GameStep(1);
	CHECK(env.w.GetLogText() != "");
	tenv.checksignal(tenv.s1, 1, RTC_ROUTE, tenv.s2, tenv.s2, 0);
	tenv.checksignal(tenv.s2, 0, RTC_NULL, 0, 0, tenv.covlp);
	env.w.ResetLogText();
}
