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
#include "signal.h"
#include "traverse.h"
#include "deserialisation-test.h"
#include "world-test.h"
#include "trackcircuit.h"
#include "track_ops.h"
#include "testutil.h"

std::string track_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "startofline", "name" : "A" }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routesignal", "name" : "S1", "routeshuntsignal" : true }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routingmarker", "overlapend" : true, "connect" : { "fromdirection" : "back", "to" : "DS1", "todirection" : "leftfront" } }, )"
	R"({ "type" : "doubleslip", "name" : "DS1", "notrack_fl_bl" : true }, )"
	R"({ "type" : "startofline", "name" : "C" }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routesignal", "name" : "S3", "shuntsignal" : true, "connect" : { "fromdirection" : "back", "to" : "DS1", "todirection" : "leftback" } }, )"
	R"({ "type" : "startofline", "name" : "D" }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routesignal", "name" : "S6", "routeshuntsignal" : true }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routingmarker", "overlapend" : true, "through_rev" : { "deny" : "route" }, "connect" : { "fromdirection" : "back", "to" : "DS1", "todirection" : "rightback" } }, )"
	R"({ "type" : "startofline", "name" : "B" }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routesignal", "name" : "S2", "routeshuntsignal" : true, "routerestrictions" : [ { "targets" : "C", "deny" : "route" } ] }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "points", "name" : "P1", "connect" : { "fromdirection" : "reverse", "to" : "DS1", "todirection" : "rightfront" } }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routingmarker", "overlapend_rev" : true, "through" : { "deny" : "all" } }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routesignal", "name" : "S4", "reverseautoconnection" : true, "routeshuntsignal" : true, "through_rev" : { "deny" : "all" }, "overlapend" : true}, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "autosignal", "name" : "S5", "reverseautoconnection" : true }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "endofline", "name" : "E" } )"
"] }";

TEST_CASE( "signal/deserialisation/general", "Test basic signal and routing deserialisation" ) {

	test_fixture_world env(track_test_str_1);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	const route_class::set shuntset = route_class::Flag(route_class::RTC_SHUNT);
	const route_class::set routeset = route_class::Flag(route_class::RTC_ROUTE);
	const route_class::set overlapset = route_class::Flag(route_class::RTC_OVERLAP);

	startofline *a = dynamic_cast<startofline *>(env.w.FindTrackByName("A"));
	REQUIRE(a != 0);
	CHECK(a->GetAvailableRouteTypes(EDGE_FRONT) == RPRT(0, 0, route_class::AllNonOverlaps()));
	CHECK(a->GetAvailableRouteTypes(EDGE_BACK) == RPRT());
	CHECK(a->GetSetRouteTypes(EDGE_FRONT) == RPRT());
	CHECK(a->GetSetRouteTypes(EDGE_BACK) == RPRT());

	routesignal *s1 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S1"));
	REQUIRE(s1 != 0);
	CHECK(s1->GetAvailableRouteTypes(EDGE_FRONT) == RPRT(shuntset | routeset | route_class::AllOverlaps(), 0, shuntset | routeset));
	CHECK(s1->GetAvailableRouteTypes(EDGE_BACK) == RPRT(0, route_class::AllNonOverlaps(), 0));
	CHECK(s1->GetSetRouteTypes(EDGE_FRONT) == RPRT());
	CHECK(s1->GetSetRouteTypes(EDGE_BACK) == RPRT());

	routingmarker *rm = dynamic_cast<routingmarker *>(env.w.FindTrackByName("#4"));
	REQUIRE(rm != 0);
	CHECK(rm->GetAvailableRouteTypes(EDGE_FRONT) == RPRT(0, route_class::All(), overlapset));
	CHECK(rm->GetAvailableRouteTypes(EDGE_BACK) == RPRT(0, route_class::All(), 0));
	CHECK(rm->GetSetRouteTypes(EDGE_FRONT) == RPRT());
	CHECK(rm->GetSetRouteTypes(EDGE_BACK) == RPRT());

	routesignal *s3 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S3"));
	REQUIRE(s3 != 0);
	CHECK(s3->GetAvailableRouteTypes(EDGE_FRONT) == RPRT(shuntset | route_class::AllOverlaps(), 0, shuntset));
	CHECK(s3->GetAvailableRouteTypes(EDGE_BACK) == RPRT(0, route_class::AllNonOverlaps(), 0));
	CHECK(s3->GetSetRouteTypes(EDGE_FRONT) == RPRT());
	CHECK(s3->GetSetRouteTypes(EDGE_BACK) == RPRT());

	routingmarker *rm2 = dynamic_cast<routingmarker *>(env.w.FindTrackByName("#13"));
	REQUIRE(rm2 != 0);
	CHECK(rm2->GetAvailableRouteTypes(EDGE_FRONT) == RPRT(0, route_class::All(), overlapset));
	CHECK(rm2->GetAvailableRouteTypes(EDGE_BACK) == RPRT(0, route_class::All() & ~routeset, 0));
	CHECK(rm2->GetSetRouteTypes(EDGE_FRONT) == RPRT());
	CHECK(rm2->GetSetRouteTypes(EDGE_BACK) == RPRT());

	routesignal *s2 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S2"));
	REQUIRE(s2 != 0);
	CHECK(s2->GetRouteRestrictions().GetRestrictionCount() == 1);
}

TEST_CASE( "signal/routing/general", "Test basic signal and routing connectivity and route creation" ) {
	test_fixture_world env(track_test_str_1);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	env.w.LayoutInit(env.ec);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	env.w.PostLayoutInit(env.ec);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	routesignal *s1 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S1"));
	REQUIRE(s1 != 0);
	routesignal *s2 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S2"));
	REQUIRE(s2 != 0);
	routesignal *s3 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S3"));
	REQUIRE(s3 != 0);
	routesignal *s4 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S4"));
	REQUIRE(s4 != 0);
	autosignal *s5 = dynamic_cast<autosignal *>(env.w.FindTrackByName("S5"));
	REQUIRE(s5 != 0);
	routesignal *s6 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S6"));
	REQUIRE(s6 != 0);

	startofline *a = dynamic_cast<startofline *>(env.w.FindTrackByName("A"));
	REQUIRE(a != 0);
	startofline *b = dynamic_cast<startofline *>(env.w.FindTrackByName("B"));
	REQUIRE(b != 0);
	startofline *c = dynamic_cast<startofline *>(env.w.FindTrackByName("C"));
	REQUIRE(c != 0);
	startofline *d = dynamic_cast<startofline *>(env.w.FindTrackByName("D"));
	REQUIRE(d != 0);
	startofline *e = dynamic_cast<startofline *>(env.w.FindTrackByName("E"));
	REQUIRE(e != 0);

	auto s5check = [&](unsigned int index, route_class::ID type) {
		REQUIRE(s5->GetRouteByIndex(index) != 0);
		CHECK(s5->GetRouteByIndex(index)->type == type);
		CHECK(s5->GetRouteByIndex(index)->end == vartrack_target_ptr<routingpoint>(s4, EDGE_FRONT));
		CHECK(s5->GetRouteByIndex(index)->pieces.size() == 1);
	};
	s5check(0, route_class::RTC_ROUTE);
	s5check(1, route_class::RTC_OVERLAP);

	std::vector<routingpoint::gmr_routeitem> routeset;
	CHECK(s1->GetMatchingRoutes(routeset, a, route_class::All()) == 0);
	CHECK(s1->GetMatchingRoutes(routeset, b, route_class::All()) == 0);
	CHECK(s1->GetMatchingRoutes(routeset, c, route_class::All()) == 0);
	CHECK(s1->GetMatchingRoutes(routeset, d, route_class::All()) == 1);
	CHECK(s1->GetMatchingRoutes(routeset, e, route_class::All()) == 0);
	CHECK(s1->GetMatchingRoutes(routeset, s1, route_class::All()) == 0);

	CHECK(s2->GetMatchingRoutes(routeset, a, route_class::All()) == 0);
	CHECK(s2->GetMatchingRoutes(routeset, b, route_class::All()) == 0);
	CHECK(s2->GetMatchingRoutes(routeset, c, route_class::All()) == 1);
	CHECK(s2->GetMatchingRoutes(routeset, d, route_class::All()) == 1);
	CHECK(s2->GetMatchingRoutes(routeset, e, route_class::All()) == 0);
	CHECK(s2->GetMatchingRoutes(routeset, s2, route_class::All()) == 0);

	CHECK(s3->GetMatchingRoutes(routeset, a, route_class::All()) == 0);
	CHECK(s3->GetMatchingRoutes(routeset, b, route_class::All()) == 1);
	CHECK(s3->GetMatchingRoutes(routeset, c, route_class::All()) == 0);
	CHECK(s3->GetMatchingRoutes(routeset, d, route_class::All()) == 0);
	CHECK(s3->GetMatchingRoutes(routeset, e, route_class::All()) == 0);
	CHECK(s3->GetMatchingRoutes(routeset, s3, route_class::All()) == 0);

	CHECK(s4->GetMatchingRoutes(routeset, a, route_class::All()) == 0);
	CHECK(s4->GetMatchingRoutes(routeset, b, route_class::All()) == 2);
	CHECK(s4->GetMatchingRoutes(routeset, c, route_class::All()) == 0);
	CHECK(s4->GetMatchingRoutes(routeset, d, route_class::All()) == 0);
	CHECK(s4->GetMatchingRoutes(routeset, e, route_class::All()) == 0);
	CHECK(s4->GetMatchingRoutes(routeset, s4, route_class::All()) == 0);

	CHECK(s5->GetMatchingRoutes(routeset, a, route_class::All()) == 0);
	CHECK(s5->GetMatchingRoutes(routeset, b, route_class::All()) == 0);
	CHECK(s5->GetMatchingRoutes(routeset, c, route_class::All()) == 0);
	CHECK(s5->GetMatchingRoutes(routeset, d, route_class::All()) == 0);
	CHECK(s5->GetMatchingRoutes(routeset, e, route_class::All()) == 0);
	CHECK(s5->GetMatchingRoutes(routeset, s4, route_class::All()) == 2);
	CHECK(s5->GetMatchingRoutes(routeset, s5, route_class::All()) == 0);

	CHECK(s6->GetMatchingRoutes(routeset, a, route_class::All()) == 2);
	CHECK(s6->GetMatchingRoutes(routeset, b, route_class::All()) == 2);
	CHECK(s6->GetMatchingRoutes(routeset, c, route_class::All()) == 0);
	CHECK(s6->GetMatchingRoutes(routeset, d, route_class::All()) == 0);
	CHECK(s6->GetMatchingRoutes(routeset, e, route_class::All()) == 0);
	CHECK(s6->GetMatchingRoutes(routeset, s6, route_class::All()) == 0);

	auto overlapcheck = [&](genericsignal *s, const std::string &target) {
		routingpoint *rp = dynamic_cast<routingpoint *>(env.w.FindTrackByName(target));
		REQUIRE(s->GetMatchingRoutes(routeset, rp, route_class::All()) == 1);
		CHECK(routeset[0].rt->end.track == rp);
		CHECK(route_class::IsOverlap(routeset[0].rt->type));

		unsigned int overlapcount = 0;
		auto callback = [&](const route *r) {
			if(route_class::IsOverlap(r->type)) overlapcount++;
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

	R"({ "type" : "typedef", "newtype" : "4aspectauto", "basetype" : "autosignal", "content" : { "maxaspect" : 3 } }, )"
	R"({ "type" : "typedef", "newtype" : "4aspectroute", "basetype" : "routesignal", "content" : { "maxaspect" : 3, "routesignal" : true } }, )"
	R"({ "type" : "startofline", "name" : "A" }, )"
	R"({ "type" : "trackseg", "length" : 50000, "trackcircuit" : "T1" }, )"
	R"({ "type" : "4aspectauto", "name" : "S1" }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S1ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T2" }, )"
	R"({ "type" : "4aspectauto", "name" : "S2" }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S2ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T3" }, )"
	R"({ "type" : "4aspectauto", "name" : "S3" }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S3ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T4" }, )"
	R"({ "type" : "4aspectauto", "name" : "S4" }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S4ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T5" }, )"
	R"({ "type" : "4aspectroute", "name" : "S5" }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S5ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T6" }, )"
	R"({ "type" : "4aspectroute", "name" : "S6" }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S6ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T7" }, )"
	R"({ "type" : "endofline", "name" : "B" } )"
"] }";

class autosig_test_class_1 {
	public:
	genericsignal *s1;
	genericsignal *s2;
	genericsignal *s3;
	genericsignal *s4;
	genericsignal *s5;
	genericsignal *s6;
	routingpoint *a;
	routingpoint *b;
	std::function<void(routingpoint *signal, unsigned int aspect, route_class::ID aspect_type, routingpoint *aspect_target, routingpoint *aspect_route_target)> checksignal;

	autosig_test_class_1(world &w) {
		s1 = w.FindTrackByNameCast<genericsignal>("S1");
		s2 = w.FindTrackByNameCast<genericsignal>("S2");
		s3 = w.FindTrackByNameCast<genericsignal>("S3");
		s4 = w.FindTrackByNameCast<genericsignal>("S4");
		s5 = w.FindTrackByNameCast<genericsignal>("S5");
		s6 = w.FindTrackByNameCast<genericsignal>("S6");
		a = w.FindTrackByNameCast<routingpoint>("A");
		b = w.FindTrackByNameCast<routingpoint>("B");

		checksignal = [&](routingpoint *signal, unsigned int aspect, route_class::ID aspect_type, routingpoint *aspect_target, routingpoint *aspect_route_target) {
			REQUIRE(signal != 0);
			SCOPED_INFO("Signal check for: " << signal->GetName() << ", at time: " << w.GetGameTime());

			CHECK(signal->GetAspect() == aspect);
			CHECK(signal->GetAspectType() == aspect_type);
			CHECK(signal->GetAspectNextTarget() == aspect_target);
			CHECK(signal->GetAspectRouteTarget() == aspect_route_target);
		};
	}
};

TEST_CASE( "signal/propagation/autosignal", "Test basic autosignal aspect propagation" ) {
	test_fixture_world_init_checked env(autosig_test_str_1);

	autosig_test_class_1 tenv(env.w);

	tenv.checksignal(tenv.a, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.b, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s1, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s2, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s3, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s4, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s5, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s6, 0, route_class::RTC_NULL, 0, 0);

	env.w.GameStep(1);

	tenv.checksignal(tenv.a, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.b, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s1, 3, route_class::RTC_ROUTE, tenv.s2, tenv.s2);
	tenv.checksignal(tenv.s2, 3, route_class::RTC_ROUTE, tenv.s3, tenv.s3);
	tenv.checksignal(tenv.s3, 2, route_class::RTC_ROUTE, tenv.s4, tenv.s4);
	tenv.checksignal(tenv.s4, 1, route_class::RTC_ROUTE, tenv.s5, tenv.s5);
	tenv.checksignal(tenv.s5, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s6, 0, route_class::RTC_NULL, 0, 0);

	track_circuit *s3ovlp = env.w.FindOrMakeTrackCircuitByName("S3ovlp");
	s3ovlp->SetTCFlagsMasked(track_circuit::TCF::FORCEOCCUPIED, track_circuit::TCF::FORCEOCCUPIED);
	env.w.GameStep(1);

	tenv.checksignal(tenv.s1, 1, route_class::RTC_ROUTE, tenv.s2, tenv.s2);
	tenv.checksignal(tenv.s2, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s3, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s4, 1, route_class::RTC_ROUTE, tenv.s5, tenv.s5);
	tenv.checksignal(tenv.s5, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s6, 0, route_class::RTC_NULL, 0, 0);
}

std::string signalmixture_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "newtype" : "4aspectroute", "basetype" : "routesignal", "content" : { "maxaspect" : 3, "routesignal" : true } }, )"
	R"({ "type" : "startofline", "name" : "A" }, )"
	R"({ "type" : "trackseg", "length" : 50000, "trackcircuit" : "T1" }, )"
	R"({ "type" : "4aspectroute", "name" : "S1" }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S1ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T2" }, )"
	R"({ "type" : "routesignal", "name" : "S2", "shuntsignal" : true }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S2ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T3" }, )"
	R"({ "type" : "4aspectroute", "name" : "S3" }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S3ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T4" }, )"
	R"({ "type" : "routesignal", "name" : "S4", "shuntsignal" : true, "end" : { "allow" : "route" } }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S4ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T5" }, )"
	R"({ "type" : "4aspectroute", "name" : "S5" }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S5ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T6" }, )"
	R"({ "type" : "4aspectroute", "name" : "S6" }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S6ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T7" }, )"
	R"({ "type" : "endofline", "name" : "B" } )"
"] }";

TEST_CASE( "signal/propagation/typemixture", "Test aspect propagation and route creation with a mixture of signal types" ) {
	test_fixture_world_init_checked env(signalmixture_test_str_1);

	autosig_test_class_1 tenv(env.w);

	std::vector<routingpoint::gmr_routeitem> routeset;
	CHECK(tenv.s1->GetMatchingRoutes(routeset, tenv.s2, route_class::All()) == 0);
	if(routeset.size()) {
		WARN("First route found has type: " << routeset[0].rt->type);
	}

	env.w.SubmitAction(action_reservepath(env.w, tenv.s1, tenv.s2));
	env.w.GameStep(1);
	CHECK(env.w.GetLogText() != "");

	env.w.ResetLogText();

	env.w.SubmitAction(action_reservepath(env.w, tenv.s2, tenv.s3));
	env.w.SubmitAction(action_reservepath(env.w, tenv.s3, tenv.s4));
	env.w.SubmitAction(action_reservepath(env.w, tenv.s4, tenv.s5));
	env.w.SubmitAction(action_reservepath(env.w, tenv.s5, tenv.s6));
	env.w.SubmitAction(action_reservepath(env.w, tenv.s6, tenv.b));

	env.w.GameStep(1);
	CHECK(env.w.GetLogText() == "");

	tenv.checksignal(tenv.a, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.b, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s1, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s2, 1, route_class::RTC_SHUNT, tenv.s3, tenv.s3);
	tenv.checksignal(tenv.s3, 1, route_class::RTC_ROUTE, tenv.s4, tenv.s4);
	tenv.checksignal(tenv.s4, 1, route_class::RTC_SHUNT, tenv.s5, tenv.s5);
	tenv.checksignal(tenv.s5, 2, route_class::RTC_ROUTE, tenv.s6, tenv.s6);
	tenv.checksignal(tenv.s6, 1, route_class::RTC_ROUTE, tenv.b, tenv.b);
}

TEST_CASE( "signal/routesignal/reserveaction", "Test basic routesignal reservation action and aspect propagation" ) {
	test_fixture_world_init_checked env(autosig_test_str_1);

	autosig_test_class_1 tenv(env.w);

	std::vector<routingpoint::gmr_routeitem> out;
	unsigned int routecount = tenv.s5->GetMatchingRoutes(out, tenv.s6, route_class::All());
	REQUIRE(routecount == 1);

	env.w.SubmitAction(action_reservetrack(env.w, *(out[0].rt)));

	tenv.checksignal(tenv.a, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.b, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s1, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s2, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s3, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s4, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s5, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s6, 0, route_class::RTC_NULL, 0, 0);

	env.w.GameStep(1);

	tenv.checksignal(tenv.a, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.b, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s1, 3, route_class::RTC_ROUTE, tenv.s2, tenv.s2);
	tenv.checksignal(tenv.s2, 3, route_class::RTC_ROUTE, tenv.s3, tenv.s3);
	tenv.checksignal(tenv.s3, 3, route_class::RTC_ROUTE, tenv.s4, tenv.s4);
	tenv.checksignal(tenv.s4, 2, route_class::RTC_ROUTE, tenv.s5, tenv.s5);
	tenv.checksignal(tenv.s5, 1, route_class::RTC_ROUTE, tenv.s6, tenv.s6);
	tenv.checksignal(tenv.s6, 0, route_class::RTC_NULL, 0, 0);

	routecount = tenv.s6->GetMatchingRoutes(out, tenv.b, route_class::All());
	REQUIRE(routecount == 1);
	REQUIRE(out[0].rt != 0);
	env.w.SubmitAction(action_reservetrack(env.w, *(out[0].rt)));

	env.w.GameStep(1);

	tenv.checksignal(tenv.a, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.b, 0, route_class::RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s1, 3, route_class::RTC_ROUTE, tenv.s2, tenv.s2);
	tenv.checksignal(tenv.s2, 3, route_class::RTC_ROUTE, tenv.s3, tenv.s3);
	tenv.checksignal(tenv.s3, 3, route_class::RTC_ROUTE, tenv.s4, tenv.s4);
	tenv.checksignal(tenv.s4, 3, route_class::RTC_ROUTE, tenv.s5, tenv.s5);
	tenv.checksignal(tenv.s5, 2, route_class::RTC_ROUTE, tenv.s6, tenv.s6);
	tenv.checksignal(tenv.s6, 1, route_class::RTC_ROUTE, tenv.b, tenv.b);
}

std::string approachlocking_test_str_1 =
R"({ "content" : [ )"

	R"({ "type" : "typedef", "newtype" : "4aspectroute", "basetype" : "routesignal", "content" : { "maxaspect" : 3, "routesignal" : true } }, )"
	R"({ "type" : "startofline", "name" : "A" }, )"
	R"({ "type" : "trackseg", "length" : 50000, "trackcircuit" : "T1" }, )"
	R"({ "type" : "4aspectroute", "name" : "S1" }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S1ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T2" }, )"
	R"({ "type" : "4aspectroute", "name" : "S2" }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S2ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T3" }, )"
	R"({ "type" : "4aspectroute", "name" : "S3" }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S3ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T4" }, )"
	R"({ "type" : "4aspectroute", "name" : "S4", "approachlockingtimeout" : 1000, "routerestrictions" : [ { "approachlockingtimeout" : 120000, "targets" : "S5" } ] }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S4ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T5" }, )"
	R"({ "type" : "4aspectroute", "name" : "S5", "approachlockingtimeout" : [ { "routeclass" : "shunt", "timeout" : 1000 }, { "routeclass" : "route", "timeout" : 60000 } ] }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S5ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T6" }, )"
	R"({ "type" : "routesignal", "name" : "S6", "shuntsignal" : true, "end" : { "allow" : "route" }, "approachlockingtimeout" : 45000 }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S6ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T7" }, )"
	R"({ "type" : "endofline", "name" : "B" } )"
"] }";

TEST_CASE( "signal/approachlocking/general", "Test basic approach locking route locking, timing and parameter serialisation" ) {
	test_fixture_world_init_checked env(approachlocking_test_str_1);

	autosig_test_class_1 tenv(env.w);

	env.w.SubmitAction(action_reservepath(env.w, tenv.s1, tenv.s2));
	env.w.SubmitAction(action_reservepath(env.w, tenv.s2, tenv.s3));
	env.w.SubmitAction(action_reservepath(env.w, tenv.s3, tenv.s4));
	env.w.SubmitAction(action_reservepath(env.w, tenv.s4, tenv.s5));
	env.w.SubmitAction(action_reservepath(env.w, tenv.s5, tenv.s6));
	env.w.SubmitAction(action_reservepath(env.w, tenv.s6, tenv.b));

	env.w.GameStep(1);
	CHECK(env.w.GetLogText() == "");

	auto checksignal2 = [&](routingpoint *signal, unsigned int aspect, unsigned int reserved_aspect, route_class::ID aspect_type, routingpoint *target, bool approachlocking) {
		tenv.checksignal(signal, aspect, aspect_type, target, target);
		SCOPED_INFO("Signal check for: " << signal->GetName() << ", at time: " << env.w.GetGameTime());
		CHECK(signal->GetReservedAspect() == reserved_aspect);
		genericsignal *sig = dynamic_cast<genericsignal*>(signal);
		if(sig) {
			CHECK(approachlocking == ((bool) (sig->GetSignalFlags() & GSF::APPROACHLOCKINGMODE)));
		}
	};

	checksignal2(tenv.a, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.b, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s1, 3, 3, route_class::RTC_ROUTE, tenv.s2, false);
	checksignal2(tenv.s2, 3, 3, route_class::RTC_ROUTE, tenv.s3, false);
	checksignal2(tenv.s3, 3, 3, route_class::RTC_ROUTE, tenv.s4, false);
	checksignal2(tenv.s4, 2, 2, route_class::RTC_ROUTE, tenv.s5, false);
	checksignal2(tenv.s5, 1, 1, route_class::RTC_ROUTE, tenv.s6, false);
	checksignal2(tenv.s6, 1, 1, route_class::RTC_SHUNT, tenv.b, false);

	auto routecheck = [&](genericsignal *s, routingpoint *backtarget, routingpoint *forwardtarget, world_time timeout) {
		SCOPED_INFO("Signal route check for: " << s->GetName());
		unsigned int count = 0;
		s->EnumerateCurrentBackwardsRoutes([&](const route *rt){
			count++;
			CHECK(rt->start.track == backtarget);
		});
		CHECK(count == 1);
		const route *rt = s->GetCurrentForwardRoute();
		REQUIRE(rt != 0);
		CHECK(rt->approachlocking_timeout == timeout);
		CHECK(rt->end.track == forwardtarget);
	};
	routecheck(tenv.s4, tenv.s3, tenv.s5, 120000);
	routecheck(tenv.s5, tenv.s4, tenv.s6, 60000);
	routecheck(tenv.s6, tenv.s5, tenv.b, 45000);

	env.w.FindOrMakeTrackCircuitByName("T3")->SetTCFlagsMasked(track_circuit::TCF::FORCEOCCUPIED, track_circuit::TCF::FORCEOCCUPIED);
	env.w.FindOrMakeTrackCircuitByName("T4")->SetTCFlagsMasked(track_circuit::TCF::FORCEOCCUPIED, track_circuit::TCF::FORCEOCCUPIED);
	env.w.GameStep(1);

	checksignal2(tenv.s1, 1, 1, route_class::RTC_ROUTE, tenv.s2, false);
	checksignal2(tenv.s2, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s4, 2, 2, route_class::RTC_ROUTE, tenv.s5, false);
	checksignal2(tenv.s5, 1, 1, route_class::RTC_ROUTE, tenv.s6, false);
	checksignal2(tenv.s6, 1, 1, route_class::RTC_SHUNT, tenv.b, false);

	env.w.SubmitAction(action_unreservetrack(env.w, *tenv.s1));
	env.w.SubmitAction(action_unreservetrack(env.w, *tenv.s4));
	env.w.GameStep(1);

	checksignal2(tenv.s1, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s4, 0, 2, route_class::RTC_ROUTE, tenv.s5, true);
	checksignal2(tenv.s5, 1, 1, route_class::RTC_ROUTE, tenv.s6, false);
	checksignal2(tenv.s6, 1, 1, route_class::RTC_SHUNT, tenv.b, false);

	env.w.SubmitAction(action_unreservetrack(env.w, *tenv.s5));
	env.w.GameStep(1);

	checksignal2(tenv.s1, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s4, 0, 2, route_class::RTC_ROUTE, tenv.s5, true);
	checksignal2(tenv.s5, 0, 1, route_class::RTC_ROUTE, tenv.s6, true);
	checksignal2(tenv.s6, 1, 1, route_class::RTC_SHUNT, tenv.b, false);

	env.w.SubmitAction(action_unreservetrack(env.w, *tenv.s6));
	env.w.GameStep(1);

	checksignal2(tenv.s1, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s4, 0, 2, route_class::RTC_ROUTE, tenv.s5, true);
	checksignal2(tenv.s5, 0, 1, route_class::RTC_ROUTE, tenv.s6, true);
	checksignal2(tenv.s6, 0, 0, route_class::RTC_NULL, 0, false);

	env.w.SubmitAction(action_reservepath(env.w, tenv.s6, tenv.b));
	env.w.FindOrMakeTrackCircuitByName("T6")->SetTCFlagsMasked(track_circuit::TCF::FORCEOCCUPIED, track_circuit::TCF::FORCEOCCUPIED);
	env.w.GameStep(1);
	env.w.SubmitAction(action_unreservetrack(env.w, *tenv.s6));
	env.w.GameStep(1);

	checksignal2(tenv.s1, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s4, 0, 2, route_class::RTC_ROUTE, tenv.s5, true);
	checksignal2(tenv.s5, 0, 1, route_class::RTC_ROUTE, tenv.s6, true);
	checksignal2(tenv.s6, 0, 1, route_class::RTC_SHUNT, tenv.b, true);

	//timing

	tenv.s6->EnumerateFutures([&](const future &f){
		CHECK(f.GetTriggerTime() > 45000);
		CHECK(f.GetTriggerTime() < 45100);
		CHECK(f.GetTypeSerialisationName() == "future_action_wrapper");
	});
	CHECK(tenv.s6->HaveFutures() == true);

	env.w.GameStep(44500);

	checksignal2(tenv.s1, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s4, 0, 2, route_class::RTC_ROUTE, tenv.s5, true);
	checksignal2(tenv.s5, 0, 1, route_class::RTC_ROUTE, tenv.s6, true);
	checksignal2(tenv.s6, 0, 1, route_class::RTC_SHUNT, tenv.b, true);

	env.w.GameStep(1000);
	env.w.GameStep(1);

	checksignal2(tenv.s1, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s4, 0, 2, route_class::RTC_ROUTE, tenv.s5, true);
	checksignal2(tenv.s5, 0, 1, route_class::RTC_ROUTE, tenv.s6, true);
	checksignal2(tenv.s6, 0, 0, route_class::RTC_NULL, 0, false);

	env.w.GameStep(14000);

	checksignal2(tenv.s1, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s4, 0, 2, route_class::RTC_ROUTE, tenv.s5, true);
	checksignal2(tenv.s5, 0, 1, route_class::RTC_ROUTE, tenv.s6, true);
	checksignal2(tenv.s6, 0, 0, route_class::RTC_NULL, 0, false);

	env.w.GameStep(1000);
	env.w.GameStep(1);

	checksignal2(tenv.s1, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s4, 0, 1, route_class::RTC_ROUTE, tenv.s5, true);	//TODO: would 2 be better than 1?
	checksignal2(tenv.s5, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s6, 0, 0, route_class::RTC_NULL, 0, false);

	env.w.GameStep(59000);

	checksignal2(tenv.s1, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s4, 0, 1, route_class::RTC_ROUTE, tenv.s5, true);	//TODO: would 2 be better than 1?
	checksignal2(tenv.s5, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s6, 0, 0, route_class::RTC_NULL, 0, false);

	env.w.GameStep(1000);
	env.w.GameStep(1);

	checksignal2(tenv.s1, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s2, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s3, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s4, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s5, 0, 0, route_class::RTC_NULL, 0, false);
	checksignal2(tenv.s6, 0, 0, route_class::RTC_NULL, 0, false);
}

std::string overlaptimeout_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "newtype" : "2aspectroute", "basetype" : "routesignal", "content" : { "maxaspect" : 1, "routesignal" : true } }, )"
	R"({ "type" : "startofline", "name" : "A" }, )"
	R"({ "type" : "trackseg", "length" : 50000, "trackcircuit" : "T1" }, )"
	R"({ "type" : "autosignal", "name" : "S1" }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S1ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T2" }, )"
	R"({ "type" : "autosignal", "name" : "S2" }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S2ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T3" }, )"
	R"({ "type" : "2aspectroute", "name" : "S3", "overlaptimeout" : 30000 }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S3ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T4" }, )"
	R"({ "type" : "2aspectroute", "name" : "S4", "routerestrictions" : [ { "overlaptimeout" : 90000, "targets" : "S4ovlpend" } ] }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S4ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true, "name" : "S4ovlpend" }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T5" }, )"
	R"({ "type" : "2aspectroute", "name" : "S5", "overlaptimeout" : 30000 }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S5ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T6" }, )"
	R"({ "type" : "2aspectroute", "name" : "S6", "overlaptimeout" : 0 }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S6ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T7" }, )"
	R"({ "type" : "endofline", "name" : "B" } )"
"] }";

TEST_CASE( "signal/overlap/timeout", "Test overlap timeouts" ) {
	test_fixture_world_init_checked env(overlaptimeout_test_str_1);

	autosig_test_class_1 tenv(env.w);

	env.w.SubmitAction(action_reservepath(env.w, tenv.s3, tenv.s4));
	env.w.SubmitAction(action_reservepath(env.w, tenv.s4, tenv.s5));
	env.w.SubmitAction(action_reservepath(env.w, tenv.s5, tenv.s6));

	env.w.GameStep(1);
	CHECK(env.w.GetLogText() == "");

	auto overlapparamcheck = [&](genericsignal *s, world_time timeout) {
		SCOPED_INFO("Overlap parameter check for signal: " << s->GetName());
		const route *ovlp = s->GetCurrentForwardOverlap();
		REQUIRE(ovlp != 0);
		CHECK(ovlp->overlap_timeout == timeout);
	};
	overlapparamcheck(tenv.s4, 90000);
	overlapparamcheck(tenv.s5, 30000);
	overlapparamcheck(tenv.s6, 0);

	auto occupytcandcancelroute = [&](std::string tc, genericsignal *s) {
		env.w.FindOrMakeTrackCircuitByName(tc)->SetTCFlagsMasked(track_circuit::TCF::FORCEOCCUPIED, track_circuit::TCF::FORCEOCCUPIED);
		env.w.SubmitAction(action_unreservetrack(env.w, *s));
		env.w.GameStep(1);
	};

	occupytcandcancelroute("T6", tenv.s5);
	occupytcandcancelroute("T5", tenv.s4);
	occupytcandcancelroute("T4", tenv.s3);
	CHECK(env.w.GetLogText() == "");

	//timing

	auto overlapcheck = [&](genericsignal *s, bool exists) {
		SCOPED_INFO("Overlap check for signal: " << s->GetName() << ", at time: " << env.w.GetGameTime());
		const route *ovlp = s->GetCurrentForwardOverlap();
		if(exists) {
			CHECK(ovlp != 0);
		}
		else {
			CHECK(ovlp == 0);
		}
	};

	overlapcheck(tenv.s2, true);
	overlapcheck(tenv.s3, true);
	overlapcheck(tenv.s4, true);
	overlapcheck(tenv.s5, true);
	overlapcheck(tenv.s6, true);

	env.w.GameStep(29500);

	overlapcheck(tenv.s2, true);
	overlapcheck(tenv.s3, true);
	overlapcheck(tenv.s4, true);
	overlapcheck(tenv.s5, true);
	overlapcheck(tenv.s6, true);

	env.w.GameStep(1000);
	env.w.GameStep(1);

	overlapcheck(tenv.s2, true);
	overlapcheck(tenv.s3, true);
	overlapcheck(tenv.s4, true);
	overlapcheck(tenv.s5, false);
	overlapcheck(tenv.s6, true);

	env.w.GameStep(59000);

	overlapcheck(tenv.s2, true);
	overlapcheck(tenv.s3, true);
	overlapcheck(tenv.s4, true);
	overlapcheck(tenv.s5, false);
	overlapcheck(tenv.s6, true);

	env.w.GameStep(1000);
	env.w.GameStep(1);

	overlapcheck(tenv.s2, true);
	overlapcheck(tenv.s3, true);
	overlapcheck(tenv.s4, false);
	overlapcheck(tenv.s5, false);
	overlapcheck(tenv.s6, true);

	env.w.GameStep(1000000);

	overlapcheck(tenv.s2, true);
	overlapcheck(tenv.s3, true);
	overlapcheck(tenv.s4, false);
	overlapcheck(tenv.s5, false);
	overlapcheck(tenv.s6, true);
}

TEST_CASE( "signal/deserialisation/flagchecks", "Test signal/route flags contradiction detection and sanity checks" ) {
	auto check = [&](const std::string &str, unsigned int errcount) {
		test_fixture_world env(str);
		if(env.ec.GetErrorCount() && env.ec.GetErrorCount() != errcount) { WARN("Error Collection: " << env.ec); }
		REQUIRE(env.ec.GetErrorCount() == errcount);
	};

	check(
	R"({ "content" : [ )"
		R"({ "type" : "routesignal", "name" : "S1", "overlapend" : true, "end" : { "allow" : "overlap" } } )"
	"] }"
	, 0);

	check(
	R"({ "content" : [ )"
		R"({ "type" : "routesignal", "name" : "S1", "overlapend" : true, "end" : { "deny" : "overlap" } } )"
	"] }"
	, 1);

	check(
	R"({ "content" : [ )"
		R"({ "type" : "routesignal", "name" : "S1", "start" : { "deny" : "overlap" } } )"
	"] }"
	, 1);

	check(
	R"({ "content" : [ )"
		R"({ "type" : "routesignal", "name" : "S1", "routerestrictions" : [ { "approachcontrol" : true, "approachcontroltriggerdelay" : 100} ] } )"
	"] }"
	, 0);

	check(
	R"({ "content" : [ )"
		R"({ "type" : "routesignal", "name" : "S1", "routerestrictions" : [ { "approachcontrol" : false, "approachcontroltriggerdelay" : 100} ] } )"
	"] }"
	, 1);
}

std::string approachcontrol_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "newtype" : "4aspectroute", "basetype" : "routesignal", "content" : { "maxaspect" : 3, "routesignal" : true } }, )"
	R"({ "type" : "startofline", "name" : "A" }, )"
	R"({ "type" : "trackseg", "length" : 50000, "trackcircuit" : "T1" }, )"
	R"({ "type" : "4aspectroute", "name" : "S1" }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S1ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T3" }, )"
	R"({ "type" : "4aspectroute", "name" : "S3", "routerestrictions" : [ { "approachcontrol" : true } ] }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S3ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000 }, )"
	R"({ "type" : "autosignal", "name" : "AS" }, )"
	R"({ "type" : "trackseg", "length" : 20000 }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T4" }, )"
	R"({ "type" : "4aspectroute", "name" : "S4", "routerestrictions" : [ { "approachcontroltriggerdelay" : 3000 } ] }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S4ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T5" }, )"
	R"({ "type" : "4aspectroute", "name" : "S5", "routerestrictions" : [ { "targets" : "C", "approachcontrol" : true } ] }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S5ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"

	R"({ "type" : "points", "name" : "P1" }, )"
	R"({ "type" : "trackseg", "length" : 500000, "trackcircuit" : "T6"  }, )"
	R"({ "type" : "routesignal", "name" : "S6", "shuntsignal" : true, "end" : { "allow" : "route" } }, )"
	R"({ "type" : "trackseg", "length" : 500000 }, )"
	R"({ "type" : "endofline", "name" : "B", "end" : { "allow" : "overlap" } }, )"

	R"({ "type" : "trackseg", "length" : 400000, "connect" : { "to" : "P1" } }, )"
	R"({ "type" : "endofline", "name" : "C" } )"
"] }";

TEST_CASE( "signal/approachcontrol/general", "Test basic approach control" ) {
	test_fixture_world_init_checked env(approachcontrol_test_str_1);

	genericsignal *s1 = PTR_CHECK(env.w.FindTrackByNameCast<genericsignal>("S1"));
	genericsignal *s3 = PTR_CHECK(env.w.FindTrackByNameCast<genericsignal>("S3"));
	genericsignal *as = PTR_CHECK(env.w.FindTrackByNameCast<genericsignal>("AS"));
	genericsignal *s4 = PTR_CHECK(env.w.FindTrackByNameCast<genericsignal>("S4"));
	genericsignal *s5 = PTR_CHECK(env.w.FindTrackByNameCast<genericsignal>("S5"));
	genericsignal *s6 = PTR_CHECK(env.w.FindTrackByNameCast<genericsignal>("S6"));
	routingpoint *b = PTR_CHECK(env.w.FindTrackByNameCast<routingpoint>("B"));

	env.w.SubmitAction(action_reservepath(env.w, s1, s3));
	env.w.SubmitAction(action_reservepath(env.w, s3, as));
	env.w.SubmitAction(action_reservepath(env.w, s4, s5));
	env.w.SubmitAction(action_reservepath(env.w, s5, s6));
	env.w.SubmitAction(action_reservepath(env.w, s6, b));

	env.w.GameStep(1);

	CHECK(env.w.GetLogText() == "");

	auto checksignals = [&](unsigned int line, unsigned int s1a, unsigned int s3a, unsigned int s4a, unsigned int s5a, unsigned int s6a) {
		SCOPED_INFO("Signal check at time: " << env.w.GetGameTime() << ", line: " << line);
		CHECK(s1->GetAspect() == s1a);
		CHECK(s3->GetAspect() == s3a);
		CHECK(s4->GetAspect() == s4a);
		CHECK(s5->GetAspect() == s5a);
		CHECK(s6->GetAspect() == s6a);
	};


	checksignals(__LINE__, 1, 0, 0, 1, 1);

	env.w.FindOrMakeTrackCircuitByName("S1ovlp")->SetTCFlagsMasked(track_circuit::TCF::FORCEOCCUPIED, track_circuit::TCF::FORCEOCCUPIED);
	env.w.FindOrMakeTrackCircuitByName("T6")->SetTCFlagsMasked(track_circuit::TCF::FORCEOCCUPIED, track_circuit::TCF::FORCEOCCUPIED);
	env.w.GameStep(1);
	checksignals(__LINE__, 0, 0, 0, 0, 1);
	env.w.FindOrMakeTrackCircuitByName("S1ovlp")->SetTCFlagsMasked(track_circuit::TCF::ZERO, track_circuit::TCF::FORCEOCCUPIED);
	env.w.FindOrMakeTrackCircuitByName("T6")->SetTCFlagsMasked(track_circuit::TCF::ZERO, track_circuit::TCF::FORCEOCCUPIED);

	env.w.FindOrMakeTrackCircuitByName("T3")->SetTCFlagsMasked(track_circuit::TCF::FORCEOCCUPIED, track_circuit::TCF::FORCEOCCUPIED);
	env.w.FindOrMakeTrackCircuitByName("T4")->SetTCFlagsMasked(track_circuit::TCF::FORCEOCCUPIED, track_circuit::TCF::FORCEOCCUPIED);
	env.w.GameStep(1);
	checksignals(__LINE__, 0, 1, 0, 1, 1);

	env.w.GameStep(2998);
	checksignals(__LINE__, 0, 1, 0, 1, 1);

	env.w.FindOrMakeTrackCircuitByName("T3")->SetTCFlagsMasked(track_circuit::TCF::ZERO, track_circuit::TCF::FORCEOCCUPIED);
	env.w.FindOrMakeTrackCircuitByName("T4")->SetTCFlagsMasked(track_circuit::TCF::ZERO, track_circuit::TCF::FORCEOCCUPIED);
	env.w.GameStep(1);
	checksignals(__LINE__, 3, 2, 0, 1, 1);

	env.w.FindOrMakeTrackCircuitByName("T4")->SetTCFlagsMasked(track_circuit::TCF::FORCEOCCUPIED, track_circuit::TCF::FORCEOCCUPIED);
	env.w.GameStep(3000);
	checksignals(__LINE__, 2, 1, 2, 1, 1);
}


std::string callon_test_str_1 =
R"({ "content" : [ )"
	R"({ "type" : "typedef", "newtype" : "4aspectroute", "basetype" : "routesignal", "content" : { "maxaspect" : 3, "routesignal" : true } }, )"
	R"({ "type" : "startofline", "name" : "A" }, )"
	R"({ "type" : "trackseg", "length" : 50000, "trackcircuit" : "T1" }, )"
	R"({ "type" : "4aspectroute", "name" : "S1" }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S1ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T2" }, )"
	R"({ "type" : "4aspectroute", "name" : "S2", "start" : { "allow" : "callon" }, "routerestrictions" : [ { "applyonly" : "callon", "exitsignalcontrol" : true } ] }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S2ovlp" }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "trackseg", "length" : 30000, "trackcircuit" : "T3" }, )"
	R"({ "type" : "4aspectroute", "name" : "S3", "end" : { "allow" : "callon" } }, )"
	R"({ "type" : "trackseg", "length" : 20000, "trackcircuit" : "S3ovlp" }, )"
	R"({ "type" : "endofline", "name" : "B", "end" : { "allow" : "overlap" } } )"
"] }";

TEST_CASE( "signal/callon/general", "Test call-on routes" ) {
	test_fixture_world_init_checked env(callon_test_str_1);

	genericsignal *s1 = PTR_CHECK(env.w.FindTrackByNameCast<genericsignal>("S1"));
	genericsignal *s2 = PTR_CHECK(env.w.FindTrackByNameCast<genericsignal>("S2"));
	genericsignal *s3 = PTR_CHECK(env.w.FindTrackByNameCast<genericsignal>("S3"));
	routingpoint *b = PTR_CHECK(env.w.FindTrackByNameCast<routingpoint>("B"));

	auto settcstate = [&](const std::string &tcname, bool enter) {
		track_circuit *tc = env.w.FindOrMakeTrackCircuitByName(tcname);
		tc->SetTCFlagsMasked(enter ? track_circuit::TCF::FORCEOCCUPIED : track_circuit::TCF::ZERO, track_circuit::TCF::FORCEOCCUPIED);
	};

	auto checkaspects = [&](unsigned int s1asp, route_class::ID s1type, unsigned int s2asp, route_class::ID s2type, unsigned int s3asp, route_class::ID s3type) {
		CHECK(s1asp == s1->GetAspect());
		CHECK(s1type == s1->GetAspectType());
		CHECK(s2asp == s2->GetAspect());
		CHECK(s2type == s2->GetAspectType());
		CHECK(s3asp == s3->GetAspect());
		CHECK(s3type == s3->GetAspectType());
	};

	env.w.SubmitAction(action_reservepath(env.w, s1, s2));
	env.w.SubmitAction(action_reservepath(env.w, s2, s3));
	env.w.SubmitAction(action_reservepath(env.w, s3, b));
	env.w.GameStep(1);

	checkaspects(3, route_class::RTC_ROUTE, 2, route_class::RTC_ROUTE, 1, route_class::RTC_ROUTE);

	settcstate("T3", true);
	env.w.GameStep(1);

	checkaspects(1, route_class::RTC_ROUTE, 0, route_class::RTC_NULL, 1, route_class::RTC_ROUTE);

	env.w.SubmitAction(action_unreservetrack(env.w, *s2));
	env.w.GameStep(1);

	env.w.SubmitAction(action_reservepath(env.w, s2, s3));
	env.w.GameStep(1);

	CHECK(env.w.GetLogText() != "");
	if(s2->GetCurrentForwardRoute()) {
		FAIL(s2->GetCurrentForwardRoute()->type);
	}

	env.w.ResetLogText();

	settcstate("T3", false);	//do this to avoid triggering approach locking
	env.w.SubmitAction(action_unreservetrack(env.w, *s3));
	env.w.GameStep(1);
	settcstate("T3", true);

	env.w.SubmitAction(action_reservepath(env.w, s2, s3));
	env.w.GameStep(1);

	CHECK(env.w.GetLogText() == "");
	REQUIRE(s2->GetCurrentForwardRoute() != 0);
	CHECK(s2->GetCurrentForwardRoute()->type == route_class::RTC_CALLON);

	checkaspects(1, route_class::RTC_ROUTE, 0, route_class::RTC_NULL, 0, route_class::RTC_NULL);

	settcstate("T2", true);
	env.w.GameStep(1);

	checkaspects(0, route_class::RTC_NULL, 1, route_class::RTC_CALLON, 0, route_class::RTC_NULL);

	env.w.SubmitAction(action_reservepath(env.w, s3, b));
	env.w.GameStep(1);

	CHECK(env.w.GetLogText() != "");
	if(s3->GetCurrentForwardRoute()) {
		FAIL(s3->GetCurrentForwardRoute()->type);
	}
}

TEST_CASE( "signal/overlap/general", "Test basic overlap assignment" ) {
	test_fixture_world_init_checked env(
		R"({ "content" : [ )"
			R"({ "type" : "typedef", "newtype" : "4aspectroute", "basetype" : "routesignal", "content" : { "maxaspect" : 3, "routesignal" : true } }, )"
			R"({ "type" : "startofline", "name" : "A" }, )"
			R"({ "type" : "4aspectroute", "name" : "S1" }, )"
			R"({ "type" : "trackseg", "length" : 30000 }, )"
			R"({ "type" : "4aspectroute", "name" : "S2" }, )"
			R"({ "type" : "trackseg", "length" : 20000 }, )"
			R"({ "type" : "endofline", "name" : "B", "end" : { "allow" : "overlap" } } )"
		"] }"
	);

	genericsignal *s1 = PTR_CHECK(env.w.FindTrackByNameCast<genericsignal>("S1"));
	genericsignal *s2 = PTR_CHECK(env.w.FindTrackByNameCast<genericsignal>("S2"));

	env.w.SubmitAction(action_reservepath(env.w, s1, s2));
	env.w.GameStep(1);
	CHECK(env.w.GetLogText() == "");

	CHECK(PTR_CHECK(s2->GetCurrentForwardOverlap())->type == route_class::ID::RTC_OVERLAP);
}

TEST_CASE( "signal/overlap/nooverlapflag", "Test signal no overlap flag" ) {
	test_fixture_world_init_checked env(
		R"({ "content" : [ )"
			R"({ "type" : "typedef", "newtype" : "4aspectroute", "basetype" : "routesignal", "content" : { "maxaspect" : 3, "routesignal" : true } }, )"
			R"({ "type" : "startofline", "name" : "A" }, )"
			R"({ "type" : "4aspectroute", "name" : "S1" }, )"
			R"({ "type" : "trackseg", "length" : 30000 }, )"
			R"({ "type" : "4aspectroute", "name" : "S2", "nooverlap" : true }, )"
			R"({ "type" : "trackseg", "length" : 20000 }, )"
			R"({ "type" : "endofline", "name" : "B" } )"
		"] }"
	);

	genericsignal *s1 = PTR_CHECK(env.w.FindTrackByNameCast<genericsignal>("S1"));
	genericsignal *s2 = PTR_CHECK(env.w.FindTrackByNameCast<genericsignal>("S2"));

	env.w.SubmitAction(action_reservepath(env.w, s1, s2));
	env.w.GameStep(1);
	CHECK(env.w.GetLogText() == "");

	CHECK(s2->GetCurrentForwardOverlap() == 0);
}

TEST_CASE( "signal/overlap/missingoverlap", "Test that a missing overlap triggers an error" ) {
	test_fixture_world env(
		R"({ "content" : [ )"
			R"({ "type" : "typedef", "newtype" : "4aspectroute", "basetype" : "routesignal", "content" : { "maxaspect" : 3, "routesignal" : true } }, )"
			R"({ "type" : "startofline", "name" : "A" }, )"
			R"({ "type" : "4aspectroute", "name" : "S1" }, )"
			R"({ "type" : "trackseg", "length" : 30000 }, )"
			R"({ "type" : "4aspectroute", "name" : "S2" }, )"
			R"({ "type" : "trackseg", "length" : 20000 }, )"
			R"({ "type" : "endofline", "name" : "B" } )"
		"] }"
	);
	env.w.LayoutInit(env.ec);
	CHECK(env.ec.GetErrorCount() == 0);
	env.w.PostLayoutInit(env.ec);
	CHECK(env.ec.GetErrorCount() > 0);
}
