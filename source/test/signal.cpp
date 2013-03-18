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
	R"({ "type" : "routingmarker", "overlapend" : true, "routethrough_rev" : false, "connect" : { "fromdirection" : "back", "to" : "DS1", "todirection" : "rightback" } }, )"
	R"({ "type" : "startofline", "name" : "B" }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routesignal", "name" : "S2", "routeshuntsignal" : true, "routerestrictions" : [ { "targets" : "C", "denyroute" : true } ] }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routingmarker", "overlapend" : true }, )"
	R"({ "type" : "points", "name" : "P1", "connect" : { "fromdirection" : "reverse", "to" : "DS1", "todirection" : "rightfront" } }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routingmarker", "overlapend_rev" : true, "through" : false }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "routesignal", "name" : "S4", "reverseautoconnection" : true, "routeshuntsignal" : true, "through_rev" : false, "overlapend" : true}, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "autosignal", "name" : "S5", "reverseautoconnection" : true }, )"
	R"({ "type" : "trackseg", "length" : 50000 }, )"
	R"({ "type" : "endofline", "name" : "E" } )"
"] }";

TEST_CASE( "signal/deserialisation/general", "Test basic signal and routing deserialisation" ) {

	test_fixture_world env(track_test_str_1);

	if(env.ec.GetErrorCount()) { WARN("Error Collection: " << env.ec); }
	REQUIRE(env.ec.GetErrorCount() == 0);

	startofline *a = dynamic_cast<startofline *>(env.w.FindTrackByName("A"));
	REQUIRE(a != 0);
	REQUIRE(a->GetAvailableRouteTypes(EDGE_FRONT) == (RPRT::SHUNTEND | RPRT::ROUTEEND));
	REQUIRE(a->GetAvailableRouteTypes(EDGE_BACK) == RPRT::ZERO);
	REQUIRE(a->GetSetRouteTypes(EDGE_FRONT) == RPRT::ZERO);
	REQUIRE(a->GetSetRouteTypes(EDGE_BACK) == RPRT::ZERO);

	routesignal *s1 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S1"));
	REQUIRE(s1 != 0);
	REQUIRE(s1->GetAvailableRouteTypes(EDGE_FRONT) == (RPRT::SHUNTEND | RPRT::ROUTEEND | RPRT::SHUNTSTART | RPRT::ROUTESTART));
	REQUIRE(s1->GetAvailableRouteTypes(EDGE_BACK) == (RPRT::SHUNTTRANS | RPRT::ROUTETRANS));
	REQUIRE(s1->GetSetRouteTypes(EDGE_FRONT) == RPRT::ZERO);
	REQUIRE(s1->GetSetRouteTypes(EDGE_BACK) == RPRT::ZERO);

	routingmarker *rm = dynamic_cast<routingmarker *>(env.w.FindTrackByName("#4"));
	REQUIRE(rm != 0);
	REQUIRE(rm->GetAvailableRouteTypes(EDGE_FRONT) == (RPRT::MASK_TRANS | RPRT::OVERLAPEND));
	REQUIRE(rm->GetAvailableRouteTypes(EDGE_BACK) == (RPRT::MASK_TRANS));
	REQUIRE(rm->GetSetRouteTypes(EDGE_FRONT) == RPRT::ZERO);
	REQUIRE(rm->GetSetRouteTypes(EDGE_BACK) == RPRT::ZERO);

	routesignal *s3 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S3"));
	REQUIRE(s3 != 0);
	REQUIRE(s3->GetAvailableRouteTypes(EDGE_FRONT) == (RPRT::SHUNTEND | RPRT::SHUNTSTART));
	REQUIRE(s3->GetAvailableRouteTypes(EDGE_BACK) == (RPRT::SHUNTTRANS | RPRT::ROUTETRANS));
	REQUIRE(s3->GetSetRouteTypes(EDGE_FRONT) == RPRT::ZERO);
	REQUIRE(s3->GetSetRouteTypes(EDGE_BACK) == RPRT::ZERO);

	routingmarker *rm2 = dynamic_cast<routingmarker *>(env.w.FindTrackByName("#13"));
	REQUIRE(rm2 != 0);
	REQUIRE(rm2->GetAvailableRouteTypes(EDGE_FRONT) == (RPRT::MASK_TRANS | RPRT::OVERLAPEND));
	REQUIRE(rm2->GetAvailableRouteTypes(EDGE_BACK) == (RPRT::MASK_TRANS & ~RPRT::ROUTETRANS));
	REQUIRE(rm2->GetSetRouteTypes(EDGE_FRONT) == RPRT::ZERO);
	REQUIRE(rm2->GetSetRouteTypes(EDGE_BACK) == RPRT::ZERO);

	routesignal *s2 = dynamic_cast<routesignal *>(env.w.FindTrackByName("S2"));
	REQUIRE(s2 != 0);
	REQUIRE(s2->GetRouteRestrictions().GetRestrictionCount() == 1);
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

	auto s5check = [&](unsigned int index, ROUTE_CLASS type) {
		REQUIRE(s5->GetRouteByIndex(index) != 0);
		REQUIRE(s5->GetRouteByIndex(index)->type == type);
		REQUIRE(s5->GetRouteByIndex(index)->end == vartrack_target_ptr<routingpoint>(s4, EDGE_FRONT));
		REQUIRE(s5->GetRouteByIndex(index)->pieces.size() == 1);
	};
	s5check(0, RTC_ROUTE);
	s5check(1, RTC_OVERLAP);

	std::vector<const route *> routeset;
	REQUIRE(s1->GetMatchingRoutes(routeset, a, via_list(), GMRF::ALL) == 0);
	REQUIRE(s1->GetMatchingRoutes(routeset, b, via_list(), GMRF::ALL) == 0);
	REQUIRE(s1->GetMatchingRoutes(routeset, c, via_list(), GMRF::ALL) == 0);
	REQUIRE(s1->GetMatchingRoutes(routeset, d, via_list(), GMRF::ALL) == 1);
	REQUIRE(s1->GetMatchingRoutes(routeset, e, via_list(), GMRF::ALL) == 0);
	REQUIRE(s1->GetMatchingRoutes(routeset, s1, via_list(), GMRF::ALL) == 0);

	REQUIRE(s2->GetMatchingRoutes(routeset, a, via_list(), GMRF::ALL) == 0);
	REQUIRE(s2->GetMatchingRoutes(routeset, b, via_list(), GMRF::ALL) == 0);
	REQUIRE(s2->GetMatchingRoutes(routeset, c, via_list(), GMRF::ALL) == 1);
	REQUIRE(s2->GetMatchingRoutes(routeset, d, via_list(), GMRF::ALL) == 1);
	REQUIRE(s2->GetMatchingRoutes(routeset, e, via_list(), GMRF::ALL) == 0);
	REQUIRE(s2->GetMatchingRoutes(routeset, s2, via_list(), GMRF::ALL) == 0);

	REQUIRE(s3->GetMatchingRoutes(routeset, a, via_list(), GMRF::ALL) == 0);
	REQUIRE(s3->GetMatchingRoutes(routeset, b, via_list(), GMRF::ALL) == 1);
	REQUIRE(s3->GetMatchingRoutes(routeset, c, via_list(), GMRF::ALL) == 0);
	REQUIRE(s3->GetMatchingRoutes(routeset, d, via_list(), GMRF::ALL) == 0);
	REQUIRE(s3->GetMatchingRoutes(routeset, e, via_list(), GMRF::ALL) == 0);
	REQUIRE(s3->GetMatchingRoutes(routeset, s3, via_list(), GMRF::ALL) == 0);

	REQUIRE(s4->GetMatchingRoutes(routeset, a, via_list(), GMRF::ALL) == 0);
	REQUIRE(s4->GetMatchingRoutes(routeset, b, via_list(), GMRF::ALL) == 2);
	REQUIRE(s4->GetMatchingRoutes(routeset, c, via_list(), GMRF::ALL) == 0);
	REQUIRE(s4->GetMatchingRoutes(routeset, d, via_list(), GMRF::ALL) == 0);
	REQUIRE(s4->GetMatchingRoutes(routeset, e, via_list(), GMRF::ALL) == 0);
	REQUIRE(s4->GetMatchingRoutes(routeset, s4, via_list(), GMRF::ALL) == 0);

	REQUIRE(s5->GetMatchingRoutes(routeset, a, via_list(), GMRF::ALL) == 0);
	REQUIRE(s5->GetMatchingRoutes(routeset, b, via_list(), GMRF::ALL) == 0);
	REQUIRE(s5->GetMatchingRoutes(routeset, c, via_list(), GMRF::ALL) == 0);
	REQUIRE(s5->GetMatchingRoutes(routeset, d, via_list(), GMRF::ALL) == 0);
	REQUIRE(s5->GetMatchingRoutes(routeset, e, via_list(), GMRF::ALL) == 0);
	REQUIRE(s5->GetMatchingRoutes(routeset, s4, via_list(), GMRF::ALL) == 2);
	REQUIRE(s5->GetMatchingRoutes(routeset, s5, via_list(), GMRF::ALL) == 0);

	REQUIRE(s6->GetMatchingRoutes(routeset, a, via_list(), GMRF::ALL) == 2);
	REQUIRE(s6->GetMatchingRoutes(routeset, b, via_list(), GMRF::ALL) == 2);
	REQUIRE(s6->GetMatchingRoutes(routeset, c, via_list(), GMRF::ALL) == 0);
	REQUIRE(s6->GetMatchingRoutes(routeset, d, via_list(), GMRF::ALL) == 0);
	REQUIRE(s6->GetMatchingRoutes(routeset, e, via_list(), GMRF::ALL) == 0);
	REQUIRE(s6->GetMatchingRoutes(routeset, s6, via_list(), GMRF::ALL) == 0);

	auto overlapcheck = [&](genericsignal *s, const std::string &target) {
		routingpoint *rp = dynamic_cast<routingpoint *>(env.w.FindTrackByName(target));
		REQUIRE(s->GetMatchingRoutes(routeset, rp, via_list(), GMRF::ALL) == 1);
		REQUIRE(routeset[0]->end.track == rp);
		REQUIRE(routeset[0]->type == RTC_OVERLAP);

		unsigned int overlapcount = 0;
		auto callback = [&](const route *r) {
			if(r->type == RTC_OVERLAP) overlapcount++;
		};
		s->EnumerateRoutes(callback);
		REQUIRE(overlapcount == 1);
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
	std::function<void(routingpoint *signal, unsigned int aspect, ROUTE_CLASS aspect_type, routingpoint *aspect_target, routingpoint *aspect_route_target)> checksignal;

	autosig_test_class_1(world &w) {
		s1 = w.FindTrackByNameCast<genericsignal>("S1");
		s2 = w.FindTrackByNameCast<genericsignal>("S2");
		s3 = w.FindTrackByNameCast<genericsignal>("S3");
		s4 = w.FindTrackByNameCast<genericsignal>("S4");
		s5 = w.FindTrackByNameCast<genericsignal>("S5");
		s6 = w.FindTrackByNameCast<genericsignal>("S6");
		a = w.FindTrackByNameCast<routingpoint>("A");
		b = w.FindTrackByNameCast<routingpoint>("B");

		checksignal = [&](routingpoint *signal, unsigned int aspect, ROUTE_CLASS aspect_type, routingpoint *aspect_target, routingpoint *aspect_route_target) {
			REQUIRE(signal != 0);
			SCOPED_INFO("Signal check for: " << signal->GetName() << ", at time: " << w.GetGameTime());

			CHECK(signal->GetAspect() == aspect);
			CHECK(signal->GetAspectType() == aspect_type);
			CHECK(signal->GetNextAspectTarget() == aspect_target);
			CHECK(signal->GetAspectSetRouteTarget() == aspect_route_target);
		};
	}
};

TEST_CASE( "signal/autosignal/propagation", "Test basic autosignal aspect propagation" ) {
	test_fixture_world env(autosig_test_str_1);

	env.w.LayoutInit(env.ec);
	env.w.PostLayoutInit(env.ec);

	REQUIRE(env.ec.GetErrorCount() == 0);

	autosig_test_class_1 tenv(env.w);

	tenv.checksignal(tenv.a, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.b, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s1, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s2, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s3, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s4, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s5, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s6, 0, RTC_NULL, 0, 0);

	env.w.GameStep(1);

	tenv.checksignal(tenv.a, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.b, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s1, 3, RTC_ROUTE, tenv.s2, tenv.s2);
	tenv.checksignal(tenv.s2, 3, RTC_ROUTE, tenv.s3, tenv.s3);
	tenv.checksignal(tenv.s3, 2, RTC_ROUTE, tenv.s4, tenv.s4);
	tenv.checksignal(tenv.s4, 1, RTC_ROUTE, tenv.s5, tenv.s5);
	tenv.checksignal(tenv.s5, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s6, 0, RTC_NULL, 0, 0);

	track_circuit *s3ovlp = env.w.FindOrMakeTrackCircuitByName("S3ovlp");
	s3ovlp->SetTCFlagsMasked(track_circuit::TCF_FORCEOCCUPIED, track_circuit::TCF_FORCEOCCUPIED);
	env.w.GameStep(1);

	tenv.checksignal(tenv.s1, 1, RTC_ROUTE, tenv.s2, tenv.s2);
	tenv.checksignal(tenv.s2, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s3, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s4, 1, RTC_ROUTE, tenv.s5, tenv.s5);
	tenv.checksignal(tenv.s5, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s6, 0, RTC_NULL, 0, 0);
}

TEST_CASE( "signal/routesignal/reserveaction", "Test basic routesignal reservation action and aspect propagation" ) {
	test_fixture_world env(autosig_test_str_1);

	env.w.LayoutInit(env.ec);
	env.w.PostLayoutInit(env.ec);

	REQUIRE(env.ec.GetErrorCount() == 0);

	autosig_test_class_1 tenv(env.w);

	std::vector<const route *> out;
	unsigned int routecount = tenv.s5->GetMatchingRoutes(out, tenv.s6, via_list(), GMRF::ALL);
	REQUIRE(routecount == 1);

	env.w.SubmitAction(action_reservetrack(env.w, *out[0]));

	tenv.checksignal(tenv.a, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.b, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s1, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s2, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s3, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s4, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s5, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s6, 0, RTC_NULL, 0, 0);

	env.w.GameStep(1);

	tenv.checksignal(tenv.a, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.b, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s1, 3, RTC_ROUTE, tenv.s2, tenv.s2);
	tenv.checksignal(tenv.s2, 3, RTC_ROUTE, tenv.s3, tenv.s3);
	tenv.checksignal(tenv.s3, 3, RTC_ROUTE, tenv.s4, tenv.s4);
	tenv.checksignal(tenv.s4, 2, RTC_ROUTE, tenv.s5, tenv.s5);
	tenv.checksignal(tenv.s5, 1, RTC_ROUTE, tenv.s6, tenv.s6);
	tenv.checksignal(tenv.s6, 0, RTC_NULL, 0, 0);

	const route* s6rt = tenv.s6->FindBestRoute(tenv.b, GMRF::ALL);
	REQUIRE(s6rt != 0);
	env.w.SubmitAction(action_reservetrack(env.w, *s6rt));

	env.w.GameStep(1);

	tenv.checksignal(tenv.a, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.b, 0, RTC_NULL, 0, 0);
	tenv.checksignal(tenv.s1, 3, RTC_ROUTE, tenv.s2, tenv.s2);
	tenv.checksignal(tenv.s2, 3, RTC_ROUTE, tenv.s3, tenv.s3);
	tenv.checksignal(tenv.s3, 3, RTC_ROUTE, tenv.s4, tenv.s4);
	tenv.checksignal(tenv.s4, 3, RTC_ROUTE, tenv.s5, tenv.s5);
	tenv.checksignal(tenv.s5, 2, RTC_ROUTE, tenv.s6, tenv.s6);
	tenv.checksignal(tenv.s6, 1, RTC_ROUTE, tenv.b, tenv.b);
}
